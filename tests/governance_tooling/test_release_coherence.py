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

if __name__=="__main__":
    unittest.main()
