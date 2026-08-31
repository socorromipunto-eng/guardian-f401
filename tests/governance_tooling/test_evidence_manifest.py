from pathlib import Path
import json
import subprocess
import tempfile
import unittest

from tools.governance_tooling.evidence_manifest import (
    build_evidence_manifest,
    canonical_document_bytes,
    verify_evidence_manifest,
    write_manifest,
)
from tools.governance_tooling.repository import GitRepository


def git(repo: Path, *args: str) -> str:
    return subprocess.run(
        ["git", "-C", str(repo), *args],
        text=True, capture_output=True, shell=False, check=True
    ).stdout.strip()


class EvidenceManifestTests(unittest.TestCase):
    def make_repo(self):
        holder = tempfile.TemporaryDirectory()
        repo = Path(holder.name) / "repo"
        repo.mkdir()
        git(repo, "init")
        git(repo, "config", "user.email", "guardian-test@example.invalid")
        git(repo, "config", "user.name", "Guardian Test")
        (repo / "base.txt").write_text("base\n", encoding="utf-8")
        git(repo, "add", "base.txt")
        git(repo, "commit", "-m", "base")
        git(repo, "branch", "-M", "main")
        bare = Path(holder.name) / "origin.git"
        subprocess.run(["git", "clone", "--bare", str(repo), str(bare)], check=True, capture_output=True)
        git(repo, "remote", "add", "origin", str(bare))
        git(repo, "fetch", "origin")
        git(repo, "switch", "-c", "feature/test")
        return holder, repo

    def test_round_trip_manifest_verifies(self):
        h, repo = self.make_repo()
        try:
            (repo / "candidate.txt").write_text("candidate\n", encoding="utf-8")
            result, envelope = build_evidence_manifest(
                GitRepository(repo), ("candidate.txt",),
                run_id="TEST-RUN", timestamp_utc="2026-08-31T00:00:00Z"
            )
            self.assertEqual(result.final, "PASS")
            manifest = Path(h.name) / "evidence.json"
            write_manifest(manifest, envelope)
            self.assertEqual(verify_evidence_manifest(GitRepository(repo), manifest).final, "PASS")
        finally:
            h.cleanup()

    def test_tampered_payload_fails(self):
        h, repo = self.make_repo()
        try:
            (repo / "candidate.txt").write_text("candidate\n", encoding="utf-8")
            _, envelope = build_evidence_manifest(
                GitRepository(repo), ("candidate.txt",),
                run_id="TEST-RUN", timestamp_utc="2026-08-31T00:00:00Z"
            )
            envelope["payload"]["run_id"] = "TAMPERED"
            manifest = Path(h.name) / "evidence.json"
            manifest.write_bytes(canonical_document_bytes(envelope))
            self.assertEqual(verify_evidence_manifest(GitRepository(repo), manifest).final, "FAIL")
        finally:
            h.cleanup()

    def test_artifact_change_fails(self):
        h, repo = self.make_repo()
        try:
            path = repo / "candidate.bin"
            path.write_bytes(b"abc\x00def\n")
            _, envelope = build_evidence_manifest(
                GitRepository(repo), ("candidate.bin",),
                run_id="TEST-RUN", timestamp_utc="2026-08-31T00:00:00Z"
            )
            manifest = Path(h.name) / "evidence.json"
            write_manifest(manifest, envelope)
            path.write_bytes(b"changed")
            self.assertEqual(verify_evidence_manifest(GitRepository(repo), manifest).final, "FAIL")
        finally:
            h.cleanup()

    def test_noncanonical_json_fails(self):
        h, repo = self.make_repo()
        try:
            (repo / "candidate.txt").write_text("candidate\n", encoding="utf-8")
            _, envelope = build_evidence_manifest(
                GitRepository(repo), ("candidate.txt",),
                run_id="TEST-RUN", timestamp_utc="2026-08-31T00:00:00Z"
            )
            manifest = Path(h.name) / "evidence.json"
            manifest.write_text(json.dumps(envelope, indent=2), encoding="utf-8")
            self.assertEqual(verify_evidence_manifest(GitRepository(repo), manifest).final, "FAIL")
        finally:
            h.cleanup()


if __name__ == "__main__":
    unittest.main()
