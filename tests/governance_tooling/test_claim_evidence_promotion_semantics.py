from __future__ import annotations

import ast
import sys
import unittest
from pathlib import Path

from jsonschema import ValidationError

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

import validate_claim_evidence_promotion as v

APPLICABILITY_KEYS = [
    "source_commit",
    "source_tree",
    "firmware_version",
    "hardware_family",
    "hardware_revision",
    "configuration_id",
    "semantic_contract_id",
    "semantic_contract_version",
    "protocol_id",
    "protocol_version",
    "toolchain_id",
    "environment_class",
    "execution_mode",
]

EVALUATION_TIME = "2026-09-05T20:00:00Z"
RULE_KEY = ("guardian:assessment-rule:test", "1.0.0")
INTAKE_REF = "guardian:intake:test"


def applicability(state="EXPLICIT_VALUE"):
    if state == "EXPLICIT_VALUE":
        return {
            key: {"state": "EXPLICIT_VALUE", "value": "test"}
            for key in APPLICABILITY_KEYS
        }
    return {
        key: {"state": state}
        for key in APPLICABILITY_KEYS
    }


def evidence(
    evidence_id,
    evidence_class,
    *,
    role="SUPPORTING",
    validation="VALID",
    observed_at=EVALUATION_TIME,
    valid_until=None,
    eligible_claim_refs=None,
    app_state="EXPLICIT_VALUE",
    intake_ref=INTAKE_REF,
):
    item = {
        "id": evidence_id,
        "version": "1",
        "evidence_class": evidence_class,
        "validation_state": validation,
        "role": role,
        "observed_at": observed_at,
        "applicability": applicability(app_state),
        "provenance": {
            "origin_type": "TOOL",
            "producer": "guardian-test",
            "captured_at": observed_at,
            "artifact_sha256": "a" * 64,
        },
        "intake_ref": intake_ref,
    }
    if valid_until is not None:
        item["valid_until"] = valid_until
    if eligible_claim_refs is not None:
        item["eligible_claim_refs"] = eligible_claim_refs
    return item


def rule(
    required,
    *,
    material=None,
    rule_id=RULE_KEY[0],
    rule_version=RULE_KEY[1],
    max_age_seconds=None,
):
    item = {
        "schema_version": "1.0.0",
        "rule_id": rule_id,
        "rule_version": rule_version,
        "required_evidence_classes": required,
        "material_contradiction_classes": (
            list(required) if material is None else material
        ),
        "contradiction_policy":
            "MATERIAL_CONTRADICTION_YIELDS_CONTRADICTED",
    }
    if max_age_seconds is not None:
        item["max_age_seconds"] = max_age_seconds
    return item


def evaluate(rule_obj, evidence_items, *, claim_id="C1", intakes=None):
    return v.evaluate_candidate(
        rule_obj,
        evidence_items,
        target_claim_id=claim_id,
        evaluation_time=EVALUATION_TIME,
        supported_rule_versions={RULE_KEY},
        governed_intake_refs={INTAKE_REF} if intakes is None else intakes,
    )


def claim(
    *,
    claim_id="C1",
    version="1",
    assessment="NOT_DEMONSTRATED",
    adjudication="UNADJUDICATED",
):
    return {
        "id": claim_id,
        "version": version,
        "assessment_state": assessment,
        "adjudication_state": adjudication,
        "evidence_refs": [],
        "contradictory_evidence_refs": [],
    }


def assessment_record(
    *,
    state="DEMONSTRATED",
    evidence_refs=None,
    contradictory_refs=None,
):
    return {
        "schema_version": "1.0.0",
        "assessment_record_id": "A1",
        "claim_id": "C1",
        "claim_version": "1",
        "rule_id": RULE_KEY[0],
        "rule_version": RULE_KEY[1],
        "assessment_state": state,
        "evidence_refs": evidence_refs or [],
        "contradictory_evidence_refs": contradictory_refs or [],
        "source_commit": "0" * 40,
        "source_tree": "1" * 40,
        "created_at": EVALUATION_TIME,
    }


def transition(
    *,
    from_assessment="NOT_DEMONSTRATED",
    to_assessment="DEMONSTRATED",
    from_adjudication="UNADJUDICATED",
    to_adjudication="UNADJUDICATED",
    evidence_refs=None,
    contradictory_refs=None,
    adjudicator=None,
):
    item = {
        "schema_version": "1.0.0",
        "transition_id": "T1",
        "claim_id": "C1",
        "claim_version": "1",
        "from_assessment_state": from_assessment,
        "to_assessment_state": to_assessment,
        "from_adjudication_state": from_adjudication,
        "to_adjudication_state": to_adjudication,
        "assessment_record_id": "A1",
        "rule_id": RULE_KEY[0],
        "rule_version": RULE_KEY[1],
        "evidence_refs": evidence_refs or [],
        "contradictory_evidence_refs": contradictory_refs or [],
        "source_commit": "0" * 40,
        "source_tree": "1" * 40,
        "created_at": EVALUATION_TIME,
        "transition_reason": "test",
    }
    if adjudicator is not None:
        item["adjudicator"] = adjudicator
    return item


class ClaimEvidencePromotionSemanticTests(unittest.TestCase):
    def test_schema_contracts_validate(self):
        self.assertTrue(v.validate_contracts())

    def test_unknown_property_fails_closed(self):
        x = {
            "schema_version": "2.0.0",
            "claims": [dict(claim(), typo=1)],
        }
        with self.assertRaises(ValidationError):
            v.validate_instance("claims-register-v2.schema.json", x)

    def test_unknown_applicability_cannot_demonstrate(self):
        state = evaluate(
            rule(["BUILD"]),
            [evidence("E1", "BUILD", app_state="UNKNOWN")],
        )
        self.assertEqual(state, "NOT_DEMONSTRATED")

    def test_rule_freshness_is_enforced(self):
        state = evaluate(
            rule(["BUILD"], max_age_seconds=60),
            [
                evidence(
                    "E1",
                    "BUILD",
                    observed_at="2020-01-01T00:00:00Z",
                )
            ],
        )
        self.assertEqual(state, "NOT_DEMONSTRATED")

    def test_unsupported_rule_version_fails_closed(self):
        bad = rule(["BUILD"], rule_version="999.999.999")
        with self.assertRaises(v.SemanticValidationError):
            v.evaluate_candidate(
                bad,
                [evidence("E1", "BUILD")],
                target_claim_id="C1",
                evaluation_time=EVALUATION_TIME,
                supported_rule_versions={RULE_KEY},
                governed_intake_refs={INTAKE_REF},
            )

    def test_duplicate_evidence_identity_rejected(self):
        item = evidence("E1", "BUILD")
        register = {
            "schema_version": "2.0.0",
            "evidence": [item, dict(item)],
        }
        with self.assertRaisesRegex(
            v.SemanticValidationError,
            "DUPLICATE_EVIDENCE_ID_VERSION",
        ):
            v.validate_evidence_register_semantics(register)

    def test_duplicate_claim_identity_rejected(self):
        item = claim()
        register = {
            "schema_version": "2.0.0",
            "claims": [item, dict(item)],
        }
        with self.assertRaisesRegex(
            v.SemanticValidationError,
            "DUPLICATE_CLAIM_ID_VERSION",
        ):
            v.validate_claims_register_semantics(register)

    def test_transition_from_state_coherence_enforced(self):
        current = claim(assessment="NOT_DEMONSTRATED")
        tx = transition(
            from_assessment="DEMONSTRATED",
            to_assessment="INVALIDATED",
        )
        with self.assertRaisesRegex(
            v.SemanticValidationError,
            "TRANSITION_FROM_ASSESSMENT_STATE_MISMATCH",
        ):
            v.validate_transition_against_claim(tx, current)

    def test_transition_preserves_contradictions(self):
        record = assessment_record(
            state="CONTRADICTED",
            evidence_refs=["E1"],
            contradictory_refs=["E2"],
        )
        tx = transition(
            to_assessment="CONTRADICTED",
            evidence_refs=["E1"],
            contradictory_refs=[],
        )
        with self.assertRaisesRegex(
            v.SemanticValidationError,
            "TRANSITION_CONTRADICTORY_EVIDENCE_SET_MISMATCH",
        ):
            v.validate_transition_against_assessment(tx, record)

    def test_v1_to_v2_migration_fails_closed(self):
        old = {
            "id": "legacy-claim",
            "version": "1",
            "status": "CONFIRMED",
        }
        migrated = v.migrate_claim_v1_to_v2(old)
        self.assertEqual(migrated["id"], old["id"])
        self.assertEqual(migrated["version"], old["version"])
        self.assertEqual(
            migrated["assessment_state"],
            "NOT_DEMONSTRATED",
        )
        self.assertEqual(
            migrated["adjudication_state"],
            "UNADJUDICATED",
        )
        self.assertEqual(migrated["evidence_refs"], [])
        self.assertEqual(migrated["contradictory_evidence_refs"], [])

    def test_governed_intake_resolution_required(self):
        item = evidence("E1", "BUILD")
        self.assertTrue(
            v.validate_instance(
                "evidence-register-v2.schema.json",
                {"schema_version": "2.0.0", "evidence": [item]},
            )
        )
        state = evaluate(
            rule(["BUILD"]),
            [item],
            intakes=set(),
        )
        self.assertEqual(state, "NOT_DEMONSTRATED")

    def test_eligible_claim_scope_enforced(self):
        state = evaluate(
            rule(["BUILD"]),
            [
                evidence(
                    "E1",
                    "BUILD",
                    eligible_claim_refs=["C2"],
                )
            ],
            claim_id="C1",
        )
        self.assertEqual(state, "NOT_DEMONSTRATED")

    def test_only_material_contradictions_block(self):
        state = evaluate(
            rule(["BUILD"], material=["BUILD"]),
            [
                evidence("E1", "BUILD"),
                evidence(
                    "E2",
                    "DOCUMENT_REVIEW",
                    role="CONTRADICTORY",
                ),
            ],
        )
        self.assertEqual(state, "DEMONSTRATED")

    def test_temporal_interval_coherence_enforced(self):
        item = evidence(
            "E1",
            "BUILD",
            observed_at="2026-09-05T20:00:00Z",
            valid_until="2026-09-04T20:00:00Z",
        )
        with self.assertRaisesRegex(
            v.SemanticValidationError,
            "VALID_UNTIL_BEFORE_OBSERVED_AT",
        ):
            v.validate_evidence_register_semantics(
                {"schema_version": "2.0.0", "evidence": [item]}
            )

    def test_assessment_support_and_contradiction_sets_disjoint(self):
        record = assessment_record(
            evidence_refs=["E1"],
            contradictory_refs=["E1"],
        )
        with self.assertRaisesRegex(
            v.SemanticValidationError,
            "ASSESSMENT_EVIDENCE_ROLE_SET_OVERLAP",
        ):
            v.validate_assessment_record_semantics(record)

    def test_host_execution_does_not_satisfy_physical_hardware(self):
        state = evaluate(
            rule(["PHYSICAL_HARDWARE"]),
            [evidence("E1", "HOST_EXECUTION")],
        )
        self.assertEqual(state, "NOT_DEMONSTRATED")

    def test_build_does_not_imply_behavioral_validation(self):
        state = evaluate(
            rule(["INTEGRATION_TEST"]),
            [evidence("E1", "BUILD")],
        )
        self.assertEqual(state, "NOT_DEMONSTRATED")

    def test_valid_material_contradiction_blocks_promotion(self):
        state = evaluate(
            rule(["BUILD"], material=["BUILD"]),
            [
                evidence("E1", "BUILD"),
                evidence("E2", "BUILD", role="CONTRADICTORY"),
            ],
        )
        self.assertEqual(state, "CONTRADICTED")

    def test_invalid_contradiction_cannot_contradict(self):
        state = evaluate(
            rule(["BUILD"], material=["BUILD"]),
            [
                evidence("E1", "BUILD"),
                evidence(
                    "E2",
                    "BUILD",
                    role="CONTRADICTORY",
                    validation="INVALID",
                ),
            ],
        )
        self.assertEqual(state, "DEMONSTRATED")

    def test_unknown_schema_version_fails_closed(self):
        with self.assertRaises(ValidationError):
            v.validate_instance(
                "claims-register-v2.schema.json",
                {
                    "schema_version": "9.9.9",
                    "claims": [claim()],
                },
            )

    def test_custom_evidence_class_requires_namespace(self):
        item = evidence("E1", "CUSTOM")
        with self.assertRaises(ValidationError):
            v.validate_instance(
                "evidence-register-v2.schema.json",
                {
                    "schema_version": "2.0.0",
                    "evidence": [item],
                },
            )

    def test_custom_evidence_does_not_satisfy_core_class(self):
        item = evidence("E1", "CUSTOM")
        item["custom_class_namespace"] = "guardian:test"
        state = evaluate(rule(["BUILD"]), [item])
        self.assertEqual(state, "NOT_DEMONSTRATED")

    def test_assessment_record_cannot_adjudicate(self):
        record = assessment_record()
        record["adjudication_state"] = "ACCEPTED"
        with self.assertRaises(ValidationError):
            v.validate_instance(
                "assessment-record-v1.schema.json",
                record,
            )

    def test_validator_has_no_persistent_mutation_surface(self):
        source = (
            ROOT / "tools/validate_claim_evidence_promotion.py"
        ).read_text(encoding="utf-8-sig")
        tree = ast.parse(source)
        prohibited = []
        for node in ast.walk(tree):
            if isinstance(node, ast.Call):
                if isinstance(node.func, ast.Attribute):
                    if node.func.attr in {
                        "write_text",
                        "write_bytes",
                        "unlink",
                        "mkdir",
                        "rename",
                        "replace",
                    }:
                        prohibited.append(node.func.attr)
                elif (
                    isinstance(node.func, ast.Name)
                    and node.func.id == "open"
                    and len(node.args) >= 2
                    and isinstance(node.args[1], ast.Constant)
                ):
                    mode = str(node.args[1].value)
                    if any(ch in mode for ch in ("w", "a", "x", "+")):
                        prohibited.append("open:" + mode)
        self.assertEqual(prohibited, [])

    def test_accepted_transition_requires_adjudicator(self):
        tx = transition(to_adjudication="ACCEPTED")
        with self.assertRaises(ValidationError):
            v.validate_instance(
                "claim-transition-v1.schema.json",
                tx,
            )

    def test_claim_contract_rejects_authority_and_actuation(self):
        for field in ("authority_granted", "actuation_authorized"):
            item = claim()
            item[field] = True
            with self.assertRaises(ValidationError):
                v.validate_instance(
                    "claims-register-v2.schema.json",
                    {
                        "schema_version": "2.0.0",
                        "claims": [item],
                    },
                )

    def test_positive_path_demonstrates_only_when_all_gates_pass(self):
        state = evaluate(
            rule(["BUILD"]),
            [evidence("E1", "BUILD")],
        )
        self.assertEqual(state, "DEMONSTRATED")

    def test_expired_valid_until_cannot_demonstrate(self):
        state = evaluate(
            rule(["BUILD"]),
            [
                evidence(
                    "E1",
                    "BUILD",
                    observed_at="2026-09-05T18:00:00Z",
                    valid_until="2026-09-05T19:00:00Z",
                )
            ],
        )
        self.assertEqual(state, "NOT_DEMONSTRATED")

    def test_future_observation_cannot_demonstrate(self):
        state = evaluate(
            rule(["BUILD"]),
            [
                evidence(
                    "E1",
                    "BUILD",
                    observed_at="2026-09-05T21:00:00Z",
                )
            ],
        )
        self.assertEqual(state, "NOT_DEMONSTRATED")

    def test_transition_to_assessment_matches_record(self):
        record = assessment_record(
            state="DEMONSTRATED",
            evidence_refs=["E1"],
        )
        tx = transition(
            to_assessment="PARTIALLY_DEMONSTRATED",
            evidence_refs=["E1"],
        )
        with self.assertRaisesRegex(
            v.SemanticValidationError,
            "TRANSITION_TO_ASSESSMENT_STATE_MISMATCH",
        ):
            v.validate_transition_against_assessment(tx, record)


if __name__ == "__main__":
    unittest.main()
