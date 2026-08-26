"""Guardian M15-03 host transactional freshness backend.

This backend provides a filesystem implementation of the backend-neutral
M15-03 persistence contract for host validation.

Integrity metadata detects corruption and incomplete content.
It does not provide authenticated storage or hardware-backed rollback
resistance against arbitrary storage replacement.
"""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
from typing import Any

from .freshness_persistence import (
    FreshnessLoadCode,
    FreshnessLoadResult,
    FreshnessPersistCode,
    FreshnessPersistResult,
    FreshnessPersistenceBackend,
    FreshnessPreparedWrite,
)
from .freshness_state import (
    FreshnessState,
    HighWaterState,
)
from .validation import validate_producer_id


_SCHEMA_VERSION = "guardian-f401:m15:freshness-state:v1"
_TOP_LEVEL_KEYS = frozenset({"schema_version", "state", "integrity_sha256"})
_STATE_KEYS = frozenset({
    "producer_id",
    "current_epoch",
    "previous_epoch",
    "high_water_state",
    "highest_logical_time",
    "transition_sequence",
    "generation",
})


class _DuplicateMemberError(ValueError):
    pass


def _reject_duplicate_members(
    pairs: list[tuple[str, Any]],
) -> dict[str, Any]:
    result: dict[str, Any] = {}

    for key, value in pairs:
        if key in result:
            raise _DuplicateMemberError(f"duplicate JSON member: {key}")
        result[key] = value

    return result


def _canonical_json(value: Any) -> bytes:
    return json.dumps(
        value,
        ensure_ascii=True,
        sort_keys=True,
        separators=(",", ":"),
        allow_nan=False,
    ).encode("utf-8")


def _state_to_mapping(state: FreshnessState) -> dict[str, Any]:
    return {
        "producer_id": state.producer_id,
        "current_epoch": state.current_epoch,
        "previous_epoch": state.previous_epoch,
        "high_water_state": state.high_water_state.value,
        "highest_logical_time": state.highest_logical_time,
        "transition_sequence": state.transition_sequence,
        "generation": state.generation,
    }


def _integrity_digest(state_mapping: dict[str, Any]) -> str:
    material = {
        "schema_version": _SCHEMA_VERSION,
        "state": state_mapping,
    }

    return hashlib.sha256(_canonical_json(material)).hexdigest().upper()


def _serialize_state(state: FreshnessState) -> bytes:
    state_mapping = _state_to_mapping(state)

    document = {
        "schema_version": _SCHEMA_VERSION,
        "state": state_mapping,
        "integrity_sha256": _integrity_digest(state_mapping),
    }

    return _canonical_json(document) + b"\n"


def _parse_state(raw: bytes) -> FreshnessState:
    if not isinstance(raw, bytes):
        raise ValueError("persistent state must be bytes")

    try:
        text = raw.decode("utf-8", errors="strict")
    except UnicodeDecodeError as exc:
        raise ValueError("persistent state is not strict UTF-8") from exc

    try:
        value = json.loads(
            text,
            object_pairs_hook=_reject_duplicate_members,
            parse_constant=lambda value: (_ for _ in ()).throw(
                ValueError(f"invalid JSON constant: {value}")
            ),
        )
    except (json.JSONDecodeError, _DuplicateMemberError, ValueError) as exc:
        raise ValueError("persistent state JSON is invalid") from exc

    if not isinstance(value, dict) or frozenset(value) != _TOP_LEVEL_KEYS:
        raise ValueError("persistent state top-level schema is invalid")

    if value["schema_version"] != _SCHEMA_VERSION:
        raise ValueError("persistent state schema version is invalid")

    state_mapping = value["state"]

    if (
        not isinstance(state_mapping, dict)
        or frozenset(state_mapping) != _STATE_KEYS
    ):
        raise ValueError("persistent freshness record schema is invalid")

    integrity = value["integrity_sha256"]

    if not isinstance(integrity, str):
        raise ValueError("persistent integrity digest is invalid")

    expected_integrity = _integrity_digest(state_mapping)

    if integrity != expected_integrity:
        raise ValueError("persistent integrity digest mismatch")

    try:
        high_water_state = HighWaterState(state_mapping["high_water_state"])
    except (TypeError, ValueError) as exc:
        raise ValueError("persistent high-water state is invalid") from exc

    return FreshnessState(
        producer_id=state_mapping["producer_id"],
        current_epoch=state_mapping["current_epoch"],
        previous_epoch=state_mapping["previous_epoch"],
        high_water_state=high_water_state,
        highest_logical_time=state_mapping["highest_logical_time"],
        transition_sequence=state_mapping["transition_sequence"],
        generation=state_mapping["generation"],
    )


class HostFreshnessPersistenceBackend(FreshnessPersistenceBackend):
    """Filesystem-backed host validation backend.

    One producer maps to one committed state file.
    Candidate writes use a sibling temporary file followed by os.replace().
    """

    def __init__(self, root: Path) -> None:
        self._root = Path(root)

    def _state_path(self, producer_id: str) -> Path:
        validate_producer_id(producer_id)
        digest = hashlib.sha256(producer_id.encode("utf-8")).hexdigest()
        return self._root / f"{digest}.freshness.json"

    def _candidate_path(self, producer_id: str) -> Path:
        path = self._state_path(producer_id)
        return path.with_name(path.name + ".candidate")

    def load(self, producer_id: str) -> FreshnessLoadResult:
        producer_id = validate_producer_id(producer_id)
        path = self._state_path(producer_id)

        try:
            if not path.exists():
                return FreshnessLoadResult(
                    code=FreshnessLoadCode.STATE_NOT_ESTABLISHED,
                    producer_id=producer_id,
                    detail="no committed host freshness state",
                )

            raw = path.read_bytes()
        except OSError:
            return FreshnessLoadResult(
                code=FreshnessLoadCode.FRESHNESS_STATE_UNAVAILABLE,
                producer_id=producer_id,
                detail="committed host freshness state unavailable",
            )

        try:
            state = _parse_state(raw)
        except (ValueError, TypeError):
            return FreshnessLoadResult(
                code=FreshnessLoadCode.FRESHNESS_STATE_INVALID,
                producer_id=producer_id,
                detail="committed host freshness state invalid",
            )

        if state.producer_id != producer_id:
            return FreshnessLoadResult(
                code=FreshnessLoadCode.FRESHNESS_STATE_INVALID,
                producer_id=producer_id,
                detail="committed state producer mismatch",
            )

        return FreshnessLoadResult(
            code=FreshnessLoadCode.STATE_VALID,
            producer_id=producer_id,
            state=state,
        )

    def prepare(
        self,
        *,
        previous_state: FreshnessState | None,
        candidate_state: FreshnessState,
    ) -> FreshnessPreparedWrite:
        if previous_state is not None:
            if candidate_state.generation <= previous_state.generation:
                raise ValueError(
                    "candidate generation must advance previous generation"
                )

        return FreshnessPreparedWrite(
            producer_id=candidate_state.producer_id,
            previous_state=previous_state,
            candidate_state=candidate_state,
        )

    def commit(
        self,
        prepared: FreshnessPreparedWrite,
    ) -> FreshnessPersistResult:
        producer_id = prepared.producer_id
        path = self._state_path(producer_id)
        candidate_path = self._candidate_path(producer_id)

        try:
            self._root.mkdir(parents=True, exist_ok=True)

            current = self.load(producer_id)

            if prepared.previous_state is None:
                if current.code is not FreshnessLoadCode.STATE_NOT_ESTABLISHED:
                    return FreshnessPersistResult(
                        code=FreshnessPersistCode.FRESHNESS_STATE_PERSIST_FAILURE,
                        producer_id=producer_id,
                        detail="unexpected existing state during initial commit",
                    )
            else:
                if (
                    current.code is not FreshnessLoadCode.STATE_VALID
                    or current.state != prepared.previous_state
                ):
                    return FreshnessPersistResult(
                        code=FreshnessPersistCode.FRESHNESS_STATE_PERSIST_FAILURE,
                        producer_id=producer_id,
                        detail="committed state changed since prepare",
                    )

            raw = _serialize_state(prepared.candidate_state)

            with candidate_path.open("wb") as stream:
                stream.write(raw)
                stream.flush()
                os.fsync(stream.fileno())

            parsed_candidate = _parse_state(candidate_path.read_bytes())

            if parsed_candidate != prepared.candidate_state:
                raise ValueError("candidate verification mismatch")

            os.replace(candidate_path, path)

            self._sync_directory()

        except (OSError, ValueError, TypeError):
            return FreshnessPersistResult(
                code=FreshnessPersistCode.FRESHNESS_STATE_PERSIST_FAILURE,
                producer_id=producer_id,
                detail="host freshness commit failed",
            )

        return FreshnessPersistResult(
            code=FreshnessPersistCode.COMMITTED,
            producer_id=producer_id,
            state=prepared.candidate_state,
        )

    def verify(
        self,
        prepared: FreshnessPreparedWrite,
    ) -> FreshnessPersistResult:
        loaded = self.load(prepared.producer_id)

        if (
            loaded.code is not FreshnessLoadCode.STATE_VALID
            or loaded.state != prepared.candidate_state
        ):
            return FreshnessPersistResult(
                code=FreshnessPersistCode.FRESHNESS_STATE_PERSIST_FAILURE,
                producer_id=prepared.producer_id,
                detail="durable host state does not match candidate",
            )

        return FreshnessPersistResult(
            code=FreshnessPersistCode.VERIFIED,
            producer_id=prepared.producer_id,
            state=prepared.candidate_state,
        )

    def _sync_directory(self) -> None:
        """Best-effort directory durability where the host supports it.

        Failure is propagated when a directory descriptor can be opened.
        Hosts that do not support directory descriptors retain os.replace
        atomicity but must not be described as proving production durability.
        """

        if os.name == "nt":
            return

        descriptor = os.open(self._root, os.O_RDONLY)

        try:
            os.fsync(descriptor)
        finally:
            os.close(descriptor)
