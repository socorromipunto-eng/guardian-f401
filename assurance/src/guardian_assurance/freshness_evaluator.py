"""Guardian M15-03 pure freshness evaluator.

This module evaluates authenticated freshness claims against one logical
FreshnessState record.

It performs no I/O, persistence, durable commit, authentication,
authorization, or final FRESH decision.
"""

from __future__ import annotations

from .freshness_state import (
    FreshnessEvaluation,
    FreshnessResultCode,
    FreshnessState,
    FreshnessStateError,
    HighWaterState,
)
from .validation import (
    validate_logical_time,
    validate_producer_epoch,
    validate_producer_id,
)


def evaluate_freshness(
    state: FreshnessState | None,
    *,
    producer_id: str,
    producer_epoch: str,
    logical_time: int,
) -> FreshnessEvaluation:
    """Evaluate one statement against current logical freshness state.

    FRESH_CANDIDATE is deliberately not final FRESH.
    """

    producer_id = validate_producer_id(producer_id)
    producer_epoch = validate_producer_epoch(producer_epoch)
    logical_time = validate_logical_time(logical_time)

    if state is None:
        return FreshnessEvaluation(
            code=FreshnessResultCode.FIRST_SEEN,
            producer_id=producer_id,
            producer_epoch=producer_epoch,
            logical_time=logical_time,
            detail="no established freshness state for producer",
        )

    if state.producer_id != producer_id:
        raise FreshnessStateError(
            "freshness state producer_id does not match evaluated producer"
        )

    if producer_epoch == state.current_epoch:
        if state.high_water_state is HighWaterState.UNSET:
            return FreshnessEvaluation(
                code=FreshnessResultCode.FRESH_CANDIDATE,
                producer_id=producer_id,
                producer_epoch=producer_epoch,
                logical_time=logical_time,
                detail="current epoch has no committed high-water value",
            )

        if state.highest_logical_time is None:
            raise FreshnessStateError(
                "HIGH_WATER_SET state has no logical_time"
            )

        if logical_time > state.highest_logical_time:
            return FreshnessEvaluation(
                code=FreshnessResultCode.FRESH_CANDIDATE,
                producer_id=producer_id,
                producer_epoch=producer_epoch,
                logical_time=logical_time,
                detail="logical_time advances committed high-water mark",
            )

        if logical_time == state.highest_logical_time:
            return FreshnessEvaluation(
                code=FreshnessResultCode.REPLAY,
                producer_id=producer_id,
                producer_epoch=producer_epoch,
                logical_time=logical_time,
                detail="logical_time equals committed high-water mark",
            )

        return FreshnessEvaluation(
            code=FreshnessResultCode.LOGICAL_TIME_ROLLBACK,
            producer_id=producer_id,
            producer_epoch=producer_epoch,
            logical_time=logical_time,
            detail="logical_time is below committed high-water mark",
        )

    if (
        state.previous_epoch is not None
        and producer_epoch == state.previous_epoch
    ):
        return FreshnessEvaluation(
            code=FreshnessResultCode.EPOCH_ROLLBACK,
            producer_id=producer_id,
            producer_epoch=producer_epoch,
            logical_time=logical_time,
            detail="authenticated statement uses retained superseded epoch",
        )

    return FreshnessEvaluation(
        code=FreshnessResultCode.EPOCH_TRANSITION_REQUIRED,
        producer_id=producer_id,
        producer_epoch=producer_epoch,
        logical_time=logical_time,
        detail="different epoch requires explicit transition authorization",
    )
