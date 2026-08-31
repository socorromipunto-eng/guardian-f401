from pathlib import Path
import subprocess
import tempfile
import unittest

from tools.governance_tooling.integration import run_foundation_phase
from tools.governance_tooling.repository import GitRepository


def git(repo: Path, *args: str) -> str:
    return subprocess.run(
        ["git", "-C", str(repo), *args],
        text=True,
        capture_output=True,
        shell=False,
        check=True,
    ).stdout.strip()


class FoundationIntegrationTests(unittest.TestCase):
    def make_repo(self):
        holder = tempfile.TemporaryDirectory()
        root = Path(holder.name)
        repo = root / "repo"
        repo.mkdir()
        git(repo, "init")
        git(repo, "config", "user.email", "guardian-test@example.invalid")
        git(repo, "config", "user.name", "Guardian Test")
        (repo / "base.txt").write_text("base\n", encoding="utf-8")
        git(repo, "add", "base.txt")
        git(repo, "commit", "-m", "base")
        git(repo, "branch", "-M", "main")
        bare = root / "origin.git"
        subprocess.run(["git", "clone", "--bare", str(repo), str(bare)], check=True, capture_output=True)
        git(repo, "remote", "add", "origin", str(bare))
        git(repo, "fetch", "origin")
        git(repo, "switch", "-c", "feature/test")
        return holder, repo

    def test_candidate_phase_passes_with_external_evidence(self):
        h, repo = self.make_repo()
        try:
            (repo / "candidate.txt").write_text("candidate\n", encoding="utf-8")
            evidence = Path(h.name) / "evidence.json"
            result, record = run_foundation_phase(
                GitRepository(repo), "candidate", ("candidate.txt",), (), evidence
            )
            self.assertEqual(result.final, "PASS")
            self.assertTrue(evidence.exists())
            self.assertEqual(record["payload"]["phase"], "candidate")
        finally:
            h.cleanup()

    def test_precommit_phase_passes_after_staging(self):
        h, repo = self.make_repo()
        try:
            (repo / "candidate.txt").write_text("candidate\n", encoding="utf-8")
            git(repo, "add", "candidate.txt")
            result, record = run_foundation_phase(
                GitRepository(repo), "precommit"
            )
            self.assertEqual(result.final, "PASS")
            self.assertEqual(
                record["payload"]["outcomes"],
                [{"name": "precommit", "final": "PASS"}],
            )
        finally:
            h.cleanup()

    def test_precommit_rejects_candidate_phase_arguments(self):
        h, repo = self.make_repo()
        try:
            (repo / "candidate.txt").write_text("candidate\n", encoding="utf-8")
            git(repo, "add", "candidate.txt")
            result, _ = run_foundation_phase(
                GitRepository(repo),
                "precommit",
                ("candidate.txt",),
                (),
                Path(h.name) / "evidence.json",
            )
            self.assertEqual(result.final, "FAIL")
        finally:
            h.cleanup()

    def test_prepr_phase_passes_after_commit(self):
        h, repo = self.make_repo()
        try:
            (repo / "candidate.txt").write_text("candidate\n", encoding="utf-8")
            git(repo, "add", "candidate.txt")
            git(repo, "commit", "-m", "candidate")
            result, record = run_foundation_phase(GitRepository(repo), "prepr")
            self.assertEqual(result.final, "PASS")
            self.assertEqual(record["payload"]["phase"], "prepr")
        finally:
            h.cleanup()

    def test_authority_remains_denied(self):
        h, repo = self.make_repo()
        try:
            (repo / "candidate.txt").write_text("candidate\n", encoding="utf-8")
            evidence = Path(h.name) / "evidence.json"
            result, _ = run_foundation_phase(
                GitRepository(repo), "candidate", ("candidate.txt",), (), evidence
            )
            self.assertTrue(all(v == "NOT_GRANTED" for v in result.authority.values()))
        finally:
            h.cleanup()


if __name__ == "__main__":
    unittest.main()
