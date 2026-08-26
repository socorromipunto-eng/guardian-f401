"""Guardian M15-03 freshness persistence contract.

This module defines backend-neutral persistence interfaces and
machine-readable persistence results.

It does not implement filesystem, database, flash, authentication,
authorization, or final FRESH semantics.
"""

from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import dataclass
from enum import Enum

from .freshness_state import FreshnessState
from .validation import validate_producer_id


class FreshnessLoadCode(str, Enum):
    STATE_VALID = "STATE_VALID"
    STATE_NOT_ESTABLISHED = "STATE_NOT_ESTABLISHED"
    FRESHNESS_STATE_INVALID = "FRESHNESS_STATE_INVALID"
    FRESHNESS_STATE_UNAVAILABLE = "FRESHNESS_STATE_UNAVAILABLE"
    FRESHNESS_STATE_ROLLBACK = "FRESHNESS_STATE_ROLLBACK"


class FreshnessPersistCode(str, Enum):
    PREPARED = "PREPARED"
    COMMITTED = "COMMITTED"
    VERIFIED = "VERIFIED"
    FRESHNESS_STATE_PERSIST_FAILURE = "FRESHNESS_STATE_PERSIST_FAILURE"


@dataclass(frozen=True)
class FreshnessLoadResult:
    code: FreshnessLoadCode
    producer_id: str
    state: FreshnessState | None = None
    detail: str | None = None

    def __post_init__(self) -> None:
        validate_producer_id(self.producer_id)

        if self.code is FreshnessLoadCode.STATE_VALID:
            if self.state is None:
                raise ValueError("STATE_VALID requires state")

            if self.state.producer_id != self.producer_id:
                raise ValueError("loaded state producer mismatch")
        elif self.state is not None:
            raise ValueError("non-STATE_VALID load result must not carry state")

    @property
    def valid(self) -> bool:
        return self.code is FreshnessLoadCode.STATE_VALID


@dataclass(frozen=True)
class FreshnessPreparedWrite:
    """Opaque backend-neutral candidate state transition.

    Creating this object does not imply durable persistence.
    """

    producer_id: str
    previous_state: FreshnessState | None
    candidate_state: FreshnessState

    def __post_init__(self) -> None:
        validate_producer_id(self.producer_id)

        if self.candidate_state.producer_id != self.producer_id:
            raise ValueError("candidate state producer mismatch")

        if (
            self.previous_state is not None
            and self.previous_state.producer_id != self.producer_id
        ):
            raise ValueError("previous state producer mismatch")


@dataclass(frozen=True)
class FreshnessPersistResult:
    code: FreshnessPersistCode
    producer_id: str
    state: FreshnessState | None = None
    detail: str | None = None

    def __post_init__(self) -> None:
        validate_producer_id(self.producer_id)

        if self.state is not None and self.state.producer_id != self.producer_id:
            raise ValueError("persist result state producer mismatch")

    @property
    def prepared(self) -> bool:
        return self.code is FreshnessPersistCode.PREPARED

    @property
    def committed(self) -> bool:
        return self.code is FreshnessPersistCode.COMMITTED

    @property
    def verified(self) -> bool:
        return self.code is FreshnessPersistCode.VERIFIED


class FreshnessPersistenceBackend(ABC):
    """Backend contract for persistent M15-03 freshness state."""

    @abstractmethod
    def load(self, producer_id: str) -> FreshnessLoadResult:
        """Load and classify one producer freshness state."""

    @abstractmethod
    def prepare(
        self,
        *,
        previous_state: FreshnessState | None,
        candidate_state: FreshnessState,
    ) -> FreshnessPreparedWrite:
        """Prepare one non-durable replacement candidate."""

    @abstractmethod
    def commit(
        self,
        prepared: FreshnessPreparedWrite,
    ) -> FreshnessPersistResult:
        """Attempt durable commit of one prepared candidate."""

    @abstractmethod
    def verify(
        self,
        prepared: FreshnessPreparedWrite,
    ) -> FreshnessPersistResult:
        """Verify that the prepared candidate is durably committed."""
