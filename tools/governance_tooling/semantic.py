"""FND-05 deterministic semantic validator."""

from __future__ import annotations

from dataclasses import asdict, dataclass, field
import json
from pathlib import Path
from typing import Any

from .evidence import DEFAULT_NOT_PROVEN

SCHEMA = "guardian.semantic.registry.v1"
ALLOWED_TYPES = {"requirement", "capability", "claim", "evidence", "authority", "actuation"}
CLAIM_STATUSES = {"SUPPORTED", "PARTIALLY_SUPPORTED", "NOT_DEMONSTRATED", "CONTRADICTED", "NOT_APPLICABLE"}
EVIDENCE_CLASSES = {"structural", "software", "simulation", "physical", "certification", "governance"}
FAIL_SEVERITY = "ERROR"
WARN_SEVERITY = "WARN"


@dataclass(frozen=True, slots=True)
class Finding:
    rule_id: str
    severity: str
    subject: str
    detail: str


@dataclass(slots=True)
class SemanticReport:
    registry: str
    schema: str | None = None
    findings: list[Finding] = field(default_factory=list)
    authority: dict[str, str] = field(default_factory=lambda: {
        "COMMIT": "NOT_GRANTED",
        "PUSH": "NOT_GRANTED",
        "PR": "NOT_GRANTED",
        "MERGE": "NOT_GRANTED",
        "ACTUATION": "NOT_GRANTED",
    })
    not_proven: list[str] = field(default_factory=lambda: list(DEFAULT_NOT_PROVEN))

    @property
    def errors(self) -> list[Finding]:
        return [f for f in self.findings if f.severity == FAIL_SEVERITY]

    @property
    def warnings(self) -> list[Finding]:
        return [f for f in self.findings if f.severity == WARN_SEVERITY]

    @property
    def final(self) -> str:
        return "PASS" if not self.errors else "FAIL"

    def add(self, rule_id: str, severity: str, subject: str, detail: str) -> None:
        self.findings.append(Finding(rule_id, severity, subject, detail))

    def to_dict(self) -> dict[str, Any]:
        return {
            "registry": self.registry,
            "schema": self.schema,
            "findings": [asdict(item) for item in self.findings],
            "error_count": len(self.errors),
            "warning_count": len(self.warnings),
            "authority": dict(self.authority),
            "not_proven": list(self.not_proven),
            "final": self.final,
        }

    def to_json(self) -> str:
        return json.dumps(self.to_dict(), indent=2, sort_keys=True)

    def to_text(self) -> str:
        lines = [
            "GUARDIAN_SEMANTIC_VALIDATOR_RESULT_V1",
            f"REGISTRY={self.registry}",
            f"SCHEMA={self.schema or 'UNKNOWN'}",
        ]
        for finding in self.findings:
            lines.append(f"FINDING={finding.rule_id}|{finding.severity}|{finding.subject}|{finding.detail}")
        lines.append(f"ERROR_COUNT={len(self.errors)}")
        lines.append(f"WARNING_COUNT={len(self.warnings)}")
        lines.extend(f"NOT_PROVEN={item}" for item in self.not_proven)
        lines.extend(f"AUTHORITY_{key}={value}" for key, value in self.authority.items())
        lines.append(f"FINAL={self.final}")
        return "\n".join(lines)


def load_registry(path: Path) -> dict[str, Any]:
    return json.loads(path.read_bytes().decode("utf-8"))


def _refs(record: dict[str, Any]) -> list[tuple[str, str]]:
    pairs: list[tuple[str, str]] = []
    for field_name in ("requirements", "capabilities", "evidence"):
        value = record.get(field_name, [])
        if isinstance(value, list):
            pairs.extend((field_name, item) for item in value if isinstance(item, str))
    for field_name in ("authority", "actuation"):
        value = record.get(field_name)
        if isinstance(value, str):
            pairs.append((field_name, value))
    return pairs


def validate_registry(data: dict[str, Any], registry_name: str = "<memory>") -> SemanticReport:
    report = SemanticReport(registry=registry_name)
    report.schema = data.get("schema") if isinstance(data, dict) else None
    report.not_proven.extend([
        "absence of semantic contradiction is not proof of correctness",
        "semantic coherence is not physical hardware validation",
        "semantic coherence is not certification or production readiness",
    ])

    if not isinstance(data, dict):
        report.add("SV-000", FAIL_SEVERITY, registry_name, "registry root must be an object")
        return report

    if data.get("schema") != SCHEMA:
        report.add("SV-000", FAIL_SEVERITY, registry_name, f"schema must equal {SCHEMA}")

    records = data.get("records")
    if not isinstance(records, list):
        report.add("SV-000", FAIL_SEVERITY, registry_name, "records must be a list")
        return report

    by_id: dict[str, dict[str, Any]] = {}
    duplicates: set[str] = set()

    for index, record in enumerate(records):
        subject = f"records[{index}]"
        if not isinstance(record, dict):
            report.add("SV-000", FAIL_SEVERITY, subject, "record must be an object")
            continue
        record_id = record.get("id")
        record_type = record.get("type")
        if not isinstance(record_id, str) or not record_id:
            report.add("SV-000", FAIL_SEVERITY, subject, "record id must be a non-empty string")
            continue
        if record_id in by_id:
            duplicates.add(record_id)
        else:
            by_id[record_id] = record
        if record_type not in ALLOWED_TYPES:
            report.add("SV-000", FAIL_SEVERITY, record_id, f"unsupported type {record_type!r}")

    for record_id in sorted(duplicates):
        report.add("SV-000", FAIL_SEVERITY, record_id, "duplicate identifier")

    for record_id, record in sorted(by_id.items()):
        for field_name, target in _refs(record):
            if target not in by_id:
                report.add("SV-001", FAIL_SEVERITY, record_id, f"{field_name} references missing id {target}")

    for record_id, record in sorted(by_id.items()):
        if record.get("type") == "evidence":
            evidence_class = record.get("evidence_class")
            if evidence_class not in EVIDENCE_CLASSES:
                report.add("SV-000", FAIL_SEVERITY, record_id, f"invalid evidence_class {evidence_class!r}")
        if record.get("type") == "claim":
            status = record.get("status")
            if status not in CLAIM_STATUSES:
                report.add("SV-000", FAIL_SEVERITY, record_id, f"invalid status {status!r}")

    referenced_evidence: set[str] = set()
    for claim_id, claim in sorted(by_id.items()):
        if claim.get("type") != "claim":
            continue
        status = claim.get("status")
        evidence_ids = [x for x in claim.get("evidence", []) if isinstance(x, str)]
        referenced_evidence.update(evidence_ids)
        evidence_records = [by_id[eid] for eid in evidence_ids if eid in by_id and by_id[eid].get("type") == "evidence"]
        evidence_classes = {item.get("evidence_class") for item in evidence_records}
        claim_class = claim.get("claim_class", "general")

        if status == "SUPPORTED" and not evidence_records:
            report.add("SV-002", FAIL_SEVERITY, claim_id, "SUPPORTED claim has no bound evidence")
        if status == "SUPPORTED" and len(evidence_records) != len(evidence_ids):
            report.add("SV-002", FAIL_SEVERITY, claim_id, "SUPPORTED claim contains non-evidence or unresolved evidence references")
        if claim_class == "semantic_proof" and evidence_records and evidence_classes <= {"structural"}:
            report.add("SV-007", FAIL_SEVERITY, claim_id, "structural evidence alone cannot establish semantic proof")
        if claim_class == "physical_validation" and status == "SUPPORTED" and "physical" not in evidence_classes:
            report.add("SV-008", FAIL_SEVERITY, claim_id, "physical validation claim lacks physical evidence")
        if claim_class == "certification" and status == "SUPPORTED" and "certification" not in evidence_classes:
            report.add("SV-009", FAIL_SEVERITY, claim_id, "certification claim lacks certification evidence")

    semantic_statuses: dict[str, set[str]] = {}
    semantic_subjects: dict[str, list[str]] = {}
    for claim_id, claim in sorted(by_id.items()):
        if claim.get("type") != "claim":
            continue
        key = claim.get("semantic_key")
        status = claim.get("status")
        if isinstance(key, str) and key and isinstance(status, str):
            semantic_statuses.setdefault(key, set()).add(status)
            semantic_subjects.setdefault(key, []).append(claim_id)

    for key, statuses in sorted(semantic_statuses.items()):
        subjects = ",".join(sorted(semantic_subjects[key]))
        if "SUPPORTED" in statuses and "CONTRADICTED" in statuses:
            report.add("SV-003", FAIL_SEVERITY, subjects, f"semantic_key {key} is both SUPPORTED and CONTRADICTED")
        if "SUPPORTED" in statuses and "NOT_DEMONSTRATED" in statuses:
            report.add("SV-004", FAIL_SEVERITY, subjects, f"semantic_key {key} is both SUPPORTED and NOT_DEMONSTRATED")

    for record_id, record in sorted(by_id.items()):
        if record.get("type") != "capability":
            continue
        advisory = record.get("advisory") is True
        authority_id = record.get("authority")
        actuation_id = record.get("actuation")

        if advisory and isinstance(authority_id, str):
            authority_record = by_id.get(authority_id)
            if isinstance(authority_record, dict) and authority_record.get("type") == "authority" and authority_record.get("grants_actuation") is True:
                report.add("SV-005", FAIL_SEVERITY, record_id, "advisory capability is bound to actuation-granting authority")

        if advisory and isinstance(actuation_id, str):
            report.add("SV-006", FAIL_SEVERITY, record_id, "advisory capability directly references actuation")

    for evidence_id, record in sorted(by_id.items()):
        if record.get("type") == "evidence" and evidence_id not in referenced_evidence:
            report.add("SV-010", WARN_SEVERITY, evidence_id, "evidence is not bound to a claim")

    return report


def validate_registry_file(path: Path) -> SemanticReport:
    try:
        data = load_registry(path)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        report = SemanticReport(registry=str(path))
        report.add("SV-000", FAIL_SEVERITY, str(path), type(exc).__name__)
        return report
    return validate_registry(data, str(path.resolve()))
