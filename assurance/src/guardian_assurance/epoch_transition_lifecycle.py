"""M15 epoch-transition authorization lifecycle.

A verified epoch-transition authorization may change the authoritative producer
epoch only through one verified composite transaction.

EPOCH_TRANSITION_REQUIRED != EPOCH_TRANSITION_AUTHORIZED
AUTHORIZATION_AUTHENTICATED != AUTHORIZATION_CANDIDATE
AUTHORIZATION_CANDIDATE != AUTHORIZATION_CONSUMED
PREPARED != COMMITTED
COMMITTED != VERIFIED
EPOCH_TRANSITION_AUTHORIZED != FRESH

The exact authenticated EpochTransitionAuthorization drives the transition.
The triggering assurance statement is not declared FRESH by this lifecycle.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum

from .authentication import AuthenticationResult
from .authorization_authentication import (
    AuthorizationAuthenticationResult,
)
from .authorization_freshness_reconciliation import (
    ReconciliationResult,
    reconcile_verified_authorization_transaction,
)
from .authorization_objects import (
    EPOCH_TRANSITION_TYPE,
    EpochTransitionAuthorization,
)
from .authorization_persistence import (
    AuthorizationPersistCode,
    AuthorizationTransactionBackend,
)
from .authorization_replay import (
    AuthorizationReplayCode,
    AuthorizationReplayOutcome,
    AuthorizationReplayState,
)
from .authorization_transaction import (
    AuthorizationTransactionCode,
    AuthorizationTransactionState,
    prepare_authorization_transaction,
)
from .freshness_orchestrator import (
    FreshnessOutcome,
    FreshnessOutcomeCode,
)
from .freshness_persistence import FreshnessPersistenceBackend
from .freshness_state import (
    FreshnessState,
    HighWaterState,
    UINT64_MAX,
)


class EpochTransitionLifecycleCode(str, Enum):
    EPOCH_TRANSITION_AUTHORIZED = "EPOCH_TRANSITION_AUTHORIZED"

    ORDINARY_AUTHENTICATION_REQUIRED = (
        "EPOCH_TRANSITION_ORDINARY_AUTHENTICATION_REQUIRED"
    )
    EPOCH_TRANSITION_REQUIRED = (
        "EPOCH_TRANSITION_FRESHNESS_REQUIRED"
    )
    AUTHORIZATION_AUTHENTICATION_REQUIRED = (
        "EPOCH_TRANSITION_AUTHORIZATION_AUTHENTICATION_REQUIRED"
    )
    AUTHORIZATION_OBJECT_INVALID = (
        "EPOCH_TRANSITION_AUTHORIZATION_OBJECT_INVALID"
    )
    AUTHORIZATION_REPLAY_NOT_CANDIDATE = (
        "EPOCH_TRANSITION_AUTHORIZATION_REPLAY_NOT_CANDIDATE"
    )

    PRODUCER_MISMATCH = "EPOCH_TRANSITION_PRODUCER_MISMATCH"
    FROM_EPOCH_MISMATCH = "EPOCH_TRANSITION_FROM_EPOCH_MISMATCH"
    TO_EPOCH_MISMATCH = "EPOCH_TRANSITION_TO_EPOCH_MISMATCH"
    TRANSITION_SEQUENCE_MISMATCH = (
        "EPOCH_TRANSITION_SEQUENCE_MISMATCH"
    )
    TRANSITION_SEQUENCE_EXHAUSTED = (
        "EPOCH_TRANSITION_SEQUENCE_EXHAUSTED"
    )
    FRESHNESS_GENERATION_EXHAUSTED = (
        "EPOCH_TRANSITION_FRESHNESS_GENERATION_EXHAUSTED"
    )

    TRANSACTION_PREPARE_FAILURE = (
        "EPOCH_TRANSITION_TRANSACTION_PREPARE_FAILURE"
    )
    TRANSACTION_BACKEND_PREPARE_FAILURE = (
        "EPOCH_TRANSITION_BACKEND_PREPARE_FAILURE"
    )
    TRANSACTION_COMMIT_FAILURE = (
        "EPOCH_TRANSITION_TRANSACTION_COMMIT_FAILURE"
    )
    TRANSACTION_VERIFY_FAILURE = (
        "EPOCH_TRANSITION_TRANSACTION_VERIFY_FAILURE"
    )
    FRESHNESS_RECONCILIATION_FAILURE = (
        "EPOCH_TRANSITION_FRESHNESS_RECONCILIATION_FAILURE"
    )


@dataclass(frozen=True)
class EpochTransitionLifecycleResult:
    code: EpochTransitionLifecycleCode
    transaction_state: AuthorizationTransactionState | None = None
    freshness_state: FreshnessState | None = None
    reconciliation: ReconciliationResult | None = None
    detail: str | None = None

    @property
    def epoch_transition_authorized(self) -> bool:
        return (
            self.code
            is EpochTransitionLifecycleCode.EPOCH_TRANSITION_AUTHORIZED
        )

    @property
    def authorization_consumed(self) -> bool:
        return self.epoch_transition_authorized


def _build_transition_freshness_state(
    *,
    previous: FreshnessState,
    authorization: EpochTransitionAuthorization,
) -> FreshnessState:
    """Construct the new-epoch state without declaring a statement FRESH."""

    if previous.transition_sequence == UINT64_MAX:
        raise OverflowError("transition_sequence exhausted")

    if previous.generation == UINT64_MAX:
        raise OverflowError("freshness generation exhausted")

    return FreshnessState(
        producer_id=previous.producer_id,
        current_epoch=authorization.to_epoch,
        previous_epoch=previous.current_epoch,
        high_water_state=HighWaterState.UNSET,
        highest_logical_time=None,
        transition_sequence=authorization.transition_sequence,
        generation=previous.generation + 1,
    )


def execute_epoch_transition_lifecycle(
    *,
    ordinary_authentication: AuthenticationResult,
    freshness: FreshnessOutcome,
    authorization_authentication: AuthorizationAuthenticationResult,
    replay: AuthorizationReplayOutcome,
    previous_replay_state: AuthorizationReplayState | None,
    previous_freshness_state: FreshnessState,
    previous_transaction_generation: int,
    transaction_backend: AuthorizationTransactionBackend,
    freshness_backend: FreshnessPersistenceBackend,
) -> EpochTransitionLifecycleResult:
    """Execute one explicit epoch transition fail closed."""

    # --------------------------------------------------------
    # Independent gates
    # --------------------------------------------------------

    if not ordinary_authentication.authenticated:
        return EpochTransitionLifecycleResult(
            EpochTransitionLifecycleCode.ORDINARY_AUTHENTICATION_REQUIRED,
            detail="ordinary assurance authentication did not succeed",
        )

    if (
        freshness.code
        is not FreshnessOutcomeCode.EPOCH_TRANSITION_REQUIRED
    ):
        return EpochTransitionLifecycleResult(
            EpochTransitionLifecycleCode.EPOCH_TRANSITION_REQUIRED,
            detail="freshness did not require an epoch transition",
        )

    if not authorization_authentication.authenticated:
        return EpochTransitionLifecycleResult(
            EpochTransitionLifecycleCode.AUTHORIZATION_AUTHENTICATION_REQUIRED,
            detail="authorization authentication did not succeed",
        )

    authorization = authorization_authentication.authorization_object

    if not isinstance(authorization, EpochTransitionAuthorization):
        return EpochTransitionLifecycleResult(
            EpochTransitionLifecycleCode.AUTHORIZATION_OBJECT_INVALID,
            detail=(
                "authenticated authorization is not "
                "EpochTransitionAuthorization"
            ),
        )

    if authorization.authorization_type != EPOCH_TRANSITION_TYPE:
        return EpochTransitionLifecycleResult(
            EpochTransitionLifecycleCode.AUTHORIZATION_OBJECT_INVALID,
            detail="authorization_type is not epoch transition",
        )

    if (
        replay.code
        is not AuthorizationReplayCode.AUTHORIZATION_CANDIDATE
    ):
        return EpochTransitionLifecycleResult(
            EpochTransitionLifecycleCode.AUTHORIZATION_REPLAY_NOT_CANDIDATE,
            detail="authorization replay evaluation is not candidate",
        )

    # --------------------------------------------------------
    # Exact producer and epoch binding
    # --------------------------------------------------------

    if authorization.producer_id != previous_freshness_state.producer_id:
        return EpochTransitionLifecycleResult(
            EpochTransitionLifecycleCode.PRODUCER_MISMATCH,
            detail="authorization producer does not match authoritative state",
        )

    if authorization.producer_id != freshness.producer_id:
        return EpochTransitionLifecycleResult(
            EpochTransitionLifecycleCode.PRODUCER_MISMATCH,
            detail="authorization producer does not match triggering statement",
        )

    if authorization.from_epoch != previous_freshness_state.current_epoch:
        return EpochTransitionLifecycleResult(
            EpochTransitionLifecycleCode.FROM_EPOCH_MISMATCH,
            detail="from_epoch is not the authoritative current_epoch",
        )

    if authorization.to_epoch != freshness.producer_epoch:
        return EpochTransitionLifecycleResult(
            EpochTransitionLifecycleCode.TO_EPOCH_MISMATCH,
            detail="to_epoch does not match triggering statement epoch",
        )

    # --------------------------------------------------------
    # Exact transition sequence
    # --------------------------------------------------------

    if previous_freshness_state.transition_sequence == UINT64_MAX:
        return EpochTransitionLifecycleResult(
            EpochTransitionLifecycleCode.TRANSITION_SEQUENCE_EXHAUSTED,
            detail="transition_sequence cannot advance",
        )

    expected_transition_sequence = (
        previous_freshness_state.transition_sequence + 1
    )

    if authorization.transition_sequence != expected_transition_sequence:
        return EpochTransitionLifecycleResult(
            EpochTransitionLifecycleCode.TRANSITION_SEQUENCE_MISMATCH,
            detail="transition_sequence is not the exact next sequence",
        )

    if previous_freshness_state.generation == UINT64_MAX:
        return EpochTransitionLifecycleResult(
            EpochTransitionLifecycleCode.FRESHNESS_GENERATION_EXHAUSTED,
            detail="freshness generation cannot advance",
        )

    # --------------------------------------------------------
    # Construct new authoritative epoch state.
    #
    # High-water is deliberately UNSET. The triggering statement
    # is NOT declared FRESH here. It must subsequently satisfy the
    # ordinary M15-03 logical-time rule in the new epoch.
    # --------------------------------------------------------

    try:
        target_freshness = _build_transition_freshness_state(
            previous=previous_freshness_state,
            authorization=authorization,
        )
    except OverflowError as exc:
        return EpochTransitionLifecycleResult(
            EpochTransitionLifecycleCode.FRESHNESS_GENERATION_EXHAUSTED,
            detail=str(exc),
        )
    except (ValueError, TypeError) as exc:
        return EpochTransitionLifecycleResult(
            EpochTransitionLifecycleCode.AUTHORIZATION_OBJECT_INVALID,
            detail=str(exc),
        )

    # --------------------------------------------------------
    # Prepare one composite transaction
    # --------------------------------------------------------

    transaction = prepare_authorization_transaction(
        replay,
        previous_replay_state=previous_replay_state,
        previous_freshness_state=previous_freshness_state,
        resulting_freshness_state=target_freshness,
        previous_generation=previous_transaction_generation,
        operation_type=EPOCH_TRANSITION_TYPE,
    )

    if (
        transaction.code is not AuthorizationTransactionCode.PREPARED
        or transaction.prepared is None
    ):
        return EpochTransitionLifecycleResult(
            EpochTransitionLifecycleCode.TRANSACTION_PREPARE_FAILURE,
            detail=transaction.detail,
        )

    prepared = transaction.prepared

    backend_prepared = transaction_backend.prepare(prepared)

    if backend_prepared.code is not AuthorizationPersistCode.PREPARED:
        return EpochTransitionLifecycleResult(
            EpochTransitionLifecycleCode.TRANSACTION_BACKEND_PREPARE_FAILURE,
            detail=backend_prepared.detail,
        )

    # --------------------------------------------------------
    # Durable composite transition
    # --------------------------------------------------------

    committed = transaction_backend.commit(prepared)

    if (
        committed.code is not AuthorizationPersistCode.COMMITTED
        or committed.state != prepared.candidate
    ):
        return EpochTransitionLifecycleResult(
            EpochTransitionLifecycleCode.TRANSACTION_COMMIT_FAILURE,
            detail=committed.detail,
        )

    verified = transaction_backend.verify(prepared)

    if (
        verified.code is not AuthorizationPersistCode.VERIFIED
        or verified.state != prepared.candidate
    ):
        return EpochTransitionLifecycleResult(
            EpochTransitionLifecycleCode.TRANSACTION_VERIFY_FAILURE,
            detail=verified.detail,
        )

    # --------------------------------------------------------
    # Reconcile normal freshness projection only from VERIFIED
    # composite state.
    # --------------------------------------------------------

    reconciliation = reconcile_verified_authorization_transaction(
        transaction_code=verified.code,
        transaction_state=verified.state,
        freshness_backend=freshness_backend,
    )

    if not reconciliation.reconciled:
        return EpochTransitionLifecycleResult(
            EpochTransitionLifecycleCode.FRESHNESS_RECONCILIATION_FAILURE,
            transaction_state=verified.state,
            freshness_state=target_freshness,
            reconciliation=reconciliation,
            detail=reconciliation.detail,
        )

    # --------------------------------------------------------
    # Only now is the authorization consumed and the epoch
    # transition authoritative.
    #
    # This result still does NOT mean the triggering statement
    # is FRESH.
    # --------------------------------------------------------

    return EpochTransitionLifecycleResult(
        EpochTransitionLifecycleCode.EPOCH_TRANSITION_AUTHORIZED,
        transaction_state=verified.state,
        freshness_state=target_freshness,
        reconciliation=reconciliation,
    )