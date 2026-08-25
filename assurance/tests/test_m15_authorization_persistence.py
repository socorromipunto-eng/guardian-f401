"""M15 authorization persistence-contract tests."""

from __future__ import annotations

import unittest

from guardian_assurance.authorization_persistence import (
    AuthorizationLoadCode,
    AuthorizationLoadResult,
    AuthorizationPersistCode,
    AuthorizationPersistResult,
)


class M15AuthorizationPersistenceContractTests(unittest.TestCase):
    def test_state_not_established_carries_no_state(self) -> None:
        result = AuthorizationLoadResult(
            AuthorizationLoadCode.STATE_NOT_ESTABLISHED
        )
        self.assertFalse(result.valid)
        self.assertIsNone(result.state)

    def test_invalid_load_cannot_carry_state(self) -> None:
        result = AuthorizationLoadResult(
            AuthorizationLoadCode.AUTHORIZATION_STATE_INVALID
        )
        self.assertFalse(result.valid)

    def test_prepared_is_not_committed_or_verified(self) -> None:
        result = AuthorizationPersistResult(
            AuthorizationPersistCode.PREPARED
        )
        self.assertTrue(result.prepared)
        self.assertFalse(result.committed)
        self.assertFalse(result.verified)

    def test_committed_is_not_verified(self) -> None:
        result = AuthorizationPersistResult(
            AuthorizationPersistCode.COMMITTED
        )
        self.assertFalse(result.prepared)
        self.assertTrue(result.committed)
        self.assertFalse(result.verified)

    def test_verified_is_distinct(self) -> None:
        result = AuthorizationPersistResult(
            AuthorizationPersistCode.VERIFIED
        )
        self.assertFalse(result.prepared)
        self.assertFalse(result.committed)
        self.assertTrue(result.verified)

    def test_failure_is_neither_prepared_committed_nor_verified(self) -> None:
        result = AuthorizationPersistResult(
            AuthorizationPersistCode.AUTHORIZATION_STATE_PERSIST_FAILURE
        )
        self.assertFalse(result.prepared)
        self.assertFalse(result.committed)
        self.assertFalse(result.verified)


if __name__ == "__main__":
    unittest.main(verbosity=2)
