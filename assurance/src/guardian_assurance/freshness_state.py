"""Guardian M15-03 persistent freshness state model.

This module defines bounded logical state and machine-readable result
classes only.

It performs no I/O, persistence, authentication, authorization, or
final FRESH decision.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from typing import Any

from .validation import (
    validate_logical_time,
    validate_producer_epoch,
    validate_producer_id,
)


PORTABLE_MAX_PRODUCER_STATES = 4_096
UINT64_MAX = (1 << 64) - 1


class HighWaterState(str, Enum):
    UNSET = "HIGH_WATER_UNSET"
    SET = "HIGH_WATER_SET"


class FreshnessResultCode(str, Enum):
    FRESH_CANDIDATE = "FRESH_CANDIDATE"
    REPLAY = "REPLAY"
    LOGICAL_TIME_ROLLBACK = "LOGICAL_TIME_ROLLBACK"
    EPOCH_TRANSITION_REQUIRED = "EPOCH_TRANSITION_REQUIRED"
    EPOCH_ROLLBACK = "EPOCH_ROLLBACK"
    FIRST_SEEN = "FIRST_SEEN"
    FRESHNESS_CAPACITY_EXCEEDED = "FRESHNESS_CAPACITY_EXCEEDED"
    FRESHNESS_SEQUENCE_EXHAUSTED = "FRESHNESS_SEQUENCE_EXHAUSTED"
    FRESHNESS_GENERATION_EXHAUSTED = "FRESHNESS_GENERATION_EXHAUSTED"
    FRESHNESS_STATE_INVALID = "FRESHNESS_STATE_INVALID"
    FRESHNESS_STATE_UNAVAILABLE = "FRESHNESS_STATE_UNAVAILABLE"
    FRESHNESS_STATE_ROLLBACK = "FRESHNESS_STATE_ROLLBACK"
    FRESHNESS_STATE_PERSIST_FAILURE = "FRESHNESS_STATE_PERSIST_FAILURE"


class FreshnessStateError(ValueError):
    """Invalid logical M15-03 freshness state."""


def validate_uint64(value: Any, field: str) -> int:
    if (
        isinstance(value, bool)
        or not isinstance(value, int)
        or not 0 <= value <= UINT64_MAX
    ):
        raise FreshnessStateError(f"invalid {field}")

    return value


@dataclass(frozen=True)
class FreshnessState:
    """One bounded logical freshness-state record for one producer."""

    producer_id: str
    current_epoch: str
    previous_epoch: str | None
    high_water_state: HighWaterState
    highest_logical_time: int | None
    transition_sequence: int
    generation: int

    def __post_init__(self) -> None:
        validate_producer_id(self.producer_id)
        validate_producer_epoch(self.current_epoch)

        if self.previous_epoch is not None:
            validate_producer_epoch(self.previous_epoch)

            if self.previous_epoch == self.current_epoch:
                raise FreshnessStateError(
                    "previous_epoch must differ from current_epoch"
                )

        if not isinstance(self.high_water_state, HighWaterState):
            raise FreshnessStateError("invalid high_water_state")

        if self.high_water_state is HighWaterState.UNSET:
            if self.highest_logical_time is not None:
                raise FreshnessStateError(
                    "HIGH_WATER_UNSET requires no logical_time value"
                )
        else:
            if self.highest_logical_time is None:
                raise FreshnessStateError(
                    "HIGH_WATER_SET requires logical_time value"
                )

            validate_logical_time(self.highest_logical_time)

        validate_uint64(
            self.transition_sequence,
            "transition_sequence",
        )

        validate_uint64(
            self.generation,
            "generation",
        )


@dataclass(frozen=True)
class FreshnessEvaluation:
    """Pure pre-persistence freshness evaluation result.

    FRESH_CANDIDATE is deliberately not final FRESH.
    """

    code: FreshnessResultCode
    producer_id: str
    producer_epoch: str
    logical_time: int
    detail: str | None = None

    @property
    def candidate(self) -> bool:
        return self.code is FreshnessResultCode.FRESH_CANDIDATE


def make_unset_state(
    *,
    producer_id: str,
    current_epoch: str,
    previous_epoch: str | None = None,
    transition_sequence: int = 0,
    generation: int = 0,
) -> FreshnessState:
    """Construct one explicit HIGH_WATER_UNSET state."""

    return FreshnessState(
        producer_id=producer_id,
        current_epoch=current_epoch,
        previous_epoch=previous_epoch,
        high_water_state=HighWaterState.UNSET,
        highest_logical_time=None,
        transition_sequence=transition_sequence,
        generation=generation,
    )
