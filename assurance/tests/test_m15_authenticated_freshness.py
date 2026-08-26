"""Guardian M15 authentication-to-freshness integration tests.

The cryptographic fixtures are deliberately reused from the authoritative
M15 authentication test module so this integration test does not create a
second signed-wrapper construction path.
"""

from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from guardian_assurance.authenticated_freshness import (
    authenticate_and_establish_freshness,
)
from guardian_assurance.authentication import (
    AuthenticationResultCode,
    authenticate_signed_assurance,
)
from guardian_assurance.freshness_host_backend import (
    HostFreshnessPersistenceBackend,
)
from guardian_assurance.freshness_orchestrator import (
    FreshnessOutcomeCode,
)
from guardian_assurance.freshness_persistence import (
    FreshnessLoadCode,
    FreshnessPersistCode,
    FreshnessPersistResult,
)
from guardian_assurance.freshness_state import make_unset_state

from test_m15_authentication import (
    M15AuthenticationTests,
    signed_wrapper,
)


class M15AuthenticatedFreshnessTests(unittest.TestCase):
    def setUp(self) -> None:
        self.auth_fixture = M15AuthenticationTests(
            methodName="test_valid_trusted_signature_authenticates"
        )
        self.auth_fixture.setUp()

        self.temp = tempfile.TemporaryDirectory()
        self.backend = HostFreshnessPersistenceBackend(
            Path(self.temp.name)
        )

    def tearDown(self) -> None:
        self.temp.cleanup()

    def wrapper(self) -> bytes:
        return signed_wrapper(
            private_key=self.auth_fixture.private_key,
        )

    def trust_store(self):
        return self.auth_fixture.trusted_store()

    def authenticate_fixture_wrapper(self):
        result = authenticate_signed_assurance(
            self.wrapper(),
            trust_store=self.trust_store(),
            provider=self.auth_fixture.provider,
        )

        self.assertTrue(result.authenticated)
        self.assertIsNotNone(result.producer_id)
        self.assertIsNotNone(result.producer_epoch)
        self.assertIsNotNone(result.logical_time)

        return result

    def install_unset_state(self) -> None:
        auth = self.authenticate_fixture_wrapper()

        state = make_unset_state(
            producer_id=auth.producer_id,
            current_epoch=auth.producer_epoch,
            generation=0,
        )

        prepared = self.backend.prepare(
            previous_state=None,
            candidate_state=state,
        )

        committed = self.backend.commit(prepared)
        self.assertEqual(
            committed.code,
            FreshnessPersistCode.COMMITTED,
        )

        verified = self.backend.verify(prepared)
        self.assertEqual(
            verified.code,
            FreshnessPersistCode.VERIFIED,
        )

    def test_authenticated_advancing_statement_becomes_fresh(self) -> None:
        self.install_unset_state()

        result = authenticate_and_establish_freshness(
            self.wrapper(),
            trust_store=self.trust_store(),
            provider=self.auth_fixture.provider,
            freshness_backend=self.backend,
        )

        self.assertTrue(result.authenticated)
        self.assertEqual(
            result.authentication.code,
            AuthenticationResultCode.AUTHENTICATED,
        )
        self.assertIsNotNone(result.freshness)
        self.assertEqual(
            result.freshness.code,
            FreshnessOutcomeCode.FRESH,
        )
        self.assertTrue(result.fresh)

    def test_authenticated_replay_is_not_fresh(self) -> None:
        self.install_unset_state()

        first = authenticate_and_establish_freshness(
            self.wrapper(),
            trust_store=self.trust_store(),
            provider=self.auth_fixture.provider,
            freshness_backend=self.backend,
        )
        self.assertTrue(first.fresh)

        second = authenticate_and_establish_freshness(
            self.wrapper(),
            trust_store=self.trust_store(),
            provider=self.auth_fixture.provider,
            freshness_backend=self.backend,
        )

        self.assertTrue(second.authenticated)
        self.assertIsNotNone(second.freshness)
        self.assertEqual(
            second.freshness.code,
            FreshnessOutcomeCode.REPLAY,
        )
        self.assertFalse(second.fresh)

    def test_authenticated_first_seen_does_not_bootstrap(self) -> None:
        result = authenticate_and_establish_freshness(
            self.wrapper(),
            trust_store=self.trust_store(),
            provider=self.auth_fixture.provider,
            freshness_backend=self.backend,
        )

        self.assertTrue(result.authenticated)
        self.assertIsNotNone(result.freshness)
        self.assertEqual(
            result.freshness.code,
            FreshnessOutcomeCode.FIRST_SEEN,
        )
        self.assertFalse(result.fresh)

        producer_id = result.authentication.producer_id
        loaded = self.backend.load(producer_id)
        self.assertEqual(
            loaded.code,
            FreshnessLoadCode.STATE_NOT_ESTABLISHED,
        )

    def test_tampered_signed_logical_time_never_reaches_freshness(self) -> None:
        raw = json.loads(self.wrapper().decode("utf-8"))
        raw["assurance_object"]["logical_time"] += 1

        tampered = json.dumps(
            raw,
            sort_keys=True,
            separators=(",", ":"),
        ).encode("utf-8")

        with mock.patch(
            "guardian_assurance.authenticated_freshness.establish_freshness"
        ) as freshness:
            result = authenticate_and_establish_freshness(
                tampered,
                trust_store=self.trust_store(),
                provider=self.auth_fixture.provider,
                freshness_backend=self.backend,
            )

        self.assertFalse(result.authenticated)
        self.assertIsNone(result.freshness)
        self.assertFalse(result.fresh)
        freshness.assert_not_called()

    def test_authentication_failure_never_touches_freshness(self) -> None:
        raw = json.loads(self.wrapper().decode("utf-8"))
        raw["signature"] = raw["signature"][:-1] + (
            "A" if raw["signature"][-1] != "A" else "B"
        )

        invalid = json.dumps(
            raw,
            sort_keys=True,
            separators=(",", ":"),
        ).encode("utf-8")

        with mock.patch(
            "guardian_assurance.authenticated_freshness.establish_freshness"
        ) as freshness:
            result = authenticate_and_establish_freshness(
                invalid,
                trust_store=self.trust_store(),
                provider=self.auth_fixture.provider,
                freshness_backend=self.backend,
            )

        self.assertFalse(result.authenticated)
        self.assertIsNone(result.freshness)
        freshness.assert_not_called()

    def test_persistence_failure_preserves_authenticated_not_fresh(self) -> None:
        self.install_unset_state()
        auth = self.authenticate_fixture_wrapper()

        with mock.patch.object(
            self.backend,
            "commit",
            return_value=FreshnessPersistResult(
                code=FreshnessPersistCode.FRESHNESS_STATE_PERSIST_FAILURE,
                producer_id=auth.producer_id,
            ),
        ):
            result = authenticate_and_establish_freshness(
                self.wrapper(),
                trust_store=self.trust_store(),
                provider=self.auth_fixture.provider,
                freshness_backend=self.backend,
            )

        self.assertTrue(result.authenticated)
        self.assertIsNotNone(result.freshness)
        self.assertEqual(
            result.freshness.code,
            FreshnessOutcomeCode.FRESHNESS_STATE_PERSIST_FAILURE,
        )
        self.assertFalse(result.fresh)

    def test_fresh_does_not_claim_authorization(self) -> None:
        self.install_unset_state()

        result = authenticate_and_establish_freshness(
            self.wrapper(),
            trust_store=self.trust_store(),
            provider=self.auth_fixture.provider,
            freshness_backend=self.backend,
        )

        self.assertTrue(result.authenticated)
        self.assertTrue(result.fresh)
        self.assertFalse(hasattr(result, "authorized"))
        self.assertFalse(hasattr(result, "actuation_allowed"))


if __name__ == "__main__":
    unittest.main(verbosity=2)
