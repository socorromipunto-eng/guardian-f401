"""Bounded JSON decoding and closed-envelope validation."""

from __future__ import annotations

import json
import re
from typing import Any

from .domain import expected_domain
from .errors import AssuranceError, ErrorCode
from .limits import AssuranceLimits
from .schemas import validate_payload

_HEX_128 = re.compile(r"^[0-9a-f]{32}$")
_PRODUCER = re.compile(r"^[A-Za-z0-9._:-]{1,128}$")
_OBJECT_TYPES = frozenset({"observation", "decision", "witness"})
_SAFE_INTEGER = 9_007_199_254_740_991
_ENVELOPE_KEYS = frozenset({"domain", "object_type", "producer_id", "producer_epoch", "object_id", "logical_time", "payload"})


def _reject_duplicates(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise AssuranceError(ErrorCode.DUPLICATE_KEY, f"duplicate member {key!r}")
        result[key] = value
    return result


def _validate_structure(value: Any, limits: AssuranceLimits) -> None:
    nodes = 0

    def bounded_string(item: str) -> None:
        if len(item.encode("utf-8")) > limits.max_string_utf8_bytes:
            raise AssuranceError(ErrorCode.STRUCTURAL_LIMIT, "string limit exceeded")

    def visit(item: Any, depth: int) -> None:
        nonlocal nodes
        nodes += 1
        if nodes > limits.max_nodes:
            raise AssuranceError(ErrorCode.STRUCTURAL_LIMIT, "node limit exceeded")
        if depth > limits.max_depth:
            raise AssuranceError(ErrorCode.STRUCTURAL_LIMIT, "depth limit exceeded")
        if isinstance(item, bool) or item is None:
            return
        if isinstance(item, int):
            if not -_SAFE_INTEGER <= item <= _SAFE_INTEGER:
                raise AssuranceError(ErrorCode.SCHEMA, "integer outside safe range")
            return
        if isinstance(item, float):
            raise AssuranceError(ErrorCode.SCHEMA, "floating-point values are prohibited")
        if isinstance(item, str):
            bounded_string(item)
            return
        if isinstance(item, dict):
            if len(item) > limits.max_object_members:
                raise AssuranceError(ErrorCode.STRUCTURAL_LIMIT, "member limit exceeded")
            for key, child in item.items():
                bounded_string(key)
                visit(child, depth + 1)
            return
        if isinstance(item, list):
            if len(item) > limits.max_array_items:
                raise AssuranceError(ErrorCode.STRUCTURAL_LIMIT, "array limit exceeded")
            for child in item:
                visit(child, depth + 1)

    visit(value, 1)


def _require_text(value: Any, field: str, pattern: re.Pattern[str] | None = None) -> str:
    if not isinstance(value, str) or (pattern is not None and pattern.fullmatch(value) is None):
        raise AssuranceError(ErrorCode.SCHEMA, f"invalid {field}")
    return value


def validate_envelope(value: Any) -> dict[str, Any]:
    if not isinstance(value, dict) or frozenset(value) != _ENVELOPE_KEYS:
        raise AssuranceError(ErrorCode.SCHEMA, "envelope must contain exactly the defined members")
    object_type = _require_text(value["object_type"], "object_type")
    if object_type not in _OBJECT_TYPES:
        raise AssuranceError(ErrorCode.SCHEMA, "unsupported object_type")
    _require_text(value["producer_id"], "producer_id", _PRODUCER)
    _require_text(value["producer_epoch"], "producer_epoch", _HEX_128)
    _require_text(value["object_id"], "object_id", _HEX_128)
    logical_time = value["logical_time"]
    if isinstance(logical_time, bool) or not isinstance(logical_time, int) or not 0 <= logical_time <= _SAFE_INTEGER:
        raise AssuranceError(ErrorCode.SCHEMA, "invalid logical_time")
    if value["domain"] != expected_domain(object_type):
        raise AssuranceError(ErrorCode.DOMAIN, "domain does not match object_type")
    validate_payload(object_type, value["payload"])
    return value


def parse_and_validate_envelope(raw: bytes, limits: AssuranceLimits | None = None) -> dict[str, Any]:
    selected = limits or AssuranceLimits()
    if not isinstance(raw, bytes):
        raise AssuranceError(ErrorCode.SCHEMA, "input must be bytes")
    if len(raw) > selected.max_raw_bytes:
        raise AssuranceError(ErrorCode.RAW_LIMIT, "raw byte limit exceeded")
    try:
        text = raw.decode("utf-8", errors="strict")
    except UnicodeDecodeError as exc:
        raise AssuranceError(ErrorCode.INVALID_UTF8, "input is not strict UTF-8") from exc
    try:
        value = json.loads(text, object_pairs_hook=_reject_duplicates)
    except AssuranceError:
        raise
    except (json.JSONDecodeError, ValueError) as exc:
        raise AssuranceError(ErrorCode.INVALID_JSON, "input is not valid JSON") from exc
    _validate_structure(value, selected)
    return validate_envelope(value)

