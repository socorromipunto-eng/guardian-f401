"""M15 composite authorization transaction state-model tests."""

from __future__ import annotations

import unittest

from guardian_assurance.authorization_objects import BOOTSTRAP_TYPE
from guardian_assurance.authorization_replay import (
    UINT64_MAX,
    AuthorizationReplayCode,
    AuthorizationReplayOutcome,
    AuthorizationReplayScope,
    AuthorizationReplayState,
)
from guardian_assurance.authorization_transaction import (
    AuthorizationTransactionCode,
    prepare_authorization_transaction,
)
from guardian_assurance.authorization_trust_store import BOOTSTRAP_AUTHORITY
from guardian_assurance.freshness_state import FreshnessState, HighWaterState


def scope() -> AuthorizationReplayScope:
    return AuthorizationReplayScope(
        "root.bootstrap-authority:01",
        BOOTSTRAP_AUTHORITY,
        "plant-a.guardian-01",
    )


def replay_state(sequence: int) -> AuthorizationReplayState:
    return AuthorizationReplayState(scope(), sequence)


def candidate(
    *,
    sequence: int = 1,
    previous: AuthorizationReplayState | None = None,
) -> AuthorizationReplayOutcome:
    return AuthorizationReplayOutcome(
        code=AuthorizationReplayCode.AUTHORIZATION_CANDIDATE,
        scope=scope(),
        previous_state=previous,
        candidate_state=replay_state(sequence),
        authorization_id="0123456789abcdef0123456789abcdef",
        authorization_sequence=sequence,
    )


def freshness(*, generation: int = 0) -> FreshnessState:
    return FreshnessState(
        producer_id="plant-a.guardian-01",
        current_epoch="11111111111111111111111111111111",
        previous_epoch=None,
        high_water_state=HighWaterState.SET,
        highest_logical_time=1,
        transition_sequence=0,
        generation=generation,
    )

class M15AuthorizationTransactionTests(unittest.TestCase):
    def test_initial_transaction_prepares_generation_zero(self) -> None:
        result = prepare_authorization_transaction(
            candidate(),
            previous_replay_state=None,
            previous_freshness_state=None,
            resulting_freshness_state=freshness(),
            previous_generation=None,
            operation_type=BOOTSTRAP_TYPE,
        )
        self.assertEqual(result.code, AuthorizationTransactionCode.PREPARED)
        self.assertTrue(result.prepared_ok)
        self.assertEqual(result.state.generation, 0)

    def test_update_advances_transaction_generation(self) -> None:
        previous = replay_state(7)
        result = prepare_authorization_transaction(
            candidate(sequence=8, previous=previous),
            previous_replay_state=previous,
            previous_freshness_state=freshness(generation=4),
            resulting_freshness_state=freshness(generation=5),
            previous_generation=9,
            operation_type=BOOTSTRAP_TYPE,
        )
        self.assertEqual(result.code, AuthorizationTransactionCode.PREPARED)
        self.assertEqual(result.state.generation, 10)

    def test_generation_exhaustion_fails_closed(self) -> None:
        result = prepare_authorization_transaction(
            candidate(),
            previous_replay_state=None,
            previous_freshness_state=None,
            resulting_freshness_state=freshness(),
            previous_generation=UINT64_MAX,
            operation_type=BOOTSTRAP_TYPE,
        )
        self.assertEqual(
            result.code,
            AuthorizationTransactionCode.AUTHORIZATION_GENERATION_EXHAUSTED,
        )

    def test_non_candidate_cannot_prepare(self) -> None:
        replay = AuthorizationReplayOutcome(
            code=AuthorizationReplayCode.AUTHORIZATION_REPLAY
        )
        result = prepare_authorization_transaction(
            replay,
            previous_replay_state=None,
            previous_freshness_state=None,
            resulting_freshness_state=freshness(),
            previous_generation=None,
            operation_type=BOOTSTRAP_TYPE,
        )
        self.assertEqual(
            result.code,
            AuthorizationTransactionCode.AUTHORIZATION_NOT_CANDIDATE,
        )

    def test_stale_previous_replay_state_fails_closed(self) -> None:
        evaluated_previous = replay_state(7)
        stale_previous = replay_state(6)
        result = prepare_authorization_transaction(
            candidate(sequence=8, previous=evaluated_previous),
            previous_replay_state=stale_previous,
            previous_freshness_state=freshness(generation=4),
            resulting_freshness_state=freshness(generation=5),
            previous_generation=9,
            operation_type=BOOTSTRAP_TYPE,
        )
        self.assertEqual(
            result.code,
            AuthorizationTransactionCode.AUTHORIZATION_TRANSACTION_INVALID,
        )

    def test_prepared_is_not_committed_verified_or_consumed(self) -> None:
        result = prepare_authorization_transaction(
            candidate(),
            previous_replay_state=None,
            previous_freshness_state=None,
            resulting_freshness_state=freshness(),
            previous_generation=None,
            operation_type=BOOTSTRAP_TYPE,
        )
        self.assertTrue(result.prepared_ok)
        self.assertFalse(result.committed)
        self.assertFalse(result.verified)
        self.assertFalse(result.consumed)

    def test_consumed_sequence_equals_replay_high_water(self) -> None:
        result = prepare_authorization_transaction(
            candidate(sequence=9),
            previous_replay_state=None,
            previous_freshness_state=None,
            resulting_freshness_state=freshness(),
            previous_generation=None,
            operation_type=BOOTSTRAP_TYPE,
        )
        self.assertEqual(result.state.authorization_sequence, 9)
        self.assertEqual(result.state.replay_state.authorization_sequence, 9)


if __name__ == "__main__":
    unittest.main(verbosity=2)
