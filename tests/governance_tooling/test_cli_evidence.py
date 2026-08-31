from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


def git(repo: Path, *args: str) -> str:
    return subprocess.run(
        ["git", "-C", str(repo), *args],
        text=True, capture_output=True, shell=False, check=True
    ).stdout.strip()


class EvidenceCliTests(unittest.TestCase):
    def test_output_inside_repository_fails_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory) / "repo"
            repo.mkdir()
            git(repo, "init")
            git(repo, "config", "user.email", "guardian-test@example.invalid")
            git(repo, "config", "user.name", "Guardian Test")
            (repo / "base.txt").write_text("base\n", encoding="utf-8")
            git(repo, "add", "base.txt")
            git(repo, "commit", "-m", "base")
            git(repo, "branch", "-M", "main")
            bare = Path(directory) / "origin.git"
            subprocess.run(["git", "clone", "--bare", str(repo), str(bare)], check=True, capture_output=True)
            git(repo, "remote", "add", "origin", str(bare))
            git(repo, "fetch", "origin")
            git(repo, "switch", "-c", "feature/test")
            (repo / "candidate.txt").write_text("candidate\n", encoding="utf-8")

            completed = subprocess.run(
                [
                    sys.executable, "-m", "tools.governance_tooling",
                    "--repo", str(repo), "evidence",
                    "--allow=candidate.txt",
                    f"--output={repo / 'evidence.json'}",
                ],
                cwd=Path(__file__).resolve().parents[2],
                text=True, capture_output=True, shell=False,
            )
            self.assertEqual(completed.returncode, 2)
            self.assertIn("EVIDENCE_OUTPUT_INSIDE_REPOSITORY", completed.stderr)
            self.assertFalse((repo / "evidence.json").exists())


if __name__ == "__main__":
    unittest.main()
