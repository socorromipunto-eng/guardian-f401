from __future__ import annotations

import csv
import io
import subprocess
import tempfile
from pathlib import Path
import unittest

from tools.release_manifest import build_entries, render_csv, verify


def git(repo: Path, *args: str) -> str:
    cp = subprocess.run(["git", "-C", str(repo), *args], capture_output=True, text=True, check=True)
    return cp.stdout.strip()


class ReleaseManifestTests(unittest.TestCase):
    def make_repo(self):
        td = tempfile.TemporaryDirectory()
        repo = Path(td.name)
        git(repo, "init")
        git(repo, "config", "user.email", "test@example.invalid")
        git(repo, "config", "user.name", "Test")
        return td, repo

    def test_manifest_hashes_git_blob_not_worktree_line_endings(self):
        td, repo = self.make_repo()
        try:
            (repo / ".gitattributes").write_text("*.txt text eol=lf\n", encoding="utf-8", newline="\n")
            (repo / "sample.txt").write_bytes(b"alpha\r\nbeta\r\n")
            git(repo, "add", ".")
            git(repo, "commit", "-m", "baseline")
            # Change working-tree bytes after commit; manifest must remain bound to HEAD blob.
            (repo / "sample.txt").write_bytes(b"alpha\r\nbeta\r\n")
            entries = {e.path: e for e in build_entries(repo, "HEAD")}
            blob = subprocess.run(
                ["git", "-C", str(repo), "cat-file", "blob", "HEAD:sample.txt"],
                capture_output=True, check=True
            ).stdout
            self.assertEqual(entries["sample.txt"].size, len(blob))
            import hashlib
            self.assertEqual(entries["sample.txt"].sha256, hashlib.sha256(blob).hexdigest().upper())
        finally:
            td.cleanup()

    def test_round_trip_verification(self):
        td, repo = self.make_repo()
        try:
            (repo / "a.txt").write_text("a\n", encoding="utf-8")
            (repo / "b.bin").write_bytes(b"\x00\x01")
            git(repo, "add", ".")
            git(repo, "commit", "-m", "baseline")
            manifest = repo / "manifest.csv"
            manifest.write_text(render_csv(build_entries(repo, "HEAD")), encoding="utf-8", newline="\n")
            self.assertEqual(verify(repo, "HEAD", manifest), [])
        finally:
            td.cleanup()

    def test_missing_row_fails(self):
        td, repo = self.make_repo()
        try:
            (repo / "a.txt").write_text("a\n", encoding="utf-8")
            (repo / "b.txt").write_text("b\n", encoding="utf-8")
            git(repo, "add", ".")
            git(repo, "commit", "-m", "baseline")
            rows = build_entries(repo, "HEAD")[:-1]
            manifest = repo / "manifest.csv"
            manifest.write_text(render_csv(rows), encoding="utf-8", newline="\n")
            self.assertTrue(any("missing manifest row" in e for e in verify(repo, "HEAD", manifest)))
        finally:
            td.cleanup()

    def test_tampered_hash_fails(self):
        td, repo = self.make_repo()
        try:
            (repo / "a.txt").write_text("a\n", encoding="utf-8")
            git(repo, "add", ".")
            git(repo, "commit", "-m", "baseline")
            manifest = repo / "manifest.csv"
            text = render_csv(build_entries(repo, "HEAD")).replace(
                build_entries(repo, "HEAD")[0].sha256,
                "0" * 64,
            )
            manifest.write_text(text, encoding="utf-8", newline="\n")
            self.assertTrue(any("SHA256 mismatch" in e for e in verify(repo, "HEAD", manifest)))
        finally:
            td.cleanup()

    def test_manifest_paths_use_forward_slashes(self):
        td, repo = self.make_repo()
        try:
            (repo / "d").mkdir()
            (repo / "d" / "a.txt").write_text("x\n", encoding="utf-8")
            git(repo, "add", ".")
            git(repo, "commit", "-m", "baseline")
            self.assertIn("d/a.txt", {e.path for e in build_entries(repo, "HEAD")})
        finally:
            td.cleanup()


if __name__ == "__main__":
    unittest.main()
