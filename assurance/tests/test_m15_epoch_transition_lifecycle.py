"""M15 epoch-transition lifecycle hostile tests."""

from __future__ import annotations

import base64
import json
import tempfile
import unittest
from pathlib import Path

from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.hazmat.primitives.serialization import Encoding, PublicFormat

from guardian_assurance.authentication import (
    AuthenticationResult,
    AuthenticationResultCode,
)
from guardian_assurance.authorization_authentication import (
    AuthorizationAuthenticationResultCode,
    authenticate_authorization,
)
from guardian_assurance.authorization_objects import (
    EPOCH_TRANSITION_SCHEMA_VERSION,
    EPOCH_TRANSITION_TYPE,
)
from guardian_assurance.authorization_persistence import (
    AuthorizationPersistCode,
    AuthorizationPersistResult,
)
from guardian_assurance.authorization_replay import (
    AuthorizationReplayCode,
    evaluate_authorization_replay,
)
from guardian_assurance.authorization_transcript import (
    AUTHORIZATION_SIGNATURE_ALGORITHM,
    AUTHORIZATION_TRANSCRIPT_VERSION,
    build_authorization_transcript,
)
from guardian_assurance.authorization_trust_store import (
    AUTHORIZATION_TRUST_STORE_SCHEMA_VERSION,
    EPOCH_TRANSITION_AUTHORITY,
    parse_and_validate_authorization_trust_store,
)
from guardian_assurance.bootstrap_lifecycle import BootstrapLifecycleCode
from guardian_assurance.crypto_provider import HostEd25519Provider
from guardian_assurance.epoch_transition_lifecycle import (
    EpochTransitionLifecycleCode,
    execute_epoch_transition_lifecycle,
)
from guardian_assurance.freshness_host_backend import (
    HostFreshnessPersistenceBackend,
)
from guardian_assurance.freshness_orchestrator import (
    FreshnessOutcome,
    FreshnessOutcomeCode,
)
from guardian_assurance.freshness_state import (
    FreshnessState,
    HighWaterState,
    UINT64_MAX,
)
from guardian_assurance.signed_authorization import (
    encode_authorization_signature_base64url,
)


PRODUCER = "plant-a.guardian-01"
OLD_EPOCH = "11111111111111111111111111111111"
NEW_EPOCH = "22222222222222222222222222222222"


def encoded(value: object) -> bytes:
    return json.dumps(
        value,
        sort_keys=True,
        separators=(",", ":"),
    ).encode("utf-8")


def key_wire(raw: bytes) -> str:
    return base64.urlsafe_b64encode(raw).rstrip(b"=").decode("ascii")


class ControlledAuthorizationBackend:
    """Deterministic transaction backend for lifecycle composition tests."""

    def __init__(
        self,
        *,
        fail_prepare: bool = False,
        fail_commit: bool = False,
        fail_verify: bool = False,
    ) -> None:
        self.fail_prepare = fail_prepare
        self.fail_commit = fail_commit
        self.fail_verify = fail_verify

    def prepare(self, prepared):
        if self.fail_prepare:
            return AuthorizationPersistResult(
                code=AuthorizationPersistCode.AUTHORIZATION_STATE_PERSIST_FAILURE,
                state=None,
                detail="forced prepare failure",
            )

        return AuthorizationPersistResult(
            code=AuthorizationPersistCode.PREPARED,
            state=prepared.candidate,
        )

    def commit(self, prepared):
        if self.fail_commit:
            return AuthorizationPersistResult(
                code=AuthorizationPersistCode.AUTHORIZATION_STATE_PERSIST_FAILURE,
                state=None,
                detail="forced commit failure",
            )

        return AuthorizationPersistResult(
            code=AuthorizationPersistCode.COMMITTED,
            state=prepared.candidate,
        )

    def verify(self, prepared):
        if self.fail_verify:
            return AuthorizationPersistResult(
                code=AuthorizationPersistCode.AUTHORIZATION_STATE_PERSIST_FAILURE,
                state=None,
                detail="forced verify failure",
            )

        return AuthorizationPersistResult(
            code=AuthorizationPersistCode.VERIFIED,
            state=prepared.candidate,
        )


class M15EpochTransitionLifecycleTests(unittest.TestCase):
    def setUp(self) -> None:
        self.private = Ed25519PrivateKey.generate()
        self.public = self.private.public_key().public_bytes(
            Encoding.Raw,
            PublicFormat.Raw,
        )
        self.provider = HostEd25519Provider()

        self.temp_fresh = tempfile.TemporaryDirectory()

        self.fresh_backend = HostFreshnessPersistenceBackend(
            Path(self.temp_fresh.name)
        )

    def tearDown(self) -> None:
        self.temp_fresh.cleanup()

    def previous_state(
        self,
        *,
        transition_sequence: int = 3,
        generation: int = 5,
    ) -> FreshnessState:
        return FreshnessState(
            producer_id=PRODUCER,
            current_epoch=OLD_EPOCH,
            previous_epoch=None,
            high_water_state=HighWaterState.SET,
            highest_logical_time=50,
            transition_sequence=transition_sequence,
            generation=generation,
        )

    def persist_previous_projection(
        self,
        state: FreshnessState,
    ) -> None:
        prepared = self.fresh_backend.prepare(
            previous_state=None,
            candidate_state=state,
        )

        committed = self.fresh_backend.commit(prepared)
        self.assertTrue(committed.committed)

        verified = self.fresh_backend.verify(prepared)
        self.assertTrue(verified.verified)

    def trust_store(self):
        value = {
            "schema_version": AUTHORIZATION_TRUST_STORE_SCHEMA_VERSION,
            "environment": "TEST",
            "records": [
                {
                    "authority_id": "root.epoch-authority:01",
                    "key_id": "epoch-key-001",
                    "algorithm": AUTHORIZATION_SIGNATURE_ALGORITHM,
                    "public_key": key_wire(self.public),
                    "lifecycle_state": "ACTIVE",
                    "capabilities": [EPOCH_TRANSITION_AUTHORITY],
                }
            ],
        }

        return parse_and_validate_authorization_trust_store(
            encoded(value),
            expected_environment="TEST",
        )

    def authorization_object(
        self,
        *,
        producer_id: str = PRODUCER,
        from_epoch: str = OLD_EPOCH,
        to_epoch: str = NEW_EPOCH,
        transition_sequence: int = 4,
        authorization_sequence: int = 1,
    ) -> dict[str, object]:
        return {
            "schema_version": EPOCH_TRANSITION_SCHEMA_VERSION,
            "authorization_type": EPOCH_TRANSITION_TYPE,
            "authorization_id": "fedcba9876543210fedcba9876543210",
            "authority_id": "root.epoch-authority:01",
            "producer_id": producer_id,
            "authorization_sequence": str(authorization_sequence),
            "from_epoch": from_epoch,
            "to_epoch": to_epoch,
            "transition_sequence": str(transition_sequence),
        }

    def signed_envelope(self, **kwargs) -> bytes:
        obj = self.authorization_object(**kwargs)
        obj_raw = encoded(obj)

        transcript = build_authorization_transcript(
            obj_raw,
            authority_id="root.epoch-authority:01",
            key_id="epoch-key-001",
            signature_algorithm=AUTHORIZATION_SIGNATURE_ALGORITHM,
            transcript_version=AUTHORIZATION_TRANSCRIPT_VERSION,
        )

        signature = self.private.sign(transcript)

        envelope = {
            "signature_version": AUTHORIZATION_TRANSCRIPT_VERSION,
            "signature_algorithm": AUTHORIZATION_SIGNATURE_ALGORITHM,
            "authority_id": "root.epoch-authority:01",
            "key_id": "epoch-key-001",
            "authorization_object": obj,
            "signature": encode_authorization_signature_base64url(signature),
        }

        return encoded(envelope)

    def authenticated_authorization(self, **kwargs):
        result = authenticate_authorization(
            self.signed_envelope(**kwargs),
            trust_store=self.trust_store(),
            provider=self.provider,
        )

        self.assertEqual(
            result.code,
            AuthorizationAuthenticationResultCode.AUTHORIZATION_AUTHENTICATED,
        )

        return result

    def ordinary_authenticated(self) -> AuthenticationResult:
        return AuthenticationResult(
            code=AuthenticationResultCode.AUTHENTICATED,
        )

    def transition_required(
        self,
        *,
        producer_id: str = PRODUCER,
        producer_epoch: str = NEW_EPOCH,
        logical_time: int = 1,
    ) -> FreshnessOutcome:
        return FreshnessOutcome(
            code=FreshnessOutcomeCode.EPOCH_TRANSITION_REQUIRED,
            producer_id=producer_id,
            producer_epoch=producer_epoch,
            logical_time=logical_time,
        )

    def replay_candidate(self, auth):
        result = evaluate_authorization_replay(
            auth,
            previous_state=None,
            established_scope_count=0,
        )

        self.assertEqual(
            result.code,
            AuthorizationReplayCode.AUTHORIZATION_CANDIDATE,
        )

        return result

    def execute(
        self,
        *,
        auth=None,
        freshness=None,
        previous=None,
        replay=None,
        backend=None,
    ):
        auth = auth or self.authenticated_authorization()
        freshness = freshness or self.transition_required()
        previous = previous or self.previous_state()
        replay = replay or self.replay_candidate(auth)
        backend = backend or ControlledAuthorizationBackend()

        self.persist_previous_projection(previous)

        return execute_epoch_transition_lifecycle(
            ordinary_authentication=self.ordinary_authenticated(),
            freshness=freshness,
            authorization_authentication=auth,
            replay=replay,
            previous_replay_state=None,
            previous_freshness_state=previous,
            previous_transaction_generation=8,
            transaction_backend=backend,
            freshness_backend=self.fresh_backend,
        )

    def test_successful_transition_is_verified_and_reconciled(self) -> None:
        result = self.execute()

        self.assertEqual(
            result.code,
            EpochTransitionLifecycleCode.EPOCH_TRANSITION_AUTHORIZED,
        )
        self.assertTrue(result.epoch_transition_authorized)
        self.assertTrue(result.authorization_consumed)

        state = result.freshness_state
        self.assertIsNotNone(state)

        self.assertEqual(state.current_epoch, NEW_EPOCH)
        self.assertEqual(state.previous_epoch, OLD_EPOCH)
        self.assertEqual(state.transition_sequence, 4)
        self.assertEqual(state.generation, 6)
        self.assertIs(state.high_water_state, HighWaterState.UNSET)
        self.assertIsNone(state.highest_logical_time)

        loaded = self.fresh_backend.load(PRODUCER)
        self.assertEqual(loaded.state, state)

    def test_epoch_transition_authorized_is_not_fresh(self) -> None:
        result = self.execute()

        self.assertTrue(result.epoch_transition_authorized)
        self.assertFalse(hasattr(result, "fresh"))

        self.assertIs(
            result.freshness_state.high_water_state,
            HighWaterState.UNSET,
        )
        self.assertIsNone(
            result.freshness_state.highest_logical_time
        )

    def test_wrong_producer_fails_closed(self) -> None:
        auth = self.authenticated_authorization(
            producer_id="plant-b.guardian-01"
        )

        replay = self.replay_candidate(auth)

        result = self.execute(
            auth=auth,
            replay=replay,
        )

        self.assertEqual(
            result.code,
            EpochTransitionLifecycleCode.PRODUCER_MISMATCH,
        )

    def test_wrong_from_epoch_fails_closed(self) -> None:
        auth = self.authenticated_authorization(
            from_epoch="33333333333333333333333333333333"
        )

        replay = self.replay_candidate(auth)

        result = self.execute(
            auth=auth,
            replay=replay,
        )

        self.assertEqual(
            result.code,
            EpochTransitionLifecycleCode.FROM_EPOCH_MISMATCH,
        )

    def test_wrong_to_epoch_fails_closed(self) -> None:
        auth = self.authenticated_authorization(
            to_epoch="33333333333333333333333333333333"
        )

        replay = self.replay_candidate(auth)

        result = self.execute(
            auth=auth,
            replay=replay,
        )

        self.assertEqual(
            result.code,
            EpochTransitionLifecycleCode.TO_EPOCH_MISMATCH,
        )

    def test_equal_transition_sequence_fails_closed(self) -> None:
        auth = self.authenticated_authorization(
            transition_sequence=3
        )

        replay = self.replay_candidate(auth)

        result = self.execute(
            auth=auth,
            replay=replay,
        )

        self.assertEqual(
            result.code,
            EpochTransitionLifecycleCode.TRANSITION_SEQUENCE_MISMATCH,
        )

    def test_skipped_transition_sequence_fails_closed(self) -> None:
        auth = self.authenticated_authorization(
            transition_sequence=5
        )

        replay = self.replay_candidate(auth)

        result = self.execute(
            auth=auth,
            replay=replay,
        )

        self.assertEqual(
            result.code,
            EpochTransitionLifecycleCode.TRANSITION_SEQUENCE_MISMATCH,
        )

    def test_transition_sequence_exhaustion_fails_closed(self) -> None:
        previous = self.previous_state(
            transition_sequence=UINT64_MAX
        )

        auth = self.authenticated_authorization(
            transition_sequence=UINT64_MAX
        )

        replay = self.replay_candidate(auth)

        result = self.execute(
            auth=auth,
            previous=previous,
            replay=replay,
        )

        self.assertEqual(
            result.code,
            EpochTransitionLifecycleCode.TRANSITION_SEQUENCE_EXHAUSTED,
        )

    def test_freshness_generation_exhaustion_fails_closed(self) -> None:
        previous = self.previous_state(
            generation=UINT64_MAX
        )

        auth = self.authenticated_authorization()
        replay = self.replay_candidate(auth)

        result = self.execute(
            auth=auth,
            previous=previous,
            replay=replay,
        )

        self.assertEqual(
            result.code,
            EpochTransitionLifecycleCode.FRESHNESS_GENERATION_EXHAUSTED,
        )

    def test_non_candidate_authorization_fails_closed(self) -> None:
        auth = self.authenticated_authorization()
        candidate = self.replay_candidate(auth)

        replay = type(candidate)(
            code=AuthorizationReplayCode.AUTHORIZATION_REPLAY,
            scope=candidate.scope,
            previous_state=candidate.previous_state,
            candidate_state=None,
            authorization_id=candidate.authorization_id,
            authorization_sequence=candidate.authorization_sequence,
            detail="forced replay",
        )

        result = self.execute(
            auth=auth,
            replay=replay,
        )

        self.assertEqual(
            result.code,
            EpochTransitionLifecycleCode.AUTHORIZATION_REPLAY_NOT_CANDIDATE,
        )

    def test_commit_failure_never_authorizes(self) -> None:
        result = self.execute(
            backend=ControlledAuthorizationBackend(
                fail_commit=True
            )
        )

        self.assertEqual(
            result.code,
            EpochTransitionLifecycleCode.TRANSACTION_COMMIT_FAILURE,
        )
        self.assertFalse(result.epoch_transition_authorized)

    def test_verify_failure_never_authorizes(self) -> None:
        result = self.execute(
            backend=ControlledAuthorizationBackend(
                fail_verify=True
            )
        )

        self.assertEqual(
            result.code,
            EpochTransitionLifecycleCode.TRANSACTION_VERIFY_FAILURE,
        )
        self.assertFalse(result.epoch_transition_authorized)


if __name__ == "__main__":
    unittest.main(verbosity=2)