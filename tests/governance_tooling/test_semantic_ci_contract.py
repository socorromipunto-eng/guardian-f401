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

    def test_project_state_workflow_routes_v1_and_v2_fail_closed(self):
        text = WORKFLOW.read_text(encoding="utf-8")
        case_pos = text.index('case "$schema_version" in')
        v1_pos = text.index('"1.0.0")', case_pos)
        v1_validator_pos = text.index(
            "python tools/validate_project_state.py --repo .", v1_pos
        )
        v2_pos = text.index('"2.0.0")', v1_validator_pos)
        v2_validator_pos = text.index(
            "python tools/validate_project_state_v2.py --repo . "
            "--project-state governance/project-state.json "
            "--materialization-commit 3f81869ab39321792e56822e0a71703ad1c0acb9",
            v2_pos,
        )
        unknown_pos = text.index(
            "REASON=UNKNOWN_PROJECT_STATE_SCHEMA_VERSION:$schema_version",
            v2_validator_pos,
        )
        exit_pos = text.index("exit 2", unknown_pos)
        esac_pos = text.index("esac", exit_pos)

        self.assertLess(case_pos, v1_pos)
        self.assertLess(v1_pos, v1_validator_pos)
        self.assertLess(v1_validator_pos, v2_pos)
        self.assertLess(v2_pos, v2_validator_pos)
        self.assertLess(v2_validator_pos, unknown_pos)
        self.assertLess(unknown_pos, exit_pos)
        self.assertLess(exit_pos, esac_pos)

    def test_project_state_v2_ci_binding_uses_authorized_materialization_commit(self):
        text = WORKFLOW.read_text(encoding="utf-8")
        self.assertIn(
            "--materialization-commit "
            "3f81869ab39321792e56822e0a71703ad1c0acb9",
            text,
        )
        self.assertIn("--project-state governance/project-state.json", text)

    def test_project_state_ci_router_does_not_fall_through_unknown_versions(self):
        text = WORKFLOW.read_text(encoding="utf-8")
        self.assertIn(
            "REASON=UNKNOWN_PROJECT_STATE_SCHEMA_VERSION:$schema_version",
            text,
        )
        self.assertIn("exit 2", text)
        self.assertNotIn("UNKNOWN_PROJECT_STATE_SCHEMA_VERSION=PASS", text)

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
