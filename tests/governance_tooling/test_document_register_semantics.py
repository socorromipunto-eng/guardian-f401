from __future__ import annotations

import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
VALIDATOR = REPO / "tools/validate_document_register.py"

def run_validator(repo: Path):
    return subprocess.run(
        [sys.executable, str(VALIDATOR), "--repo", str(repo)],
        text=True, capture_output=True, shell=False
    )

def git(repo: Path, *args):
    return subprocess.run(
        ["git", *args],
        cwd=repo,
        text=True,
        capture_output=True,
        check=True
    ).stdout.strip()

def init_repo(td: Path):
    git(td, "init")
    git(td, "config", "user.email", "guardian@example.invalid")
    git(td, "config", "user.name", "Guardian Test")
    (td / "governance").mkdir(parents=True, exist_ok=True)
    (td / "docs").mkdir(parents=True, exist_ok=True)

def commit_doc(td: Path, path: str, content: str):
    p = td / path
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(content, encoding="utf-8", newline="\n")
    git(td, "add", path)
    git(td, "commit", "-m", "document")
    commit = git(td, "rev-parse", "HEAD")

    # The v2 integrity identity is the canonical Git blob, not checkout bytes.
    blob = subprocess.run(
        ["git", "show", f"{commit}:{path}"],
        cwd=td,
        capture_output=True,
        check=True,
        shell=False,
    ).stdout
    digest = hashlib.sha256(blob).hexdigest().upper()
    return commit, digest

def scope(component="guardian", **kwargs):
    out = {"component": [component]}
    for k, v in kwargs.items():
        out[k] = [v] if isinstance(v, str) else v
    return out

def entry(
    *,
    doc_id,
    version,
    path,
    sha256,
    source_commit,
    document_class="SPECIFICATION",
    lifecycle_status="APPROVED",
    authority_status="AUTHORITATIVE",
    authority_domain="guardian:test:domain",
    doc_scope=None,
    supersedes=None,
):
    return {
        "id": doc_id,
        "document_class": document_class,
        "version": version,
        "path": path,
        "sha256": sha256,
        "source_commit": source_commit,
        "lifecycle_status": lifecycle_status,
        "authority_status": authority_status,
        "authority_domain": authority_domain,
        "scope": doc_scope or scope(),
        "supersedes": supersedes or [],
    }

def write_register(td: Path, documents, completeness="NOT_DEMONSTRATED", version="2.0.0"):
    data = {
        "schema_version": version,
        "controlled_scope": {
            "id": "guardian:test:controlled-documents",
            "description": "test scope",
            "completeness_status": completeness,
        },
        "documents": documents,
    }
    (td / "governance/document-register.json").write_text(
        json.dumps(data, indent=2) + "\n",
        encoding="utf-8",
    )

class DocumentRegisterSemanticContractTests(unittest.TestCase):
    def test_v1_backward_compatibility_nonclaims_are_fixture_based(self):
        with tempfile.TemporaryDirectory() as raw:
            td = Path(raw)
            init_repo(td)

            (td / "governance/document-register.json").write_text(
                json.dumps({
                    "schema_version": "1.0.0",
                    "documents": [],
                }, indent=2) + "\n",
                encoding="utf-8",
            )

            cp = run_validator(td)
            self.assertEqual(cp.returncode, 0, cp.stdout + cp.stderr)

            for marker in (
                "DOCUMENT_REGISTER_SCHEMA=1.0.0",
                "REGISTERED_DOES_NOT_IMPLY_APPROVED=PASS",
                "REGISTERED_DOES_NOT_IMPLY_IMPLEMENTED=PASS",
                "REGISTERED_DOES_NOT_IMPLY_VALIDATED=PASS",
                "REGISTERED_DOES_NOT_IMPLY_EVIDENCE_PRESENT=PASS",
                "CONTROLLED_DOCUMENT_COMPLETENESS=NOT_DEMONSTRATED",
            ):
                self.assertIn(marker, cp.stdout)

    def test_valid_v2_single_authoritative_document_passes(self):
        with tempfile.TemporaryDirectory() as raw:
            td = Path(raw)
            init_repo(td)
            commit, digest = commit_doc(td, "docs/spec.md", "spec-v1\n")
            write_register(td, [
                entry(
                    doc_id="guardian:doc:test",
                    version="1.0.0",
                    path="docs/spec.md",
                    sha256=digest,
                    source_commit=commit,
                )
            ])
            cp = run_validator(td)
            self.assertEqual(cp.returncode, 0, cp.stdout + cp.stderr)
            self.assertIn("DOCUMENT_REGISTER_SCHEMA=2.0.0", cp.stdout)
            self.assertIn("DOCUMENT_REGISTER_V2_AUTHORITY_RESOLUTION=PASS", cp.stdout)

    def test_unknown_schema_fails_closed(self):
        with tempfile.TemporaryDirectory() as raw:
            td = Path(raw)
            init_repo(td)
            (td / "governance/document-register.json").write_text(
                '{"schema_version":"9.9.9","documents":[]}\n',
                encoding="utf-8",
            )
            cp = run_validator(td)
            self.assertNotEqual(cp.returncode, 0)
            self.assertIn("UNSUPPORTED_SCHEMA_VERSION", cp.stdout)

    def test_duplicate_identity_version_fails(self):
        with tempfile.TemporaryDirectory() as raw:
            td = Path(raw)
            init_repo(td)
            c1, h1 = commit_doc(td, "docs/a.md", "a\n")
            c2, h2 = commit_doc(td, "docs/b.md", "b\n")
            docs = [
                entry(doc_id="guardian:doc:x", version="1", path="docs/a.md", sha256=h1, source_commit=c1, authority_status="NONE"),
                entry(doc_id="guardian:doc:x", version="1", path="docs/b.md", sha256=h2, source_commit=c2, authority_status="NONE"),
            ]
            write_register(td, docs)
            cp = run_validator(td)
            self.assertNotEqual(cp.returncode, 0)
            self.assertIn("DUPLICATE_DOCUMENT_ID_VERSION", cp.stdout)

    def test_source_commit_hash_mismatch_fails(self):
        with tempfile.TemporaryDirectory() as raw:
            td = Path(raw)
            init_repo(td)
            commit, _ = commit_doc(td, "docs/a.md", "a\n")
            write_register(td, [
                entry(
                    doc_id="guardian:doc:a",
                    version="1",
                    path="docs/a.md",
                    sha256="0" * 64,
                    source_commit=commit,
                    authority_status="NONE",
                )
            ])
            cp = run_validator(td)
            self.assertNotEqual(cp.returncode, 0)
            self.assertIn("SOURCE_COMMIT_HASH_MISMATCH", cp.stdout)

    def test_self_status_candidate_vs_approved_fails(self):
        with tempfile.TemporaryDirectory() as raw:
            td = Path(raw)
            init_repo(td)
            commit, digest = commit_doc(td, "docs/a.md", "# A\n\nStatus: CANDIDATE\n")
            write_register(td, [
                entry(
                    doc_id="guardian:doc:a",
                    version="1",
                    path="docs/a.md",
                    sha256=digest,
                    source_commit=commit,
                    lifecycle_status="APPROVED",
                    authority_status="AUTHORITATIVE",
                )
            ])
            cp = run_validator(td)
            self.assertNotEqual(cp.returncode, 0)
            self.assertIn("STATUS_METADATA_DRIFT", cp.stdout)

    def test_self_status_candidate_vs_candidate_passes(self):
        with tempfile.TemporaryDirectory() as raw:
            td = Path(raw)
            init_repo(td)
            commit, digest = commit_doc(td, "docs/a.md", "# A\n\nStatus: CANDIDATE\n")
            write_register(td, [
                entry(
                    doc_id="guardian:doc:a",
                    version="1",
                    path="docs/a.md",
                    sha256=digest,
                    source_commit=commit,
                    lifecycle_status="CANDIDATE",
                    authority_status="NONE",
                )
            ])
            cp = run_validator(td)
            self.assertEqual(cp.returncode, 0, cp.stdout + cp.stderr)

    def test_self_status_human_adjudicated_vs_approved_passes(self):
        with tempfile.TemporaryDirectory() as raw:
            td = Path(raw)
            init_repo(td)
            commit, digest = commit_doc(
                td,
                "docs/a.md",
                "# A\n\nStatus: APPROVED - HUMAN ADJUDICATED\n",
            )
            write_register(td, [
                entry(
                    doc_id="guardian:doc:a",
                    version="1",
                    path="docs/a.md",
                    sha256=digest,
                    source_commit=commit,
                    lifecycle_status="APPROVED",
                    authority_status="AUTHORITATIVE",
                )
            ])
            cp = run_validator(td)
            self.assertEqual(cp.returncode, 0, cp.stdout + cp.stderr)

    def test_self_status_section_accepted_vs_approved_passes(self):
        with tempfile.TemporaryDirectory() as raw:
            td = Path(raw)
            init_repo(td)
            commit, digest = commit_doc(
                td,
                "docs/a.md",
                "# A\n\n## Status\n\nACCEPTED\n\n## Context\ntext\n",
            )
            write_register(td, [
                entry(
                    doc_id="guardian:doc:a",
                    version="1",
                    path="docs/a.md",
                    sha256=digest,
                    source_commit=commit,
                    lifecycle_status="APPROVED",
                    authority_status="AUTHORITATIVE",
                )
            ])
            cp = run_validator(td)
            self.assertEqual(cp.returncode, 0, cp.stdout + cp.stderr)

    def test_non_lifecycle_status_metadata_does_not_promote(self):
        with tempfile.TemporaryDirectory() as raw:
            td = Path(raw)
            init_repo(td)
            commit, digest = commit_doc(
                td,
                "docs/a.md",
                "# A\n\nStatus: CONTROLLED PROJECT GUIDANCE\n",
            )
            write_register(td, [
                entry(
                    doc_id="guardian:doc:a",
                    version="1",
                    path="docs/a.md",
                    sha256=digest,
                    source_commit=commit,
                    lifecycle_status="APPROVED",
                    authority_status="AUTHORITATIVE",
                )
            ])
            cp = run_validator(td)
            self.assertEqual(cp.returncode, 0, cp.stdout + cp.stderr)

    def test_absent_self_status_uses_register_lifecycle(self):
        with tempfile.TemporaryDirectory() as raw:
            td = Path(raw)
            init_repo(td)
            commit, digest = commit_doc(td, "docs/a.md", "# A\n\nNo lifecycle metadata.\n")
            write_register(td, [
                entry(
                    doc_id="guardian:doc:a",
                    version="1",
                    path="docs/a.md",
                    sha256=digest,
                    source_commit=commit,
                    lifecycle_status="APPROVED",
                    authority_status="AUTHORITATIVE",
                )
            ])
            cp = run_validator(td)
            self.assertEqual(cp.returncode, 0, cp.stdout + cp.stderr)

    def test_information_document_cannot_be_authoritative(self):
        with tempfile.TemporaryDirectory() as raw:
            td = Path(raw)
            init_repo(td)
            commit, digest = commit_doc(td, "docs/info.md", "info\n")
            write_register(td, [
                entry(
                    doc_id="guardian:doc:info",
                    version="1",
                    path="docs/info.md",
                    sha256=digest,
                    source_commit=commit,
                    document_class="INFORMATIONAL",
                    authority_status="AUTHORITATIVE",
                )
            ])
            cp = run_validator(td)
            self.assertNotEqual(cp.returncode, 0)
            self.assertIn("NON_NORMATIVE_CLASS_CANNOT_BE_AUTHORITATIVE", cp.stdout)

    def test_candidate_document_cannot_be_authoritative(self):
        with tempfile.TemporaryDirectory() as raw:
            td = Path(raw)
            init_repo(td)
            commit, digest = commit_doc(td, "docs/a.md", "a\n")
            write_register(td, [
                entry(
                    doc_id="guardian:doc:a",
                    version="1",
                    path="docs/a.md",
                    sha256=digest,
                    source_commit=commit,
                    lifecycle_status="CANDIDATE",
                    authority_status="AUTHORITATIVE",
                )
            ])
            cp = run_validator(td)
            self.assertNotEqual(cp.returncode, 0)
            self.assertIn("UNAPPROVED_DOCUMENT_CANNOT_BE_AUTHORITATIVE", cp.stdout)

    def test_overlapping_current_authority_same_domain_fails_closed(self):
        with tempfile.TemporaryDirectory() as raw:
            td = Path(raw)
            init_repo(td)
            c1, h1 = commit_doc(td, "docs/a.md", "a\n")
            c2, h2 = commit_doc(td, "docs/b.md", "b\n")
            docs = [
                entry(
                    doc_id="guardian:doc:a", version="1", path="docs/a.md",
                    sha256=h1, source_commit=c1,
                    authority_domain="guardian:domain:x",
                    doc_scope=scope(component="nodelink", chip_family="STM32F401"),
                ),
                entry(
                    doc_id="guardian:doc:b", version="1", path="docs/b.md",
                    sha256=h2, source_commit=c2,
                    authority_domain="guardian:domain:x",
                    doc_scope=scope(component="nodelink", chip_family="STM32F401"),
                ),
            ]
            write_register(td, docs)
            cp = run_validator(td)
            self.assertNotEqual(cp.returncode, 0)
            self.assertIn("AMBIGUOUS_CURRENT_AUTHORITY", cp.stdout)

    def test_disjoint_current_authority_same_domain_is_allowed(self):
        with tempfile.TemporaryDirectory() as raw:
            td = Path(raw)
            init_repo(td)
            c1, h1 = commit_doc(td, "docs/a.md", "a\n")
            c2, h2 = commit_doc(td, "docs/b.md", "b\n")
            docs = [
                entry(
                    doc_id="guardian:doc:a", version="1", path="docs/a.md",
                    sha256=h1, source_commit=c1,
                    authority_domain="guardian:domain:x",
                    doc_scope=scope(component="nodelink", chip_family="STM32F401"),
                ),
                entry(
                    doc_id="guardian:doc:b", version="1", path="docs/b.md",
                    sha256=h2, source_commit=c2,
                    authority_domain="guardian:domain:x",
                    doc_scope=scope(component="nodelink", chip_family="MCXN947"),
                ),
            ]
            write_register(td, docs)
            cp = run_validator(td)
            self.assertEqual(cp.returncode, 0, cp.stdout + cp.stderr)

    def test_scope_wildcard_cannot_mix_with_specific_values(self):
        with tempfile.TemporaryDirectory() as raw:
            td = Path(raw)
            init_repo(td)
            commit, digest = commit_doc(td, "docs/a.md", "a\n")
            write_register(td, [
                entry(
                    doc_id="guardian:doc:a",
                    version="1",
                    path="docs/a.md",
                    sha256=digest,
                    source_commit=commit,
                    authority_status="NONE",
                    doc_scope={"component": ["*", "nodelink"]},
                )
            ])
            cp = run_validator(td)
            self.assertNotEqual(cp.returncode, 0)
            self.assertIn("INVALID_SCOPE_SELECTOR", cp.stdout)

    def test_unknown_superseded_predecessor_fails(self):
        with tempfile.TemporaryDirectory() as raw:
            td = Path(raw)
            init_repo(td)
            commit, digest = commit_doc(td, "docs/a.md", "a\n")
            write_register(td, [
                entry(
                    doc_id="guardian:doc:a",
                    version="2",
                    path="docs/a.md",
                    sha256=digest,
                    source_commit=commit,
                    authority_status="NONE",
                    supersedes=[{"id": "guardian:doc:a", "version": "1"}],
                )
            ])
            cp = run_validator(td)
            self.assertNotEqual(cp.returncode, 0)
            self.assertIn("SUPERSEDES_UNKNOWN_PREDECESSOR", cp.stdout)

    def test_supersession_cycle_fails(self):
        with tempfile.TemporaryDirectory() as raw:
            td = Path(raw)
            init_repo(td)
            c1, h1 = commit_doc(td, "docs/a1.md", "a1\n")
            c2, h2 = commit_doc(td, "docs/a2.md", "a2\n")
            docs = [
                entry(
                    doc_id="guardian:doc:a", version="1", path="docs/a1.md",
                    sha256=h1, source_commit=c1, authority_status="NONE",
                    supersedes=[{"id": "guardian:doc:a", "version": "2"}],
                ),
                entry(
                    doc_id="guardian:doc:a", version="2", path="docs/a2.md",
                    sha256=h2, source_commit=c2, authority_status="NONE",
                    supersedes=[{"id": "guardian:doc:a", "version": "1"}],
                ),
            ]
            write_register(td, docs)
            cp = run_validator(td)
            self.assertNotEqual(cp.returncode, 0)
            self.assertIn("SUPERSESSION_CYCLE", cp.stdout)

    def test_v1_is_not_interpreted_with_v2_defaults(self):
        with tempfile.TemporaryDirectory() as raw:
            td = Path(raw)
            init_repo(td)
            (td / "governance/document-register.json").write_text(
                json.dumps({
                    "schema_version": "1.0.0",
                    "controlled_scope": {
                        "id": "x",
                        "description": "x",
                        "completeness_status": "NOT_DEMONSTRATED",
                    },
                    "documents": [],
                }) + "\n",
                encoding="utf-8",
            )
            cp = run_validator(td)
            self.assertNotEqual(cp.returncode, 0)
            self.assertIn("V1_TOP_LEVEL_FIELDS_INVALID", cp.stdout)

if __name__ == "__main__":
    unittest.main()
