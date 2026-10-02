from __future__ import annotations
import tempfile
from pathlib import Path
import unittest
from tools.check_release_coherence import check

V="0.15.0"
D="10.5281/zenodo.22218923"
C="10.5281/zenodo.21981233"
DATE="2026-09-01"

def make(root: Path):
    (root/"docs/governance").mkdir(parents=True)
    (root/"VERSION").write_text(V+"\n")
    (root/"README.md").write_text(
        f"M15 governance / semantic assurance foundation\nProject release: v{V}\n"
        f"Firmware semantic version: {V}\nreleases/tag/v{V}\n"
        f"docs/release-evidence-v{V}.md\nhttps://doi.org/{D}\nhttps://doi.org/{C}\n"
    )
    (root/"CITATION.cff").write_text(
        f'version: "{V}"\ndate-released: "{DATE}"\nvalue: "{D}"\nvalue: "{C}"\n'
    )
    (root/"CHANGELOG.md").write_text(f"## [{V}] - {DATE}\n")
    (root/"docs/governance/M15-Governance-Closure.md").write_text(
        f"Status: PUBLISHED / BOUNDED\nZenodo version DOI: `{D}`\nZenodo concept DOI: `{C}`\n"
    )
    (root/f"docs/release-evidence-v{V}.md").write_text(
        f"# Guardian F401 v{V} Release Evidence\nPublication date: {DATE}\n"
        f"Zenodo version DOI: `{D}`\nZenodo concept DOI: `{C}`\n"
    )

class Tests(unittest.TestCase):
    def test_pass(self):
        with tempfile.TemporaryDirectory() as td:
            r=Path(td); make(r); self.assertEqual(check(r), [])
    def test_readme_drift(self):
        with tempfile.TemporaryDirectory() as td:
            r=Path(td); make(r)
            p=r/"README.md"; p.write_text(p.read_text().replace("Project release: v0.15.0","Project release: v0.14.2"))
            self.assertTrue(check(r))
    def test_citation_drift(self):
        with tempfile.TemporaryDirectory() as td:
            r=Path(td); make(r)
            p=r/"CITATION.cff"; p.write_text(p.read_text().replace('version: "0.15.0"','version: "0.14.2"'))
            self.assertTrue(check(r))
    def test_changelog_drift(self):
        with tempfile.TemporaryDirectory() as td:
            r=Path(td); make(r); (r/"CHANGELOG.md").write_text("## [0.14.2] - 2026-08-24\n")
            self.assertTrue(check(r))
    def test_candidate_closure_fails(self):
        with tempfile.TemporaryDirectory() as td:
            r=Path(td); make(r)
            p=r/"docs/governance/M15-Governance-Closure.md"; p.write_text(p.read_text().replace("PUBLISHED / BOUNDED","CANDIDATE"))
            self.assertTrue(check(r))



def make_candidate(root: Path):
    make(root)
    version = "0.16.0"
    (root / "VERSION").write_text(version + "\n", encoding="utf-8")
    (root / "README.md").write_text(
        f"M15 governance / semantic assurance foundation\nProject release: v{version}\n"
        f"Firmware semantic version: {version}\nPublication state: PREPUBLICATION_CANDIDATE\n"
        f"Last published release: v{V}\n"
        f"Planned GitHub release: https://github.com/socorromipunto-eng/guardian-f401/releases/tag/v{version}\n"
        f"docs/release-evidence-v{version}.md\nhttps://doi.org/{C}\n", encoding="utf-8")
    (root / "CITATION.cff").write_text(
        f'version: "{version}"\nmessage: "This is a prepublication candidate."\n'
        f'identifiers:\n  - type: doi\n    value: "{C}"\n', encoding="utf-8")
    (root / "CHANGELOG.md").write_text(f"## [{version}] - Unreleased\n", encoding="utf-8")
    fields = {
        "Status": "PREPUBLICATION_CANDIDATE", "Planned tag": "v0.16.0",
        "Publication performed": "NO", "Version DOI allocation": "PENDING",
        "Publication date allocation": "PENDING", "PhysicalHardwareValidation": "PENDING",
        "TargetGeometryQualification": "NOT_DEMONSTRATED", "RuntimeStackSufficiency": "NOT_DEMONSTRATED",
        "StorageOwnershipQualification": "NOT_DEMONSTRATED", "BackupChipDeployment": "NOT_DEMONSTRATED",
        "ProductionSecurityQualification": "NOT_DEMONSTRATED", "CertificationClaimAuthorized": "NO",
        "Zenodo concept DOI": f"`{C}`",
    }
    (root / "docs/release-evidence-v0.16.0.md").write_text(
        "\n".join(f"{key}: {value}" for key, value in fields.items()) + "\n", encoding="utf-8")


class PublicationStateTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        make_candidate(self.root)

    def change(self, name, old, new):
        path = self.root / name
        text = path.read_text(encoding="utf-8")
        self.assertEqual(text.count(old), 1)
        path.write_text(text.replace(old, new), encoding="utf-8")

    def test_candidate_coherence_is_not_publication(self):
        self.assertEqual(check(self.root), [])
        self.assertTrue(check(self.root, require_published=True))

    def test_candidate_has_no_fabricated_version_doi(self):
        with (self.root / "CITATION.cff").open("a", encoding="utf-8") as stream:
            stream.write(f'  - type: doi\n    value: "{D}"\n')
        self.assertTrue(check(self.root))

    def test_candidate_rejects_citation_date(self):
        with (self.root / "CITATION.cff").open("a", encoding="utf-8") as stream:
            stream.write('date-released: "2026-10-02"\n')
        self.assertTrue(check(self.root))

    def test_candidate_rejects_publication_fields(self):
        with (self.root / "docs/release-evidence-v0.16.0.md").open("a", encoding="utf-8") as stream:
            stream.write(f"Zenodo version DOI: `{D}`\nPublication date: 2026-10-02\n")
        self.assertTrue(check(self.root))

    def test_missing_and_false_candidate_limits_fail(self):
        for label in ("RuntimeStackSufficiency", "TargetGeometryQualification", "StorageOwnershipQualification",
                      "BackupChipDeployment", "ProductionSecurityQualification", "CertificationClaimAuthorized"):
            with self.subTest(label=label):
                path = self.root / "docs/release-evidence-v0.16.0.md"
                original = path.read_text(encoding="utf-8")
                path.write_text("\n".join(line for line in original.splitlines() if not line.startswith(label + ":")) + "\n", encoding="utf-8")
                self.assertTrue(check(self.root))
                path.write_text(original, encoding="utf-8")
                self.change(str(path.relative_to(self.root)), label + ":", label + ": QUALIFIED #")
                self.assertTrue(check(self.root))
                path.write_text(original, encoding="utf-8")

    def test_unknown_state_fails(self):
        self.change("docs/release-evidence-v0.16.0.md", "Status: PREPUBLICATION_CANDIDATE", "Status: READY")
        self.assertTrue(check(self.root))

    def test_duplicate_state_fails(self):
        self.change("docs/release-evidence-v0.16.0.md", "Status: PREPUBLICATION_CANDIDATE", "Status: PREPUBLICATION_CANDIDATE\nStatus: PREPUBLICATION_CANDIDATE")
        self.assertTrue(check(self.root))

    def test_new_version_missing_state_fails(self):
        self.change("docs/release-evidence-v0.16.0.md", "Status: PREPUBLICATION_CANDIDATE\n", "")
        self.assertTrue(check(self.root))

    def test_candidate_claiming_published_link_fails(self):
        self.change("README.md", "Planned GitHub release:", "GitHub release:")
        self.assertTrue(check(self.root))

    def test_candidate_date_in_changelog_fails(self):
        self.change("CHANGELOG.md", "Unreleased", "2026-10-02")
        self.assertTrue(check(self.root))

    def test_candidate_drift_fails(self):
        self.change("CITATION.cff", 'version: "0.16.0"', 'version: "0.17.0"')
        self.assertTrue(check(self.root))

    def test_historical_closure_must_keep_its_own_doi(self):
        self.change("docs/governance/M15-Governance-Closure.md", D, "10.5281/zenodo.99999999")
        self.assertTrue(check(self.root))

    def test_concept_family_drift_fails(self):
        self.change("docs/release-evidence-v0.16.0.md", C, "10.5281/zenodo.99999999")
        self.assertTrue(check(self.root))

    def publish_fixture(self, current_doi="10.5281/zenodo.99999999"):
        version = "0.16.0"
        date = "2026-10-02"
        (self.root / "README.md").write_text(
            f"M15 governance / semantic assurance foundation\nProject release: v{version}\n"
            f"Firmware semantic version: {version}\nreleases/tag/v{version}\n"
            f"docs/release-evidence-v{version}.md\nhttps://doi.org/{current_doi}\nhttps://doi.org/{C}\n", encoding="utf-8")
        (self.root / "CITATION.cff").write_text(
            f'version: "{version}"\ndate-released: "{date}"\nvalue: "{current_doi}"\nvalue: "{C}"\n', encoding="utf-8")
        (self.root / "CHANGELOG.md").write_text(f"## [{version}] - {date}\n", encoding="utf-8")
        (self.root / "docs/release-evidence-v0.16.0.md").write_text(
            f"Status: POST-PUBLICATION EVIDENCE RECORD\nPublication date: {date}\n"
            f"Zenodo version DOI: `{current_doi}`\nZenodo concept DOI: `{C}`\n", encoding="utf-8")

    def test_new_published_version_keeps_historical_closure(self):
        before = (self.root / "docs/governance/M15-Governance-Closure.md").read_bytes()
        self.publish_fixture()
        self.assertEqual(check(self.root, require_published=True), [])
        self.assertEqual(before, (self.root / "docs/governance/M15-Governance-Closure.md").read_bytes())

    def test_new_published_version_cannot_reuse_old_doi(self):
        self.publish_fixture(D)
        self.assertTrue(check(self.root, require_published=True))

    def test_published_invalid_date_fails(self):
        self.publish_fixture()
        self.change("docs/release-evidence-v0.16.0.md", "2026-10-02", "2026-02-30")
        self.assertTrue(check(self.root))

    def test_published_invalid_doi_fails(self):
        self.publish_fixture("PENDING")
        self.assertTrue(check(self.root))


if __name__ == "__main__":
    unittest.main()
