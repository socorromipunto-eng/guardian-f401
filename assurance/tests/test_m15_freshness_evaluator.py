"""Guardian M15-03 pure freshness evaluator tests."""

from __future__ import annotations

import unittest

from guardian_assurance.errors import AssuranceError
from guardian_assurance.freshness_evaluator import evaluate_freshness
from guardian_assurance.freshness_state import (
    FreshnessResultCode,
    FreshnessState,
    FreshnessStateError,
    HighWaterState,
    make_unset_state,
)


PRODUCER = "guardian-primary"
OTHER_PRODUCER = "guardian-secondary"
EPOCH_A = "0" * 32
EPOCH_B = "1" * 32
EPOCH_C = "2" * 32


def set_state(
    *,
    current_epoch: str = EPOCH_A,
    previous_epoch: str | None = None,
    high_water: int = 10,
) -> FreshnessState:
    return FreshnessState(
        producer_id=PRODUCER,
        current_epoch=current_epoch,
        previous_epoch=previous_epoch,
        high_water_state=HighWaterState.SET,
        highest_logical_time=high_water,
        transition_sequence=0,
        generation=0,
    )


class M15FreshnessEvaluatorTests(unittest.TestCase):

    def test_no_state_is_first_seen(self) -> None:
        result = evaluate_freshness(
            None,
            producer_id=PRODUCER,
            producer_epoch=EPOCH_A,
            logical_time=0,
        )

        self.assertEqual(result.code, FreshnessResultCode.FIRST_SEEN)
        self.assertFalse(result.candidate)

    def test_unset_current_epoch_is_candidate(self) -> None:
        state = make_unset_state(
            producer_id=PRODUCER,
            current_epoch=EPOCH_A,
        )

        result = evaluate_freshness(
            state,
            producer_id=PRODUCER,
            producer_epoch=EPOCH_A,
            logical_time=0,
        )

        self.assertEqual(
            result.code,
            FreshnessResultCode.FRESH_CANDIDATE,
        )
        self.assertTrue(result.candidate)

    def test_greater_time_is_candidate(self) -> None:
        result = evaluate_freshness(
            set_state(high_water=10),
            producer_id=PRODUCER,
            producer_epoch=EPOCH_A,
            logical_time=11,
        )

        self.assertEqual(
            result.code,
            FreshnessResultCode.FRESH_CANDIDATE,
        )

    def test_equal_time_is_replay(self) -> None:
        result = evaluate_freshness(
            set_state(high_water=10),
            producer_id=PRODUCER,
            producer_epoch=EPOCH_A,
            logical_time=10,
        )

        self.assertEqual(result.code, FreshnessResultCode.REPLAY)
        self.assertFalse(result.candidate)

    def test_lower_time_is_logical_time_rollback(self) -> None:
        result = evaluate_freshness(
            set_state(high_water=10),
            producer_id=PRODUCER,
            producer_epoch=EPOCH_A,
            logical_time=9,
        )

        self.assertEqual(
            result.code,
            FreshnessResultCode.LOGICAL_TIME_ROLLBACK,
        )

    def test_previous_epoch_is_epoch_rollback(self) -> None:
        state = set_state(
            current_epoch=EPOCH_B,
            previous_epoch=EPOCH_A,
            high_water=4,
        )

        result = evaluate_freshness(
            state,
            producer_id=PRODUCER,
            producer_epoch=EPOCH_A,
            logical_time=100,
        )

        self.assertEqual(
            result.code,
            FreshnessResultCode.EPOCH_ROLLBACK,
        )

    def test_unknown_different_epoch_requires_transition(self) -> None:
        state = set_state(
            current_epoch=EPOCH_B,
            previous_epoch=EPOCH_A,
        )

        result = evaluate_freshness(
            state,
            producer_id=PRODUCER,
            producer_epoch=EPOCH_C,
            logical_time=0,
        )

        self.assertEqual(
            result.code,
            FreshnessResultCode.EPOCH_TRANSITION_REQUIRED,
        )

    def test_previous_epoch_precedence_beats_high_logical_time(self) -> None:
        state = set_state(
            current_epoch=EPOCH_B,
            previous_epoch=EPOCH_A,
            high_water=2,
        )

        result = evaluate_freshness(
            state,
            producer_id=PRODUCER,
            producer_epoch=EPOCH_A,
            logical_time=999,
        )

        self.assertEqual(
            result.code,
            FreshnessResultCode.EPOCH_ROLLBACK,
        )

    def test_different_epoch_never_becomes_candidate_from_time(self) -> None:
        result = evaluate_freshness(
            set_state(current_epoch=EPOCH_A, high_water=100),
            producer_id=PRODUCER,
            producer_epoch=EPOCH_B,
            logical_time=1000,
        )

        self.assertEqual(
            result.code,
            FreshnessResultCode.EPOCH_TRANSITION_REQUIRED,
        )

    def test_state_producer_mismatch_fails_closed(self) -> None:
        with self.assertRaises(FreshnessStateError):
            evaluate_freshness(
                set_state(),
                producer_id=OTHER_PRODUCER,
                producer_epoch=EPOCH_A,
                logical_time=11,
            )

    def test_invalid_producer_uses_shared_contract(self) -> None:
        with self.assertRaises(AssuranceError):
            evaluate_freshness(
                None,
                producer_id="bad producer",
                producer_epoch=EPOCH_A,
                logical_time=1,
            )

    def test_invalid_epoch_uses_shared_contract(self) -> None:
        with self.assertRaises(AssuranceError):
            evaluate_freshness(
                None,
                producer_id=PRODUCER,
                producer_epoch="x" * 32,
                logical_time=1,
            )

    def test_invalid_logical_time_uses_shared_contract(self) -> None:
        with self.assertRaises(AssuranceError):
            evaluate_freshness(
                None,
                producer_id=PRODUCER,
                producer_epoch=EPOCH_A,
                logical_time=True,
            )

    def test_evaluator_does_not_mutate_state(self) -> None:
        state = set_state(high_water=10)

        before = state

        evaluate_freshness(
            state,
            producer_id=PRODUCER,
            producer_epoch=EPOCH_A,
            logical_time=11,
        )

        self.assertEqual(state, before)
        self.assertEqual(state.highest_logical_time, 10)
        self.assertEqual(state.generation, 0)

    def test_candidate_does_not_claim_final_fresh(self) -> None:
        result = evaluate_freshness(
            set_state(high_water=10),
            producer_id=PRODUCER,
            producer_epoch=EPOCH_A,
            logical_time=11,
        )

        self.assertTrue(result.candidate)
        self.assertFalse(hasattr(result, "fresh"))
        self.assertFalse(hasattr(result, "authorized"))


if __name__ == "__main__":
    unittest.main(verbosity=2)
