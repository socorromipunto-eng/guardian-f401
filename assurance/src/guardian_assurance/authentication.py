"""Guardian M15 authentication orchestration.

This module composes already-approved M15 boundaries:

- signed-wrapper structural validation;
- M14 assurance validation;
- deterministic transcript construction;
- trusted credential resolution;
- Ed25519 cryptographic verification.

It does not perform freshness evaluation, physical-truth evaluation,
policy authorization, or physical actuation.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum

from .crypto_provider import (
    CryptoResultCode,
    HostEd25519Provider,
)
from .signed_wrapper import (
    SignedWrapperError,
    parse_and_validate_signed_wrapper,
)
from .transcript import (
    TranscriptError,
    build_signed_assurance_transcript,
)
from .trust_store import (
    TrustStore,
    TrustStoreErrorCode,
    resolve_trusted_credential,
)


class AuthenticationResultCode(str, Enum):
    AUTHENTICATED = "AUTHENTICATED"
    IDENTITY_UNKNOWN = "IDENTITY_UNKNOWN"
    KEY_UNKNOWN = "KEY_UNKNOWN"
    KEY_REVOKED = "KEY_REVOKED"
    KEY_RETIRED_FOR_NEW_USE = "KEY_RETIRED_FOR_NEW_USE"
    ALGORITHM_UNSUPPORTED = "ALGORITHM_UNSUPPORTED"
    SIGNATURE_ENCODING_INVALID = "SIGNATURE_ENCODING_INVALID"
    SIGNATURE_INVALID = "SIGNATURE_INVALID"
    TRANSCRIPT_VERSION_UNSUPPORTED = "TRANSCRIPT_VERSION_UNSUPPORTED"
    TRUST_STORE_INVALID = "TRUST_STORE_INVALID"
    TRUST_STORE_UNAVAILABLE = "TRUST_STORE_UNAVAILABLE"
    CRYPTO_PROVIDER_UNAVAILABLE = "CRYPTO_PROVIDER_UNAVAILABLE"
    CRYPTO_PROVIDER_FAILURE = "CRYPTO_PROVIDER_FAILURE"
    INTERNAL_ERROR = "INTERNAL_ERROR"


@dataclass(frozen=True)
class AuthenticationResult:
    """Structured M15 authentication result.

    AUTHENTICATED establishes authenticated provenance under the selected
    trusted credential only.

    It does not establish freshness, physical truth, or authority.
    """

    code: AuthenticationResultCode
    producer_id: str | None = None
    key_id: str | None = None
    algorithm: str | None = None
    trust_code: TrustStoreErrorCode | None = None
    crypto_code: CryptoResultCode | None = None
    provider_identifier: str | None = None
    provider_version: str | None = None
    detail: str | None = None

    @property
    def authenticated(self) -> bool:
        return self.code is AuthenticationResultCode.AUTHENTICATED


_TRUST_RESULT_MAP = {
    TrustStoreErrorCode.IDENTITY_UNKNOWN:
        AuthenticationResultCode.IDENTITY_UNKNOWN,
    TrustStoreErrorCode.KEY_UNKNOWN:
        AuthenticationResultCode.KEY_UNKNOWN,
    TrustStoreErrorCode.KEY_REVOKED:
        AuthenticationResultCode.KEY_REVOKED,
    TrustStoreErrorCode.KEY_RETIRED_FOR_NEW_USE:
        AuthenticationResultCode.KEY_RETIRED_FOR_NEW_USE,
    TrustStoreErrorCode.ALGORITHM_UNSUPPORTED:
        AuthenticationResultCode.ALGORITHM_UNSUPPORTED,
}


_CRYPTO_RESULT_MAP = {
    CryptoResultCode.SIGNATURE_INVALID:
        AuthenticationResultCode.SIGNATURE_INVALID,
    CryptoResultCode.ALGORITHM_UNSUPPORTED:
        AuthenticationResultCode.ALGORITHM_UNSUPPORTED,
    CryptoResultCode.CRYPTO_PROVIDER_UNAVAILABLE:
        AuthenticationResultCode.CRYPTO_PROVIDER_UNAVAILABLE,
    CryptoResultCode.CRYPTO_PROVIDER_FAILURE:
        AuthenticationResultCode.CRYPTO_PROVIDER_FAILURE,
    CryptoResultCode.INTERNAL_ERROR:
        AuthenticationResultCode.INTERNAL_ERROR,
}


def _wrapper_security_result(
    exc: SignedWrapperError,
) -> AuthenticationResult | None:
    """Map only M15 authentication-specific wrapper failures.

    Structural/input-validation failures remain outside the M15
    authentication result namespace and are therefore re-raised by the
    caller.
    """

    value = exc.code.value

    mapping = {
        "SIGNATURE_ENCODING_INVALID":
            AuthenticationResultCode.SIGNATURE_ENCODING_INVALID,
        "TRANSCRIPT_VERSION_UNSUPPORTED":
            AuthenticationResultCode.TRANSCRIPT_VERSION_UNSUPPORTED,
        "ALGORITHM_UNSUPPORTED":
            AuthenticationResultCode.ALGORITHM_UNSUPPORTED,
    }

    code = mapping.get(value)

    if code is None:
        return None

    return AuthenticationResult(
        code=code,
        detail=exc.detail,
    )


def authenticate_signed_assurance(
    raw_wrapper: bytes,
    *,
    trust_store: TrustStore,
    provider: HostEd25519Provider,
) -> AuthenticationResult:
    """Authenticate one signed Guardian assurance message.

    Processing order:

        signed-wrapper validation
        -> trusted credential resolution
        -> deterministic transcript construction
        -> Ed25519 verification
        -> AUTHENTICATED only if all required boundaries succeed

    No automatic producer, key, algorithm, version, or trust fallback is
    permitted.
    """

    try:
        wrapper = parse_and_validate_signed_wrapper(raw_wrapper)
    except SignedWrapperError as exc:
        mapped = _wrapper_security_result(exc)
        if mapped is not None:
            return mapped

        # Existing input-validation failures remain in their originating
        # namespace rather than being mislabeled as authentication failure.
        raise

    producer_id = wrapper.assurance_object["producer_id"]
    key_id = wrapper.key_id
    algorithm = wrapper.signature_algorithm

    trust = resolve_trusted_credential(
        trust_store,
        producer_id=producer_id,
        key_id=key_id,
        algorithm=algorithm,
        for_new_authentication=True,
    )

    if not trust.resolved:
        mapped = _TRUST_RESULT_MAP.get(trust.code)

        if mapped is None:
            return AuthenticationResult(
                code=AuthenticationResultCode.INTERNAL_ERROR,
                producer_id=producer_id,
                key_id=key_id,
                algorithm=algorithm,
                trust_code=trust.code,
                detail="unmapped trust resolution result",
            )

        return AuthenticationResult(
            code=mapped,
            producer_id=producer_id,
            key_id=key_id,
            algorithm=algorithm,
            trust_code=trust.code,
            detail=trust.detail,
        )

    if trust.record is None:
        return AuthenticationResult(
            code=AuthenticationResultCode.INTERNAL_ERROR,
            producer_id=producer_id,
            key_id=key_id,
            algorithm=algorithm,
            trust_code=trust.code,
            detail="resolved trust result contained no credential record",
        )

    try:
        transcript = build_signed_assurance_transcript(
            wrapper.assurance_object_raw,
            wrapper.key_id,
        )
    except TranscriptError as exc:
        return AuthenticationResult(
            code=AuthenticationResultCode.INTERNAL_ERROR,
            producer_id=producer_id,
            key_id=key_id,
            algorithm=algorithm,
            trust_code=trust.code,
            detail=f"transcript invariant failure: {exc}",
        )

    crypto = provider.verify(
        algorithm=algorithm,
        public_key=trust.record.public_key_bytes,
        transcript=transcript,
        signature=wrapper.signature_bytes,
    )

    if crypto.code is not CryptoResultCode.VERIFIED:
        mapped = _CRYPTO_RESULT_MAP.get(crypto.code)

        if mapped is None:
            mapped = AuthenticationResultCode.INTERNAL_ERROR

        return AuthenticationResult(
            code=mapped,
            producer_id=producer_id,
            key_id=key_id,
            algorithm=algorithm,
            trust_code=trust.code,
            crypto_code=crypto.code,
            provider_identifier=crypto.provider_identifier,
            provider_version=crypto.provider_version,
            detail=crypto.detail,
        )

    return AuthenticationResult(
        code=AuthenticationResultCode.AUTHENTICATED,
        producer_id=producer_id,
        key_id=key_id,
        algorithm=algorithm,
        trust_code=trust.code,
        crypto_code=crypto.code,
        provider_identifier=crypto.provider_identifier,
        provider_version=crypto.provider_version,
    )
