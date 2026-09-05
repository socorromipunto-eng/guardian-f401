from __future__ import annotations
import subprocess
import sys
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
VALIDATOR = REPO / "tools/validate_document_register.py"

class DocumentRegisterSemanticContractTests(unittest.TestCase):
    def test_registration_nonclaims_are_explicit(self):
        cp = subprocess.run(
            [sys.executable, str(VALIDATOR), "--repo", str(REPO)],
            text=True, capture_output=True, shell=False
        )
        self.assertEqual(cp.returncode, 0, cp.stdout + cp.stderr)
        for marker in (
            "REGISTERED_DOES_NOT_IMPLY_APPROVED=PASS",
            "REGISTERED_DOES_NOT_IMPLY_IMPLEMENTED=PASS",
            "REGISTERED_DOES_NOT_IMPLY_VALIDATED=PASS",
            "REGISTERED_DOES_NOT_IMPLY_EVIDENCE_PRESENT=PASS",
            "CONTROLLED_DOCUMENT_COMPLETENESS=NOT_DEMONSTRATED",
        ):
            self.assertIn(marker, cp.stdout)

if __name__ == "__main__":
    unittest.main()
