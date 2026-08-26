"""Closed M15 bootstrap and epoch-transition authorization objects.

This module performs bounded structural and semantic validation only.

A validated authorization object is not authenticated, replay-accepted,
consumed, fresh, authorized for general policy, or permitted to actuate.
"""

from __future__ import annotations

import json
from dataclasses import dataclass
from enum import Enum
from typing import Any

from .authorization_fields import (
    UINT64_MAX,
    AuthorizationFieldError,
    parse_uint64_decimal_wire,
    validate_authority_id,
    validate_authorization_id,
    validate_authorization_sequence,
)
from .validation import (
    AssuranceError,
    validate_logical_time,
    validate_producer_epoch,
    validate_producer_id,
)


BOOTSTRAP_SCHEMA_VERSION = "guardian-f401:m15:bootstrap-authorization:v1"
EPOCH_TRANSITION_SCHEMA_VERSION = "guardian-f401:m15:epoch-transition-authorization:v1"

BOOTSTRAP_TYPE = "bootstrap"
EPOCH_TRANSITION_TYPE = "epoch-transition"

HIGH_WATER_UNSET = "HIGH_WATER_UNSET"
HIGH_WATER_ESTABLISHED = "HIGH_WATER_ESTABLISHED"

# A standalone authorization object cannot exceed the frozen raw signed
# authorization-envelope maximum. Canonical-size enforcement belongs to
# the later canonical/transcript slice.
AUTHORIZATION_OBJECT_MAX_RAW_BYTES = 16_384

AUTHORIZATION_MAX_DEPTH = 8
AUTHORIZATION_MAX_NODES = 256
AUTHORIZATION_MAX_OBJECT_MEMBERS = 32
AUTHORIZATION_MAX_ARRAY_ITEMS = 16
AUTHORIZATION_MAX_STRING_UTF8_BYTES = 512

_COMMON_KEYS = frozenset({
    "schema_version",
    "authorization_type",
    "authorization_id",
    "authority_id",
    "producer_id",
    "authorization_sequence",
})

_BOOTSTRAP_KEYS = _COMMON_KEYS | frozenset({
    "producer_epoch",
    "initial_high_water_state",
    "initial_logical_time",
})

_EPOCH_TRANSITION_KEYS = _COMMON_KEYS | frozenset({
    "from_epoch",
    "to_epoch",
    "transition_sequence",
})


class AuthorizationObjectErrorCode(str, Enum):
    RAW_LIMIT = "AUTHORIZATION_OBJECT_RAW_LIMIT"
    INVALID_UTF8 = "AUTHORIZATION_OBJECT_INVALID_UTF8"
    INVALID_JSON = "AUTHORIZATION_OBJECT_INVALID_JSON"
    DUPLICATE_KEY = "AUTHORIZATION_OBJECT_DUPLICATE_KEY"
    STRUCTURE_LIMIT = "AUTHORIZATION_OBJECT_STRUCTURE_LIMIT"
    SCHEMA = "AUTHORIZATION_OBJECT_INVALID"


class AuthorizationObjectError(ValueError):
    """Structured failure for one authorization-object contract."""

    def __init__(
        self,
        code: AuthorizationObjectErrorCode,
        detail: str,
    ) -> None:
        super().__init__(detail)
        self.code = code
        self.detail = detail


@dataclass(frozen=True)
class BootstrapAuthorization:
    schema_version: str
    authorization_type: str
    authorization_id: str
    authority_id: str
    producer_id: str
    authorization_sequence: int
    producer_epoch: str
    initial_high_water_state: str
    initial_logical_time: int | None


@dataclass(frozen=True)
class EpochTransitionAuthorization:
    schema_version: str
    authorization_type: str
    authorization_id: str
    authority_id: str
    producer_id: str
    authorization_sequence: int
    from_epoch: str
    to_epoch: str
    transition_sequence: int


ValidatedAuthorizationObject = BootstrapAuthorization | EpochTransitionAuthorization


def _reject_constant(value: str) -> None:
    raise AuthorizationObjectError(
        AuthorizationObjectErrorCode.INVALID_JSON,
        f"non-standard JSON constant: {value}",
    )


def _pairs_hook(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    value: dict[str, Any] = {}

    for key, item in pairs:
        if key in value:
            raise AuthorizationObjectError(
                AuthorizationObjectErrorCode.DUPLICATE_KEY,
                f"duplicate member {key!r}",
            )

        value[key] = item

    return value


def _validate_structure(value: Any) -> None:
    nodes = 0

    def bounded_string(item: str) -> None:
        if len(item.encode("utf-8")) > AUTHORIZATION_MAX_STRING_UTF8_BYTES:
            raise AuthorizationObjectError(
                AuthorizationObjectErrorCode.STRUCTURE_LIMIT,
                "UTF-8 string limit exceeded",
            )

    def walk(item: Any, depth: int) -> None:
        nonlocal nodes

        nodes += 1
        if nodes > AUTHORIZATION_MAX_NODES:
            raise AuthorizationObjectError(
                AuthorizationObjectErrorCode.STRUCTURE_LIMIT,
                "structural node limit exceeded",
            )

        if depth > AUTHORIZATION_MAX_DEPTH:
            raise AuthorizationObjectError(
                AuthorizationObjectErrorCode.STRUCTURE_LIMIT,
                "nesting depth limit exceeded",
            )

        if isinstance(item, str):
            bounded_string(item)
            return

        if item is None or isinstance(item, bool):
            return

        if isinstance(item, int):
            return

        if isinstance(item, float):
            raise AuthorizationObjectError(
                AuthorizationObjectErrorCode.SCHEMA,
                "floating-point values are prohibited",
            )

        if isinstance(item, dict):
            if len(item) > AUTHORIZATION_MAX_OBJECT_MEMBERS:
                raise AuthorizationObjectError(
                    AuthorizationObjectErrorCode.STRUCTURE_LIMIT,
                    "object-member limit exceeded",
                )

            for key, child in item.items():
                bounded_string(key)
                walk(child, depth + 1)
            return

        if isinstance(item, list):
            if len(item) > AUTHORIZATION_MAX_ARRAY_ITEMS:
                raise AuthorizationObjectError(
                    AuthorizationObjectErrorCode.STRUCTURE_LIMIT,
                    "array-item limit exceeded",
                )

            for child in item:
                walk(child, depth + 1)
            return

        raise AuthorizationObjectError(
            AuthorizationObjectErrorCode.SCHEMA,
            "unsupported JSON value",
        )

    walk(value, 0)


def _require_exact_members(
    value: dict[str, Any],
    expected: frozenset[str],
    authorization_type: str,
) -> None:
    if frozenset(value) != expected:
        raise AuthorizationObjectError(
            AuthorizationObjectErrorCode.SCHEMA,
            f"{authorization_type} authorization members do not match schema",
        )


def _exact_text(value: Any, field: str, expected: str) -> str:
    if not isinstance(value, str) or value != expected:
        raise AuthorizationObjectError(
            AuthorizationObjectErrorCode.SCHEMA,
            f"invalid {field}",
        )

    return value


def _field_call(function: Any, value: Any, field: str) -> Any:
    try:
        return function(value)
    except (AuthorizationFieldError, AssuranceError) as exc:
        raise AuthorizationObjectError(
            AuthorizationObjectErrorCode.SCHEMA,
            f"invalid {field}",
        ) from exc


def _parse_authorization_sequence_wire(value: Any) -> int:
    return parse_uint64_decimal_wire(
        value,
        "authorization_sequence",
    )


def _validate_transition_sequence(value: Any) -> int:
    try:
        return parse_uint64_decimal_wire(
            value,
            "transition_sequence",
        )
    except AuthorizationFieldError as exc:
        raise AuthorizationObjectError(
            AuthorizationObjectErrorCode.SCHEMA,
            "invalid transition_sequence",
        ) from exc




def _validate_bootstrap(value: dict[str, Any]) -> BootstrapAuthorization:
    _require_exact_members(value, _BOOTSTRAP_KEYS, BOOTSTRAP_TYPE)

    schema_version = _exact_text(
        value["schema_version"],
        "schema_version",
        BOOTSTRAP_SCHEMA_VERSION,
    )
    authorization_type = _exact_text(
        value["authorization_type"],
        "authorization_type",
        BOOTSTRAP_TYPE,
    )

    authorization_id = _field_call(
        validate_authorization_id,
        value["authorization_id"],
        "authorization_id",
    )
    authority_id = _field_call(
        validate_authority_id,
        value["authority_id"],
        "authority_id",
    )
    producer_id = _field_call(
        validate_producer_id,
        value["producer_id"],
        "producer_id",
    )
    authorization_sequence = _field_call(
        _parse_authorization_sequence_wire,
        value["authorization_sequence"],
        "authorization_sequence",
    )
    producer_epoch = _field_call(
        validate_producer_epoch,
        value["producer_epoch"],
        "producer_epoch",
    )

    high_water = value["initial_high_water_state"]
    logical_time = value["initial_logical_time"]

    if high_water == HIGH_WATER_UNSET:
        if logical_time is not None:
            raise AuthorizationObjectError(
                AuthorizationObjectErrorCode.SCHEMA,
                "HIGH_WATER_UNSET requires null initial_logical_time",
            )

    elif high_water == HIGH_WATER_ESTABLISHED:
        logical_time = _field_call(
            validate_logical_time,
            logical_time,
            "initial_logical_time",
        )

    else:
        raise AuthorizationObjectError(
            AuthorizationObjectErrorCode.SCHEMA,
            "invalid initial_high_water_state",
        )

    return BootstrapAuthorization(
        schema_version=schema_version,
        authorization_type=authorization_type,
        authorization_id=authorization_id,
        authority_id=authority_id,
        producer_id=producer_id,
        authorization_sequence=authorization_sequence,
        producer_epoch=producer_epoch,
        initial_high_water_state=high_water,
        initial_logical_time=logical_time,
    )


def _validate_epoch_transition(
    value: dict[str, Any],
) -> EpochTransitionAuthorization:
    _require_exact_members(
        value,
        _EPOCH_TRANSITION_KEYS,
        EPOCH_TRANSITION_TYPE,
    )

    schema_version = _exact_text(
        value["schema_version"],
        "schema_version",
        EPOCH_TRANSITION_SCHEMA_VERSION,
    )
    authorization_type = _exact_text(
        value["authorization_type"],
        "authorization_type",
        EPOCH_TRANSITION_TYPE,
    )

    authorization_id = _field_call(
        validate_authorization_id,
        value["authorization_id"],
        "authorization_id",
    )
    authority_id = _field_call(
        validate_authority_id,
        value["authority_id"],
        "authority_id",
    )
    producer_id = _field_call(
        validate_producer_id,
        value["producer_id"],
        "producer_id",
    )
    authorization_sequence = _field_call(
        _parse_authorization_sequence_wire,
        value["authorization_sequence"],
        "authorization_sequence",
    )
    from_epoch = _field_call(
        validate_producer_epoch,
        value["from_epoch"],
        "from_epoch",
    )
    to_epoch = _field_call(
        validate_producer_epoch,
        value["to_epoch"],
        "to_epoch",
    )

    if from_epoch == to_epoch:
        raise AuthorizationObjectError(
            AuthorizationObjectErrorCode.SCHEMA,
            "from_epoch must differ from to_epoch",
        )

    transition_sequence = _validate_transition_sequence(
        value["transition_sequence"]
    )

    return EpochTransitionAuthorization(
        schema_version=schema_version,
        authorization_type=authorization_type,
        authorization_id=authorization_id,
        authority_id=authority_id,
        producer_id=producer_id,
        authorization_sequence=authorization_sequence,
        from_epoch=from_epoch,
        to_epoch=to_epoch,
        transition_sequence=transition_sequence,
    )


def validate_authorization_object(
    value: Any,
) -> ValidatedAuthorizationObject:
    """Validate one already-decoded closed authorization object."""

    if not isinstance(value, dict):
        raise AuthorizationObjectError(
            AuthorizationObjectErrorCode.SCHEMA,
            "authorization object must be an object",
        )

    authorization_type = value.get("authorization_type")

    if authorization_type == BOOTSTRAP_TYPE:
        return _validate_bootstrap(value)

    if authorization_type == EPOCH_TRANSITION_TYPE:
        return _validate_epoch_transition(value)

    raise AuthorizationObjectError(
        AuthorizationObjectErrorCode.SCHEMA,
        "unsupported authorization_type",
    )


def parse_and_validate_authorization_object(
    raw: bytes,
) -> ValidatedAuthorizationObject:
    """Parse one bounded authorization object without authenticating it."""

    if not isinstance(raw, bytes):
        raise AuthorizationObjectError(
            AuthorizationObjectErrorCode.SCHEMA,
            "input must be bytes",
        )

    if len(raw) > AUTHORIZATION_OBJECT_MAX_RAW_BYTES:
        raise AuthorizationObjectError(
            AuthorizationObjectErrorCode.RAW_LIMIT,
            "authorization object raw-byte limit exceeded",
        )

    try:
        text = raw.decode("utf-8", errors="strict")
    except UnicodeDecodeError as exc:
        raise AuthorizationObjectError(
            AuthorizationObjectErrorCode.INVALID_UTF8,
            "authorization object is not strict UTF-8",
        ) from exc

    try:
        value = json.loads(
            text,
            object_pairs_hook=_pairs_hook,
            parse_constant=_reject_constant,
        )
    except AuthorizationObjectError:
        raise
    except (json.JSONDecodeError, ValueError) as exc:
        raise AuthorizationObjectError(
            AuthorizationObjectErrorCode.INVALID_JSON,
            "authorization object is not valid JSON",
        ) from exc

    _validate_structure(value)
    return validate_authorization_object(value)
