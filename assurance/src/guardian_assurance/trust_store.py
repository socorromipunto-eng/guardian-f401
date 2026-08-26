"""Guardian M15 trust-store parsing and credential resolution.

This module validates an external trust store and resolves trusted
credential records.

It does not perform signature verification and does not produce
AUTHENTICATED or authority decisions.
"""

from __future__ import annotations

import base64
import binascii
import hashlib
import json
from dataclasses import dataclass
from enum import Enum
from typing import Any

import rfc8785

from .transcript import TranscriptError, validate_key_id
from .validation import AssuranceError, validate_producer_id


TRUST_STORE_SCHEMA_VERSION = "guardian-f401:m15:trust-store:v1"
TRUST_STORE_MAX_RAW_BYTES = 1_048_576
TRUST_STORE_MAX_RECORDS = 4_096
ED25519_ALGORITHM = "ed25519"
ED25519_PUBLIC_KEY_BYTES = 32

_TOP_LEVEL_KEYS = frozenset({"schema_version", "environment", "records"})
_RECORD_KEYS = frozenset(
    {"producer_id", "key_id", "algorithm", "public_key", "lifecycle_state"}
)
_ENVIRONMENTS = frozenset({"TEST", "PRODUCTION"})
_LIFECYCLE_STATES = frozenset({"ACTIVE", "RETIRED", "REVOKED"})
_BASE64URL_ALPHABET = frozenset(
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789-_"
)


class TrustStoreErrorCode(str, Enum):
    RAW_LIMIT = "TRUST_STORE_RAW_LIMIT"
    INVALID_UTF8 = "TRUST_STORE_INVALID_UTF8"
    INVALID_JSON = "TRUST_STORE_INVALID_JSON"
    DUPLICATE_KEY = "TRUST_STORE_DUPLICATE_KEY"
    SCHEMA = "TRUST_STORE_INVALID"
    ENVIRONMENT_MISMATCH = "TRUST_STORE_INVALID"
    RECORD_LIMIT = "TRUST_STORE_RECORD_LIMIT"
    IDENTITY_UNKNOWN = "IDENTITY_UNKNOWN"
    KEY_UNKNOWN = "KEY_UNKNOWN"
    ALGORITHM_UNSUPPORTED = "ALGORITHM_UNSUPPORTED"
    KEY_REVOKED = "KEY_REVOKED"
    KEY_RETIRED_FOR_NEW_USE = "KEY_RETIRED_FOR_NEW_USE"
    RESOLVED = "RESOLVED"


class TrustStoreError(ValueError):
    def __init__(self, code: TrustStoreErrorCode, detail: str) -> None:
        super().__init__(f"{code.value}: {detail}")
        self.code = code
        self.detail = detail


@dataclass(frozen=True)
class TrustRecord:
    producer_id: str
    key_id: str
    algorithm: str
    public_key: str
    public_key_bytes: bytes
    lifecycle_state: str


@dataclass(frozen=True)
class TrustStore:
    schema_version: str
    environment: str
    records: tuple[TrustRecord, ...]
    raw_sha256: str
    canonical_sha256: str


@dataclass(frozen=True)
class TrustResolutionResult:
    code: TrustStoreErrorCode
    record: TrustRecord | None = None
    detail: str | None = None

    @property
    def resolved(self) -> bool:
        return self.code is TrustStoreErrorCode.RESOLVED and self.record is not None


def _reject_constant(value: str) -> None:
    raise ValueError(f"non-standard JSON constant: {value}")


def _pairs_hook(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}

    for key, value in pairs:
        if key in result:
            raise TrustStoreError(
                TrustStoreErrorCode.DUPLICATE_KEY,
                f"duplicate JSON member: {key}",
            )
        result[key] = value

    return result


def _decode_public_key(value: Any) -> bytes:
    if not isinstance(value, str):
        raise TrustStoreError(
            TrustStoreErrorCode.SCHEMA,
            "public_key must be a string",
        )

    if "=" in value:
        raise TrustStoreError(
            TrustStoreErrorCode.SCHEMA,
            "public_key padding is prohibited",
        )

    if not value or any(char not in _BASE64URL_ALPHABET for char in value):
        raise TrustStoreError(
            TrustStoreErrorCode.SCHEMA,
            "public_key is not canonical base64url",
        )

    padding = "=" * ((4 - (len(value) % 4)) % 4)

    try:
        decoded = base64.b64decode(
            value + padding,
            altchars=b"-_",
            validate=True,
        )
    except (binascii.Error, ValueError) as exc:
        raise TrustStoreError(
            TrustStoreErrorCode.SCHEMA,
            "public_key base64url decoding failed",
        ) from exc

    if len(decoded) != ED25519_PUBLIC_KEY_BYTES:
        raise TrustStoreError(
            TrustStoreErrorCode.SCHEMA,
            "Ed25519 public key must decode to exactly 32 bytes",
        )

    canonical = base64.urlsafe_b64encode(decoded).rstrip(b"=").decode("ascii")

    if canonical != value:
        raise TrustStoreError(
            TrustStoreErrorCode.SCHEMA,
            "public_key encoding is not canonical",
        )

    return decoded


def _validate_record(value: Any) -> TrustRecord:
    if not isinstance(value, dict) or frozenset(value) != _RECORD_KEYS:
        raise TrustStoreError(
            TrustStoreErrorCode.SCHEMA,
            "TrustRecord must contain exactly the defined members",
        )

    try:
        producer_id = validate_producer_id(value["producer_id"])
    except AssuranceError as exc:
        raise TrustStoreError(
            TrustStoreErrorCode.SCHEMA,
            "invalid producer_id",
        ) from exc

    try:
        validate_key_id(value["key_id"])
        key_id = value["key_id"]
    except (TranscriptError, TypeError) as exc:
        raise TrustStoreError(
            TrustStoreErrorCode.SCHEMA,
            "invalid key_id",
        ) from exc

    algorithm = value["algorithm"]
    if algorithm != ED25519_ALGORITHM:
        raise TrustStoreError(
            TrustStoreErrorCode.ALGORITHM_UNSUPPORTED,
            "unsupported trust-record algorithm",
        )

    public_key = value["public_key"]
    public_key_bytes = _decode_public_key(public_key)

    lifecycle_state = value["lifecycle_state"]
    if lifecycle_state not in _LIFECYCLE_STATES:
        raise TrustStoreError(
            TrustStoreErrorCode.SCHEMA,
            "invalid lifecycle_state",
        )

    return TrustRecord(
        producer_id=producer_id,
        key_id=key_id,
        algorithm=algorithm,
        public_key=public_key,
        public_key_bytes=public_key_bytes,
        lifecycle_state=lifecycle_state,
    )


def parse_and_validate_trust_store(
    raw: bytes,
    *,
    expected_environment: str,
) -> TrustStore:
    if not isinstance(raw, bytes):
        raise TrustStoreError(
            TrustStoreErrorCode.SCHEMA,
            "trust-store input must be bytes",
        )

    if len(raw) > TRUST_STORE_MAX_RAW_BYTES:
        raise TrustStoreError(
            TrustStoreErrorCode.RAW_LIMIT,
            "trust store exceeds 1,048,576-byte limit",
        )

    if expected_environment not in _ENVIRONMENTS:
        raise TrustStoreError(
            TrustStoreErrorCode.SCHEMA,
            "invalid configured verifier environment",
        )

    try:
        text = raw.decode("utf-8", errors="strict")
    except UnicodeDecodeError as exc:
        raise TrustStoreError(
            TrustStoreErrorCode.INVALID_UTF8,
            "trust store is not valid UTF-8",
        ) from exc

    try:
        value = json.loads(
            text,
            object_pairs_hook=_pairs_hook,
            parse_constant=_reject_constant,
        )
    except TrustStoreError:
        raise
    except (json.JSONDecodeError, ValueError, RecursionError) as exc:
        raise TrustStoreError(
            TrustStoreErrorCode.INVALID_JSON,
            "trust store is not valid strict JSON",
        ) from exc

    if not isinstance(value, dict) or frozenset(value) != _TOP_LEVEL_KEYS:
        raise TrustStoreError(
            TrustStoreErrorCode.SCHEMA,
            "trust store must contain exactly schema_version, environment, records",
        )

    if value["schema_version"] != TRUST_STORE_SCHEMA_VERSION:
        raise TrustStoreError(
            TrustStoreErrorCode.SCHEMA,
            "unsupported trust-store schema_version",
        )

    environment = value["environment"]
    if environment not in _ENVIRONMENTS:
        raise TrustStoreError(
            TrustStoreErrorCode.SCHEMA,
            "invalid trust-store environment",
        )

    if environment != expected_environment:
        raise TrustStoreError(
            TrustStoreErrorCode.ENVIRONMENT_MISMATCH,
            "trust-store environment does not match verifier configuration",
        )

    raw_records = value["records"]
    if not isinstance(raw_records, list):
        raise TrustStoreError(
            TrustStoreErrorCode.SCHEMA,
            "records must be an array",
        )

    if len(raw_records) > TRUST_STORE_MAX_RECORDS:
        raise TrustStoreError(
            TrustStoreErrorCode.RECORD_LIMIT,
            "trust store exceeds 4,096-record limit",
        )

    records = tuple(_validate_record(item) for item in raw_records)

    seen_exact: set[tuple[str, str, str]] = set()
    seen_pair: dict[tuple[str, str], TrustRecord] = {}

    for record in records:
        exact = (record.producer_id, record.key_id, record.algorithm)
        pair = (record.producer_id, record.key_id)

        if exact in seen_exact:
            raise TrustStoreError(
                TrustStoreErrorCode.SCHEMA,
                "duplicate credential record",
            )

        seen_exact.add(exact)

        previous = seen_pair.get(pair)
        if previous is not None:
            raise TrustStoreError(
                TrustStoreErrorCode.SCHEMA,
                "duplicate producer_id + key_id mapping",
            )

        seen_pair[pair] = record

    try:
        canonical = rfc8785.dumps(value)
    except Exception as exc:
        raise TrustStoreError(
            TrustStoreErrorCode.SCHEMA,
            "RFC 8785 canonicalization failed",
        ) from exc

    return TrustStore(
        schema_version=TRUST_STORE_SCHEMA_VERSION,
        environment=environment,
        records=records,
        raw_sha256=hashlib.sha256(raw).hexdigest().upper(),
        canonical_sha256=hashlib.sha256(canonical).hexdigest().upper(),
    )


def resolve_trusted_credential(
    store: TrustStore,
    *,
    producer_id: str,
    key_id: str,
    algorithm: str,
    for_new_authentication: bool = True,
) -> TrustResolutionResult:
    if algorithm != ED25519_ALGORITHM:
        return TrustResolutionResult(
            TrustStoreErrorCode.ALGORITHM_UNSUPPORTED,
            detail="unsupported algorithm",
        )

    producer_records = [
        record for record in store.records
        if record.producer_id == producer_id
    ]

    if not producer_records:
        return TrustResolutionResult(
            TrustStoreErrorCode.IDENTITY_UNKNOWN,
            detail="producer_id not found",
        )

    key_records = [
        record for record in producer_records
        if record.key_id == key_id
    ]

    if not key_records:
        return TrustResolutionResult(
            TrustStoreErrorCode.KEY_UNKNOWN,
            detail="key_id not found for producer",
        )

    matches = [
        record for record in key_records
        if record.algorithm == algorithm
    ]

    if len(matches) != 1:
        return TrustResolutionResult(
            TrustStoreErrorCode.ALGORITHM_UNSUPPORTED,
            detail="exact producer/key/algorithm mapping not found",
        )

    record = matches[0]

    if record.lifecycle_state == "REVOKED":
        return TrustResolutionResult(
            TrustStoreErrorCode.KEY_REVOKED,
            record=record,
            detail="credential is revoked",
        )

    if record.lifecycle_state == "RETIRED" and for_new_authentication:
        return TrustResolutionResult(
            TrustStoreErrorCode.KEY_RETIRED_FOR_NEW_USE,
            record=record,
            detail="retired credential is not valid for new authentication",
        )

    return TrustResolutionResult(
        TrustStoreErrorCode.RESOLVED,
        record=record,
    )
