"""Closed M15 signed-message wrapper validation.

This module validates wrapper structure and signature representation only.

It does not verify Ed25519 signatures, establish authenticated provenance,
evaluate freshness, grant authority, or perform physical actuation.
"""

from __future__ import annotations

import base64
import binascii
import json
from dataclasses import dataclass
from enum import Enum
from typing import Any

from .errors import AssuranceError
from .transcript import (
    SIGNATURE_ALGORITHM,
    TRANSCRIPT_VERSION,
    TranscriptError,
    validate_key_id,
)
from .validation import parse_and_validate_envelope


SIGNED_WRAPPER_MAX_RAW_BYTES = 66_048
M14_ASSURANCE_MAX_RAW_BYTES = 65_536
ED25519_SIGNATURE_BYTES = 64
ED25519_BASE64URL_CHARS = 86

_WRAPPER_KEYS = frozenset(
    {
        "signature_version",
        "signature_algorithm",
        "key_id",
        "assurance_object",
        "signature",
    }
)

_BASE64URL_ALPHABET = frozenset(
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789-_"
)


class SignedWrapperErrorCode(str, Enum):
    """Wrapper-local structural error taxonomy.

    These are not cryptographic authentication results.
    """

    RAW_LIMIT = "WRAPPER_RAW_LIMIT"
    INVALID_UTF8 = "WRAPPER_INVALID_UTF8"
    INVALID_JSON = "WRAPPER_INVALID_JSON"
    DUPLICATE_KEY = "WRAPPER_DUPLICATE_KEY"
    SCHEMA = "WRAPPER_SCHEMA"
    TRANSCRIPT_VERSION_UNSUPPORTED = "TRANSCRIPT_VERSION_UNSUPPORTED"
    ALGORITHM_UNSUPPORTED = "ALGORITHM_UNSUPPORTED"
    KEY_ID_INVALID = "KEY_ID_INVALID"
    SIGNATURE_ENCODING_INVALID = "SIGNATURE_ENCODING_INVALID"
    ASSURANCE_OBJECT_INVALID = "ASSURANCE_OBJECT_INVALID"


class SignedWrapperError(ValueError):
    """Deterministic structural rejection of an M15 signed wrapper."""

    def __init__(self, code: SignedWrapperErrorCode, detail: str) -> None:
        super().__init__(f"{code.value}: {detail}")
        self.code = code
        self.detail = detail


@dataclass(frozen=True)
class ValidatedSignedWrapper:
    """Structurally validated wrapper.

    `signature_bytes` are decoded bytes only. They have not been
    cryptographically verified.
    """

    signature_version: str
    signature_algorithm: str
    key_id: str
    assurance_object: dict[str, Any]
    assurance_object_raw: bytes
    signature: str
    signature_bytes: bytes


def _reject_constant(value: str) -> None:
    raise ValueError(f"non-standard JSON constant: {value}")


def _parse_pairs(text: str) -> list[tuple[str, Any]]:
    try:
        value = json.loads(
            text,
            object_pairs_hook=lambda pairs: pairs,
            parse_constant=_reject_constant,
        )
    except (json.JSONDecodeError, ValueError, RecursionError) as exc:
        raise SignedWrapperError(
            SignedWrapperErrorCode.INVALID_JSON,
            "wrapper is not valid strict JSON",
        ) from exc

    if not isinstance(value, list):
        raise SignedWrapperError(
            SignedWrapperErrorCode.SCHEMA,
            "wrapper must be a JSON object",
        )

    if any(
        not isinstance(item, tuple) or len(item) != 2
        for item in value
    ):
        raise SignedWrapperError(
            SignedWrapperErrorCode.SCHEMA,
            "wrapper must be a JSON object",
        )

    return value


def _top_level_mapping(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}

    for key, value in pairs:
        if key in result:
            raise SignedWrapperError(
                SignedWrapperErrorCode.DUPLICATE_KEY,
                f"duplicate wrapper member: {key}",
            )
        result[key] = value

    if frozenset(result) != _WRAPPER_KEYS:
        raise SignedWrapperError(
            SignedWrapperErrorCode.SCHEMA,
            "wrapper must contain exactly the five defined members",
        )

    return result


def _skip_ws(text: str, index: int) -> int:
    while index < len(text) and text[index] in " \t\r\n":
        index += 1
    return index


def _scan_string_end(text: str, start: int) -> int:
    if start >= len(text) or text[start] != '"':
        raise SignedWrapperError(
            SignedWrapperErrorCode.INVALID_JSON,
            "expected JSON string",
        )

    index = start + 1
    escaped = False

    while index < len(text):
        char = text[index]

        if escaped:
            escaped = False
            index += 1
            continue

        if char == "\\":
            escaped = True
            index += 1
            continue

        if char == '"':
            return index + 1

        index += 1

    raise SignedWrapperError(
        SignedWrapperErrorCode.INVALID_JSON,
        "unterminated JSON string",
    )


def _scan_value_end(text: str, start: int) -> int:
    """Return the exclusive end offset of one already-valid JSON value."""

    if start >= len(text):
        raise SignedWrapperError(
            SignedWrapperErrorCode.INVALID_JSON,
            "missing JSON value",
        )

    if text[start] == '"':
        return _scan_string_end(text, start)

    if text[start] in "[{":
        stack = [text[start]]
        index = start + 1
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

                index += 1
                continue

            if char == '"':
                in_string = True
                index += 1
                continue

            if char in "[{":
                stack.append(char)
            elif char == "}":
                if not stack or stack[-1] != "{":
                    raise SignedWrapperError(
                        SignedWrapperErrorCode.INVALID_JSON,
                        "invalid JSON nesting",
                    )
                stack.pop()
                if not stack:
                    return index + 1
            elif char == "]":
                if not stack or stack[-1] != "[":
                    raise SignedWrapperError(
                        SignedWrapperErrorCode.INVALID_JSON,
                        "invalid JSON nesting",
                    )
                stack.pop()
                if not stack:
                    return index + 1

            index += 1

        raise SignedWrapperError(
            SignedWrapperErrorCode.INVALID_JSON,
            "unterminated JSON container",
        )

    index = start

    while index < len(text):
        if text[index] in ",}":
            break
        index += 1

    return index


def _extract_top_level_member_raw(text: str, target: str) -> str:
    """Extract the exact source text of one top-level member value.

    The complete wrapper has already passed JSON syntax validation.
    """

    index = _skip_ws(text, 0)

    if index >= len(text) or text[index] != "{":
        raise SignedWrapperError(
            SignedWrapperErrorCode.SCHEMA,
            "wrapper must be a JSON object",
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
            raise SignedWrapperError(
                SignedWrapperErrorCode.INVALID_JSON,
                "invalid wrapper member name",
            ) from exc

        index = _skip_ws(text, key_end)

        if index >= len(text) or text[index] != ":":
            raise SignedWrapperError(
                SignedWrapperErrorCode.INVALID_JSON,
                "missing wrapper member colon",
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

        raise SignedWrapperError(
            SignedWrapperErrorCode.INVALID_JSON,
            "invalid wrapper member separator",
        )

    raise SignedWrapperError(
        SignedWrapperErrorCode.SCHEMA,
        f"required wrapper member not found: {target}",
    )


def decode_signature_base64url(value: str) -> bytes:
    """Decode canonical unpadded base64url Ed25519 signature bytes."""

    if not isinstance(value, str):
        raise SignedWrapperError(
            SignedWrapperErrorCode.SIGNATURE_ENCODING_INVALID,
            "signature must be a string",
        )

    if len(value) != ED25519_BASE64URL_CHARS:
        raise SignedWrapperError(
            SignedWrapperErrorCode.SIGNATURE_ENCODING_INVALID,
            "Ed25519 signature encoding must be exactly 86 characters",
        )

    if "=" in value:
        raise SignedWrapperError(
            SignedWrapperErrorCode.SIGNATURE_ENCODING_INVALID,
            "base64url padding is prohibited",
        )

    if any(char not in _BASE64URL_ALPHABET for char in value):
        raise SignedWrapperError(
            SignedWrapperErrorCode.SIGNATURE_ENCODING_INVALID,
            "signature contains non-base64url characters",
        )

    try:
        decoded = base64.b64decode(
            value + "==",
            altchars=b"-_",
            validate=True,
        )
    except (binascii.Error, ValueError) as exc:
        raise SignedWrapperError(
            SignedWrapperErrorCode.SIGNATURE_ENCODING_INVALID,
            "signature is malformed base64url",
        ) from exc

    if len(decoded) != ED25519_SIGNATURE_BYTES:
        raise SignedWrapperError(
            SignedWrapperErrorCode.SIGNATURE_ENCODING_INVALID,
            "decoded Ed25519 signature must be exactly 64 bytes",
        )

    canonical = (
        base64.urlsafe_b64encode(decoded)
        .rstrip(b"=")
        .decode("ascii")
    )

    if canonical != value:
        raise SignedWrapperError(
            SignedWrapperErrorCode.SIGNATURE_ENCODING_INVALID,
            "signature representation is not canonical base64url",
        )

    return decoded


def encode_signature_base64url(signature: bytes) -> str:
    """Encode exactly 64 raw Ed25519 signature bytes."""

    if not isinstance(signature, bytes):
        raise SignedWrapperError(
            SignedWrapperErrorCode.SIGNATURE_ENCODING_INVALID,
            "signature bytes must be bytes",
        )

    if len(signature) != ED25519_SIGNATURE_BYTES:
        raise SignedWrapperError(
            SignedWrapperErrorCode.SIGNATURE_ENCODING_INVALID,
            "raw Ed25519 signature must be exactly 64 bytes",
        )

    return (
        base64.urlsafe_b64encode(signature)
        .rstrip(b"=")
        .decode("ascii")
    )


def parse_and_validate_signed_wrapper(raw: bytes) -> ValidatedSignedWrapper:
    """Strictly validate an M15 signed-message wrapper.

    This function validates structure and representation only.
    It does not perform cryptographic signature verification.
    """

    if not isinstance(raw, bytes):
        raise SignedWrapperError(
            SignedWrapperErrorCode.SCHEMA,
            "raw wrapper must be bytes",
        )

    if len(raw) > SIGNED_WRAPPER_MAX_RAW_BYTES:
        raise SignedWrapperError(
            SignedWrapperErrorCode.RAW_LIMIT,
            "signed wrapper exceeds 66,048-byte raw limit",
        )

    try:
        text = raw.decode("utf-8", errors="strict")
    except UnicodeDecodeError as exc:
        raise SignedWrapperError(
            SignedWrapperErrorCode.INVALID_UTF8,
            "wrapper is not valid UTF-8",
        ) from exc

    pairs = _parse_pairs(text)
    wrapper = _top_level_mapping(pairs)

    signature_version = wrapper["signature_version"]

    if signature_version != TRANSCRIPT_VERSION:
        raise SignedWrapperError(
            SignedWrapperErrorCode.TRANSCRIPT_VERSION_UNSUPPORTED,
            "unsupported signature_version",
        )

    signature_algorithm = wrapper["signature_algorithm"]

    if signature_algorithm != SIGNATURE_ALGORITHM:
        raise SignedWrapperError(
            SignedWrapperErrorCode.ALGORITHM_UNSUPPORTED,
            "unsupported signature_algorithm",
        )

    key_id = wrapper["key_id"]

    try:
        validate_key_id(key_id)
    except (TranscriptError, TypeError) as exc:
        raise SignedWrapperError(
            SignedWrapperErrorCode.KEY_ID_INVALID,
            "invalid key_id",
        ) from exc

    signature = wrapper["signature"]
    signature_bytes = decode_signature_base64url(signature)

    assurance_source = _extract_top_level_member_raw(
        text,
        "assurance_object",
    )

    assurance_raw = assurance_source.encode("utf-8")

    try:
        assurance_object = parse_and_validate_envelope(assurance_raw)
    except AssuranceError as exc:
        raise SignedWrapperError(
            SignedWrapperErrorCode.ASSURANCE_OBJECT_INVALID,
            f"M14 assurance object rejected: {exc.code.value}",
        ) from exc

    return ValidatedSignedWrapper(
        signature_version=signature_version,
        signature_algorithm=signature_algorithm,
        key_id=key_id,
        assurance_object=assurance_object,
        assurance_object_raw=assurance_raw,
        signature=signature,
        signature_bytes=signature_bytes,
    )