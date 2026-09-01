"""Phase-aware end-to-end integration gate for the governance tooling foundation."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from .candidate import run_candidate
from .evidence import DEFAULT_NOT_PROVEN
from .evidence_manifest import (
    build_evidence_manifest,
    canonical_document_bytes,
    payload_sha256,
    verify_evidence_manifest,
    write_manifest,
)
from .gates import run_precommit, run_prepr
from .repository import GitRepository
from .result import Result

FOUNDATION_SCHEMA = "guardian.governance.foundation.v1"
PHASES = ("candidate", "precommit", "prepr")


@dataclass(frozen=True, slots=True)
class PhaseOutcome:
    name: str
    final: str


def _authority_denied(result: Result) -> bool:
    return all(result.authority.get(k) == "NOT_GRANTED" for k in ("COMMIT", "PUSH", "PR", "MERGE"))


def _inherit_nonclaims(result: Result) -> None:
    result.not_proven.extend(DEFAULT_NOT_PROVEN)
    result.not_proven.extend(
        [
            "physical hardware validation completeness",
            "product certification or production readiness",
            "external-framework compliance",
            "human authorization for repository mutation",
        ]
    )


def run_foundation_phase(
    repo: GitRepository,
    phase: str,
    allow: tuple[str, ...] = (),
    forbid: tuple[str, ...] = (),
    evidence_output: Path | None = None,
) -> tuple[Result, dict]:
    if phase not in PHASES:
        raise ValueError("unsupported foundation phase: " + phase)

    result = Result(command="foundation:" + phase, repository=str(repo.root))
    result.source_of_truth = repo.head()
    _inherit_nonclaims(result)
    outcomes: list[PhaseOutcome] = []
    evidence_hash: str | None = None

    # Lifecycle phases are state-specific and must not be re-adjudicated
    # against a later Git state.
    if phase == "candidate":
        candidate = run_candidate(repo, allow, forbid)
        outcomes.append(PhaseOutcome("candidate", candidate.final))
        result.add("CANDIDATE_GATE", candidate.final == "PASS")

        if evidence_output is None:
            result.add("EVIDENCE_OUTPUT_PRESENT", False)
        else:
            candidate_result, envelope = build_evidence_manifest(repo, allow, forbid)
            write_manifest(evidence_output, envelope)
            verified = verify_evidence_manifest(repo, evidence_output)
            evidence_hash = envelope["manifest_sha256"]
            outcomes.append(PhaseOutcome("evidence", candidate_result.final))
            outcomes.append(PhaseOutcome("verify-evidence", verified.final))
            result.add("EVIDENCE_GENERATION", candidate_result.final == "PASS")
            result.add("EVIDENCE_VERIFICATION", verified.final == "PASS")

    if phase == "precommit":
        result.add(
            "PHASE_ARGUMENTS",
            evidence_output is None and not allow and not forbid,
            "candidate inputs belong to candidate phase",
        )
        gate = run_precommit(repo)
        outcomes.append(PhaseOutcome("precommit", gate.final))
        result.add("PRECOMMIT_GATE", gate.final == "PASS")

    if phase == "prepr":
        gate = run_prepr(repo)
        outcomes.append(PhaseOutcome("prepr", gate.final))
        result.add("PREPR_GATE", gate.final == "PASS")

    result.add("AUTHORITY_BOUNDARY", _authority_denied(result))
    result.proven.extend(
        [
            "foundation phase is " + phase,
            "current branch is " + (repo.branch() or "DETACHED"),
            "HEAD is " + repo.head(),
        ]
    )
    if evidence_hash:
        result.proven.append("evidence manifest sha256 is " + evidence_hash)

    payload = {
        "schema": FOUNDATION_SCHEMA,
        "phase": phase,
        "repository": str(repo.root),
        "branch": repo.branch(),
        "head": repo.head(),
        "origin_main": repo.origin_main(),
        "outcomes": [{"name": item.name, "final": item.final} for item in outcomes],
        "evidence_manifest_sha256": evidence_hash,
        "authority": dict(result.authority),
        "final": result.final,
    }
    record = {
        "foundation_record_sha256": payload_sha256(payload),
        "payload": payload,
    }
    return result, record


def write_foundation_record(path: Path, record: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(canonical_document_bytes(record))
