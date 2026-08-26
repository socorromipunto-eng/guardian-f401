"""Guardian M15-03 host transactional backend tests."""

from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from guardian_assurance.freshness_host_backend import (
    HostFreshnessPersistenceBackend,
    _serialize_state,
)
from guardian_assurance.freshness_persistence import (
    FreshnessLoadCode,
    FreshnessPersistCode,
)
from guardian_assurance.freshness_state import (
    FreshnessState,
    HighWaterState,
    make_unset_state,
)


PRODUCER = "guardian-primary"
EPOCH_A = "0" * 32
EPOCH_B = "1" * 32


def state(
    *,
    generation: int,
    logical_time: int = 1,
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


class M15FreshnessHostBackendTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.backend = HostFreshnessPersistenceBackend(self.root)

    def tearDown(self) -> None:
        self.temp.cleanup()

    def test_first_load_is_not_established(self) -> None:
        loaded = self.backend.load(PRODUCER)
        self.assertEqual(
            loaded.code,
            FreshnessLoadCode.STATE_NOT_ESTABLISHED,
        )

    def test_prepare_does_not_create_storage(self) -> None:
        candidate = state(generation=0)

        prepared = self.backend.prepare(
            previous_state=None,
            candidate_state=candidate,
        )

        self.assertEqual(prepared.candidate_state, candidate)
        self.assertEqual(list(self.root.iterdir()), [])

    def test_initial_commit_and_verify_round_trip(self) -> None:
        candidate = state(generation=0)
        prepared = self.backend.prepare(
            previous_state=None,
            candidate_state=candidate,
        )

        committed = self.backend.commit(prepared)
        self.assertEqual(committed.code, FreshnessPersistCode.COMMITTED)

        verified = self.backend.verify(prepared)
        self.assertEqual(verified.code, FreshnessPersistCode.VERIFIED)

        loaded = self.backend.load(PRODUCER)
        self.assertEqual(loaded.code, FreshnessLoadCode.STATE_VALID)
        self.assertEqual(loaded.state, candidate)

    def test_prepare_rejects_non_advancing_generation(self) -> None:
        previous = state(generation=3)
        candidate = state(generation=3, logical_time=2)

        with self.assertRaises(ValueError):
            self.backend.prepare(
                previous_state=previous,
                candidate_state=candidate,
            )

    def test_update_commit_requires_exact_previous_state(self) -> None:
        first = state(generation=0)
        first_prepared = self.backend.prepare(
            previous_state=None,
            candidate_state=first,
        )
        self.backend.commit(first_prepared)

        stale_previous = state(generation=0, logical_time=99)
        candidate = state(generation=1, logical_time=2)
        prepared = self.backend.prepare(
            previous_state=stale_previous,
            candidate_state=candidate,
        )

        result = self.backend.commit(prepared)
        self.assertEqual(
            result.code,
            FreshnessPersistCode.FRESHNESS_STATE_PERSIST_FAILURE,
        )

        loaded = self.backend.load(PRODUCER)
        self.assertEqual(loaded.state, first)

    def test_update_commit_advances_generation(self) -> None:
        first = state(generation=0)
        first_prepared = self.backend.prepare(
            previous_state=None,
            candidate_state=first,
        )
        self.backend.commit(first_prepared)

        second = state(generation=1, logical_time=2)
        second_prepared = self.backend.prepare(
            previous_state=first,
            candidate_state=second,
        )

        committed = self.backend.commit(second_prepared)
        self.assertEqual(committed.code, FreshnessPersistCode.COMMITTED)

        verified = self.backend.verify(second_prepared)
        self.assertEqual(verified.code, FreshnessPersistCode.VERIFIED)

        self.assertEqual(self.backend.load(PRODUCER).state, second)

    def test_corrupted_committed_file_is_invalid(self) -> None:
        candidate = state(generation=0)
        prepared = self.backend.prepare(
            previous_state=None,
            candidate_state=candidate,
        )
        self.backend.commit(prepared)

        path = self.backend._state_path(PRODUCER)
        raw = bytearray(path.read_bytes())
        raw[len(raw) // 2] ^= 1
        path.write_bytes(bytes(raw))

        loaded = self.backend.load(PRODUCER)
        self.assertEqual(
            loaded.code,
            FreshnessLoadCode.FRESHNESS_STATE_INVALID,
        )

    def test_truncated_committed_file_is_invalid(self) -> None:
        candidate = state(generation=0)
        prepared = self.backend.prepare(
            previous_state=None,
            candidate_state=candidate,
        )
        self.backend.commit(prepared)

        path = self.backend._state_path(PRODUCER)
        raw = path.read_bytes()
        path.write_bytes(raw[: max(1, len(raw) // 3)])

        loaded = self.backend.load(PRODUCER)
        self.assertEqual(
            loaded.code,
            FreshnessLoadCode.FRESHNESS_STATE_INVALID,
        )

    def test_duplicate_json_member_is_invalid(self) -> None:
        path = self.backend._state_path(PRODUCER)
        self.root.mkdir(parents=True, exist_ok=True)
        path.write_text(
            '{"schema_version":"guardian-f401:m15:freshness-state:v1",'
            '"schema_version":"guardian-f401:m15:freshness-state:v1",'
            '"state":{},"integrity_sha256":"00"}',
            encoding="utf-8",
        )

        loaded = self.backend.load(PRODUCER)
        self.assertEqual(
            loaded.code,
            FreshnessLoadCode.FRESHNESS_STATE_INVALID,
        )

    def test_integrity_digest_mismatch_is_invalid(self) -> None:
        candidate = state(generation=0)
        prepared = self.backend.prepare(
            previous_state=None,
            candidate_state=candidate,
        )
        self.backend.commit(prepared)

        path = self.backend._state_path(PRODUCER)
        document = json.loads(path.read_text(encoding="utf-8"))
        document["state"]["highest_logical_time"] = 999
        path.write_text(
            json.dumps(document, sort_keys=True, separators=(",", ":")),
            encoding="utf-8",
        )

        loaded = self.backend.load(PRODUCER)
        self.assertEqual(
            loaded.code,
            FreshnessLoadCode.FRESHNESS_STATE_INVALID,
        )

    def test_stale_candidate_file_is_not_committed_state(self) -> None:
        self.root.mkdir(parents=True, exist_ok=True)
        stale = state(generation=99, logical_time=99)
        self.backend._candidate_path(PRODUCER).write_bytes(
            _serialize_state(stale)
        )

        loaded = self.backend.load(PRODUCER)
        self.assertEqual(
            loaded.code,
            FreshnessLoadCode.STATE_NOT_ESTABLISHED,
        )

    def test_failed_replace_preserves_previous_committed_state(self) -> None:
        first = state(generation=0)
        first_prepared = self.backend.prepare(
            previous_state=None,
            candidate_state=first,
        )
        self.backend.commit(first_prepared)

        second = state(generation=1, logical_time=2)
        second_prepared = self.backend.prepare(
            previous_state=first,
            candidate_state=second,
        )

        with mock.patch(
            "guardian_assurance.freshness_host_backend.os.replace",
            side_effect=OSError("simulated replace failure"),
        ):
            result = self.backend.commit(second_prepared)

        self.assertEqual(
            result.code,
            FreshnessPersistCode.FRESHNESS_STATE_PERSIST_FAILURE,
        )
        self.assertEqual(self.backend.load(PRODUCER).state, first)

    def test_failed_candidate_write_preserves_previous_state(self) -> None:
        first = state(generation=0)
        first_prepared = self.backend.prepare(
            previous_state=None,
            candidate_state=first,
        )
        self.backend.commit(first_prepared)

        second = state(generation=1, logical_time=2)
        second_prepared = self.backend.prepare(
            previous_state=first,
            candidate_state=second,
        )

        candidate_path = self.backend._candidate_path(PRODUCER)

        with mock.patch.object(
            Path,
            "open",
            side_effect=OSError("simulated write failure"),
        ):
            result = self.backend.commit(second_prepared)

        self.assertEqual(
            result.code,
            FreshnessPersistCode.FRESHNESS_STATE_PERSIST_FAILURE,
        )
        self.assertEqual(self.backend.load(PRODUCER).state, first)
        self.assertFalse(candidate_path.exists())

    def test_verify_fails_when_committed_state_differs(self) -> None:
        first = state(generation=0)
        prepared = self.backend.prepare(
            previous_state=None,
            candidate_state=first,
        )
        self.backend.commit(prepared)

        different = state(generation=1, logical_time=2)
        different_prepared = self.backend.prepare(
            previous_state=first,
            candidate_state=different,
        )

        verified = self.backend.verify(different_prepared)
        self.assertEqual(
            verified.code,
            FreshnessPersistCode.FRESHNESS_STATE_PERSIST_FAILURE,
        )

    def test_serializer_is_reproducible(self) -> None:
        value = state(generation=7, logical_time=44)
        self.assertEqual(_serialize_state(value), _serialize_state(value))

    def test_digest_does_not_claim_storage_authenticity(self) -> None:
        value = state(generation=0)
        raw = _serialize_state(value)
        self.assertIn(b"integrity_sha256", raw)
        self.assertNotIn(b"authenticated_storage", raw)
        self.assertNotIn(b"hardware_rollback", raw)


if __name__ == "__main__":
    unittest.main(verbosity=2)
