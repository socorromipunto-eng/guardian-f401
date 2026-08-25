"""M15 host authorization transaction backend tests."""

from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from guardian_assurance.authorization_host_backend import (
    HostAuthorizationTransactionBackend,
    _serialize_state,
    replay_scope_key,
)
from guardian_assurance.authorization_objects import BOOTSTRAP_TYPE
from guardian_assurance.authorization_persistence import (
    AuthorizationLoadCode,
    AuthorizationPersistCode,
)
from guardian_assurance.authorization_replay import (
    AuthorizationReplayScope,
    AuthorizationReplayState,
)
from guardian_assurance.authorization_transaction import (
    AuthorizationPreparedTransaction,
    AuthorizationTransactionState,
)
from guardian_assurance.authorization_trust_store import BOOTSTRAP_AUTHORITY
from guardian_assurance.freshness_state import FreshnessState, HighWaterState


def scope() -> AuthorizationReplayScope:
    return AuthorizationReplayScope(
        authority_id="root.bootstrap-authority:01",
        purpose_domain=BOOTSTRAP_AUTHORITY,
        producer_id="plant-a.guardian-01",
    )


def replay_state(sequence: int) -> AuthorizationReplayState:
    return AuthorizationReplayState(
        scope=scope(),
        authorization_sequence=sequence,
    )


def freshness(
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


def transaction_state(
    *,
    authorization_sequence: int,
    generation: int,
    freshness_generation: int,
    logical_time: int,
) -> AuthorizationTransactionState:
    replay = replay_state(authorization_sequence)

    return AuthorizationTransactionState(
        replay_state=replay,
        replay_scope=scope(),
        generation=generation,
        resulting_freshness_state=freshness(
            generation=freshness_generation,
            logical_time=logical_time,
        ),
        operation_type=BOOTSTRAP_TYPE,
        authorization_id="0123456789abcdef0123456789abcdef",
        authorization_sequence=authorization_sequence,
    )


def prepared_initial() -> AuthorizationPreparedTransaction:
    return AuthorizationPreparedTransaction(
        previous_replay_state=None,
        previous_freshness_state=None,
        previous_generation=None,
        candidate=transaction_state(
            authorization_sequence=1,
            generation=0,
            freshness_generation=0,
            logical_time=1,
        ),
    )


def prepared_update(
    previous: AuthorizationTransactionState,
) -> AuthorizationPreparedTransaction:
    return AuthorizationPreparedTransaction(
        previous_replay_state=previous.replay_state,
        previous_freshness_state=previous.resulting_freshness_state,
        previous_generation=previous.generation,
        candidate=transaction_state(
            authorization_sequence=previous.authorization_sequence + 1,
            generation=previous.generation + 1,
            freshness_generation=(
                previous.resulting_freshness_state.generation + 1
            ),
            logical_time=(
                previous.resulting_freshness_state.highest_logical_time + 1
            ),
        ),
    )


class M15AuthorizationHostBackendTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.backend = HostAuthorizationTransactionBackend(self.root)

    def tearDown(self) -> None:
        self.temp.cleanup()

    def test_missing_state_is_not_established(self) -> None:
        key = replay_scope_key(scope())
        loaded = self.backend.load(key)

        self.assertEqual(
            loaded.code,
            AuthorizationLoadCode.STATE_NOT_ESTABLISHED,
        )
        self.assertIsNone(loaded.state)

    def test_initial_commit_and_verify_round_trip(self) -> None:
        prepared = prepared_initial()

        prepared_result = self.backend.prepare(prepared)
        self.assertEqual(
            prepared_result.code,
            AuthorizationPersistCode.PREPARED,
        )

        committed = self.backend.commit(prepared)
        self.assertEqual(
            committed.code,
            AuthorizationPersistCode.COMMITTED,
        )

        verified = self.backend.verify(prepared)
        self.assertEqual(
            verified.code,
            AuthorizationPersistCode.VERIFIED,
        )

        key = replay_scope_key(prepared.candidate.replay_scope)
        loaded = self.backend.load(key)

        self.assertEqual(
            loaded.code,
            AuthorizationLoadCode.STATE_VALID,
        )
        self.assertEqual(loaded.state, prepared.candidate)

    def test_prepared_candidate_is_not_authoritative(self) -> None:
        prepared = prepared_initial()

        result = self.backend.prepare(prepared)
        self.assertEqual(
            result.code,
            AuthorizationPersistCode.PREPARED,
        )

        key = replay_scope_key(prepared.candidate.replay_scope)
        loaded = self.backend.load(key)

        self.assertEqual(
            loaded.code,
            AuthorizationLoadCode.STATE_NOT_ESTABLISHED,
        )

    def test_candidate_file_alone_is_not_authoritative(self) -> None:
        prepared = prepared_initial()
        key = replay_scope_key(prepared.candidate.replay_scope)

        candidate_path = self.backend._candidate_path(key)
        candidate_path.parent.mkdir(parents=True, exist_ok=True)
        candidate_path.write_bytes(
            _serialize_state(prepared.candidate)
        )

        loaded = self.backend.load(key)

        self.assertEqual(
            loaded.code,
            AuthorizationLoadCode.STATE_NOT_ESTABLISHED,
        )

    def test_corrupted_committed_state_fails_closed(self) -> None:
        prepared = prepared_initial()
        committed = self.backend.commit(prepared)
        self.assertEqual(
            committed.code,
            AuthorizationPersistCode.COMMITTED,
        )

        key = replay_scope_key(prepared.candidate.replay_scope)
        path = self.backend._state_path(key)
        raw = bytearray(path.read_bytes())
        raw[-2] ^= 0x01
        path.write_bytes(bytes(raw))

        loaded = self.backend.load(key)

        self.assertEqual(
            loaded.code,
            AuthorizationLoadCode.AUTHORIZATION_STATE_INVALID,
        )

    def test_truncated_committed_state_fails_closed(self) -> None:
        prepared = prepared_initial()
        self.backend.commit(prepared)

        key = replay_scope_key(prepared.candidate.replay_scope)
        path = self.backend._state_path(key)
        raw = path.read_bytes()
        path.write_bytes(raw[: max(1, len(raw) // 2)])

        loaded = self.backend.load(key)

        self.assertEqual(
            loaded.code,
            AuthorizationLoadCode.AUTHORIZATION_STATE_INVALID,
        )

    def test_duplicate_json_member_fails_closed(self) -> None:
        prepared = prepared_initial()
        key = replay_scope_key(prepared.candidate.replay_scope)
        path = self.backend._state_path(key)
        path.parent.mkdir(parents=True, exist_ok=True)

        raw = _serialize_state(prepared.candidate).decode("utf-8")
        raw = raw.replace(
            '"schema_version":',
            '"schema_version":"duplicate","schema_version":',
            1,
        )
        path.write_text(raw, encoding="utf-8")

        loaded = self.backend.load(key)

        self.assertEqual(
            loaded.code,
            AuthorizationLoadCode.AUTHORIZATION_STATE_INVALID,
        )

    def test_scope_substitution_fails_closed(self) -> None:
        prepared = prepared_initial()
        self.backend.commit(prepared)

        original_key = replay_scope_key(
            prepared.candidate.replay_scope
        )

        substituted_scope = AuthorizationReplayScope(
            authority_id="root.other-authority:01",
            purpose_domain=BOOTSTRAP_AUTHORITY,
            producer_id="plant-a.guardian-01",
        )
        substituted_key = replay_scope_key(substituted_scope)

        source = self.backend._state_path(original_key)
        target = self.backend._state_path(substituted_key)

        target.write_bytes(source.read_bytes())

        loaded = self.backend.load(substituted_key)

        self.assertEqual(
            loaded.code,
            AuthorizationLoadCode.AUTHORIZATION_STATE_INVALID,
        )

    def test_update_round_trip_advances_generation(self) -> None:
        first = prepared_initial()
        self.backend.commit(first)

        second = prepared_update(first.candidate)

        prepared_result = self.backend.prepare(second)
        self.assertEqual(
            prepared_result.code,
            AuthorizationPersistCode.PREPARED,
        )

        committed = self.backend.commit(second)
        self.assertEqual(
            committed.code,
            AuthorizationPersistCode.COMMITTED,
        )

        verified = self.backend.verify(second)
        self.assertEqual(
            verified.code,
            AuthorizationPersistCode.VERIFIED,
        )

        self.assertEqual(
            verified.state.generation,
            first.candidate.generation + 1,
        )

    def test_stale_preparation_cannot_overwrite_newer_state(self) -> None:
        first = prepared_initial()
        self.backend.commit(first)

        second = prepared_update(first.candidate)
        self.backend.commit(second)

        stale_candidate = transaction_state(
            authorization_sequence=2,
            generation=1,
            freshness_generation=1,
            logical_time=2,
        )

        stale = AuthorizationPreparedTransaction(
            previous_replay_state=first.candidate.replay_state,
            previous_freshness_state=(
                first.candidate.resulting_freshness_state
            ),
            previous_generation=first.candidate.generation,
            candidate=stale_candidate,
        )

        result = self.backend.commit(stale)

        self.assertEqual(
            result.code,
            AuthorizationPersistCode.AUTHORIZATION_STATE_STALE_PREPARATION,
        )

        key = replay_scope_key(scope())
        loaded = self.backend.load(key)

        self.assertEqual(loaded.state, second.candidate)

    def test_failed_replace_preserves_previous_authoritative_state(self) -> None:
        first = prepared_initial()
        self.backend.commit(first)

        second = prepared_update(first.candidate)

        with patch(
            "guardian_assurance.authorization_host_backend.os.replace",
            side_effect=OSError("forced replace failure"),
        ):
            result = self.backend.commit(second)

        self.assertEqual(
            result.code,
            AuthorizationPersistCode.AUTHORIZATION_STATE_PERSIST_FAILURE,
        )

        key = replay_scope_key(scope())
        loaded = self.backend.load(key)

        self.assertEqual(
            loaded.code,
            AuthorizationLoadCode.STATE_VALID,
        )
        self.assertEqual(loaded.state, first.candidate)

    def test_verify_fails_if_durable_state_differs(self) -> None:
        first = prepared_initial()
        self.backend.commit(first)

        second = prepared_update(first.candidate)

        verified = self.backend.verify(second)

        self.assertEqual(
            verified.code,
            AuthorizationPersistCode.AUTHORIZATION_STATE_PERSIST_FAILURE,
        )

    def test_committed_is_not_verified(self) -> None:
        prepared = prepared_initial()

        committed = self.backend.commit(prepared)

        self.assertEqual(
            committed.code,
            AuthorizationPersistCode.COMMITTED,
        )
        self.assertFalse(committed.verified)

    def test_backend_does_not_claim_consumed(self) -> None:
        prepared = prepared_initial()

        self.backend.commit(prepared)
        verified = self.backend.verify(prepared)

        self.assertTrue(verified.verified)
        self.assertFalse(hasattr(verified, "consumed"))


if __name__ == "__main__":
    unittest.main(verbosity=2)