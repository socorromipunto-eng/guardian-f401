"""Guardian M15-03 freshness orchestration.

This module composes:

load -> evaluate -> candidate -> prepare -> commit -> verify

Only a successfully VERIFIED durable candidate becomes FRESH.

FIRST_SEEN does not bootstrap state.
EPOCH_TRANSITION_REQUIRED does not authorize an epoch transition.
Authentication and policy integration remain outside this module.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum

from .freshness_evaluator import evaluate_freshness
from .freshness_persistence import (
    FreshnessLoadCode,
    FreshnessPersistCode,
    FreshnessPersistenceBackend,
)
from .freshness_state import (
    FreshnessResultCode,
    FreshnessState,
    HighWaterState,
    UINT64_MAX,
)
from .validation import (
    validate_logical_time,
    validate_producer_epoch,
    validate_producer_id,
)


class FreshnessOutcomeCode(str, Enum):
    FRESH = "FRESH"
    FIRST_SEEN = "FIRST_SEEN"
    REPLAY = "REPLAY"
    LOGICAL_TIME_ROLLBACK = "LOGICAL_TIME_ROLLBACK"
    EPOCH_TRANSITION_REQUIRED = "EPOCH_TRANSITION_REQUIRED"
    EPOCH_ROLLBACK = "EPOCH_ROLLBACK"
    FRESHNESS_STATE_INVALID = "FRESHNESS_STATE_INVALID"
    FRESHNESS_STATE_UNAVAILABLE = "FRESHNESS_STATE_UNAVAILABLE"
    FRESHNESS_STATE_ROLLBACK = "FRESHNESS_STATE_ROLLBACK"
    FRESHNESS_STATE_PERSIST_FAILURE = "FRESHNESS_STATE_PERSIST_FAILURE"
    FRESHNESS_GENERATION_EXHAUSTED = "FRESHNESS_GENERATION_EXHAUSTED"


@dataclass(frozen=True)
class FreshnessOutcome:
    code: FreshnessOutcomeCode
    producer_id: str
    producer_epoch: str
    logical_time: int
    state: FreshnessState | None = None
    detail: str | None = None

    @property
    def fresh(self) -> bool:
        return self.code is FreshnessOutcomeCode.FRESH


def _terminal(
    code: FreshnessOutcomeCode,
    *,
    producer_id: str,
    producer_epoch: str,
    logical_time: int,
    detail: str | None = None,
) -> FreshnessOutcome:
    return FreshnessOutcome(
        code=code,
        producer_id=producer_id,
        producer_epoch=producer_epoch,
        logical_time=logical_time,
        detail=detail,
    )


def establish_freshness(
    backend: FreshnessPersistenceBackend,
    *,
    producer_id: str,
    producer_epoch: str,
    logical_time: int,
) -> FreshnessOutcome:
    """Evaluate and durably establish freshness for one existing producer state.

    FIRST_SEEN is deliberately non-establishing in this operation.
    """

    producer_id = validate_producer_id(producer_id)
    producer_epoch = validate_producer_epoch(producer_epoch)
    logical_time = validate_logical_time(logical_time)

    loaded = backend.load(producer_id)

    if loaded.code is FreshnessLoadCode.STATE_NOT_ESTABLISHED:
        return _terminal(
            FreshnessOutcomeCode.FIRST_SEEN,
            producer_id=producer_id,
            producer_epoch=producer_epoch,
            logical_time=logical_time,
            detail="bootstrap authorization is required outside freshness orchestration",
        )

    if loaded.code is FreshnessLoadCode.FRESHNESS_STATE_INVALID:
        return _terminal(
            FreshnessOutcomeCode.FRESHNESS_STATE_INVALID,
            producer_id=producer_id,
            producer_epoch=producer_epoch,
            logical_time=logical_time,
        )

    if loaded.code is FreshnessLoadCode.FRESHNESS_STATE_UNAVAILABLE:
        return _terminal(
            FreshnessOutcomeCode.FRESHNESS_STATE_UNAVAILABLE,
            producer_id=producer_id,
            producer_epoch=producer_epoch,
            logical_time=logical_time,
        )

    if loaded.code is FreshnessLoadCode.FRESHNESS_STATE_ROLLBACK:
        return _terminal(
            FreshnessOutcomeCode.FRESHNESS_STATE_ROLLBACK,
            producer_id=producer_id,
            producer_epoch=producer_epoch,
            logical_time=logical_time,
        )

    if loaded.code is not FreshnessLoadCode.STATE_VALID or loaded.state is None:
        return _terminal(
            FreshnessOutcomeCode.FRESHNESS_STATE_INVALID,
            producer_id=producer_id,
            producer_epoch=producer_epoch,
            logical_time=logical_time,
            detail="unexpected persistence load result",
        )

    previous_state = loaded.state

    evaluated = evaluate_freshness(
        previous_state,
        producer_id=producer_id,
        producer_epoch=producer_epoch,
        logical_time=logical_time,
    )

    terminal_codes = {
        FreshnessResultCode.REPLAY: FreshnessOutcomeCode.REPLAY,
        FreshnessResultCode.LOGICAL_TIME_ROLLBACK: FreshnessOutcomeCode.LOGICAL_TIME_ROLLBACK,
        FreshnessResultCode.EPOCH_TRANSITION_REQUIRED: FreshnessOutcomeCode.EPOCH_TRANSITION_REQUIRED,
        FreshnessResultCode.EPOCH_ROLLBACK: FreshnessOutcomeCode.EPOCH_ROLLBACK,
        FreshnessResultCode.FIRST_SEEN: FreshnessOutcomeCode.FIRST_SEEN,
    }

    if evaluated.code in terminal_codes:
        return _terminal(
            terminal_codes[evaluated.code],
            producer_id=producer_id,
            producer_epoch=producer_epoch,
            logical_time=logical_time,
            detail=evaluated.detail,
        )

    if evaluated.code is not FreshnessResultCode.FRESH_CANDIDATE:
        return _terminal(
            FreshnessOutcomeCode.FRESHNESS_STATE_INVALID,
            producer_id=producer_id,
            producer_epoch=producer_epoch,
            logical_time=logical_time,
            detail="unexpected freshness evaluator result",
        )

    if previous_state.generation == UINT64_MAX:
        return _terminal(
            FreshnessOutcomeCode.FRESHNESS_GENERATION_EXHAUSTED,
            producer_id=producer_id,
            producer_epoch=producer_epoch,
            logical_time=logical_time,
        )

    candidate_state = FreshnessState(
        producer_id=previous_state.producer_id,
        current_epoch=previous_state.current_epoch,
        previous_epoch=previous_state.previous_epoch,
        high_water_state=HighWaterState.SET,
        highest_logical_time=logical_time,
        transition_sequence=previous_state.transition_sequence,
        generation=previous_state.generation + 1,
    )

    try:
        prepared = backend.prepare(
            previous_state=previous_state,
            candidate_state=candidate_state,
        )
    except (ValueError, TypeError):
        return _terminal(
            FreshnessOutcomeCode.FRESHNESS_STATE_PERSIST_FAILURE,
            producer_id=producer_id,
            producer_epoch=producer_epoch,
            logical_time=logical_time,
            detail="persistence prepare failed",
        )

    committed = backend.commit(prepared)

    if committed.code is not FreshnessPersistCode.COMMITTED:
        return _terminal(
            FreshnessOutcomeCode.FRESHNESS_STATE_PERSIST_FAILURE,
            producer_id=producer_id,
            producer_epoch=producer_epoch,
            logical_time=logical_time,
            detail="persistent commit failed",
        )

    verified = backend.verify(prepared)

    if (
        verified.code is not FreshnessPersistCode.VERIFIED
        or verified.state != candidate_state
    ):
        return _terminal(
            FreshnessOutcomeCode.FRESHNESS_STATE_PERSIST_FAILURE,
            producer_id=producer_id,
            producer_epoch=producer_epoch,
            logical_time=logical_time,
            detail="persistent commit verification failed",
        )

    return FreshnessOutcome(
        code=FreshnessOutcomeCode.FRESH,
        producer_id=producer_id,
        producer_epoch=producer_epoch,
        logical_time=logical_time,
        state=candidate_state,
        detail="durable freshness state committed and verified",
    )
