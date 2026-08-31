from pathlib import Path
import tempfile
import unittest

from tools.governance_tooling.errors import FailureClass, GovernanceToolError
from tools.governance_tooling.repository import GitRepository


class AdversarialTests(unittest.TestCase):
    def test_missing_repository_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            missing = Path(directory) / "missing"
            with self.assertRaises(GovernanceToolError) as context:
                GitRepository(missing)
            self.assertEqual(context.exception.failure_class, FailureClass.ENVIRONMENT)
            self.assertEqual(context.exception.code, "REPO_NOT_FOUND")


if __name__ == "__main__":
    unittest.main()
