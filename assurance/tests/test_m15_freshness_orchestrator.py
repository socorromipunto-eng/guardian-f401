"""Guardian M15-03 freshness orchestrator tests."""

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path
from unittest import mock

from guardian_assurance.freshness_host_backend import HostFreshnessPersistenceBackend
from guardian_assurance.freshness_orchestrator import (
    FreshnessOutcomeCode,
    establish_freshness,
)
from guardian_assurance.freshness_persistence import (
    FreshnessLoadCode,
    FreshnessLoadResult,
    FreshnessPersistCode,
    FreshnessPersistResult,
)
from guardian_assurance.freshness_state import (
    FreshnessState,
    HighWaterState,
    UINT64_MAX,
)


PRODUCER = "guardian-primary"
EPOCH_A = "0" * 32
EPOCH_B = "1" * 32
EPOCH_C = "2" * 32


def state(
    *,
    generation: int = 0,
    logical_time: int = 10,
    current_epoch: str = EPOCH_A,
    previous_epoch: str | None = None,
) -> FreshnessState:
    return FreshnessState(
        producer_id=PRODUCER,
        current_epoch=current_epoch,
        previous_epoch=previous_epoch,
        high_water_state=HighWaterState.SET,
        highest_logical_time=logical_time,
        transition_sequence=0,
        generation=generation,
    )


class M15FreshnessOrchestratorTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.backend = HostFreshnessPersistenceBackend(Path(self.temp.name))

    def tearDown(self) -> None:
        self.temp.cleanup()

    def _install(self, value: FreshnessState) -> None:
        prepared = self.backend.prepare(
            previous_state=None,
            candidate_state=value,
        )
        committed = self.backend.commit(prepared)
        self.assertEqual(committed.code, FreshnessPersistCode.COMMITTED)
        verified = self.backend.verify(prepared)
        self.assertEqual(verified.code, FreshnessPersistCode.VERIFIED)

    def test_first_seen_does_not_bootstrap_or_become_fresh(self) -> None:
        result = establish_freshness(
            self.backend,
            producer_id=PRODUCER,
            producer_epoch=EPOCH_A,
            logical_time=1,
        )

        self.assertEqual(result.code, FreshnessOutcomeCode.FIRST_SEEN)
        self.assertFalse(result.fresh)
        self.assertEqual(
            self.backend.load(PRODUCER).code,
            FreshnessLoadCode.STATE_NOT_ESTABLISHED,
        )

    def test_candidate_becomes_fresh_only_after_verified_commit(self) -> None:
        initial = state(generation=0, logical_time=10)
        self._install(initial)

        result = establish_freshness(
            self.backend,
            producer_id=PRODUCER,
            producer_epoch=EPOCH_A,
            logical_time=11,
        )

        self.assertEqual(result.code, FreshnessOutcomeCode.FRESH)
        self.assertTrue(result.fresh)
        self.assertEqual(result.state.generation, 1)
        self.assertEqual(result.state.highest_logical_time, 11)

    def test_replay_is_terminal_without_write(self) -> None:
        initial = state(generation=0, logical_time=10)
        self._install(initial)

        result = establish_freshness(
            self.backend,
            producer_id=PRODUCER,
            producer_epoch=EPOCH_A,
            logical_time=10,
        )

        self.assertEqual(result.code, FreshnessOutcomeCode.REPLAY)
        self.assertFalse(result.fresh)
        self.assertEqual(self.backend.load(PRODUCER).state, initial)

    def test_lower_time_is_terminal_without_write(self) -> None:
        initial = state(generation=0, logical_time=10)
        self._install(initial)

        result = establish_freshness(
            self.backend,
            producer_id=PRODUCER,
            producer_epoch=EPOCH_A,
            logical_time=9,
        )

        self.assertEqual(
            result.code,
            FreshnessOutcomeCode.LOGICAL_TIME_ROLLBACK,
        )
        self.assertEqual(self.backend.load(PRODUCER).state, initial)

    def test_previous_epoch_is_terminal_without_write(self) -> None:
        initial = state(
            generation=3,
            logical_time=5,
            current_epoch=EPOCH_B,
            previous_epoch=EPOCH_A,
        )
        self._install(initial)

        result = establish_freshness(
            self.backend,
            producer_id=PRODUCER,
            producer_epoch=EPOCH_A,
            logical_time=100,
        )

        self.assertEqual(result.code, FreshnessOutcomeCode.EPOCH_ROLLBACK)
        self.assertEqual(self.backend.load(PRODUCER).state, initial)

    def test_different_epoch_requires_external_transition_authorization(self) -> None:
        initial = state(
            generation=1,
            current_epoch=EPOCH_B,
            previous_epoch=EPOCH_A,
        )
        self._install(initial)

        result = establish_freshness(
            self.backend,
            producer_id=PRODUCER,
            producer_epoch=EPOCH_C,
            logical_time=999,
        )

        self.assertEqual(
            result.code,
            FreshnessOutcomeCode.EPOCH_TRANSITION_REQUIRED,
        )
        self.assertFalse(result.fresh)
        self.assertEqual(self.backend.load(PRODUCER).state, initial)

    def test_invalid_state_is_never_first_seen(self) -> None:
        path = self.backend._state_path(PRODUCER)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(b"not-json")

        result = establish_freshness(
            self.backend,
            producer_id=PRODUCER,
            producer_epoch=EPOCH_A,
            logical_time=1,
        )

        self.assertEqual(
            result.code,
            FreshnessOutcomeCode.FRESHNESS_STATE_INVALID,
        )
        self.assertNotEqual(result.code, FreshnessOutcomeCode.FIRST_SEEN)

    def test_unavailable_state_is_never_first_seen(self) -> None:
        with mock.patch.object(
            self.backend,
            "load",
            return_value=FreshnessLoadResult(
                code=FreshnessLoadCode.FRESHNESS_STATE_UNAVAILABLE,
                producer_id=PRODUCER,
            ),
        ):
            result = establish_freshness(
                self.backend,
                producer_id=PRODUCER,
                producer_epoch=EPOCH_A,
                logical_time=1,
            )

        self.assertEqual(
            result.code,
            FreshnessOutcomeCode.FRESHNESS_STATE_UNAVAILABLE,
        )

    def test_detected_state_rollback_is_never_first_seen(self) -> None:
        with mock.patch.object(
            self.backend,
            "load",
            return_value=FreshnessLoadResult(
                code=FreshnessLoadCode.FRESHNESS_STATE_ROLLBACK,
                producer_id=PRODUCER,
            ),
        ):
            result = establish_freshness(
                self.backend,
                producer_id=PRODUCER,
                producer_epoch=EPOCH_A,
                logical_time=1,
            )

        self.assertEqual(
            result.code,
            FreshnessOutcomeCode.FRESHNESS_STATE_ROLLBACK,
        )

    def test_commit_failure_never_becomes_fresh(self) -> None:
        initial = state(generation=0, logical_time=10)
        self._install(initial)

        with mock.patch.object(
            self.backend,
            "commit",
            return_value=FreshnessPersistResult(
                code=FreshnessPersistCode.FRESHNESS_STATE_PERSIST_FAILURE,
                producer_id=PRODUCER,
            ),
        ):
            result = establish_freshness(
                self.backend,
                producer_id=PRODUCER,
                producer_epoch=EPOCH_A,
                logical_time=11,
            )

        self.assertEqual(
            result.code,
            FreshnessOutcomeCode.FRESHNESS_STATE_PERSIST_FAILURE,
        )
        self.assertFalse(result.fresh)

    def test_verify_failure_never_becomes_fresh(self) -> None:
        initial = state(generation=0, logical_time=10)
        self._install(initial)

        with mock.patch.object(
            self.backend,
            "verify",
            return_value=FreshnessPersistResult(
                code=FreshnessPersistCode.FRESHNESS_STATE_PERSIST_FAILURE,
                producer_id=PRODUCER,
            ),
        ):
            result = establish_freshness(
                self.backend,
                producer_id=PRODUCER,
                producer_epoch=EPOCH_A,
                logical_time=11,
            )

        self.assertEqual(
            result.code,
            FreshnessOutcomeCode.FRESHNESS_STATE_PERSIST_FAILURE,
        )
        self.assertFalse(result.fresh)

    def test_generation_exhaustion_fails_closed_before_write(self) -> None:
        initial = state(generation=UINT64_MAX, logical_time=10)
        self._install(initial)

        result = establish_freshness(
            self.backend,
            producer_id=PRODUCER,
            producer_epoch=EPOCH_A,
            logical_time=11,
        )

        self.assertEqual(
            result.code,
            FreshnessOutcomeCode.FRESHNESS_GENERATION_EXHAUSTED,
        )
        self.assertEqual(self.backend.load(PRODUCER).state, initial)

    def test_success_preserves_epoch_and_transition_sequence(self) -> None:
        initial = FreshnessState(
            producer_id=PRODUCER,
            current_epoch=EPOCH_B,
            previous_epoch=EPOCH_A,
            high_water_state=HighWaterState.SET,
            highest_logical_time=10,
            transition_sequence=7,
            generation=3,
        )
        self._install(initial)

        result = establish_freshness(
            self.backend,
            producer_id=PRODUCER,
            producer_epoch=EPOCH_B,
            logical_time=11,
        )

        self.assertTrue(result.fresh)
        self.assertEqual(result.state.current_epoch, EPOCH_B)
        self.assertEqual(result.state.previous_epoch, EPOCH_A)
        self.assertEqual(result.state.transition_sequence, 7)
        self.assertEqual(result.state.generation, 4)


if __name__ == "__main__":
    unittest.main(verbosity=2)
