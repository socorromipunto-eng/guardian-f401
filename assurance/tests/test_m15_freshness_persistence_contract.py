"""Guardian M15-03 freshness persistence-interface tests."""

from __future__ import annotations

import unittest

from guardian_assurance.freshness_persistence import (
    FreshnessLoadCode,
    FreshnessLoadResult,
    FreshnessPersistCode,
    FreshnessPersistResult,
    FreshnessPersistenceBackend,
    FreshnessPreparedWrite,
)
from guardian_assurance.freshness_state import (
    FreshnessState,
    HighWaterState,
    make_unset_state,
)


PRODUCER = "guardian-primary"
OTHER = "guardian-secondary"
EPOCH = "0" * 32


def state(
    *,
    producer_id: str = PRODUCER,
    generation: int = 0,
) -> FreshnessState:
    return FreshnessState(
        producer_id=producer_id,
        current_epoch=EPOCH,
        previous_epoch=None,
        high_water_state=HighWaterState.SET,
        highest_logical_time=1,
        transition_sequence=0,
        generation=generation,
    )


class DummyBackend(FreshnessPersistenceBackend):
    def load(self, producer_id: str) -> FreshnessLoadResult:
        return FreshnessLoadResult(
            code=FreshnessLoadCode.STATE_NOT_ESTABLISHED,
            producer_id=producer_id,
        )

    def prepare(
        self,
        *,
        previous_state: FreshnessState | None,
        candidate_state: FreshnessState,
    ) -> FreshnessPreparedWrite:
        return FreshnessPreparedWrite(
            producer_id=candidate_state.producer_id,
            previous_state=previous_state,
            candidate_state=candidate_state,
        )

    def commit(
        self,
        prepared: FreshnessPreparedWrite,
    ) -> FreshnessPersistResult:
        return FreshnessPersistResult(
            code=FreshnessPersistCode.COMMITTED,
            producer_id=prepared.producer_id,
            state=prepared.candidate_state,
        )

    def verify(
        self,
        prepared: FreshnessPreparedWrite,
    ) -> FreshnessPersistResult:
        return FreshnessPersistResult(
            code=FreshnessPersistCode.VERIFIED,
            producer_id=prepared.producer_id,
            state=prepared.candidate_state,
        )


class M15FreshnessPersistenceContractTests(unittest.TestCase):

    def test_load_codes_are_exact(self) -> None:
        expected = {
            "STATE_VALID",
            "STATE_NOT_ESTABLISHED",
            "FRESHNESS_STATE_INVALID",
            "FRESHNESS_STATE_UNAVAILABLE",
            "FRESHNESS_STATE_ROLLBACK",
        }

        self.assertEqual(set(FreshnessLoadCode.__members__), expected)

    def test_persist_codes_are_exact(self) -> None:
        expected = {
            "PREPARED",
            "COMMITTED",
            "VERIFIED",
            "FRESHNESS_STATE_PERSIST_FAILURE",
        }

        self.assertEqual(set(FreshnessPersistCode.__members__), expected)

    def test_state_valid_requires_state(self) -> None:
        with self.assertRaises(ValueError):
            FreshnessLoadResult(
                code=FreshnessLoadCode.STATE_VALID,
                producer_id=PRODUCER,
            )

    def test_non_valid_load_cannot_carry_state(self) -> None:
        with self.assertRaises(ValueError):
            FreshnessLoadResult(
                code=FreshnessLoadCode.STATE_NOT_ESTABLISHED,
                producer_id=PRODUCER,
                state=state(),
            )

    def test_valid_load_state_must_match_producer(self) -> None:
        with self.assertRaises(ValueError):
            FreshnessLoadResult(
                code=FreshnessLoadCode.STATE_VALID,
                producer_id=PRODUCER,
                state=state(producer_id=OTHER),
            )

    def test_valid_load_result(self) -> None:
        result = FreshnessLoadResult(
            code=FreshnessLoadCode.STATE_VALID,
            producer_id=PRODUCER,
            state=state(),
        )

        self.assertTrue(result.valid)

    def test_prepared_write_binds_producer(self) -> None:
        prepared = FreshnessPreparedWrite(
            producer_id=PRODUCER,
            previous_state=None,
            candidate_state=state(),
        )

        self.assertEqual(prepared.producer_id, PRODUCER)

    def test_candidate_producer_mismatch_rejected(self) -> None:
        with self.assertRaises(ValueError):
            FreshnessPreparedWrite(
                producer_id=PRODUCER,
                previous_state=None,
                candidate_state=state(producer_id=OTHER),
            )

    def test_previous_producer_mismatch_rejected(self) -> None:
        with self.assertRaises(ValueError):
            FreshnessPreparedWrite(
                producer_id=PRODUCER,
                previous_state=state(producer_id=OTHER),
                candidate_state=state(),
            )

    def test_persist_result_state_must_match_producer(self) -> None:
        with self.assertRaises(ValueError):
            FreshnessPersistResult(
                code=FreshnessPersistCode.COMMITTED,
                producer_id=PRODUCER,
                state=state(producer_id=OTHER),
            )

    def test_result_properties_do_not_equate_commit_and_verify(self) -> None:
        committed = FreshnessPersistResult(
            code=FreshnessPersistCode.COMMITTED,
            producer_id=PRODUCER,
            state=state(),
        )

        verified = FreshnessPersistResult(
            code=FreshnessPersistCode.VERIFIED,
            producer_id=PRODUCER,
            state=state(),
        )

        self.assertTrue(committed.committed)
        self.assertFalse(committed.verified)
        self.assertTrue(verified.verified)
        self.assertFalse(verified.committed)

    def test_backend_contract_can_be_implemented(self) -> None:
        backend = DummyBackend()
        candidate = state(generation=1)

        loaded = backend.load(PRODUCER)
        self.assertEqual(
            loaded.code,
            FreshnessLoadCode.STATE_NOT_ESTABLISHED,
        )

        prepared = backend.prepare(
            previous_state=None,
            candidate_state=candidate,
        )
        self.assertEqual(prepared.candidate_state, candidate)

        committed = backend.commit(prepared)
        self.assertTrue(committed.committed)
        self.assertFalse(committed.verified)

        verified = backend.verify(prepared)
        self.assertTrue(verified.verified)

    def test_interface_has_exact_abstract_operations(self) -> None:
        expected = {"load", "prepare", "commit", "verify"}

        self.assertEqual(
            FreshnessPersistenceBackend.__abstractmethods__,
            expected,
        )

    def test_interface_has_no_final_fresh_property(self) -> None:
        self.assertFalse(hasattr(FreshnessPersistResult, "fresh"))
        self.assertFalse(hasattr(FreshnessPreparedWrite, "fresh"))


if __name__ == "__main__":
    unittest.main(verbosity=2)
