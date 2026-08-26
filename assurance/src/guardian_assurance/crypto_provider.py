"""Guardian M15 host/test Ed25519 provider.

This module performs cryptographic Ed25519 operations against explicitly
supplied key material.

It does not resolve trust, classify credential lifecycle, establish freshness,
establish physical truth, grant authority, or perform physical actuation.

A successful cryptographic verification means only that the supplied
signature verifies against the supplied public key and transcript.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum

from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric.ed25519 import (
    Ed25519PrivateKey,
    Ed25519PublicKey,
)


ED25519_ALGORITHM = "ed25519"
ED25519_PUBLIC_KEY_BYTES = 32
ED25519_PRIVATE_KEY_BYTES = 32
ED25519_SIGNATURE_BYTES = 64


class CryptoResultCode(str, Enum):
    """Stable result states for the isolated cryptographic provider."""

    VERIFIED = "VERIFIED"
    SIGNATURE_INVALID = "SIGNATURE_INVALID"
    ALGORITHM_UNSUPPORTED = "ALGORITHM_UNSUPPORTED"
    CRYPTO_PROVIDER_UNAVAILABLE = "CRYPTO_PROVIDER_UNAVAILABLE"
    CRYPTO_PROVIDER_FAILURE = "CRYPTO_PROVIDER_FAILURE"
    INTERNAL_ERROR = "INTERNAL_ERROR"


@dataclass(frozen=True)
class VerificationResult:
    """Machine-readable cryptographic verification result.

    VERIFIED is intentionally not named AUTHENTICATED.

    Authentication additionally requires trusted credential resolution outside
    this provider boundary.
    """

    code: CryptoResultCode
    algorithm: str
    provider_identifier: str
    provider_version: str
    detail: str | None = None

    @property
    def verified(self) -> bool:
        return self.code is CryptoResultCode.VERIFIED


@dataclass(frozen=True)
class SigningResult:
    """Machine-readable host/test signing result."""

    code: CryptoResultCode
    algorithm: str
    provider_identifier: str
    provider_version: str
    signature: bytes | None = None
    detail: str | None = None

    @property
    def signed(self) -> bool:
        return (
            self.code is CryptoResultCode.VERIFIED
            and self.signature is not None
        )


class HostEd25519Provider:
    """Reference host/test Ed25519 provider using Python cryptography."""

    provider_identifier = "python-cryptography-ed25519"

    def __init__(self) -> None:
        try:
            import cryptography

            self.provider_version = cryptography.__version__
        except Exception:
            self.provider_version = "unavailable"

    def verify(
        self,
        *,
        algorithm: str,
        public_key: bytes,
        transcript: bytes,
        signature: bytes,
    ) -> VerificationResult:
        """Verify one raw Ed25519 signature.

        The caller is responsible for trust resolution before using the
        resulting cryptographic fact as authenticated provenance.
        """

        if algorithm != ED25519_ALGORITHM:
            return VerificationResult(
                code=CryptoResultCode.ALGORITHM_UNSUPPORTED,
                algorithm=algorithm,
                provider_identifier=self.provider_identifier,
                provider_version=self.provider_version,
                detail="unsupported cryptographic algorithm",
            )

        if not isinstance(public_key, bytes):
            return VerificationResult(
                code=CryptoResultCode.CRYPTO_PROVIDER_FAILURE,
                algorithm=algorithm,
                provider_identifier=self.provider_identifier,
                provider_version=self.provider_version,
                detail="public key must be bytes",
            )

        if len(public_key) != ED25519_PUBLIC_KEY_BYTES:
            return VerificationResult(
                code=CryptoResultCode.CRYPTO_PROVIDER_FAILURE,
                algorithm=algorithm,
                provider_identifier=self.provider_identifier,
                provider_version=self.provider_version,
                detail="Ed25519 public key must be exactly 32 bytes",
            )

        if not isinstance(transcript, bytes):
            return VerificationResult(
                code=CryptoResultCode.CRYPTO_PROVIDER_FAILURE,
                algorithm=algorithm,
                provider_identifier=self.provider_identifier,
                provider_version=self.provider_version,
                detail="transcript must be bytes",
            )

        if not isinstance(signature, bytes):
            return VerificationResult(
                code=CryptoResultCode.CRYPTO_PROVIDER_FAILURE,
                algorithm=algorithm,
                provider_identifier=self.provider_identifier,
                provider_version=self.provider_version,
                detail="signature must be bytes",
            )

        if len(signature) != ED25519_SIGNATURE_BYTES:
            return VerificationResult(
                code=CryptoResultCode.SIGNATURE_INVALID,
                algorithm=algorithm,
                provider_identifier=self.provider_identifier,
                provider_version=self.provider_version,
                detail="Ed25519 signature must be exactly 64 bytes",
            )

        try:
            key = Ed25519PublicKey.from_public_bytes(public_key)
        except (TypeError, ValueError) as exc:
            return VerificationResult(
                code=CryptoResultCode.CRYPTO_PROVIDER_FAILURE,
                algorithm=algorithm,
                provider_identifier=self.provider_identifier,
                provider_version=self.provider_version,
                detail=f"public key construction failed: {type(exc).__name__}",
            )
        except Exception as exc:
            return VerificationResult(
                code=CryptoResultCode.INTERNAL_ERROR,
                algorithm=algorithm,
                provider_identifier=self.provider_identifier,
                provider_version=self.provider_version,
                detail=f"unexpected key-construction failure: {type(exc).__name__}",
            )

        try:
            key.verify(signature, transcript)
        except InvalidSignature:
            return VerificationResult(
                code=CryptoResultCode.SIGNATURE_INVALID,
                algorithm=algorithm,
                provider_identifier=self.provider_identifier,
                provider_version=self.provider_version,
                detail="signature does not verify",
            )
        except Exception as exc:
            return VerificationResult(
                code=CryptoResultCode.CRYPTO_PROVIDER_FAILURE,
                algorithm=algorithm,
                provider_identifier=self.provider_identifier,
                provider_version=self.provider_version,
                detail=f"provider verification failure: {type(exc).__name__}",
            )

        return VerificationResult(
            code=CryptoResultCode.VERIFIED,
            algorithm=algorithm,
            provider_identifier=self.provider_identifier,
            provider_version=self.provider_version,
        )

    def sign_for_test(
        self,
        *,
        algorithm: str,
        private_key: bytes,
        transcript: bytes,
    ) -> SigningResult:
        """Sign a transcript with raw host/test private-key bytes.

        This method exists only for explicitly designated host/test workflows.

        It is not a production key-custody architecture.
        """

        if algorithm != ED25519_ALGORITHM:
            return SigningResult(
                code=CryptoResultCode.ALGORITHM_UNSUPPORTED,
                algorithm=algorithm,
                provider_identifier=self.provider_identifier,
                provider_version=self.provider_version,
                detail="unsupported cryptographic algorithm",
            )

        if not isinstance(private_key, bytes):
            return SigningResult(
                code=CryptoResultCode.CRYPTO_PROVIDER_FAILURE,
                algorithm=algorithm,
                provider_identifier=self.provider_identifier,
                provider_version=self.provider_version,
                detail="private key must be bytes",
            )

        if len(private_key) != ED25519_PRIVATE_KEY_BYTES:
            return SigningResult(
                code=CryptoResultCode.CRYPTO_PROVIDER_FAILURE,
                algorithm=algorithm,
                provider_identifier=self.provider_identifier,
                provider_version=self.provider_version,
                detail="Ed25519 private key must be exactly 32 bytes",
            )

        if not isinstance(transcript, bytes):
            return SigningResult(
                code=CryptoResultCode.CRYPTO_PROVIDER_FAILURE,
                algorithm=algorithm,
                provider_identifier=self.provider_identifier,
                provider_version=self.provider_version,
                detail="transcript must be bytes",
            )

        try:
            key = Ed25519PrivateKey.from_private_bytes(private_key)
            signature = key.sign(transcript)
        except (TypeError, ValueError) as exc:
            return SigningResult(
                code=CryptoResultCode.CRYPTO_PROVIDER_FAILURE,
                algorithm=algorithm,
                provider_identifier=self.provider_identifier,
                provider_version=self.provider_version,
                detail=f"private key operation failed: {type(exc).__name__}",
            )
        except Exception as exc:
            return SigningResult(
                code=CryptoResultCode.INTERNAL_ERROR,
                algorithm=algorithm,
                provider_identifier=self.provider_identifier,
                provider_version=self.provider_version,
                detail=f"unexpected signing failure: {type(exc).__name__}",
            )

        if len(signature) != ED25519_SIGNATURE_BYTES:
            return SigningResult(
                code=CryptoResultCode.CRYPTO_PROVIDER_FAILURE,
                algorithm=algorithm,
                provider_identifier=self.provider_identifier,
                provider_version=self.provider_version,
                detail="provider returned invalid Ed25519 signature length",
            )

        return SigningResult(
            code=CryptoResultCode.VERIFIED,
            algorithm=algorithm,
            provider_identifier=self.provider_identifier,
            provider_version=self.provider_version,
            signature=signature,
        )


def generate_test_keypair() -> tuple[bytes, bytes]:
    """Generate one ephemeral Ed25519 keypair for host/test use only."""

    private = Ed25519PrivateKey.generate()

    private_bytes = private.private_bytes(
        encoding=serialization.Encoding.Raw,
        format=serialization.PrivateFormat.Raw,
        encryption_algorithm=serialization.NoEncryption(),
    )

    public_bytes = private.public_key().public_bytes(
        encoding=serialization.Encoding.Raw,
        format=serialization.PublicFormat.Raw,
    )

    return private_bytes, public_bytes