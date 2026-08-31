import unittest

from tools.governance_tooling.result import Result


class ResultTests(unittest.TestCase):
    def test_pass_when_all_checks_pass(self) -> None:
        result = Result(command="test", repository="repo")
        result.add("ONE", True)
        self.assertEqual(result.final, "PASS")

    def test_fail_when_any_check_fails(self) -> None:
        result = Result(command="test", repository="repo")
        result.add("ONE", True)
        result.add("TWO", False)
        self.assertEqual(result.final, "FAIL")


if __name__ == "__main__":
    unittest.main()
