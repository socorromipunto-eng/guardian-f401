"""Host persistence backend for composite M15 authorization transactions.

One authorization replay scope maps to one committed transaction-state file.

Candidate state is non-authoritative.
COMMITTED is distinct from VERIFIED.
VERIFIED is distinct from AUTHORIZATION_CONSUMED.

The SHA-256 integrity field detects accidental/corrupt modification only.
This backend does not claim arbitrary-storage rollback resistance,
hardware-backed rollback resistance, or STM32F401 production durability.
"""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
from typing import Any

from .authorization_persistence import (
    AuthorizationLoadCode,
    AuthorizationLoadResult,
    AuthorizationPersistCode,
    AuthorizationPersistResult,
    AuthorizationTransactionBackend,
)
from .authorization_replay import (
    AuthorizationReplayScope,
    AuthorizationReplayState,
)
from .authorization_transaction import (
    AuthorizationPreparedTransaction,
    AuthorizationTransactionState,
)
from .freshness_state import FreshnessState, HighWaterState


_SCHEMA_VERSION = "guardian-f401:m15:authorization-transaction-state:v1"

_TOP_LEVEL_KEYS = frozenset(
    {
        "schema_version",
        "state",
        "integrity_sha256",
    }
)

_STATE_KEYS = frozenset(
    {
        "replay_state",
        "replay_scope",
        "generation",
        "resulting_freshness_state",
        "operation_type",
        "authorization_id",
        "authorization_sequence",
    }
)

_SCOPE_KEYS = frozenset(
    {
        "authority_id",
        "purpose_domain",
        "producer_id",
    }
)

_REPLAY_STATE_KEYS = frozenset(
    {
        "scope",
        "authorization_sequence",
    }
)

_FRESHNESS_KEYS = frozenset(
    {
        "producer_id",
        "current_epoch",
        "previous_epoch",
        "high_water_state",
        "highest_logical_time",
        "transition_sequence",
        "generation",
    }
)


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


def _scope_to_mapping(
    scope: AuthorizationReplayScope,
) -> dict[str, Any]:
    return {
        "authority_id": scope.authority_id,
        "purpose_domain": scope.purpose_domain,
        "producer_id": scope.producer_id,
    }


def _replay_state_to_mapping(
    state: AuthorizationReplayState,
) -> dict[str, Any]:
    return {
        "scope": _scope_to_mapping(state.scope),
        "authorization_sequence": state.authorization_sequence,
    }


def _freshness_to_mapping(
    state: FreshnessState,
) -> dict[str, Any]:
    return {
        "producer_id": state.producer_id,
        "current_epoch": state.current_epoch,
        "previous_epoch": state.previous_epoch,
        "high_water_state": state.high_water_state.value,
        "highest_logical_time": state.highest_logical_time,
        "transition_sequence": state.transition_sequence,
        "generation": state.generation,
    }


def _state_to_mapping(
    state: AuthorizationTransactionState,
) -> dict[str, Any]:
    return {
        "replay_state": _replay_state_to_mapping(state.replay_state),
        "replay_scope": _scope_to_mapping(state.replay_scope),
        "generation": state.generation,
        "resulting_freshness_state": _freshness_to_mapping(
            state.resulting_freshness_state
        ),
        "operation_type": state.operation_type,
        "authorization_id": state.authorization_id,
        "authorization_sequence": state.authorization_sequence,
    }


def _integrity_digest(
    state_mapping: dict[str, Any],
) -> str:
    material = {
        "schema_version": _SCHEMA_VERSION,
        "state": state_mapping,
    }

    return hashlib.sha256(
        _canonical_json(material)
    ).hexdigest().upper()


def _serialize_state(
    state: AuthorizationTransactionState,
) -> bytes:
    state_mapping = _state_to_mapping(state)

    document = {
        "schema_version": _SCHEMA_VERSION,
        "state": state_mapping,
        "integrity_sha256": _integrity_digest(state_mapping),
    }

    return _canonical_json(document) + b"\n"


def _parse_scope(value: Any) -> AuthorizationReplayScope:
    if not isinstance(value, dict):
        raise ValueError("persistent replay scope must be an object")

    if frozenset(value) != _SCOPE_KEYS:
        raise ValueError("persistent replay scope schema is invalid")

    return AuthorizationReplayScope(
        authority_id=value["authority_id"],
        purpose_domain=value["purpose_domain"],
        producer_id=value["producer_id"],
    )


def _parse_replay_state(
    value: Any,
) -> AuthorizationReplayState:
    if not isinstance(value, dict):
        raise ValueError("persistent replay state must be an object")

    if frozenset(value) != _REPLAY_STATE_KEYS:
        raise ValueError("persistent replay state schema is invalid")

    return AuthorizationReplayState(
        scope=_parse_scope(value["scope"]),
        authorization_sequence=value["authorization_sequence"],
    )


def _parse_freshness(
    value: Any,
) -> FreshnessState:
    if not isinstance(value, dict):
        raise ValueError("persistent freshness state must be an object")

    if frozenset(value) != _FRESHNESS_KEYS:
        raise ValueError("persistent freshness state schema is invalid")

    try:
        high_water_state = HighWaterState(value["high_water_state"])
    except (TypeError, ValueError) as exc:
        raise ValueError(
            "persistent freshness high-water state is invalid"
        ) from exc

    return FreshnessState(
        producer_id=value["producer_id"],
        current_epoch=value["current_epoch"],
        previous_epoch=value["previous_epoch"],
        high_water_state=high_water_state,
        highest_logical_time=value["highest_logical_time"],
        transition_sequence=value["transition_sequence"],
        generation=value["generation"],
    )


def _parse_state(
    raw: bytes,
) -> AuthorizationTransactionState:
    if not isinstance(raw, bytes):
        raise ValueError("persistent authorization state must be bytes")

    try:
        text = raw.decode("utf-8", errors="strict")
    except UnicodeDecodeError as exc:
        raise ValueError(
            "persistent authorization state is not strict UTF-8"
        ) from exc

    try:
        document = json.loads(
            text,
            object_pairs_hook=_reject_duplicate_members,
            parse_constant=lambda value: (_ for _ in ()).throw(
                ValueError(f"invalid JSON constant: {value}")
            ),
        )
    except (
        json.JSONDecodeError,
        _DuplicateMemberError,
        ValueError,
    ) as exc:
        raise ValueError(
            "persistent authorization state JSON is invalid"
        ) from exc

    if (
        not isinstance(document, dict)
        or frozenset(document) != _TOP_LEVEL_KEYS
    ):
        raise ValueError(
            "persistent authorization top-level schema is invalid"
        )

    if document["schema_version"] != _SCHEMA_VERSION:
        raise ValueError(
            "persistent authorization schema version is invalid"
        )

    state_mapping = document["state"]

    if (
        not isinstance(state_mapping, dict)
        or frozenset(state_mapping) != _STATE_KEYS
    ):
        raise ValueError(
            "persistent authorization transaction schema is invalid"
        )

    integrity = document["integrity_sha256"]

    if not isinstance(integrity, str):
        raise ValueError(
            "persistent authorization integrity digest is invalid"
        )

    if integrity != _integrity_digest(state_mapping):
        raise ValueError(
            "persistent authorization integrity digest mismatch"
        )

    return AuthorizationTransactionState(
        replay_state=_parse_replay_state(
            state_mapping["replay_state"]
        ),
        replay_scope=_parse_scope(
            state_mapping["replay_scope"]
        ),
        generation=state_mapping["generation"],
        resulting_freshness_state=_parse_freshness(
            state_mapping["resulting_freshness_state"]
        ),
        operation_type=state_mapping["operation_type"],
        authorization_id=state_mapping["authorization_id"],
        authorization_sequence=state_mapping[
            "authorization_sequence"
        ],
    )


def replay_scope_key(
    scope: AuthorizationReplayScope,
) -> str:
    """Return deterministic host-storage identity for one replay scope."""

    material = _canonical_json(_scope_to_mapping(scope))

    return hashlib.sha256(material).hexdigest()


class HostAuthorizationTransactionBackend(
    AuthorizationTransactionBackend
):
    """Filesystem-backed bounded host transaction backend."""

    def __init__(self, root: Path) -> None:
        self._root = Path(root)

    def _state_path(
        self,
        replay_scope_key: str,
    ) -> Path:
        if (
            not isinstance(replay_scope_key, str)
            or len(replay_scope_key) != 64
            or any(
                char not in "0123456789abcdef"
                for char in replay_scope_key
            )
        ):
            raise ValueError("invalid authorization replay scope key")

        return self._root / (
            f"{replay_scope_key}.authorization.json"
        )

    def _candidate_path(
        self,
        replay_scope_key: str,
    ) -> Path:
        path = self._state_path(replay_scope_key)
        return path.with_name(path.name + ".candidate")

    def load(
        self,
        scope_key: str,
    ) -> AuthorizationLoadResult:
        try:
            path = self._state_path(scope_key)
        except ValueError:
            return AuthorizationLoadResult(
                code=AuthorizationLoadCode.AUTHORIZATION_STATE_INVALID,
                detail="authorization replay scope key is invalid",
            )

        try:
            if not path.exists():
                return AuthorizationLoadResult(
                    code=AuthorizationLoadCode.STATE_NOT_ESTABLISHED,
                    detail="no committed host authorization state",
                )

            raw = path.read_bytes()

        except OSError:
            return AuthorizationLoadResult(
                code=AuthorizationLoadCode.AUTHORIZATION_STATE_UNAVAILABLE,
                detail="committed host authorization state unavailable",
            )

        try:
            state = _parse_state(raw)
        except (ValueError, TypeError, KeyError):
            return AuthorizationLoadResult(
                code=AuthorizationLoadCode.AUTHORIZATION_STATE_INVALID,
                detail="committed host authorization state invalid",
            )

        if replay_scope_key(state.replay_scope) != scope_key:
            return AuthorizationLoadResult(
                code=AuthorizationLoadCode.AUTHORIZATION_STATE_INVALID,
                detail="committed authorization replay scope mismatch",
            )

        return AuthorizationLoadResult(
            code=AuthorizationLoadCode.STATE_VALID,
            state=state,
        )

    def prepare(
        self,
        prepared: AuthorizationPreparedTransaction,
    ) -> AuthorizationPersistResult:
        candidate = prepared.candidate

        if (
            prepared.previous_generation is not None
            and candidate.generation
            <= prepared.previous_generation
        ):
            return AuthorizationPersistResult(
                code=AuthorizationPersistCode.AUTHORIZATION_STATE_STALE_PREPARATION,
                detail="candidate generation does not advance prior generation",
            )

        return AuthorizationPersistResult(
            code=AuthorizationPersistCode.PREPARED,
            state=candidate,
        )

    def commit(
        self,
        prepared: AuthorizationPreparedTransaction,
    ) -> AuthorizationPersistResult:
        candidate = prepared.candidate
        scope_key = replay_scope_key(candidate.replay_scope)

        path = self._state_path(scope_key)
        candidate_path = self._candidate_path(scope_key)

        try:
            self._root.mkdir(parents=True, exist_ok=True)

            current = self.load(scope_key)

            if prepared.previous_generation is None:
                if (
                    current.code
                    is not AuthorizationLoadCode.STATE_NOT_ESTABLISHED
                ):
                    return AuthorizationPersistResult(
                        code=AuthorizationPersistCode.AUTHORIZATION_STATE_STALE_PREPARATION,
                        detail="unexpected authoritative state during initial commit",
                    )

            else:
                if (
                    current.code
                    is not AuthorizationLoadCode.STATE_VALID
                    or current.state is None
                    or current.state.generation
                    != prepared.previous_generation
                    or current.state.replay_state
                    != prepared.previous_replay_state
                    or current.state.resulting_freshness_state
                    != prepared.previous_freshness_state
                ):
                    return AuthorizationPersistResult(
                        code=AuthorizationPersistCode.AUTHORIZATION_STATE_STALE_PREPARATION,
                        detail="authoritative state changed since prepare",
                    )

            raw = _serialize_state(candidate)

            with candidate_path.open("wb") as stream:
                stream.write(raw)
                stream.flush()
                os.fsync(stream.fileno())

            parsed_candidate = _parse_state(
                candidate_path.read_bytes()
            )

            if parsed_candidate != candidate:
                raise ValueError(
                    "authorization candidate verification mismatch"
                )

            os.replace(candidate_path, path)
            self._sync_directory()

        except (OSError, ValueError, TypeError, KeyError):
            return AuthorizationPersistResult(
                code=AuthorizationPersistCode.AUTHORIZATION_STATE_PERSIST_FAILURE,
                detail="host authorization transaction commit failed",
            )

        return AuthorizationPersistResult(
            code=AuthorizationPersistCode.COMMITTED,
            state=candidate,
        )

    def verify(
        self,
        prepared: AuthorizationPreparedTransaction,
    ) -> AuthorizationPersistResult:
        candidate = prepared.candidate
        scope_key = replay_scope_key(candidate.replay_scope)

        loaded = self.load(scope_key)

        if (
            loaded.code is not AuthorizationLoadCode.STATE_VALID
            or loaded.state != candidate
        ):
            return AuthorizationPersistResult(
                code=AuthorizationPersistCode.AUTHORIZATION_STATE_PERSIST_FAILURE,
                detail="durable authorization state does not match candidate",
            )

        return AuthorizationPersistResult(
            code=AuthorizationPersistCode.VERIFIED,
            state=candidate,
        )

    def _sync_directory(self) -> None:
        """Best-effort host directory durability where supported.

        Windows retains os.replace atomicity but this function does not
        claim power-loss durability or hardware-backed rollback resistance.
        """

        if os.name == "nt":
            return

        descriptor = os.open(self._root, os.O_RDONLY)

        try:
            os.fsync(descriptor)
        finally:
            os.close(descriptor)