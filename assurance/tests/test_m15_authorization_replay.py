"""M15 pure authorization anti-replay tests."""

from __future__ import annotations

import unittest

from guardian_assurance.authorization_authentication import (
    AuthorizationAuthenticationResult,
    AuthorizationAuthenticationResultCode,
)
from guardian_assurance.authorization_objects import (
    BOOTSTRAP_TYPE,
    EPOCH_TRANSITION_TYPE,
)
from guardian_assurance.authorization_replay import (
    AUTHORIZATION_REPLAY_MAX_SCOPES,
    UINT64_MAX,
    AuthorizationReplayCode,
    AuthorizationReplayScope,
    AuthorizationReplayState,
    evaluate_authorization_replay,
)
from guardian_assurance.authorization_trust_store import (
    BOOTSTRAP_AUTHORITY,
    EPOCH_TRANSITION_AUTHORITY,
)


def authenticated(
    *,
    authorization_type: str = BOOTSTRAP_TYPE,
    authority_id: str = "root.bootstrap-authority:01",
    producer_id: str = "plant-a.guardian-01",
    authorization_id: str = "0123456789abcdef0123456789abcdef",
    authorization_sequence: int = 1,
) -> AuthorizationAuthenticationResult:
    return AuthorizationAuthenticationResult(
        code=AuthorizationAuthenticationResultCode.AUTHORIZATION_AUTHENTICATED,
        authority_id=authority_id,
        key_id="bootstrap-key-001",
        algorithm="ed25519",
        authorization_type=authorization_type,
        authorization_id=authorization_id,
        authorization_sequence=authorization_sequence,
        producer_id=producer_id,
    )


class M15AuthorizationReplayTests(unittest.TestCase):
    def test_first_sequence_is_candidate(self) -> None:
        result = evaluate_authorization_replay(
            authenticated(),
            previous_state=None,
            established_scope_count=0,
        )
        self.assertEqual(result.code, AuthorizationReplayCode.AUTHORIZATION_CANDIDATE)
        self.assertTrue(result.candidate)
        self.assertEqual(result.candidate_state.authorization_sequence, 1)

    def test_equal_sequence_is_replay(self) -> None:
        scope = AuthorizationReplayScope(
            "root.bootstrap-authority:01",
            BOOTSTRAP_AUTHORITY,
            "plant-a.guardian-01",
        )
        previous = AuthorizationReplayState(scope, 7)
        result = evaluate_authorization_replay(
            authenticated(authorization_sequence=7),
            previous_state=previous,
            established_scope_count=1,
        )
        self.assertEqual(result.code, AuthorizationReplayCode.AUTHORIZATION_REPLAY)

    def test_older_sequence_is_replay(self) -> None:
        scope = AuthorizationReplayScope(
            "root.bootstrap-authority:01",
            BOOTSTRAP_AUTHORITY,
            "plant-a.guardian-01",
        )
        previous = AuthorizationReplayState(scope, 7)
        result = evaluate_authorization_replay(
            authenticated(authorization_sequence=6),
            previous_state=previous,
            established_scope_count=1,
        )
        self.assertEqual(result.code, AuthorizationReplayCode.AUTHORIZATION_REPLAY)

    def test_strictly_newer_sequence_is_candidate(self) -> None:
        scope = AuthorizationReplayScope(
            "root.bootstrap-authority:01",
            BOOTSTRAP_AUTHORITY,
            "plant-a.guardian-01",
        )
        previous = AuthorizationReplayState(scope, 7)
        result = evaluate_authorization_replay(
            authenticated(authorization_sequence=8),
            previous_state=previous,
            established_scope_count=1,
        )
        self.assertEqual(result.code, AuthorizationReplayCode.AUTHORIZATION_CANDIDATE)
        self.assertEqual(result.candidate_state.authorization_sequence, 8)

    def test_bootstrap_and_epoch_transition_use_distinct_scopes(self) -> None:
        bootstrap = evaluate_authorization_replay(
            authenticated(),
            previous_state=None,
            established_scope_count=0,
        )
        transition = evaluate_authorization_replay(
            authenticated(
                authorization_type=EPOCH_TRANSITION_TYPE,
                authority_id="root.bootstrap-authority:01",
            ),
            previous_state=None,
            established_scope_count=0,
        )
        self.assertEqual(bootstrap.scope.purpose_domain, BOOTSTRAP_AUTHORITY)
        self.assertEqual(
            transition.scope.purpose_domain,
            EPOCH_TRANSITION_AUTHORITY,
        )
        self.assertNotEqual(bootstrap.scope, transition.scope)

    def test_capacity_4096_fails_new_scope_closed(self) -> None:
        result = evaluate_authorization_replay(
            authenticated(),
            previous_state=None,
            established_scope_count=AUTHORIZATION_REPLAY_MAX_SCOPES,
        )
        self.assertEqual(
            result.code,
            AuthorizationReplayCode.AUTHORIZATION_CAPACITY_EXCEEDED,
        )

    def test_existing_scope_is_not_blocked_by_global_capacity(self) -> None:
        scope = AuthorizationReplayScope(
            "root.bootstrap-authority:01",
            BOOTSTRAP_AUTHORITY,
            "plant-a.guardian-01",
        )
        previous = AuthorizationReplayState(scope, 7)
        result = evaluate_authorization_replay(
            authenticated(authorization_sequence=8),
            previous_state=previous,
            established_scope_count=AUTHORIZATION_REPLAY_MAX_SCOPES,
        )
        self.assertEqual(result.code, AuthorizationReplayCode.AUTHORIZATION_CANDIDATE)

    def test_uint64_high_water_exhaustion_fails_closed(self) -> None:
        scope = AuthorizationReplayScope(
            "root.bootstrap-authority:01",
            BOOTSTRAP_AUTHORITY,
            "plant-a.guardian-01",
        )
        previous = AuthorizationReplayState(scope, UINT64_MAX)
        result = evaluate_authorization_replay(
            authenticated(authorization_sequence=UINT64_MAX),
            previous_state=previous,
            established_scope_count=1,
        )
        self.assertEqual(
            result.code,
            AuthorizationReplayCode.AUTHORIZATION_SEQUENCE_EXHAUSTED,
        )

    def test_scope_mismatch_fails_closed(self) -> None:
        previous = AuthorizationReplayState(
            AuthorizationReplayScope(
                "different-authority",
                BOOTSTRAP_AUTHORITY,
                "plant-a.guardian-01",
            ),
            1,
        )
        result = evaluate_authorization_replay(
            authenticated(authorization_sequence=2),
            previous_state=previous,
            established_scope_count=1,
        )
        self.assertEqual(
            result.code,
            AuthorizationReplayCode.AUTHORIZATION_STATE_INVALID,
        )

    def test_non_authenticated_input_never_becomes_candidate(self) -> None:
        value = AuthorizationAuthenticationResult(
            code=AuthorizationAuthenticationResultCode.SIGNATURE_INVALID,
            authority_id="root.bootstrap-authority:01",
            authorization_type=BOOTSTRAP_TYPE,
            authorization_sequence=10,
            producer_id="plant-a.guardian-01",
        )
        result = evaluate_authorization_replay(
            value,
            previous_state=None,
            established_scope_count=0,
        )
        self.assertEqual(
            result.code,
            AuthorizationReplayCode.AUTHORIZATION_NOT_AUTHENTICATED,
        )

    def test_candidate_does_not_claim_consumed(self) -> None:
        result = evaluate_authorization_replay(
            authenticated(),
            previous_state=None,
            established_scope_count=0,
        )
        self.assertTrue(result.candidate)
        self.assertFalse(hasattr(result, "consumed"))
        self.assertFalse(hasattr(result, "authorized"))
        self.assertFalse(hasattr(result, "fresh"))


if __name__ == "__main__":
    unittest.main(verbosity=2)
