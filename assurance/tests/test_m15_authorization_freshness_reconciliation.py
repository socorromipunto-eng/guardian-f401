"""M15 authorization/freshness reconciliation tests."""

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from guardian_assurance.authorization_freshness_reconciliation import (
    ReconciliationCode,
    reconcile_verified_authorization_transaction,
)
from guardian_assurance.authorization_objects import BOOTSTRAP_TYPE
from guardian_assurance.authorization_persistence import (
    AuthorizationPersistCode,
)
from guardian_assurance.authorization_replay import (
    AuthorizationReplayScope,
    AuthorizationReplayState,
)
from guardian_assurance.authorization_transaction import (
    AuthorizationTransactionState,
)
from guardian_assurance.authorization_trust_store import BOOTSTRAP_AUTHORITY
from guardian_assurance.freshness_host_backend import (
    HostFreshnessPersistenceBackend,
)
from guardian_assurance.freshness_state import (
    FreshnessState,
    HighWaterState,
)


def scope() -> AuthorizationReplayScope:
    return AuthorizationReplayScope(
        authority_id="root.bootstrap-authority:01",
        purpose_domain=BOOTSTRAP_AUTHORITY,
        producer_id="plant-a.guardian-01",
    )


def freshness_state(
    *,
    generation: int,
    logical_time: int,
) -> FreshnessState:
    return FreshnessState(
        producer_id="plant-a.guardian-01",
        current_epoch="11111111111111111111111111111111",
        previous_epoch=None,
        high_water_state=HighWaterState.SET,
        highest_logical_time=logical_time,
        transition_sequence=0,
        generation=generation,
    )


def transaction(
    *,
    freshness_generation: int = 0,
    logical_time: int = 1,
    transaction_generation: int = 0,
) -> AuthorizationTransactionState:
    replay_scope = scope()

    replay = AuthorizationReplayState(
        scope=replay_scope,
        authorization_sequence=1,
    )

    return AuthorizationTransactionState(
        replay_state=replay,
        replay_scope=replay_scope,
        generation=transaction_generation,
        resulting_freshness_state=freshness_state(
            generation=freshness_generation,
            logical_time=logical_time,
        ),
        operation_type=BOOTSTRAP_TYPE,
        authorization_id="0123456789abcdef0123456789abcdef",
        authorization_sequence=1,
    )


class M15AuthorizationFreshnessReconciliationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.backend = HostFreshnessPersistenceBackend(
            Path(self.temp.name)
        )

    def tearDown(self) -> None:
        self.temp.cleanup()

    def persist_projection(
        self,
        state: FreshnessState,
    ) -> None:
        prepared = self.backend.prepare(
            previous_state=None,
            candidate_state=state,
        )

        committed = self.backend.commit(prepared)
        self.assertTrue(committed.committed)

        verified = self.backend.verify(prepared)
        self.assertTrue(verified.verified)

    def test_non_verified_transaction_cannot_reconcile(self) -> None:
        target = transaction()

        result = reconcile_verified_authorization_transaction(
            transaction_code=AuthorizationPersistCode.COMMITTED,
            transaction_state=target,
            freshness_backend=self.backend,
        )

        self.assertEqual(
            result.code,
            ReconciliationCode.TRANSACTION_NOT_VERIFIED,
        )

        loaded = self.backend.load(
            target.resulting_freshness_state.producer_id
        )

        self.assertIsNone(loaded.state)

    def test_verified_transaction_establishes_missing_projection(
        self,
    ) -> None:
        target = transaction()

        result = reconcile_verified_authorization_transaction(
            transaction_code=AuthorizationPersistCode.VERIFIED,
            transaction_state=target,
            freshness_backend=self.backend,
        )

        self.assertEqual(
            result.code,
            ReconciliationCode.RECONCILED,
        )

        loaded = self.backend.load(
            target.resulting_freshness_state.producer_id
        )

        self.assertEqual(
            loaded.state,
            target.resulting_freshness_state,
        )

    def test_reconciliation_is_idempotent(self) -> None:
        target = transaction()

        first = reconcile_verified_authorization_transaction(
            transaction_code=AuthorizationPersistCode.VERIFIED,
            transaction_state=target,
            freshness_backend=self.backend,
        )

        second = reconcile_verified_authorization_transaction(
            transaction_code=AuthorizationPersistCode.VERIFIED,
            transaction_state=target,
            freshness_backend=self.backend,
        )

        self.assertEqual(
            first.code,
            ReconciliationCode.RECONCILED,
        )
        self.assertEqual(
            second.code,
            ReconciliationCode.ALREADY_RECONCILED,
        )
        self.assertTrue(second.reconciled)

    def test_older_projection_may_advance_to_verified_target(
        self,
    ) -> None:
        previous = freshness_state(
            generation=3,
            logical_time=4,
        )

        self.persist_projection(previous)

        target = transaction(
            freshness_generation=4,
            logical_time=5,
            transaction_generation=8,
        )

        result = reconcile_verified_authorization_transaction(
            transaction_code=AuthorizationPersistCode.VERIFIED,
            transaction_state=target,
            freshness_backend=self.backend,
        )

        self.assertEqual(
            result.code,
            ReconciliationCode.RECONCILED,
        )

        loaded = self.backend.load(previous.producer_id)

        self.assertEqual(
            loaded.state,
            target.resulting_freshness_state,
        )

    def test_newer_projection_is_not_overwritten(self) -> None:
        newer = freshness_state(
            generation=5,
            logical_time=6,
        )

        self.persist_projection(newer)

        target = transaction(
            freshness_generation=4,
            logical_time=5,
        )

        result = reconcile_verified_authorization_transaction(
            transaction_code=AuthorizationPersistCode.VERIFIED,
            transaction_state=target,
            freshness_backend=self.backend,
        )

        self.assertEqual(
            result.code,
            ReconciliationCode.FRESHNESS_STATE_CONFLICT,
        )

        loaded = self.backend.load(newer.producer_id)

        self.assertEqual(
            loaded.state,
            newer,
        )

    def test_equal_generation_conflict_is_not_overwritten(
        self,
    ) -> None:
        current = freshness_state(
            generation=3,
            logical_time=3,
        )

        self.persist_projection(current)

        target = transaction(
            freshness_generation=3,
            logical_time=4,
        )

        result = reconcile_verified_authorization_transaction(
            transaction_code=AuthorizationPersistCode.VERIFIED,
            transaction_state=target,
            freshness_backend=self.backend,
        )

        self.assertEqual(
            result.code,
            ReconciliationCode.FRESHNESS_STATE_CONFLICT,
        )

        loaded = self.backend.load(current.producer_id)

        self.assertEqual(
            loaded.state,
            current,
        )


if __name__ == "__main__":
    unittest.main(verbosity=2)