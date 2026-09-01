from __future__ import annotations

import json
from pathlib import Path
import unittest

REPO = Path(__file__).resolve().parents[2]
REGISTRY = REPO / "governance" / "semantic" / "m15-semantic-registry.json"
WORKFLOW = REPO / ".github" / "workflows" / "m15-semantic-governance.yml"

class SemanticCiContractTests(unittest.TestCase):
    def test_registry_exists_and_is_controlled_index(self):
        data = json.loads(REGISTRY.read_text(encoding="utf-8"))
        self.assertEqual(data["schema"], "guardian.semantic.registry.v1")
        self.assertEqual(data["role"], "CONTROLLED_INDEX")
        self.assertFalse(data["governance"]["replaces_adr"])
        self.assertFalse(data["governance"]["replaces_evidence"])
        self.assertEqual(data["governance"]["claim_status_change"], "HUMAN_GOVERNED")
        self.assertFalse(data["governance"]["validator_can_change_status"])
        self.assertFalse(data["governance"]["warning_blocks_ci"])
        self.assertTrue(data["governance"]["error_blocks_ci"])

    def test_not_demonstrated_limits_are_materialized(self):
        data = json.loads(REGISTRY.read_text(encoding="utf-8"))
        claims = {r["id"]: r for r in data["records"] if r.get("type") == "claim"}
        self.assertEqual(claims["CLAIM-M15-HW-001"]["status"], "NOT_DEMONSTRATED")
        self.assertEqual(claims["CLAIM-M15-CERT-001"]["status"], "NOT_DEMONSTRATED")
        self.assertEqual(claims["CLAIM-M15-PROD-001"]["status"], "NOT_DEMONSTRATED")

    def test_workflow_is_read_only_and_invokes_semantic_validator(self):
        text = WORKFLOW.read_text(encoding="utf-8")
        self.assertIn("permissions:\n  contents: read", text)
        self.assertIn('python-version: "3.12"', text)
        self.assertIn(
            "python -m tools.governance_tooling --repo . semantic-validate --registry governance/semantic/m15-semantic-registry.json",
            text,
        )

    def test_workflow_does_not_ignore_validator_exit_code(self):
        text = WORKFLOW.read_text(encoding="utf-8")
        self.assertNotIn("|| true", text)
        self.assertNotIn("continue-on-error: true", text)
        self.assertNotIn("set +e", text)

    def test_workflow_does_not_request_write_permissions(self):
        text = WORKFLOW.read_text(encoding="utf-8")
        for marker in ("contents: write", "pull-requests: write", "actions: write", "checks: write", "issues: write"):
            self.assertNotIn(marker, text)

if __name__ == "__main__":
    unittest.main()
