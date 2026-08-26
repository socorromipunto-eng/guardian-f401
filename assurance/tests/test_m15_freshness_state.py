"""Guardian M15-03 pure freshness-state model tests."""

from __future__ import annotations

import unittest

from guardian_assurance.errors import AssuranceError
from guardian_assurance.freshness_state import (
    FreshnessEvaluation,
    FreshnessResultCode,
    FreshnessState,
    FreshnessStateError,
    HighWaterState,
    PORTABLE_MAX_PRODUCER_STATES,
    UINT64_MAX,
    make_unset_state,
    validate_uint64,
)


PRODUCER = "guardian-primary"
EPOCH_A = "0" * 32
EPOCH_B = "1" * 32


class M15FreshnessStateTests(unittest.TestCase):

    def test_portable_capacity_is_4096(self) -> None:
        self.assertEqual(PORTABLE_MAX_PRODUCER_STATES, 4096)

    def test_unset_state_is_explicit(self) -> None:
        state = make_unset_state(
            producer_id=PRODUCER,
            current_epoch=EPOCH_A,
        )

        self.assertEqual(state.high_water_state, HighWaterState.UNSET)
        self.assertIsNone(state.highest_logical_time)

    def test_set_state_requires_logical_time(self) -> None:
        with self.assertRaises(FreshnessStateError):
            FreshnessState(
                producer_id=PRODUCER,
                current_epoch=EPOCH_A,
                previous_epoch=None,
                high_water_state=HighWaterState.SET,
                highest_logical_time=None,
                transition_sequence=0,
                generation=0,
            )

    def test_unset_state_rejects_numeric_high_water(self) -> None:
        with self.assertRaises(FreshnessStateError):
            FreshnessState(
                producer_id=PRODUCER,
                current_epoch=EPOCH_A,
                previous_epoch=None,
                high_water_state=HighWaterState.UNSET,
                highest_logical_time=0,
                transition_sequence=0,
                generation=0,
            )

    def test_logical_time_zero_is_valid_when_set(self) -> None:
        state = FreshnessState(
            producer_id=PRODUCER,
            current_epoch=EPOCH_A,
            previous_epoch=None,
            high_water_state=HighWaterState.SET,
            highest_logical_time=0,
            transition_sequence=0,
            generation=0,
        )

        self.assertEqual(state.highest_logical_time, 0)

    def test_previous_epoch_must_differ(self) -> None:
        with self.assertRaises(FreshnessStateError):
            make_unset_state(
                producer_id=PRODUCER,
                current_epoch=EPOCH_A,
                previous_epoch=EPOCH_A,
            )

    def test_previous_epoch_may_be_distinct(self) -> None:
        state = make_unset_state(
            producer_id=PRODUCER,
            current_epoch=EPOCH_B,
            previous_epoch=EPOCH_A,
        )

        self.assertEqual(state.previous_epoch, EPOCH_A)

    def test_invalid_producer_uses_m14_contract(self) -> None:
        with self.assertRaises(AssuranceError):
            make_unset_state(
                producer_id="bad producer",
                current_epoch=EPOCH_A,
            )

    def test_invalid_epoch_uses_m14_contract(self) -> None:
        with self.assertRaises(AssuranceError):
            make_unset_state(
                producer_id=PRODUCER,
                current_epoch="x" * 32,
            )

    def test_uint64_boundaries(self) -> None:
        self.assertEqual(validate_uint64(0, "value"), 0)
        self.assertEqual(validate_uint64(UINT64_MAX, "value"), UINT64_MAX)

    def test_uint64_rejects_negative(self) -> None:
        with self.assertRaises(FreshnessStateError):
            validate_uint64(-1, "value")

    def test_uint64_rejects_overflow(self) -> None:
        with self.assertRaises(FreshnessStateError):
            validate_uint64(UINT64_MAX + 1, "value")

    def test_uint64_rejects_boolean(self) -> None:
        with self.assertRaises(FreshnessStateError):
            validate_uint64(True, "value")

    def test_result_code_contains_no_final_fresh(self) -> None:
        self.assertNotIn("FRESH", FreshnessResultCode.__members__)
        self.assertIn("FRESH_CANDIDATE", FreshnessResultCode.__members__)

    def test_fresh_candidate_is_not_final_fresh(self) -> None:
        result = FreshnessEvaluation(
            code=FreshnessResultCode.FRESH_CANDIDATE,
            producer_id=PRODUCER,
            producer_epoch=EPOCH_A,
            logical_time=1,
        )

        self.assertTrue(result.candidate)
        self.assertFalse(hasattr(result, "fresh"))
        self.assertFalse(hasattr(result, "authorized"))

    def test_all_frozen_result_codes_are_present(self) -> None:
        expected = {
            "FRESH_CANDIDATE",
            "REPLAY",
            "LOGICAL_TIME_ROLLBACK",
            "EPOCH_TRANSITION_REQUIRED",
            "EPOCH_ROLLBACK",
            "FIRST_SEEN",
            "FRESHNESS_CAPACITY_EXCEEDED",
            "FRESHNESS_SEQUENCE_EXHAUSTED",
            "FRESHNESS_GENERATION_EXHAUSTED",
            "FRESHNESS_STATE_INVALID",
            "FRESHNESS_STATE_UNAVAILABLE",
            "FRESHNESS_STATE_ROLLBACK",
            "FRESHNESS_STATE_PERSIST_FAILURE",
        }

        self.assertEqual(set(FreshnessResultCode.__members__), expected)


if __name__ == "__main__":
    unittest.main(verbosity=2)
