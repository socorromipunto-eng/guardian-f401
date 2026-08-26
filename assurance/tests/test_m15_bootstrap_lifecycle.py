"""M15 bootstrap lifecycle hostile tests."""

from __future__ import annotations

import base64
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

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
from guardian_assurance.authorization_host_backend import (
    HostAuthorizationTransactionBackend,
)
from guardian_assurance.authorization_persistence import (
    AuthorizationPersistCode,
)
from guardian_assurance.authorization_objects import (
    BOOTSTRAP_SCHEMA_VERSION,
    BOOTSTRAP_TYPE,
    HIGH_WATER_ESTABLISHED,
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
    BOOTSTRAP_AUTHORITY,
    parse_and_validate_authorization_trust_store,
)
from guardian_assurance.bootstrap_lifecycle import (
    BootstrapLifecycleCode,
    execute_bootstrap_lifecycle,
)
from guardian_assurance.crypto_provider import HostEd25519Provider
from guardian_assurance.freshness_host_backend import (
    HostFreshnessPersistenceBackend,
)
from guardian_assurance.freshness_orchestrator import (
    FreshnessOutcome,
    FreshnessOutcomeCode,
)
from guardian_assurance.signed_authorization import (
    encode_authorization_signature_base64url,
)


def encoded(value: object) -> bytes:
    return json.dumps(
        value,
        sort_keys=True,
        separators=(",", ":"),
    ).encode("utf-8")


def key_wire(raw: bytes) -> str:
    return base64.urlsafe_b64encode(raw).rstrip(b"=").decode("ascii")


class M15BootstrapLifecycleTests(unittest.TestCase):
    def setUp(self) -> None:
        self.private = Ed25519PrivateKey.generate()
        self.public = self.private.public_key().public_bytes(
            Encoding.Raw,
            PublicFormat.Raw,
        )
        self.provider = HostEd25519Provider()

        self.temp_tx = tempfile.TemporaryDirectory()
        self.temp_fresh = tempfile.TemporaryDirectory()

        self.tx_backend = HostAuthorizationTransactionBackend(
            Path(self.temp_tx.name)
        )
        self.fresh_backend = HostFreshnessPersistenceBackend(
            Path(self.temp_fresh.name)
        )

    def tearDown(self) -> None:
        self.temp_tx.cleanup()
        self.temp_fresh.cleanup()

    def authorization_object(
        self,
        *,
        producer_id: str = "plant-a.guardian-01",
        producer_epoch: str = "11111111111111111111111111111111",
        logical_time: int = 7,
        authorization_sequence: int = 1,
    ) -> dict[str, object]:
        return {
            "schema_version": BOOTSTRAP_SCHEMA_VERSION,
            "authorization_type": BOOTSTRAP_TYPE,
            "authorization_id": "0123456789abcdef0123456789abcdef",
            "authority_id": "root.bootstrap-authority:01",
            "producer_id": producer_id,
            "authorization_sequence": str(authorization_sequence),
            "producer_epoch": producer_epoch,
            "initial_high_water_state": HIGH_WATER_ESTABLISHED,
            "initial_logical_time": logical_time,
        }

    def trust_store(self) -> object:
        value = {
            "schema_version": AUTHORIZATION_TRUST_STORE_SCHEMA_VERSION,
            "environment": "host-test",
            "records": [
                {
                    "authority_id": "root.bootstrap-authority:01",
                    "key_id": "bootstrap-key-01",
                    "algorithm": AUTHORIZATION_SIGNATURE_ALGORITHM,
                    "public_key": key_wire(self.public),
                    "lifecycle_state": "ACTIVE",
                    "capabilities": [BOOTSTRAP_AUTHORITY],
                }
            ],
        }
        return parse_and_validate_authorization_trust_store(
            encoded(value),
            expected_environment="host-test",
        )

    def signed_envelope(
        self,
        *,
        producer_id: str = "plant-a.guardian-01",
        producer_epoch: str = "11111111111111111111111111111111",
        logical_time: int = 7,
        authorization_sequence: int = 1,
    ) -> bytes:
        obj = self.authorization_object(
            producer_id=producer_id,
            producer_epoch=producer_epoch,
            logical_time=logical_time,
            authorization_sequence=authorization_sequence,
        )

        obj_raw = encoded(obj)

        transcript = build_authorization_transcript(
            raw_authorization_object=obj_raw,
            authority_id="root.bootstrap-authority:01",
            key_id="bootstrap-key-01",
            signature_algorithm=AUTHORIZATION_SIGNATURE_ALGORITHM,
            transcript_version=AUTHORIZATION_TRANSCRIPT_VERSION,
        )

        signature = self.private.sign(transcript)

        envelope = {
            "signature_version": AUTHORIZATION_TRANSCRIPT_VERSION,
            "signature_algorithm": AUTHORIZATION_SIGNATURE_ALGORITHM,
            "authority_id": "root.bootstrap-authority:01",
            "key_id": "bootstrap-key-01",
            "authorization_object": obj,
            "signature": encode_authorization_signature_base64url(signature),
        }

        return encoded(envelope)

    def authenticated_authorization(
        self,
        **kwargs: object,
    ):
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

    def first_seen(
        self,
        *,
        producer_id: str = "plant-a.guardian-01",
        producer_epoch: str = "11111111111111111111111111111111",
        logical_time: int = 7,
    ) -> FreshnessOutcome:
        return FreshnessOutcome(
            code=FreshnessOutcomeCode.FIRST_SEEN,
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

    def test_successful_bootstrap_authorizes_only_after_verified_reconciliation(
        self,
    ) -> None:
        auth = self.authenticated_authorization()
        replay = self.replay_candidate(auth)

        result = execute_bootstrap_lifecycle(
            ordinary_authentication=self.ordinary_authenticated(),
            freshness=self.first_seen(),
            authorization_authentication=auth,
            replay=replay,
            previous_replay_state=None,
            established_scope_count=0,
            transaction_backend=self.tx_backend,
            freshness_backend=self.fresh_backend,
        )

        self.assertEqual(
            result.code,
            BootstrapLifecycleCode.BOOTSTRAP_AUTHORIZED,
        )
        self.assertTrue(result.bootstrap_authorized)
        self.assertTrue(result.authorization_consumed)

        loaded = self.fresh_backend.load("plant-a.guardian-01")
        self.assertEqual(loaded.state, result.freshness_state)

    def test_ordinary_authentication_required(self) -> None:
        auth = self.authenticated_authorization()
        replay = self.replay_candidate(auth)

        ordinary = AuthenticationResult(
            code=AuthenticationResultCode.SIGNATURE_INVALID,
        )

        result = execute_bootstrap_lifecycle(
            ordinary_authentication=ordinary,
            freshness=self.first_seen(),
            authorization_authentication=auth,
            replay=replay,
            previous_replay_state=None,
            established_scope_count=0,
            transaction_backend=self.tx_backend,
            freshness_backend=self.fresh_backend,
        )

        self.assertEqual(
            result.code,
            BootstrapLifecycleCode.ORDINARY_AUTHENTICATION_REQUIRED,
        )
        self.assertFalse(result.bootstrap_authorized)

    def test_first_seen_required(self) -> None:
        auth = self.authenticated_authorization()
        replay = self.replay_candidate(auth)

        freshness = FreshnessOutcome(
            code=FreshnessOutcomeCode.FRESH,
            producer_id="plant-a.guardian-01",
            producer_epoch="11111111111111111111111111111111",
            logical_time=7,
        )

        result = execute_bootstrap_lifecycle(
            ordinary_authentication=self.ordinary_authenticated(),
            freshness=freshness,
            authorization_authentication=auth,
            replay=replay,
            previous_replay_state=None,
            established_scope_count=0,
            transaction_backend=self.tx_backend,
            freshness_backend=self.fresh_backend,
        )

        self.assertEqual(
            result.code,
            BootstrapLifecycleCode.FIRST_SEEN_REQUIRED,
        )

    def test_producer_mismatch_fails_closed(self) -> None:
        auth = self.authenticated_authorization()
        replay = self.replay_candidate(auth)

        result = execute_bootstrap_lifecycle(
            ordinary_authentication=self.ordinary_authenticated(),
            freshness=self.first_seen(producer_id="plant-b.guardian-01"),
            authorization_authentication=auth,
            replay=replay,
            previous_replay_state=None,
            established_scope_count=0,
            transaction_backend=self.tx_backend,
            freshness_backend=self.fresh_backend,
        )

        self.assertEqual(
            result.code,
            BootstrapLifecycleCode.PRODUCER_MISMATCH,
        )

    def test_epoch_mismatch_fails_closed(self) -> None:
        auth = self.authenticated_authorization()
        replay = self.replay_candidate(auth)

        result = execute_bootstrap_lifecycle(
            ordinary_authentication=self.ordinary_authenticated(),
            freshness=self.first_seen(
                producer_epoch="22222222222222222222222222222222"
            ),
            authorization_authentication=auth,
            replay=replay,
            previous_replay_state=None,
            established_scope_count=0,
            transaction_backend=self.tx_backend,
            freshness_backend=self.fresh_backend,
        )

        self.assertEqual(
            result.code,
            BootstrapLifecycleCode.EPOCH_MISMATCH,
        )

    def test_logical_time_mismatch_fails_closed(self) -> None:
        auth = self.authenticated_authorization(logical_time=7)
        replay = self.replay_candidate(auth)

        result = execute_bootstrap_lifecycle(
            ordinary_authentication=self.ordinary_authenticated(),
            freshness=self.first_seen(logical_time=8),
            authorization_authentication=auth,
            replay=replay,
            previous_replay_state=None,
            established_scope_count=0,
            transaction_backend=self.tx_backend,
            freshness_backend=self.fresh_backend,
        )

        self.assertEqual(
            result.code,
            BootstrapLifecycleCode.LOGICAL_TIME_MISMATCH,
        )

    def test_non_candidate_replay_fails_closed(self) -> None:
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

        result = execute_bootstrap_lifecycle(
            ordinary_authentication=self.ordinary_authenticated(),
            freshness=self.first_seen(),
            authorization_authentication=auth,
            replay=replay,
            previous_replay_state=None,
            established_scope_count=0,
            transaction_backend=self.tx_backend,
            freshness_backend=self.fresh_backend,
        )

        self.assertEqual(
            result.code,
            BootstrapLifecycleCode.AUTHORIZATION_REPLAY_NOT_CANDIDATE,
        )

    def test_commit_failure_never_authorizes(self) -> None:
        auth = self.authenticated_authorization()
        replay = self.replay_candidate(auth)

        with patch.object(
            self.tx_backend,
            "commit",
            wraps=self.tx_backend.commit,
        ) as commit:
            commit.side_effect = OSError("forced commit failure")

            with self.assertRaises(OSError):
                execute_bootstrap_lifecycle(
                    ordinary_authentication=self.ordinary_authenticated(),
                    freshness=self.first_seen(),
                    authorization_authentication=auth,
                    replay=replay,
                    previous_replay_state=None,
                    established_scope_count=0,
                    transaction_backend=self.tx_backend,
                    freshness_backend=self.fresh_backend,
                )

    def test_verify_failure_never_authorizes(self) -> None:
        auth = self.authenticated_authorization()
        replay = self.replay_candidate(auth)

        original_verify = self.tx_backend.verify

        def forced_failure(prepared):
            result = original_verify(prepared)
            return type(result)(
                code=AuthorizationPersistCode.AUTHORIZATION_STATE_PERSIST_FAILURE,
                state=None,
                detail="forced verification failure",
            )

        with patch.object(
            self.tx_backend,
            "verify",
            side_effect=forced_failure,
        ):
            result = execute_bootstrap_lifecycle(
                ordinary_authentication=self.ordinary_authenticated(),
                freshness=self.first_seen(),
                authorization_authentication=auth,
                replay=replay,
                previous_replay_state=None,
                established_scope_count=0,
                transaction_backend=self.tx_backend,
                freshness_backend=self.fresh_backend,
            )

        self.assertEqual(
            result.code,
            BootstrapLifecycleCode.TRANSACTION_VERIFY_FAILURE,
        )
        self.assertFalse(result.bootstrap_authorized)

    def test_authenticated_object_is_the_lifecycle_object(self) -> None:
        auth = self.authenticated_authorization(
            producer_epoch="11111111111111111111111111111111",
            logical_time=7,
        )
        replay = self.replay_candidate(auth)

        result = execute_bootstrap_lifecycle(
            ordinary_authentication=self.ordinary_authenticated(),
            freshness=self.first_seen(),
            authorization_authentication=auth,
            replay=replay,
            previous_replay_state=None,
            established_scope_count=0,
            transaction_backend=self.tx_backend,
            freshness_backend=self.fresh_backend,
        )

        self.assertTrue(result.bootstrap_authorized)
        self.assertEqual(
            result.freshness_state.current_epoch,
            auth.authorization_object.producer_epoch,
        )
        self.assertEqual(
            result.freshness_state.highest_logical_time,
            auth.authorization_object.initial_logical_time,
        )


if __name__ == "__main__":
    unittest.main(verbosity=2)