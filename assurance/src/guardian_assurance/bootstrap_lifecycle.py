"""M15 bootstrap authorization lifecycle.

Bootstrap is permitted only after all independent preconditions have already
been established:

- ordinary assurance authentication succeeded;
- freshness produced FIRST_SEEN;
- the exact signed authorization object authenticated successfully;
- authorization anti-replay produced AUTHORIZATION_CANDIDATE;
- producer and epoch bindings match exactly.

FIRST_SEEN alone never establishes state.
AUTHORIZATION_AUTHENTICATED alone never consumes authorization.
AUTHORIZATION_CANDIDATE alone never consumes authorization.
COMMITTED is not VERIFIED.
A bootstrap result becomes authoritative only after VERIFIED composite
persistence and successful freshness projection reconciliation.

This module does not perform policy authorization or permit actuation.
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
    BOOTSTRAP_TYPE,
    HIGH_WATER_ESTABLISHED,
    HIGH_WATER_UNSET,
    BootstrapAuthorization,
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
    make_unset_state,
)


class BootstrapLifecycleCode(str, Enum):
    BOOTSTRAP_AUTHORIZED = "BOOTSTRAP_AUTHORIZED"

    ORDINARY_AUTHENTICATION_REQUIRED = (
        "BOOTSTRAP_ORDINARY_AUTHENTICATION_REQUIRED"
    )
    FIRST_SEEN_REQUIRED = "BOOTSTRAP_FIRST_SEEN_REQUIRED"
    AUTHORIZATION_AUTHENTICATION_REQUIRED = (
        "BOOTSTRAP_AUTHORIZATION_AUTHENTICATION_REQUIRED"
    )
    AUTHORIZATION_OBJECT_INVALID = (
        "BOOTSTRAP_AUTHORIZATION_OBJECT_INVALID"
    )
    AUTHORIZATION_REPLAY_NOT_CANDIDATE = (
        "BOOTSTRAP_AUTHORIZATION_REPLAY_NOT_CANDIDATE"
    )
    PRODUCER_MISMATCH = "BOOTSTRAP_PRODUCER_MISMATCH"
    EPOCH_MISMATCH = "BOOTSTRAP_EPOCH_MISMATCH"
    LOGICAL_TIME_MISMATCH = "BOOTSTRAP_LOGICAL_TIME_MISMATCH"

    TRANSACTION_PREPARE_FAILURE = (
        "BOOTSTRAP_TRANSACTION_PREPARE_FAILURE"
    )
    TRANSACTION_BACKEND_PREPARE_FAILURE = (
        "BOOTSTRAP_TRANSACTION_BACKEND_PREPARE_FAILURE"
    )
    TRANSACTION_COMMIT_FAILURE = (
        "BOOTSTRAP_TRANSACTION_COMMIT_FAILURE"
    )
    TRANSACTION_VERIFY_FAILURE = (
        "BOOTSTRAP_TRANSACTION_VERIFY_FAILURE"
    )
    FRESHNESS_RECONCILIATION_FAILURE = (
        "BOOTSTRAP_FRESHNESS_RECONCILIATION_FAILURE"
    )


@dataclass(frozen=True)
class BootstrapLifecycleResult:
    code: BootstrapLifecycleCode
    transaction_state: AuthorizationTransactionState | None = None
    freshness_state: FreshnessState | None = None
    reconciliation: ReconciliationResult | None = None
    detail: str | None = None

    @property
    def authorization_consumed(self) -> bool:
        return self.code is BootstrapLifecycleCode.BOOTSTRAP_AUTHORIZED

    @property
    def bootstrap_authorized(self) -> bool:
        return self.code is BootstrapLifecycleCode.BOOTSTRAP_AUTHORIZED


def _initial_freshness_state(
    authorization: BootstrapAuthorization,
) -> FreshnessState:
    if authorization.initial_high_water_state == HIGH_WATER_UNSET:
        return make_unset_state(
            producer_id=authorization.producer_id,
            current_epoch=authorization.producer_epoch,
            previous_epoch=None,
            transition_sequence=0,
            generation=0,
        )

    if authorization.initial_high_water_state == HIGH_WATER_ESTABLISHED:
        if authorization.initial_logical_time is None:
            raise ValueError(
                "HIGH_WATER_ESTABLISHED requires initial_logical_time"
            )

        return FreshnessState(
            producer_id=authorization.producer_id,
            current_epoch=authorization.producer_epoch,
            previous_epoch=None,
            high_water_state=HighWaterState.SET,
            highest_logical_time=authorization.initial_logical_time,
            transition_sequence=0,
            generation=0,
        )

    raise ValueError("unsupported bootstrap high-water state")


def execute_bootstrap_lifecycle(
    *,
    ordinary_authentication: AuthenticationResult,
    freshness: FreshnessOutcome,
    authorization_authentication: AuthorizationAuthenticationResult,
    replay: AuthorizationReplayOutcome,
    previous_replay_state: AuthorizationReplayState | None,
    established_scope_count: int,
    transaction_backend: AuthorizationTransactionBackend,
    freshness_backend: FreshnessPersistenceBackend,
) -> BootstrapLifecycleResult:
    """Execute one bounded bootstrap lifecycle fail closed."""

    # --------------------------------------------------------
    # Independent preconditions
    # --------------------------------------------------------

    if not ordinary_authentication.authenticated:
        return BootstrapLifecycleResult(
            BootstrapLifecycleCode.ORDINARY_AUTHENTICATION_REQUIRED,
            detail="ordinary assurance authentication did not succeed",
        )

    if freshness.code is not FreshnessOutcomeCode.FIRST_SEEN:
        return BootstrapLifecycleResult(
            BootstrapLifecycleCode.FIRST_SEEN_REQUIRED,
            detail="bootstrap requires FIRST_SEEN freshness outcome",
        )

    if not authorization_authentication.authenticated:
        return BootstrapLifecycleResult(
            BootstrapLifecycleCode.AUTHORIZATION_AUTHENTICATION_REQUIRED,
            detail="authorization authentication did not succeed",
        )

    authorization = authorization_authentication.authorization_object

    if not isinstance(authorization, BootstrapAuthorization):
        return BootstrapLifecycleResult(
            BootstrapLifecycleCode.AUTHORIZATION_OBJECT_INVALID,
            detail="authenticated authorization is not BootstrapAuthorization",
        )

    if authorization.authorization_type != BOOTSTRAP_TYPE:
        return BootstrapLifecycleResult(
            BootstrapLifecycleCode.AUTHORIZATION_OBJECT_INVALID,
            detail="authenticated authorization_type is not bootstrap",
        )

    if replay.code is not AuthorizationReplayCode.AUTHORIZATION_CANDIDATE:
        return BootstrapLifecycleResult(
            BootstrapLifecycleCode.AUTHORIZATION_REPLAY_NOT_CANDIDATE,
            detail="authorization replay evaluation is not candidate",
        )

    # --------------------------------------------------------
    # Exact subject binding
    # --------------------------------------------------------

    if authorization.producer_id != freshness.producer_id:
        return BootstrapLifecycleResult(
            BootstrapLifecycleCode.PRODUCER_MISMATCH,
            detail="bootstrap producer does not match FIRST_SEEN producer",
        )

    if authorization.producer_epoch != freshness.producer_epoch:
        return BootstrapLifecycleResult(
            BootstrapLifecycleCode.EPOCH_MISMATCH,
            detail="bootstrap epoch does not match FIRST_SEEN epoch",
        )

    if (
        authorization.initial_high_water_state
        == HIGH_WATER_ESTABLISHED
        and authorization.initial_logical_time != freshness.logical_time
    ):
        return BootstrapLifecycleResult(
            BootstrapLifecycleCode.LOGICAL_TIME_MISMATCH,
            detail=(
                "bootstrap established high-water does not match "
                "triggering FIRST_SEEN logical_time"
            ),
        )

    # --------------------------------------------------------
    # Construct exact initial freshness target
    # --------------------------------------------------------

    try:
        resulting_freshness_state = _initial_freshness_state(
            authorization
        )
    except (ValueError, TypeError):
        return BootstrapLifecycleResult(
            BootstrapLifecycleCode.AUTHORIZATION_OBJECT_INVALID,
            detail="bootstrap initial freshness state is invalid",
        )

    # --------------------------------------------------------
    # Prepare one composite authorization transaction
    # --------------------------------------------------------

    transaction = prepare_authorization_transaction(
        replay,
        previous_replay_state=previous_replay_state,
        previous_freshness_state=None,
        resulting_freshness_state=resulting_freshness_state,
        previous_generation=None,
        operation_type=BOOTSTRAP_TYPE,
    )

    if (
        transaction.code is not AuthorizationTransactionCode.PREPARED
        or transaction.prepared is None
    ):
        return BootstrapLifecycleResult(
            BootstrapLifecycleCode.TRANSACTION_PREPARE_FAILURE,
            detail=transaction.detail,
        )

    prepared = transaction.prepared

    backend_prepared = transaction_backend.prepare(prepared)

    if backend_prepared.code is not AuthorizationPersistCode.PREPARED:
        return BootstrapLifecycleResult(
            BootstrapLifecycleCode.TRANSACTION_BACKEND_PREPARE_FAILURE,
            detail=backend_prepared.detail,
        )

    # --------------------------------------------------------
    # Durable composite transaction
    # --------------------------------------------------------

    committed = transaction_backend.commit(prepared)

    if (
        committed.code is not AuthorizationPersistCode.COMMITTED
        or committed.state != prepared.candidate
    ):
        return BootstrapLifecycleResult(
            BootstrapLifecycleCode.TRANSACTION_COMMIT_FAILURE,
            detail=committed.detail,
        )

    verified = transaction_backend.verify(prepared)

    if (
        verified.code is not AuthorizationPersistCode.VERIFIED
        or verified.state != prepared.candidate
    ):
        return BootstrapLifecycleResult(
            BootstrapLifecycleCode.TRANSACTION_VERIFY_FAILURE,
            detail=verified.detail,
        )

    # --------------------------------------------------------
    # Reconcile ordinary freshness projection from VERIFIED
    # composite state.
    # --------------------------------------------------------

    reconciliation = reconcile_verified_authorization_transaction(
        transaction_code=verified.code,
        transaction_state=verified.state,
        freshness_backend=freshness_backend,
    )

    if not reconciliation.reconciled:
        return BootstrapLifecycleResult(
            BootstrapLifecycleCode.FRESHNESS_RECONCILIATION_FAILURE,
            transaction_state=verified.state,
            freshness_state=resulting_freshness_state,
            reconciliation=reconciliation,
            detail=reconciliation.detail,
        )

    # --------------------------------------------------------
    # Only here is authorization consumed and bootstrap
    # authoritative.
    # --------------------------------------------------------

    return BootstrapLifecycleResult(
        BootstrapLifecycleCode.BOOTSTRAP_AUTHORIZED,
        transaction_state=verified.state,
        freshness_state=resulting_freshness_state,
        reconciliation=reconciliation,
    )