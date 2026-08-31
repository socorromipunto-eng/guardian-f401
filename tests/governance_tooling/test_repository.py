from pathlib import Path
import subprocess
import tempfile
import unittest

from tools.governance_tooling.repository import GitRepository


def git(repo: Path, *args: str) -> str:
    completed = subprocess.run(
        ["git", "-C", str(repo), *args],
        capture_output=True,
        text=True,
        check=True,
        shell=False,
    )
    return completed.stdout.strip()


class RepositoryTests(unittest.TestCase):
    def test_repository_observes_branch_and_head(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory) / "repo"
            repo.mkdir()
            git(repo, "init")
            git(repo, "config", "user.email", "test@example.invalid")
            git(repo, "config", "user.name", "Guardian Test")
            (repo / "a.txt").write_text("a\n", encoding="utf-8")
            git(repo, "add", "a.txt")
            git(repo, "commit", "-m", "initial")
            observed = GitRepository(repo)
            self.assertTrue(observed.is_inside_work_tree())
            self.assertEqual(len(observed.head()), 40)
            self.assertTrue(observed.branch())


if __name__ == "__main__":
    unittest.main()
