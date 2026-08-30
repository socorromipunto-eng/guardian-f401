"""Strict, domain-neutral JSON decoding primitives."""

from __future__ import annotations

import json
from typing import Any


class StrictJsonError(ValueError):
    """Base class for deterministic strict-JSON decoding failures."""


class InvalidUtf8Error(StrictJsonError):
    """Raised when input bytes are not strict UTF-8."""


class InvalidJsonError(StrictJsonError):
    """Raised when decoded text is not valid JSON."""


class DuplicateMemberError(StrictJsonError):
    """Raised when a JSON object contains a duplicate member name."""

    def __init__(self, key: str) -> None:
        self.key = key
        super().__init__(f"duplicate member {key!r}")


def _reject_duplicate_members(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise DuplicateMemberError(key)
        result[key] = value
    return result


def decode_strict_json(raw: bytes) -> Any:
    """Decode strict UTF-8 JSON while rejecting duplicate object members."""

    if not isinstance(raw, bytes):
        raise TypeError("raw must be bytes")

    try:
        text = raw.decode("utf-8", errors="strict")
    except UnicodeDecodeError as exc:
        raise InvalidUtf8Error("input is not strict UTF-8") from exc

    try:
        return json.loads(text, object_pairs_hook=_reject_duplicate_members)
    except DuplicateMemberError:
        raise
    except (json.JSONDecodeError, ValueError) as exc:
        raise InvalidJsonError("input is not valid JSON") from exc
