"""Dedicated bounded trust domain for M15 authorization authorities.

This module resolves authorization credentials only.

A resolved credential is not an authenticated authorization, replay
candidate, consumed authorization, freshness result, policy authorization,
or actuation permission.
"""

from __future__ import annotations

import base64
import hashlib
import json
from dataclasses import dataclass
from enum import Enum
from typing import Any

from .authorization_fields import (
    AuthorizationFieldError,
    validate_authority_id,
)
from .transcript import TranscriptError, validate_key_id


AUTHORIZATION_TRUST_STORE_SCHEMA_VERSION = (
    "guardian-f401:m15:authorization-trust-store:v1"
)

AUTHORIZATION_TRUST_STORE_MAX_RAW_BYTES = 262_144
AUTHORIZATION_TRUST_STORE_MAX_RECORDS = 1_024
AUTHORIZATION_TRUST_RECORD_MAX_CAPABILITIES = 2

BOOTSTRAP_AUTHORITY = "BOOTSTRAP_AUTHORITY"
EPOCH_TRANSITION_AUTHORITY = "EPOCH_TRANSITION_AUTHORITY"

_CAPABILITIES = frozenset({
    BOOTSTRAP_AUTHORITY,
    EPOCH_TRANSITION_AUTHORITY,
})

_TOP_LEVEL_KEYS = frozenset({
    "schema_version",
    "environment",
    "records",
})

_RECORD_KEYS = frozenset({
    "authority_id",
    "key_id",
    "algorithm",
    "public_key",
    "lifecycle_state",
    "capabilities",
})

_LIFECYCLE_STATES = frozenset({
    "ACTIVE",
    "RETIRED",
    "REVOKED",
})

_ALGORITHM = "ed25519"
_ED25519_PUBLIC_KEY_BYTES = 32


class AuthorizationTrustErrorCode(str, Enum):
    RESOLVED = "AUTHORIZATION_TRUST_RESOLVED"
    RAW_LIMIT = "AUTHORIZATION_TRUST_RAW_LIMIT"
    INVALID_UTF8 = "AUTHORIZATION_TRUST_INVALID_UTF8"
    INVALID_JSON = "AUTHORIZATION_TRUST_INVALID_JSON"
    DUPLICATE_KEY = "AUTHORIZATION_TRUST_DUPLICATE_KEY"
    SCHEMA = "AUTHORIZATION_TRUST_INVALID"
    ENVIRONMENT_MISMATCH = "AUTHORIZATION_TRUST_ENVIRONMENT_MISMATCH"
    AUTHORITY_UNKNOWN = "AUTHORIZATION_AUTHORITY_UNKNOWN"
    KEY_UNKNOWN = "AUTHORIZATION_KEY_UNKNOWN"
    ALGORITHM_UNSUPPORTED = "AUTHORIZATION_ALGORITHM_UNSUPPORTED"
    CAPABILITY_MISMATCH = "AUTHORIZATION_CAPABILITY_MISMATCH"
    KEY_RETIRED_FOR_NEW_USE = "AUTHORIZATION_KEY_RETIRED_FOR_NEW_USE"
    KEY_REVOKED = "AUTHORIZATION_KEY_REVOKED"


class AuthorizationTrustError(ValueError):
    def __init__(
        self,
        code: AuthorizationTrustErrorCode,
        detail: str,
    ) -> None:
        super().__init__(detail)
        self.code = code
        self.detail = detail


@dataclass(frozen=True)
class AuthorizationTrustRecord:
    authority_id: str
    key_id: str
    algorithm: str
    public_key: bytes
    lifecycle_state: str
    capabilities: frozenset[str]


@dataclass(frozen=True)
class AuthorizationTrustStore:
    schema_version: str
    environment: str
    records: tuple[AuthorizationTrustRecord, ...]
    raw_sha256: str


@dataclass(frozen=True)
class AuthorizationTrustResolutionResult:
    code: AuthorizationTrustErrorCode
    record: AuthorizationTrustRecord | None = None
    detail: str | None = None

    @property
    def resolved(self) -> bool:
        return self.code is AuthorizationTrustErrorCode.RESOLVED


def _reject_constant(value: str) -> None:
    raise AuthorizationTrustError(
        AuthorizationTrustErrorCode.INVALID_JSON,
        f"non-standard JSON constant: {value}",
    )


def _pairs_hook(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    value: dict[str, Any] = {}

    for key, item in pairs:
        if key in value:
            raise AuthorizationTrustError(
                AuthorizationTrustErrorCode.DUPLICATE_KEY,
                f"duplicate member {key!r}",
            )

        value[key] = item

    return value


def _decode_public_key(value: Any) -> bytes:
    if not isinstance(value, str):
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.SCHEMA,
            "public_key must be a string",
        )

    if "=" in value:
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.SCHEMA,
            "public_key must use unpadded base64url",
        )

    try:
        encoded = value.encode("ascii")
    except UnicodeEncodeError as exc:
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.SCHEMA,
            "public_key must be ASCII",
        ) from exc

    padding = b"=" * ((4 - len(encoded) % 4) % 4)

    try:
        decoded = base64.b64decode(
            encoded + padding,
            altchars=b"-_",
            validate=True,
        )
    except (ValueError, base64.binascii.Error) as exc:
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.SCHEMA,
            "invalid public_key encoding",
        ) from exc

    if len(decoded) != _ED25519_PUBLIC_KEY_BYTES:
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.SCHEMA,
            "public_key must decode to 32 bytes",
        )

    return decoded


def _validate_record(value: Any) -> AuthorizationTrustRecord:
    if not isinstance(value, dict) or frozenset(value) != _RECORD_KEYS:
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.SCHEMA,
            "authorization trust record members do not match schema",
        )

    try:
        authority_id = validate_authority_id(value["authority_id"])
    except AuthorizationFieldError as exc:
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.SCHEMA,
            "invalid authority_id",
        ) from exc

    try:
        validate_key_id(value["key_id"])
    except TranscriptError as exc:
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.SCHEMA,
            "invalid key_id",
        ) from exc

    key_id = value["key_id"]

    algorithm = value["algorithm"]
    if algorithm != _ALGORITHM:
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.SCHEMA,
            "unsupported authorization trust algorithm",
        )

    public_key = _decode_public_key(value["public_key"])

    lifecycle_state = value["lifecycle_state"]
    if lifecycle_state not in _LIFECYCLE_STATES:
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.SCHEMA,
            "invalid lifecycle_state",
        )

    capabilities_value = value["capabilities"]
    if not isinstance(capabilities_value, list):
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.SCHEMA,
            "capabilities must be an array",
        )

    if not 1 <= len(capabilities_value) <= AUTHORIZATION_TRUST_RECORD_MAX_CAPABILITIES:
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.SCHEMA,
            "invalid capability count",
        )

    if any(not isinstance(item, str) for item in capabilities_value):
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.SCHEMA,
            "capability must be a string",
        )

    capabilities = frozenset(capabilities_value)

    if len(capabilities) != len(capabilities_value):
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.SCHEMA,
            "duplicate capability",
        )

    if not capabilities <= _CAPABILITIES:
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.SCHEMA,
            "unknown authorization capability",
        )

    return AuthorizationTrustRecord(
        authority_id=authority_id,
        key_id=key_id,
        algorithm=algorithm,
        public_key=public_key,
        lifecycle_state=lifecycle_state,
        capabilities=capabilities,
    )


def parse_and_validate_authorization_trust_store(
    raw: bytes,
    *,
    expected_environment: str,
) -> AuthorizationTrustStore:
    if not isinstance(raw, bytes):
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.SCHEMA,
            "input must be bytes",
        )

    if len(raw) > AUTHORIZATION_TRUST_STORE_MAX_RAW_BYTES:
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.RAW_LIMIT,
            "authorization trust store raw-byte limit exceeded",
        )

    try:
        text = raw.decode("utf-8", errors="strict")
    except UnicodeDecodeError as exc:
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.INVALID_UTF8,
            "authorization trust store is not strict UTF-8",
        ) from exc

    try:
        value = json.loads(
            text,
            object_pairs_hook=_pairs_hook,
            parse_constant=_reject_constant,
        )
    except AuthorizationTrustError:
        raise
    except (json.JSONDecodeError, ValueError) as exc:
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.INVALID_JSON,
            "authorization trust store is not valid JSON",
        ) from exc

    if not isinstance(value, dict) or frozenset(value) != _TOP_LEVEL_KEYS:
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.SCHEMA,
            "authorization trust store members do not match schema",
        )

    if value["schema_version"] != AUTHORIZATION_TRUST_STORE_SCHEMA_VERSION:
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.SCHEMA,
            "unsupported authorization trust-store schema_version",
        )

    environment = value["environment"]
    if not isinstance(environment, str) or environment != expected_environment:
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.ENVIRONMENT_MISMATCH,
            "authorization trust-store environment mismatch",
        )

    records_value = value["records"]
    if not isinstance(records_value, list):
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.SCHEMA,
            "records must be an array",
        )

    if len(records_value) > AUTHORIZATION_TRUST_STORE_MAX_RECORDS:
        raise AuthorizationTrustError(
            AuthorizationTrustErrorCode.SCHEMA,
            "authorization trust-store record limit exceeded",
        )

    records = tuple(_validate_record(item) for item in records_value)

    exact_credentials: set[tuple[str, str, str]] = set()
    authority_key_pairs: set[tuple[str, str]] = set()

    for record in records:
        exact = (record.authority_id, record.key_id, record.algorithm)
        pair = (record.authority_id, record.key_id)

        if exact in exact_credentials:
            raise AuthorizationTrustError(
                AuthorizationTrustErrorCode.SCHEMA,
                "duplicate authority_id + key_id + algorithm mapping",
            )

        if pair in authority_key_pairs:
            raise AuthorizationTrustError(
                AuthorizationTrustErrorCode.SCHEMA,
                "duplicate authority_id + key_id mapping",
            )

        exact_credentials.add(exact)
        authority_key_pairs.add(pair)

    canonical = json.dumps(
        value,
        sort_keys=True,
        separators=(",", ":"),
        ensure_ascii=True,
    ).encode("utf-8")

    return AuthorizationTrustStore(
        schema_version=AUTHORIZATION_TRUST_STORE_SCHEMA_VERSION,
        environment=environment,
        records=records,
        raw_sha256=hashlib.sha256(raw).hexdigest().upper(),
    )


def resolve_authorization_credential(
    trust_store: AuthorizationTrustStore,
    *,
    authority_id: str,
    key_id: str,
    algorithm: str,
    required_capability: str,
    for_new_authorization: bool = True,
) -> AuthorizationTrustResolutionResult:
    if required_capability not in _CAPABILITIES:
        return AuthorizationTrustResolutionResult(
            AuthorizationTrustErrorCode.CAPABILITY_MISMATCH,
            detail="unsupported required capability",
        )

    authority_records = [
        record for record in trust_store.records
        if record.authority_id == authority_id
    ]

    if not authority_records:
        return AuthorizationTrustResolutionResult(
            AuthorizationTrustErrorCode.AUTHORITY_UNKNOWN,
            detail="authority_id not found",
        )

    key_records = [
        record for record in authority_records
        if record.key_id == key_id
    ]

    if not key_records:
        return AuthorizationTrustResolutionResult(
            AuthorizationTrustErrorCode.KEY_UNKNOWN,
            detail="key_id not found for authority",
        )

    matches = [
        record for record in key_records
        if record.algorithm == algorithm
    ]

    if len(matches) != 1:
        return AuthorizationTrustResolutionResult(
            AuthorizationTrustErrorCode.ALGORITHM_UNSUPPORTED,
            detail="exact authority/key/algorithm mapping not found",
        )

    record = matches[0]

    if record.lifecycle_state == "REVOKED":
        return AuthorizationTrustResolutionResult(
            AuthorizationTrustErrorCode.KEY_REVOKED,
            record=record,
            detail="authorization credential is revoked",
        )

    if record.lifecycle_state == "RETIRED" and for_new_authorization:
        return AuthorizationTrustResolutionResult(
            AuthorizationTrustErrorCode.KEY_RETIRED_FOR_NEW_USE,
            record=record,
            detail="retired authorization credential is not valid for new use",
        )

    if required_capability not in record.capabilities:
        return AuthorizationTrustResolutionResult(
            AuthorizationTrustErrorCode.CAPABILITY_MISMATCH,
            record=record,
            detail="authorization credential lacks required capability",
        )

    return AuthorizationTrustResolutionResult(
        AuthorizationTrustErrorCode.RESOLVED,
        record=record,
    )
