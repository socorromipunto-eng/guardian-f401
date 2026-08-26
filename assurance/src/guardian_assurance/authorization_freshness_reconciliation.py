"""Reconcile one VERIFIED authorization transaction into freshness storage.

The composite authorization transaction is authoritative for the protected
bootstrap or epoch-transition mutation. The ordinary freshness backend is a
projection used by normal freshness processing.

Only a VERIFIED composite transaction may update that projection.

This module does not authenticate authorizations, consume authorizations,
execute bootstrap, execute epoch transition, authorize policy, or permit
actuation.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum

from .authorization_persistence import AuthorizationPersistCode
from .authorization_transaction import AuthorizationTransactionState
from .freshness_persistence import (
    FreshnessLoadCode,
    FreshnessPersistCode,
    FreshnessPersistenceBackend,
)
from .freshness_state import FreshnessState


class ReconciliationCode(str, Enum):
    RECONCILED = "AUTHORIZATION_FRESHNESS_RECONCILED"
    ALREADY_RECONCILED = "AUTHORIZATION_FRESHNESS_ALREADY_RECONCILED"
    TRANSACTION_NOT_VERIFIED = "AUTHORIZATION_TRANSACTION_NOT_VERIFIED"
    FRESHNESS_STATE_INVALID = "AUTHORIZATION_FRESHNESS_STATE_INVALID"
    FRESHNESS_STATE_UNAVAILABLE = "AUTHORIZATION_FRESHNESS_STATE_UNAVAILABLE"
    FRESHNESS_STATE_ROLLBACK = "AUTHORIZATION_FRESHNESS_STATE_ROLLBACK"
    FRESHNESS_STATE_CONFLICT = "AUTHORIZATION_FRESHNESS_STATE_CONFLICT"
    FRESHNESS_STATE_PERSIST_FAILURE = (
        "AUTHORIZATION_FRESHNESS_STATE_PERSIST_FAILURE"
    )


@dataclass(frozen=True)
class ReconciliationResult:
    code: ReconciliationCode
    state: FreshnessState | None = None
    detail: str | None = None

    @property
    def reconciled(self) -> bool:
        return self.code in (
            ReconciliationCode.RECONCILED,
            ReconciliationCode.ALREADY_RECONCILED,
        )


def reconcile_verified_authorization_transaction(
    *,
    transaction_code: AuthorizationPersistCode,
    transaction_state: AuthorizationTransactionState,
    freshness_backend: FreshnessPersistenceBackend,
) -> ReconciliationResult:
    """Project one VERIFIED composite transaction into freshness storage."""

    if transaction_code is not AuthorizationPersistCode.VERIFIED:
        return ReconciliationResult(
            ReconciliationCode.TRANSACTION_NOT_VERIFIED,
            detail="composite authorization transaction is not VERIFIED",
        )

    target = transaction_state.resulting_freshness_state
    loaded = freshness_backend.load(target.producer_id)

    if loaded.code is FreshnessLoadCode.FRESHNESS_STATE_INVALID:
        return ReconciliationResult(
            ReconciliationCode.FRESHNESS_STATE_INVALID,
            detail="freshness projection is invalid",
        )

    if loaded.code is FreshnessLoadCode.FRESHNESS_STATE_UNAVAILABLE:
        return ReconciliationResult(
            ReconciliationCode.FRESHNESS_STATE_UNAVAILABLE,
            detail="freshness projection is unavailable",
        )

    if loaded.code is FreshnessLoadCode.FRESHNESS_STATE_ROLLBACK:
        return ReconciliationResult(
            ReconciliationCode.FRESHNESS_STATE_ROLLBACK,
            detail="freshness projection reports rollback",
        )

    if loaded.code is FreshnessLoadCode.STATE_VALID:
        current = loaded.state

        if current is None:
            return ReconciliationResult(
                ReconciliationCode.FRESHNESS_STATE_INVALID,
                detail="STATE_VALID returned no freshness state",
            )

        if current == target:
            return ReconciliationResult(
                ReconciliationCode.ALREADY_RECONCILED,
                state=target,
            )

        if current.generation >= target.generation:
            return ReconciliationResult(
                ReconciliationCode.FRESHNESS_STATE_CONFLICT,
                state=current,
                detail="freshness projection is newer or conflicting",
            )

        previous = current

    elif loaded.code is FreshnessLoadCode.STATE_NOT_ESTABLISHED:
        previous = None

    else:
        return ReconciliationResult(
            ReconciliationCode.FRESHNESS_STATE_INVALID,
            detail="unexpected freshness load result",
        )

    try:
        prepared = freshness_backend.prepare(
            previous_state=previous,
            candidate_state=target,
        )
    except (ValueError, TypeError):
        return ReconciliationResult(
            ReconciliationCode.FRESHNESS_STATE_PERSIST_FAILURE,
            detail="freshness projection prepare failed",
        )

    committed = freshness_backend.commit(prepared)

    if (
        committed.code is not FreshnessPersistCode.COMMITTED
        or committed.state != target
    ):
        return ReconciliationResult(
            ReconciliationCode.FRESHNESS_STATE_PERSIST_FAILURE,
            detail="freshness projection commit failed",
        )

    verified = freshness_backend.verify(prepared)

    if (
        verified.code is not FreshnessPersistCode.VERIFIED
        or verified.state != target
    ):
        return ReconciliationResult(
            ReconciliationCode.FRESHNESS_STATE_PERSIST_FAILURE,
            detail="freshness projection verification failed",
        )

    return ReconciliationResult(
        ReconciliationCode.RECONCILED,
        state=target,
    )