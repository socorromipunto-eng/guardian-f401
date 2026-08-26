"""Strict SignedAuthorizationEnvelopeV1 parsing.

This module validates the outer signed-authorization representation only.

It preserves the exact raw authorization_object bytes used by the later
transcript builder. It does not perform trust resolution, cryptographic
verification, replay evaluation, consumption, freshness, policy
authorization, or actuation.
"""

from __future__ import annotations

import base64
import json
from dataclasses import dataclass
from enum import Enum
from typing import Any

from .authorization_fields import (
    AuthorizationFieldError,
    validate_authority_id,
)
from .authorization_objects import (
    AuthorizationObjectError,
    ValidatedAuthorizationObject,
    parse_and_validate_authorization_object,
)
from .authorization_transcript import (
    AUTHORIZATION_SIGNATURE_ALGORITHM,
    AUTHORIZATION_TRANSCRIPT_VERSION,
)
from .transcript import TranscriptError, validate_key_id


SIGNED_AUTHORIZATION_MAX_RAW_BYTES = 16_384
ED25519_SIGNATURE_BYTES = 64
ED25519_SIGNATURE_BASE64URL_CHARS = 86

_BASE64URL_ALPHABET = frozenset(
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"
)

_ENVELOPE_KEYS = frozenset({
    "signature_version",
    "signature_algorithm",
    "authority_id",
    "key_id",
    "authorization_object",
    "signature",
})


class SignedAuthorizationErrorCode(str, Enum):
    RAW_LIMIT = "SIGNED_AUTHORIZATION_RAW_LIMIT"
    INVALID_UTF8 = "SIGNED_AUTHORIZATION_INVALID_UTF8"
    INVALID_JSON = "SIGNED_AUTHORIZATION_INVALID_JSON"
    DUPLICATE_KEY = "SIGNED_AUTHORIZATION_DUPLICATE_KEY"
    SCHEMA = "SIGNED_AUTHORIZATION_INVALID"
    VERSION_UNSUPPORTED = "SIGNED_AUTHORIZATION_VERSION_UNSUPPORTED"
    ALGORITHM_UNSUPPORTED = "SIGNED_AUTHORIZATION_ALGORITHM_UNSUPPORTED"
    AUTHORITY_ID_INVALID = "SIGNED_AUTHORIZATION_AUTHORITY_ID_INVALID"
    AUTHORITY_ID_MISMATCH = "SIGNED_AUTHORIZATION_AUTHORITY_ID_MISMATCH"
    KEY_ID_INVALID = "SIGNED_AUTHORIZATION_KEY_ID_INVALID"
    SIGNATURE_ENCODING_INVALID = "SIGNED_AUTHORIZATION_SIGNATURE_ENCODING_INVALID"
    AUTHORIZATION_OBJECT_INVALID = "SIGNED_AUTHORIZATION_OBJECT_INVALID"


class SignedAuthorizationError(ValueError):
    def __init__(
        self,
        code: SignedAuthorizationErrorCode,
        detail: str,
    ) -> None:
        super().__init__(detail)
        self.code = code
        self.detail = detail


@dataclass(frozen=True)
class ValidatedSignedAuthorization:
    """Structurally validated signed authorization envelope.

    signature_bytes are decoded bytes only; they are not verified.
    """

    signature_version: str
    signature_algorithm: str
    authority_id: str
    key_id: str
    authorization_object: ValidatedAuthorizationObject
    authorization_object_raw: bytes
    signature: str
    signature_bytes: bytes


def _reject_constant(value: str) -> None:
    raise SignedAuthorizationError(
        SignedAuthorizationErrorCode.INVALID_JSON,
        f"non-standard JSON constant: {value}",
    )


def _parse_pairs(text: str) -> list[tuple[str, Any]]:
    try:
        value = json.loads(
            text,
            object_pairs_hook=lambda pairs: pairs,
            parse_constant=_reject_constant,
        )
    except SignedAuthorizationError:
        raise
    except (json.JSONDecodeError, ValueError) as exc:
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.INVALID_JSON,
            "signed authorization is not valid JSON",
        ) from exc

    if not isinstance(value, list):
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.SCHEMA,
            "signed authorization envelope must be an object",
        )

    return value


def _top_level_mapping(
    pairs: list[tuple[str, Any]],
) -> dict[str, Any]:
    result: dict[str, Any] = {}

    for key, value in pairs:
        if not isinstance(key, str):
            raise SignedAuthorizationError(
                SignedAuthorizationErrorCode.SCHEMA,
                "envelope member name must be a string",
            )

        if key in result:
            raise SignedAuthorizationError(
                SignedAuthorizationErrorCode.DUPLICATE_KEY,
                f"duplicate envelope member {key!r}",
            )

        result[key] = value

    if frozenset(result) != _ENVELOPE_KEYS:
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.SCHEMA,
            "signed authorization envelope must contain exactly the defined members",
        )

    return result


def _skip_ws(text: str, index: int) -> int:
    while index < len(text) and text[index] in " \t\r\n":
        index += 1
    return index


def _scan_string_end(text: str, start: int) -> int:
    if start >= len(text) or text[start] != '"':
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.INVALID_JSON,
            "expected JSON string",
        )

    index = start + 1
    escaped = False

    while index < len(text):
        char = text[index]

        if escaped:
            escaped = False
        elif char == "\\":
            escaped = True
        elif char == '"':
            return index + 1

        index += 1

    raise SignedAuthorizationError(
        SignedAuthorizationErrorCode.INVALID_JSON,
        "unterminated JSON string",
    )


def _scan_value_end(text: str, start: int) -> int:
    if start >= len(text):
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.INVALID_JSON,
            "missing JSON value",
        )

    if text[start] == '"':
        return _scan_string_end(text, start)

    if text[start] in "[{":
        stack: list[str] = []
        index = start
        in_string = False
        escaped = False

        while index < len(text):
            char = text[index]

            if in_string:
                if escaped:
                    escaped = False
                elif char == "\\":
                    escaped = True
                elif char == '"':
                    in_string = False
            else:
                if char == '"':
                    in_string = True
                elif char in "[{":
                    stack.append(char)
                elif char in "]}":
                    if not stack:
                        break

                    opener = stack.pop()
                    if (opener, char) not in (("[", "]"), ("{", "}")):
                        raise SignedAuthorizationError(
                            SignedAuthorizationErrorCode.INVALID_JSON,
                            "mismatched JSON container",
                        )

                    if not stack:
                        return index + 1

            index += 1

        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.INVALID_JSON,
            "unterminated JSON container",
        )

    index = start
    while index < len(text) and text[index] not in ",}]":
        index += 1
    return index


def _extract_top_level_member_raw(text: str, target: str) -> str:
    index = _skip_ws(text, 0)

    if index >= len(text) or text[index] != "{":
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.INVALID_JSON,
            "signed authorization envelope must be an object",
        )

    index += 1

    while True:
        index = _skip_ws(text, index)

        if index >= len(text):
            break

        if text[index] == "}":
            break

        key_start = index
        key_end = _scan_string_end(text, key_start)

        try:
            key = json.loads(text[key_start:key_end])
        except json.JSONDecodeError as exc:
            raise SignedAuthorizationError(
                SignedAuthorizationErrorCode.INVALID_JSON,
                "invalid envelope member name",
            ) from exc

        index = _skip_ws(text, key_end)

        if index >= len(text) or text[index] != ":":
            raise SignedAuthorizationError(
                SignedAuthorizationErrorCode.INVALID_JSON,
                "missing envelope member colon",
            )

        index = _skip_ws(text, index + 1)
        value_start = index
        value_end = _scan_value_end(text, value_start)

        if key == target:
            return text[value_start:value_end]

        index = _skip_ws(text, value_end)

        if index < len(text) and text[index] == ",":
            index += 1
            continue

        if index < len(text) and text[index] == "}":
            break

        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.INVALID_JSON,
            "invalid envelope member separator",
        )

    raise SignedAuthorizationError(
        SignedAuthorizationErrorCode.SCHEMA,
        f"missing envelope member {target!r}",
    )


def decode_authorization_signature_base64url(value: Any) -> bytes:
    """Decode canonical unpadded base64url Ed25519 signature bytes."""

    if not isinstance(value, str):
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.SIGNATURE_ENCODING_INVALID,
            "signature must be a string",
        )

    if len(value) != ED25519_SIGNATURE_BASE64URL_CHARS:
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.SIGNATURE_ENCODING_INVALID,
            "Ed25519 signature encoding must be exactly 86 characters",
        )

    if "=" in value:
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.SIGNATURE_ENCODING_INVALID,
            "signature must use unpadded base64url",
        )

    if any(char not in _BASE64URL_ALPHABET for char in value):
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.SIGNATURE_ENCODING_INVALID,
            "signature contains non-base64url characters",
        )

    try:
        encoded = value.encode("ascii")
        decoded = base64.b64decode(
            encoded + b"==",
            altchars=b"-_",
            validate=True,
        )
    except (UnicodeEncodeError, ValueError, base64.binascii.Error) as exc:
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.SIGNATURE_ENCODING_INVALID,
            "signature is malformed base64url",
        ) from exc

    if len(decoded) != ED25519_SIGNATURE_BYTES:
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.SIGNATURE_ENCODING_INVALID,
            "decoded Ed25519 signature must be exactly 64 bytes",
        )

    canonical = base64.urlsafe_b64encode(decoded).rstrip(b"=").decode("ascii")

    if canonical != value:
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.SIGNATURE_ENCODING_INVALID,
            "signature representation is not canonical base64url",
        )

    return decoded


def encode_authorization_signature_base64url(signature: bytes) -> str:
    """Encode exactly 64 raw Ed25519 signature bytes for host/test use."""

    if not isinstance(signature, bytes) or len(signature) != ED25519_SIGNATURE_BYTES:
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.SIGNATURE_ENCODING_INVALID,
            "raw Ed25519 signature must be exactly 64 bytes",
        )

    return base64.urlsafe_b64encode(signature).rstrip(b"=").decode("ascii")


def parse_and_validate_signed_authorization(
    raw: bytes,
) -> ValidatedSignedAuthorization:
    """Parse SignedAuthorizationEnvelopeV1 without verifying its signature."""

    if not isinstance(raw, bytes):
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.SCHEMA,
            "input must be bytes",
        )

    if len(raw) > SIGNED_AUTHORIZATION_MAX_RAW_BYTES:
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.RAW_LIMIT,
            "signed authorization envelope exceeds 16384 bytes",
        )

    try:
        text = raw.decode("utf-8", errors="strict")
    except UnicodeDecodeError as exc:
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.INVALID_UTF8,
            "signed authorization envelope is not strict UTF-8",
        ) from exc

    pairs = _parse_pairs(text)
    envelope = _top_level_mapping(pairs)

    signature_version = envelope["signature_version"]
    if signature_version != AUTHORIZATION_TRANSCRIPT_VERSION:
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.VERSION_UNSUPPORTED,
            "unsupported authorization signature_version",
        )

    signature_algorithm = envelope["signature_algorithm"]
    if signature_algorithm != AUTHORIZATION_SIGNATURE_ALGORITHM:
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.ALGORITHM_UNSUPPORTED,
            "unsupported authorization signature_algorithm",
        )

    authority_id = envelope["authority_id"]
    try:
        authority_id = validate_authority_id(authority_id)
    except AuthorizationFieldError as exc:
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.AUTHORITY_ID_INVALID,
            "invalid authority_id",
        ) from exc

    key_id = envelope["key_id"]
    try:
        validate_key_id(key_id)
    except TranscriptError as exc:
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.KEY_ID_INVALID,
            "invalid authority key_id",
        ) from exc

    signature = envelope["signature"]
    signature_bytes = decode_authorization_signature_base64url(signature)

    authorization_raw_text = _extract_top_level_member_raw(
        text,
        "authorization_object",
    )

    try:
        authorization_raw = authorization_raw_text.encode("utf-8")
    except UnicodeEncodeError as exc:
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.AUTHORIZATION_OBJECT_INVALID,
            "authorization_object raw bytes are not UTF-8",
        ) from exc

    try:
        authorization_object = parse_and_validate_authorization_object(
            authorization_raw
        )
    except AuthorizationObjectError as exc:
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.AUTHORIZATION_OBJECT_INVALID,
            "authorization_object failed closed validation",
        ) from exc

    if authorization_object.authority_id != authority_id:
        raise SignedAuthorizationError(
            SignedAuthorizationErrorCode.AUTHORITY_ID_MISMATCH,
            "envelope authority_id does not match authorization_object authority_id",
        )

    return ValidatedSignedAuthorization(
        signature_version=signature_version,
        signature_algorithm=signature_algorithm,
        authority_id=authority_id,
        key_id=key_id,
        authorization_object=authorization_object,
        authorization_object_raw=authorization_raw,
        signature=signature,
        signature_bytes=signature_bytes,
    )
