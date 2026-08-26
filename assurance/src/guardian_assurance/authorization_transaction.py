"""Pure M15 composite authorization transaction model.

The transaction binds authorization anti-replay advancement and the
resulting producer freshness state into one logical generation.

This module defines transaction state only. It does not persist, commit,
verify durable storage, perform bootstrap, perform epoch transition, grant
policy authorization, or permit actuation.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum

from .authorization_objects import BOOTSTRAP_TYPE, EPOCH_TRANSITION_TYPE
from .authorization_replay import (
    UINT64_MAX,
    AuthorizationReplayCode,
    AuthorizationReplayOutcome,
    AuthorizationReplayScope,
    AuthorizationReplayState,
)
from .freshness_state import FreshnessState


class AuthorizationTransactionCode(str, Enum):
    PREPARED = "PREPARED"
    COMMITTED = "COMMITTED"
    VERIFIED = "VERIFIED"
    AUTHORIZATION_CONSUMED = "AUTHORIZATION_CONSUMED"
    AUTHORIZATION_NOT_CANDIDATE = "AUTHORIZATION_NOT_CANDIDATE"
    AUTHORIZATION_GENERATION_EXHAUSTED = "AUTHORIZATION_GENERATION_EXHAUSTED"
    AUTHORIZATION_TRANSACTION_INVALID = "AUTHORIZATION_TRANSACTION_INVALID"


@dataclass(frozen=True)
class AuthorizationTransactionState:
    """One complete logical authorization transaction generation."""

    replay_state: AuthorizationReplayState
    replay_scope: AuthorizationReplayScope
    generation: int
    resulting_freshness_state: FreshnessState
    operation_type: str
    authorization_id: str
    authorization_sequence: int

    def __post_init__(self) -> None:
        if isinstance(self.generation, bool) or not isinstance(self.generation, int):
            raise ValueError("generation must be an integer")

        if not 0 <= self.generation <= UINT64_MAX:
            raise ValueError("generation outside uint64")

        if self.replay_state.scope != self.replay_scope:
            raise ValueError("replay_state scope does not match replay_scope")

        if self.replay_state.authorization_sequence != self.authorization_sequence:
            raise ValueError("replay high-water does not match consumed sequence")

        if self.operation_type not in (BOOTSTRAP_TYPE, EPOCH_TRANSITION_TYPE):
            raise ValueError("unsupported authorization operation_type")


@dataclass(frozen=True)
class AuthorizationPreparedTransaction:
    """Prepared candidate bound to exact prior authoritative states."""

    previous_replay_state: AuthorizationReplayState | None
    previous_freshness_state: FreshnessState | None
    previous_generation: int | None
    candidate: AuthorizationTransactionState


@dataclass(frozen=True)
class AuthorizationTransactionResult:
    code: AuthorizationTransactionCode
    prepared: AuthorizationPreparedTransaction | None = None
    state: AuthorizationTransactionState | None = None
    detail: str | None = None

    @property
    def prepared_ok(self) -> bool:
        return self.code is AuthorizationTransactionCode.PREPARED

    @property
    def committed(self) -> bool:
        return self.code is AuthorizationTransactionCode.COMMITTED

    @property
    def verified(self) -> bool:
        return self.code is AuthorizationTransactionCode.VERIFIED

    @property
    def consumed(self) -> bool:
        return self.code is AuthorizationTransactionCode.AUTHORIZATION_CONSUMED


def prepare_authorization_transaction(
    replay: AuthorizationReplayOutcome,
    *,
    previous_replay_state: AuthorizationReplayState | None,
    previous_freshness_state: FreshnessState | None,
    resulting_freshness_state: FreshnessState,
    previous_generation: int | None,
    operation_type: str,
) -> AuthorizationTransactionResult:
    """Prepare one composite transaction without making it authoritative."""

    if replay.code is not AuthorizationReplayCode.AUTHORIZATION_CANDIDATE:
        return AuthorizationTransactionResult(
            code=AuthorizationTransactionCode.AUTHORIZATION_NOT_CANDIDATE,
            detail="authorization replay evaluation is not a candidate",
        )

    if replay.candidate_state is None or replay.scope is None:
        return AuthorizationTransactionResult(
            code=AuthorizationTransactionCode.AUTHORIZATION_TRANSACTION_INVALID,
            detail="candidate replay outcome lacks required state",
        )

    if replay.authorization_id is None or replay.authorization_sequence is None:
        return AuthorizationTransactionResult(
            code=AuthorizationTransactionCode.AUTHORIZATION_TRANSACTION_INVALID,
            detail="candidate replay outcome lacks authorization identity",
        )

    if previous_replay_state != replay.previous_state:
        return AuthorizationTransactionResult(
            code=AuthorizationTransactionCode.AUTHORIZATION_TRANSACTION_INVALID,
            detail="previous replay state does not match evaluated candidate",
        )

    if previous_generation is None:
        generation = 0
    else:
        if (
            isinstance(previous_generation, bool)
            or not isinstance(previous_generation, int)
            or not 0 <= previous_generation <= UINT64_MAX
        ):
            return AuthorizationTransactionResult(
                code=AuthorizationTransactionCode.AUTHORIZATION_TRANSACTION_INVALID,
                detail="previous transaction generation is invalid",
            )

        if previous_generation == UINT64_MAX:
            return AuthorizationTransactionResult(
                code=AuthorizationTransactionCode.AUTHORIZATION_GENERATION_EXHAUSTED,
                detail="authorization transaction generation exhausted",
            )

        generation = previous_generation + 1

    try:
        state = AuthorizationTransactionState(
            replay_state=replay.candidate_state,
            replay_scope=replay.scope,
            generation=generation,
            resulting_freshness_state=resulting_freshness_state,
            operation_type=operation_type,
            authorization_id=replay.authorization_id,
            authorization_sequence=replay.authorization_sequence,
        )
    except ValueError as exc:
        return AuthorizationTransactionResult(
            code=AuthorizationTransactionCode.AUTHORIZATION_TRANSACTION_INVALID,
            detail=str(exc),
        )

    prepared = AuthorizationPreparedTransaction(
        previous_replay_state=previous_replay_state,
        previous_freshness_state=previous_freshness_state,
        previous_generation=previous_generation,
        candidate=state,
    )

    return AuthorizationTransactionResult(
        code=AuthorizationTransactionCode.PREPARED,
        prepared=prepared,
        state=state,
    )
