#!/usr/bin/env python3
from __future__ import annotations

import json
from datetime import datetime, timezone
from pathlib import Path

from jsonschema import Draft202012Validator, FormatChecker, ValidationError

ROOT = Path(__file__).resolve().parents[1]
NAMES = [
    "claims-register-v2.schema.json",
    "evidence-register-v2.schema.json",
    "assessment-rule-v1.schema.json",
    "assessment-record-v1.schema.json",
    "claim-transition-v1.schema.json",
]


class SemanticValidationError(ValueError):
    pass


def schema(name):
    return json.loads(
        (ROOT / "governance/schemas" / name).read_text(encoding="utf-8-sig")
    )


def validate_contracts():
    for name in NAMES:
        Draft202012Validator.check_schema(schema(name))
    return True


def validate_instance(name, instance):
    Draft202012Validator(
        schema(name),
        format_checker=FormatChecker(),
    ).validate(instance)
    return True


def _parse_datetime(value):
    if isinstance(value, datetime):
        dt = value
    elif isinstance(value, str):
        text = value[:-1] + "+00:00" if value.endswith("Z") else value
        try:
            dt = datetime.fromisoformat(text)
        except ValueError as exc:
            raise SemanticValidationError("INVALID_DATETIME") from exc
    else:
        raise SemanticValidationError("INVALID_DATETIME_TYPE")
    if dt.tzinfo is None:
        raise SemanticValidationError("NAIVE_DATETIME_PROHIBITED")
    return dt.astimezone(timezone.utc)


def validate_claims_register_semantics(register):
    validate_instance("claims-register-v2.schema.json", register)
    seen = set()
    for claim in register["claims"]:
        identity = (claim["id"], claim["version"])
        if identity in seen:
            raise SemanticValidationError("DUPLICATE_CLAIM_ID_VERSION")
        seen.add(identity)
    return True


def validate_evidence_register_semantics(register):
    validate_instance("evidence-register-v2.schema.json", register)
    seen = set()
    for item in register["evidence"]:
        identity = (item["id"], item["version"])
        if identity in seen:
            raise SemanticValidationError("DUPLICATE_EVIDENCE_ID_VERSION")
        seen.add(identity)
        observed = _parse_datetime(item["observed_at"])
        if "valid_until" in item:
            valid_until = _parse_datetime(item["valid_until"])
            if valid_until < observed:
                raise SemanticValidationError(
                    "VALID_UNTIL_BEFORE_OBSERVED_AT"
                )
    return True


def validate_assessment_record_semantics(record):
    validate_instance("assessment-record-v1.schema.json", record)
    supporting = set(record["evidence_refs"])
    contradictory = set(record["contradictory_evidence_refs"])
    if supporting.intersection(contradictory):
        raise SemanticValidationError(
            "ASSESSMENT_EVIDENCE_ROLE_SET_OVERLAP"
        )
    return True


def resolve_assessment_rule(rule, supported_rule_versions):
    validate_instance("assessment-rule-v1.schema.json", rule)
    supported = set(supported_rule_versions)
    key = (rule["rule_id"], rule["rule_version"])
    if key not in supported:
        raise SemanticValidationError("UNSUPPORTED_RULE_ID_VERSION")
    return True


def _applicability_is_eligible(item):
    values = item.get("applicability", {})
    return bool(values) and all(
        value.get("state") != "UNKNOWN"
        for value in values.values()
    )


def _claim_scope_is_eligible(item, target_claim_id):
    refs = item.get("eligible_claim_refs")
    return refs is None or target_claim_id in refs


def _intake_is_eligible(item, governed_intake_refs):
    return item.get("intake_ref") in set(governed_intake_refs)


def _temporal_is_eligible(item, rule, evaluation_time):
    try:
        evaluation = _parse_datetime(evaluation_time)
        observed = _parse_datetime(item["observed_at"])
        if observed > evaluation:
            return False
        if "valid_until" in item:
            valid_until = _parse_datetime(item["valid_until"])
            if valid_until < observed:
                return False
            if evaluation > valid_until:
                return False
        max_age = rule.get("max_age_seconds")
        if max_age is not None:
            age = (evaluation - observed).total_seconds()
            if age < 0 or age > max_age:
                return False
    except (KeyError, SemanticValidationError):
        return False
    return True


def _evidence_is_eligible(
    item,
    rule,
    *,
    target_claim_id,
    evaluation_time,
    governed_intake_refs,
):
    try:
        validate_instance(
            "evidence-register-v2.schema.json",
            {"schema_version": "2.0.0", "evidence": [item]},
        )
    except ValidationError:
        return False

    if item["validation_state"] != "VALID":
        return False
    if not _applicability_is_eligible(item):
        return False
    if not _claim_scope_is_eligible(item, target_claim_id):
        return False
    if not _intake_is_eligible(item, governed_intake_refs):
        return False
    if not _temporal_is_eligible(item, rule, evaluation_time):
        return False
    return True


def classify_material_contradictions(
    rule,
    evidence,
    *,
    target_claim_id,
    evaluation_time,
    governed_intake_refs,
):
    material_classes = set(rule["material_contradiction_classes"])
    out = []
    for item in evidence:
        if item.get("role") != "CONTRADICTORY":
            continue
        if item.get("evidence_class") not in material_classes:
            continue
        if _evidence_is_eligible(
            item,
            rule,
            target_claim_id=target_claim_id,
            evaluation_time=evaluation_time,
            governed_intake_refs=governed_intake_refs,
        ):
            out.append(item)
    return out


def evaluate_candidate(
    rule,
    evidence,
    *,
    target_claim_id,
    evaluation_time,
    supported_rule_versions,
    governed_intake_refs,
):
    if not isinstance(target_claim_id, str) or not target_claim_id:
        raise SemanticValidationError("TARGET_CLAIM_ID_REQUIRED")

    resolve_assessment_rule(rule, supported_rule_versions)

    contradictions = classify_material_contradictions(
        rule,
        evidence,
        target_claim_id=target_claim_id,
        evaluation_time=evaluation_time,
        governed_intake_refs=governed_intake_refs,
    )
    if contradictions:
        return "CONTRADICTED"

    required = set(rule["required_evidence_classes"])
    support = {
        item["evidence_class"]
        for item in evidence
        if item.get("role") == "SUPPORTING"
        and _evidence_is_eligible(
            item,
            rule,
            target_claim_id=target_claim_id,
            evaluation_time=evaluation_time,
            governed_intake_refs=governed_intake_refs,
        )
    }

    if required and required.issubset(support):
        return "DEMONSTRATED"
    if required.intersection(support):
        return "PARTIALLY_DEMONSTRATED"
    return "NOT_DEMONSTRATED"


def validate_transition_against_claim(transition, claim):
    validate_instance("claim-transition-v1.schema.json", transition)
    validate_claims_register_semantics(
        {"schema_version": "2.0.0", "claims": [claim]}
    )
    if transition["claim_id"] != claim.get("id"):
        raise SemanticValidationError("TRANSITION_CLAIM_ID_MISMATCH")
    if transition["claim_version"] != claim.get("version"):
        raise SemanticValidationError("TRANSITION_CLAIM_VERSION_MISMATCH")
    if (
        transition["from_assessment_state"]
        != claim.get("assessment_state")
    ):
        raise SemanticValidationError(
            "TRANSITION_FROM_ASSESSMENT_STATE_MISMATCH"
        )
    if (
        transition["from_adjudication_state"]
        != claim.get("adjudication_state")
    ):
        raise SemanticValidationError(
            "TRANSITION_FROM_ADJUDICATION_STATE_MISMATCH"
        )
    return True


def validate_transition_against_assessment(transition, assessment):
    validate_instance("claim-transition-v1.schema.json", transition)
    validate_assessment_record_semantics(assessment)

    scalar_pairs = [
        ("assessment_record_id", "assessment_record_id"),
        ("claim_id", "claim_id"),
        ("claim_version", "claim_version"),
        ("rule_id", "rule_id"),
        ("rule_version", "rule_version"),
    ]
    for transition_key, assessment_key in scalar_pairs:
        if transition[transition_key] != assessment[assessment_key]:
            raise SemanticValidationError(
                "TRANSITION_ASSESSMENT_BINDING_MISMATCH:"
                + transition_key
            )

    if (
        transition["to_assessment_state"]
        != assessment["assessment_state"]
    ):
        raise SemanticValidationError(
            "TRANSITION_TO_ASSESSMENT_STATE_MISMATCH"
        )

    if set(transition["evidence_refs"]) != set(assessment["evidence_refs"]):
        raise SemanticValidationError(
            "TRANSITION_SUPPORTING_EVIDENCE_SET_MISMATCH"
        )

    if set(transition["contradictory_evidence_refs"]) != set(
        assessment["contradictory_evidence_refs"]
    ):
        raise SemanticValidationError(
            "TRANSITION_CONTRADICTORY_EVIDENCE_SET_MISMATCH"
        )
    return True


def migrate_claim_v1_to_v2(v1_claim):
    if not isinstance(v1_claim, dict):
        raise SemanticValidationError("V1_CLAIM_OBJECT_REQUIRED")
    claim_id = v1_claim.get("id")
    version = v1_claim.get("version")
    if not isinstance(claim_id, str) or not claim_id:
        raise SemanticValidationError("V1_CLAIM_ID_REQUIRED")
    if not isinstance(version, str) or not version:
        raise SemanticValidationError("V1_CLAIM_VERSION_REQUIRED")
    return {
        "id": claim_id,
        "version": version,
        "assessment_state": "NOT_DEMONSTRATED",
        "adjudication_state": "UNADJUDICATED",
        "evidence_refs": [],
        "contradictory_evidence_refs": [],
    }


if __name__ == "__main__":
    validate_contracts()
    print("CLAIM_EVIDENCE_PROMOTION_SCHEMA_CONTRACTS=PASS")
    print("EVALUATOR_MUTATION_AUTHORITY=NO")
    print("PERSISTENT_ADJUDICATION_BY_VALIDATOR=PROHIBITED")
