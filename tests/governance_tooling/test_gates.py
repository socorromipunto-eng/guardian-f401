from pathlib import Path
import subprocess
import tempfile
import unittest

from tools.governance_tooling.gates import run_precommit, run_prepr, run_verify
from tools.governance_tooling.repository import GitRepository


def git(repo: Path, *args: str) -> str:
    completed = subprocess.run(
        ["git", "-C", str(repo), *args],
        text=True,
        capture_output=True,
        shell=False,
        check=True,
    )
    return completed.stdout.strip()


class GateTests(unittest.TestCase):
    def make_repo(self) -> tuple[tempfile.TemporaryDirectory, Path]:
        holder = tempfile.TemporaryDirectory()
        repo = Path(holder.name) / "repo"
        repo.mkdir()
        git(repo, "init")
        git(repo, "config", "user.email", "guardian-test@example.invalid")
        git(repo, "config", "user.name", "Guardian Test")
        (repo / "a.txt").write_text("a\n", encoding="utf-8")
        git(repo, "add", "a.txt")
        git(repo, "commit", "-m", "initial")
        git(repo, "branch", "-M", "main")
        bare = Path(holder.name) / "origin.git"
        subprocess.run(
            ["git", "clone", "--bare", str(repo), str(bare)],
            check=True,
            text=True,
            capture_output=True,
            shell=False,
        )
        git(repo, "remote", "add", "origin", str(bare))
        git(repo, "fetch", "origin")
        return holder, repo

    def test_verify_clean_main_passes(self) -> None:
        holder, repo = self.make_repo()
        try:
            result = run_verify(GitRepository(repo))
            self.assertEqual(result.final, "PASS")
        finally:
            holder.cleanup()

    def test_precommit_requires_staged_content(self) -> None:
        holder, repo = self.make_repo()
        try:
            git(repo, "switch", "-c", "feature/test")
            result = run_precommit(GitRepository(repo))
            self.assertEqual(result.final, "FAIL")
            (repo / "b.txt").write_text("b\n", encoding="utf-8")
            git(repo, "add", "b.txt")
            result = run_precommit(GitRepository(repo))
            self.assertEqual(result.final, "PASS")
        finally:
            holder.cleanup()

    def test_prepr_requires_feature_ahead_of_main(self) -> None:
        holder, repo = self.make_repo()
        try:
            git(repo, "switch", "-c", "feature/test")
            before = run_prepr(GitRepository(repo))
            self.assertEqual(before.final, "FAIL")
            (repo / "b.txt").write_text("b\n", encoding="utf-8")
            git(repo, "add", "b.txt")
            git(repo, "commit", "-m", "feature")
            after = run_prepr(GitRepository(repo))
            self.assertEqual(after.final, "PASS")
        finally:
            holder.cleanup()


if __name__ == "__main__":
    unittest.main()
