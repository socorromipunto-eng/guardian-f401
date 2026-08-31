import unittest

from tools.governance_tooling.authority import MUTATING_ACTIONS, mutation_allowed


class AuthorityTests(unittest.TestCase):
    def test_all_mutating_actions_are_denied(self) -> None:
        self.assertTrue(MUTATING_ACTIONS)
        for action in MUTATING_ACTIONS:
            self.assertFalse(mutation_allowed(action))


if __name__ == "__main__":
    unittest.main()
