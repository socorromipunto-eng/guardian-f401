from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


class CliSemanticsTests(unittest.TestCase):
    def test_inspect_is_observational_on_dirty_repo(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory) / "repo"
            repo.mkdir()
            subprocess.run(
                ["git", "-C", str(repo), "init"],
                check=True,
                capture_output=True,
            )
            subprocess.run(
                ["git", "-C", str(repo), "config", "user.email", "guardian-test@example.invalid"],
                check=True,
            )
            subprocess.run(
                ["git", "-C", str(repo), "config", "user.name", "Guardian Test"],
                check=True,
            )
            (repo / "a.txt").write_text("a\n", encoding="utf-8")
            subprocess.run(
                ["git", "-C", str(repo), "add", "a.txt"],
                check=True,
            )
            subprocess.run(
                ["git", "-C", str(repo), "commit", "-m", "initial"],
                check=True,
            )
            (repo / "a.txt").write_text("dirty\n", encoding="utf-8")

            completed = subprocess.run(
                [
                    sys.executable,
                    "-m",
                    "tools.governance_tooling",
                    "--repo",
                    str(repo),
                    "inspect",
                ],
                cwd=Path(__file__).resolve().parents[2],
                text=True,
                capture_output=True,
                shell=False,
            )

            self.assertEqual(completed.returncode, 0)
            self.assertIn("FINAL=PASS", completed.stdout)


if __name__ == "__main__":
    unittest.main()
