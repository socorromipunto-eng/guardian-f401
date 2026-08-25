"""Cryptographic authentication for signed M15 authorization objects.

This module composes four already-separated facts:

    signed authorization validation
    -> authorization trust resolution
    -> deterministic authorization transcript
    -> Ed25519 verification

Only successful completion of all four establishes
AUTHORIZATION_AUTHENTICATED.

Authentication here does not perform replay acceptance, authorization
consumption, bootstrap state mutation, epoch transition, freshness, policy
authorization, or actuation.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum

from .authorization_objects import (
    BOOTSTRAP_TYPE,
    EPOCH_TRANSITION_TYPE,
)
from .authorization_transcript import (
    AuthorizationTranscriptError,
    build_authorization_transcript,
)
from .authorization_trust_store import (
    BOOTSTRAP_AUTHORITY,
    EPOCH_TRANSITION_AUTHORITY,
    AuthorizationTrustErrorCode,
    AuthorizationTrustStore,
    resolve_authorization_credential,
)
from .crypto_provider import CryptoResultCode, HostEd25519Provider
from .signed_authorization import (
    SignedAuthorizationError,
    SignedAuthorizationErrorCode,
    parse_and_validate_signed_authorization,
)


class AuthorizationAuthenticationResultCode(str, Enum):
    AUTHORIZATION_AUTHENTICATED = "AUTHORIZATION_AUTHENTICATED"
    SIGNED_AUTHORIZATION_INVALID = "SIGNED_AUTHORIZATION_INVALID"
    AUTHORITY_UNKNOWN = "AUTHORIZATION_AUTHORITY_UNKNOWN"
    KEY_UNKNOWN = "AUTHORIZATION_KEY_UNKNOWN"
    CAPABILITY_MISMATCH = "AUTHORIZATION_CAPABILITY_MISMATCH"
    KEY_RETIRED = "AUTHORIZATION_KEY_RETIRED"
    KEY_REVOKED = "AUTHORIZATION_KEY_REVOKED"
    ALGORITHM_UNSUPPORTED = "AUTHORIZATION_ALGORITHM_UNSUPPORTED"
    SIGNATURE_INVALID = "AUTHORIZATION_SIGNATURE_INVALID"
    CRYPTO_PROVIDER_FAILURE = "AUTHORIZATION_CRYPTO_PROVIDER_FAILURE"
    INTERNAL_ERROR = "AUTHORIZATION_AUTHENTICATION_INTERNAL_ERROR"


@dataclass(frozen=True)
class AuthorizationAuthenticationResult:
    """Machine-readable authorization authentication result.

    AUTHORIZATION_AUTHENTICATED establishes authenticated authorization
    provenance only. It does not establish replay acceptance, consumption,
    bootstrap approval, epoch-transition approval, freshness, policy
    authorization, or actuation permission.
    """

    code: AuthorizationAuthenticationResultCode
    authority_id: str | None = None
    key_id: str | None = None
    algorithm: str | None = None
    authorization_type: str | None = None
    authorization_id: str | None = None
    authorization_sequence: int | None = None
    producer_id: str | None = None
    trust_code: AuthorizationTrustErrorCode | None = None
    crypto_code: CryptoResultCode | None = None
    provider_identifier: str | None = None
    provider_version: str | None = None
    detail: str | None = None

    @property
    def authenticated(self) -> bool:
        return (
            self.code
            is AuthorizationAuthenticationResultCode.AUTHORIZATION_AUTHENTICATED
        )


_TRUST_RESULT_MAP = {
    AuthorizationTrustErrorCode.AUTHORITY_UNKNOWN:
        AuthorizationAuthenticationResultCode.AUTHORITY_UNKNOWN,
    AuthorizationTrustErrorCode.KEY_UNKNOWN:
        AuthorizationAuthenticationResultCode.KEY_UNKNOWN,
    AuthorizationTrustErrorCode.CAPABILITY_MISMATCH:
        AuthorizationAuthenticationResultCode.CAPABILITY_MISMATCH,
    AuthorizationTrustErrorCode.KEY_RETIRED_FOR_NEW_USE:
        AuthorizationAuthenticationResultCode.KEY_RETIRED,
    AuthorizationTrustErrorCode.KEY_REVOKED:
        AuthorizationAuthenticationResultCode.KEY_REVOKED,
    AuthorizationTrustErrorCode.ALGORITHM_UNSUPPORTED:
        AuthorizationAuthenticationResultCode.ALGORITHM_UNSUPPORTED,
}


_CRYPTO_RESULT_MAP = {
    CryptoResultCode.SIGNATURE_INVALID:
        AuthorizationAuthenticationResultCode.SIGNATURE_INVALID,
    CryptoResultCode.ALGORITHM_UNSUPPORTED:
        AuthorizationAuthenticationResultCode.ALGORITHM_UNSUPPORTED,
    CryptoResultCode.CRYPTO_PROVIDER_UNAVAILABLE:
        AuthorizationAuthenticationResultCode.CRYPTO_PROVIDER_FAILURE,
    CryptoResultCode.CRYPTO_PROVIDER_FAILURE:
        AuthorizationAuthenticationResultCode.CRYPTO_PROVIDER_FAILURE,
    CryptoResultCode.INTERNAL_ERROR:
        AuthorizationAuthenticationResultCode.INTERNAL_ERROR,
}


def _required_capability(authorization_type: str) -> str:
    if authorization_type == BOOTSTRAP_TYPE:
        return BOOTSTRAP_AUTHORITY

    if authorization_type == EPOCH_TRANSITION_TYPE:
        return EPOCH_TRANSITION_AUTHORITY

    raise ValueError("unsupported authorization_type")


def authenticate_authorization(
    raw_envelope: bytes,
    *,
    trust_store: AuthorizationTrustStore,
    provider: HostEd25519Provider,
) -> AuthorizationAuthenticationResult:
    """Authenticate one signed authorization envelope.

    Processing order is fixed:

        envelope validation
        -> capability derivation
        -> exact authorization trust resolution
        -> transcript construction from preserved authorization_object bytes
        -> Ed25519 verification
        -> AUTHORIZATION_AUTHENTICATED

    No ordinary assurance-trust fallback is permitted.
    """

    try:
        envelope = parse_and_validate_signed_authorization(raw_envelope)
    except SignedAuthorizationError as exc:
        return AuthorizationAuthenticationResult(
            code=AuthorizationAuthenticationResultCode.SIGNED_AUTHORIZATION_INVALID,
            detail=f"{exc.code.value}: {exc.detail}",
        )

    authorization = envelope.authorization_object
    authority_id = envelope.authority_id
    key_id = envelope.key_id
    algorithm = envelope.signature_algorithm
    authorization_type = authorization.authorization_type
    authorization_id = authorization.authorization_id
    authorization_sequence = authorization.authorization_sequence
    producer_id = authorization.producer_id

    try:
        capability = _required_capability(authorization_type)
    except ValueError as exc:
        return AuthorizationAuthenticationResult(
            code=AuthorizationAuthenticationResultCode.INTERNAL_ERROR,
            authority_id=authority_id,
            key_id=key_id,
            algorithm=algorithm,
            authorization_type=authorization_type,
            authorization_id=authorization_id,
            authorization_sequence=authorization_sequence,
            producer_id=producer_id,
            detail=str(exc),
        )

    trust = resolve_authorization_credential(
        trust_store,
        authority_id=authority_id,
        key_id=key_id,
        algorithm=algorithm,
        required_capability=capability,
        for_new_authorization=True,
    )

    if not trust.resolved:
        mapped = _TRUST_RESULT_MAP.get(trust.code)

        if mapped is None:
            mapped = AuthorizationAuthenticationResultCode.INTERNAL_ERROR

        return AuthorizationAuthenticationResult(
            code=mapped,
            authority_id=authority_id,
            key_id=key_id,
            algorithm=algorithm,
            authorization_type=authorization_type,
            authorization_id=authorization_id,
            authorization_sequence=authorization_sequence,
            producer_id=producer_id,
            trust_code=trust.code,
            detail=trust.detail,
        )

    if trust.record is None:
        return AuthorizationAuthenticationResult(
            code=AuthorizationAuthenticationResultCode.INTERNAL_ERROR,
            authority_id=authority_id,
            key_id=key_id,
            algorithm=algorithm,
            authorization_type=authorization_type,
            authorization_id=authorization_id,
            authorization_sequence=authorization_sequence,
            producer_id=producer_id,
            trust_code=trust.code,
            detail="resolved authorization trust result contained no credential",
        )

    try:
        transcript = build_authorization_transcript(
            envelope.authorization_object_raw,
            key_id=key_id,
            authority_id=authority_id,
            signature_algorithm=algorithm,
            transcript_version=envelope.signature_version,
        )
    except AuthorizationTranscriptError as exc:
        return AuthorizationAuthenticationResult(
            code=AuthorizationAuthenticationResultCode.INTERNAL_ERROR,
            authority_id=authority_id,
            key_id=key_id,
            algorithm=algorithm,
            authorization_type=authorization_type,
            authorization_id=authorization_id,
            authorization_sequence=authorization_sequence,
            producer_id=producer_id,
            trust_code=trust.code,
            detail=f"authorization transcript invariant failure: {exc}",
        )

    crypto = provider.verify(
        algorithm=algorithm,
        public_key=trust.record.public_key,
        transcript=transcript,
        signature=envelope.signature_bytes,
    )

    if crypto.code is not CryptoResultCode.VERIFIED:
        mapped = _CRYPTO_RESULT_MAP.get(crypto.code)

        if mapped is None:
            mapped = AuthorizationAuthenticationResultCode.INTERNAL_ERROR

        return AuthorizationAuthenticationResult(
            code=mapped,
            authority_id=authority_id,
            key_id=key_id,
            algorithm=algorithm,
            authorization_type=authorization_type,
            authorization_id=authorization_id,
            authorization_sequence=authorization_sequence,
            producer_id=producer_id,
            trust_code=trust.code,
            crypto_code=crypto.code,
            provider_identifier=crypto.provider_identifier,
            provider_version=crypto.provider_version,
            detail=crypto.detail,
        )

    return AuthorizationAuthenticationResult(
        code=AuthorizationAuthenticationResultCode.AUTHORIZATION_AUTHENTICATED,
        authority_id=authority_id,
        key_id=key_id,
        algorithm=algorithm,
        authorization_type=authorization_type,
        authorization_id=authorization_id,
        authorization_sequence=authorization_sequence,
        producer_id=producer_id,
        trust_code=trust.code,
        crypto_code=crypto.code,
        provider_identifier=crypto.provider_identifier,
        provider_version=crypto.provider_version,
    )
