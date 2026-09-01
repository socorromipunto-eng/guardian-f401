import unittest

from tools.governance_tooling.semantic import validate_registry


def registry(records):
    return {"schema": "guardian.semantic.registry.v1", "records": records}


class SemanticValidatorTests(unittest.TestCase):
    def test_supported_software_claim_passes(self):
        report = validate_registry(registry([
            {"id": "REQ-1", "type": "requirement"},
            {"id": "EV-1", "type": "evidence", "evidence_class": "software"},
            {"id": "CLAIM-1", "type": "claim", "status": "SUPPORTED", "claim_class": "software",
             "semantic_key": "software.behavior", "requirements": ["REQ-1"], "evidence": ["EV-1"]},
        ]))
        self.assertEqual(report.final, "PASS")

    def test_missing_reference_fails(self):
        report = validate_registry(registry([
            {"id": "CLAIM-1", "type": "claim", "status": "NOT_DEMONSTRATED", "requirements": ["REQ-MISSING"]}
        ]))
        self.assertTrue(any(f.rule_id == "SV-001" for f in report.errors))

    def test_supported_claim_without_evidence_fails(self):
        report = validate_registry(registry([{"id": "CLAIM-1", "type": "claim", "status": "SUPPORTED"}]))
        self.assertTrue(any(f.rule_id == "SV-002" for f in report.errors))

    def test_supported_and_not_demonstrated_same_key_fails(self):
        report = validate_registry(registry([
            {"id": "EV-1", "type": "evidence", "evidence_class": "software"},
            {"id": "CLAIM-A", "type": "claim", "status": "SUPPORTED", "semantic_key": "same", "evidence": ["EV-1"]},
            {"id": "CLAIM-B", "type": "claim", "status": "NOT_DEMONSTRATED", "semantic_key": "same"},
        ]))
        self.assertTrue(any(f.rule_id == "SV-004" for f in report.errors))

    def test_advisory_capability_cannot_bind_actuation_authority(self):
        report = validate_registry(registry([
            {"id": "AUTH-1", "type": "authority", "grants_actuation": True},
            {"id": "CAP-AI", "type": "capability", "advisory": True, "authority": "AUTH-1"},
        ]))
        self.assertTrue(any(f.rule_id == "SV-005" for f in report.errors))

    def test_advisory_capability_direct_actuation_fails(self):
        report = validate_registry(registry([
            {"id": "ACT-1", "type": "actuation"},
            {"id": "CAP-AI", "type": "capability", "advisory": True, "actuation": "ACT-1"},
        ]))
        self.assertTrue(any(f.rule_id == "SV-006" for f in report.errors))

    def test_structural_evidence_cannot_be_semantic_proof(self):
        report = validate_registry(registry([
            {"id": "EV-1", "type": "evidence", "evidence_class": "structural"},
            {"id": "CLAIM-1", "type": "claim", "status": "SUPPORTED", "claim_class": "semantic_proof", "evidence": ["EV-1"]},
        ]))
        self.assertTrue(any(f.rule_id == "SV-007" for f in report.errors))

    def test_software_evidence_cannot_prove_physical_validation(self):
        report = validate_registry(registry([
            {"id": "EV-1", "type": "evidence", "evidence_class": "software"},
            {"id": "CLAIM-1", "type": "claim", "status": "SUPPORTED", "claim_class": "physical_validation", "evidence": ["EV-1"]},
        ]))
        self.assertTrue(any(f.rule_id == "SV-008" for f in report.errors))

    def test_internal_evidence_cannot_prove_certification(self):
        report = validate_registry(registry([
            {"id": "EV-1", "type": "evidence", "evidence_class": "governance"},
            {"id": "CLAIM-1", "type": "claim", "status": "SUPPORTED", "claim_class": "certification", "evidence": ["EV-1"]},
        ]))
        self.assertTrue(any(f.rule_id == "SV-009" for f in report.errors))

    def test_orphan_evidence_warns_but_does_not_fail(self):
        report = validate_registry(registry([{"id": "EV-1", "type": "evidence", "evidence_class": "software"}]))
        self.assertEqual(report.final, "PASS")
        self.assertTrue(any(f.rule_id == "SV-010" for f in report.warnings))

    def test_not_demonstrated_is_not_false(self):
        report = validate_registry(registry([
            {"id": "CLAIM-1", "type": "claim", "status": "NOT_DEMONSTRATED", "semantic_key": "pending"}
        ]))
        self.assertEqual(report.final, "PASS")

    def test_authority_is_never_granted_by_validator(self):
        report = validate_registry(registry([]))
        self.assertTrue(all(value == "NOT_GRANTED" for value in report.authority.values()))


if __name__ == "__main__":
    unittest.main()
