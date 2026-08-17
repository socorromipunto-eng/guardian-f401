"""Closed, versioned payload contracts for Guardian F401 M14-S2."""

from __future__ import annotations

import re
from typing import Any

from .errors import AssuranceError, ErrorCode

_IDENTIFIER = re.compile(r"^[A-Za-z0-9._:-]+$")
_LOWER_HEX_32 = re.compile(r"^[0-9a-f]{32}$")
_LOWER_HEX_64 = re.compile(r"^[0-9a-f]{64}$")
_OBSERVATION_RESULTS = frozenset({"PRESENT", "ABSENT", "UNKNOWN", "CONFLICTING"})
_DECISION_RESULTS = frozenset({"CONFIRMED", "NOT_CONFIRMED", "INSUFFICIENT_EVIDENCE", "CONFLICTING_EVIDENCE", "POSSIBLE_CROSS_SITE_ATTACK", "HUMAN_REVIEW_REQUIRED"})


def _exact_members(payload: dict[str, Any], expected: frozenset[str]) -> None:
    if frozenset(payload) != expected:
        raise AssuranceError(ErrorCode.SCHEMA, "payload members do not match schema")


def _identifier(value: Any, field: str, maximum: int) -> str:
    if not isinstance(value, str) or not 1 <= len(value) <= maximum or _IDENTIFIER.fullmatch(value) is None:
        raise AssuranceError(ErrorCode.SCHEMA, f"invalid {field}")
    return value


def _exact_text(value: Any, field: str, expected: str) -> None:
    if not isinstance(value, str) or value != expected:
        raise AssuranceError(ErrorCode.SCHEMA, f"invalid {field}")


def _enum(value: Any, field: str, allowed: frozenset[str]) -> None:
    if not isinstance(value, str) or value not in allowed:
        raise AssuranceError(ErrorCode.SCHEMA, f"invalid {field}")


def _hex(value: Any, field: str, pattern: re.Pattern[str]) -> None:
    if not isinstance(value, str) or pattern.fullmatch(value) is None:
        raise AssuranceError(ErrorCode.SCHEMA, f"invalid {field}")


def _unique_array(value: Any, field: str, minimum: int, maximum: int, kind: str) -> None:
    if not isinstance(value, list) or not minimum <= len(value) <= maximum:
        raise AssuranceError(ErrorCode.SCHEMA, f"invalid {field} cardinality")
    seen: set[str] = set()
    for item in value:
        if kind == "identifier":
            checked = _identifier(item, field, 64)
        else:
            _hex(item, field, _LOWER_HEX_32)
            checked = item
        if checked in seen:
            raise AssuranceError(ErrorCode.SCHEMA, f"duplicate {field} item")
        seen.add(checked)


def _observation(payload: dict[str, Any]) -> None:
    _exact_members(payload, frozenset({"schema_version", "subject_id", "claim", "result", "evidence_digest"}))
    _exact_text(payload["schema_version"], "schema_version", "m14.observation.v1")
    _identifier(payload["subject_id"], "subject_id", 128)
    _identifier(payload["claim"], "claim", 64)
    _enum(payload["result"], "result", _OBSERVATION_RESULTS)
    _hex(payload["evidence_digest"], "evidence_digest", _LOWER_HEX_64)


def _decision(payload: dict[str, Any]) -> None:
    _exact_members(payload, frozenset({"schema_version", "policy_id", "result", "reason_codes", "evidence_ids"}))
    _exact_text(payload["schema_version"], "schema_version", "m14.decision.v1")
    _identifier(payload["policy_id"], "policy_id", 128)
    _enum(payload["result"], "result", _DECISION_RESULTS)
    _unique_array(payload["reason_codes"], "reason_codes", 1, 16, "identifier")
    _unique_array(payload["evidence_ids"], "evidence_ids", 1, 32, "hex")


def _witness(payload: dict[str, Any]) -> None:
    _exact_members(payload, frozenset({"schema_version", "subject_id", "result", "observed_object_ids", "evidence_digest"}))
    _exact_text(payload["schema_version"], "schema_version", "m14.witness.v1")
    _identifier(payload["subject_id"], "subject_id", 128)
    _enum(payload["result"], "result", _DECISION_RESULTS)
    _unique_array(payload["observed_object_ids"], "observed_object_ids", 1, 32, "hex")
    _hex(payload["evidence_digest"], "evidence_digest", _LOWER_HEX_64)


def validate_payload(object_type: str, payload: Any) -> None:
    if not isinstance(payload, dict):
        raise AssuranceError(ErrorCode.SCHEMA, "payload must be an object")
    validator = {"observation": _observation, "decision": _decision, "witness": _witness}.get(object_type)
    if validator is None:
        raise AssuranceError(ErrorCode.SCHEMA, "unsupported object_type")
    validator(payload)
