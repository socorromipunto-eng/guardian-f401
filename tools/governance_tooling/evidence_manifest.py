"""Canonical, tamper-evident evidence manifests for Guardian governance tooling."""

from __future__ import annotations

from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import uuid
from typing import Any

from .candidate import FORBIDDEN_PROVEN_CLAIMS, run_candidate
from .evidence import DEFAULT_NOT_PROVEN
from .repository import GitRepository
from .result import Result

MANIFEST_SCHEMA = "guardian.governance.evidence.v1"


def utc_timestamp() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def canonical_json_bytes(value: Any) -> bytes:
    return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode("utf-8")


def canonical_document_bytes(value: Any) -> bytes:
    return canonical_json_bytes(value) + b"\n"


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def payload_sha256(payload: dict[str, Any]) -> str:
    return sha256_bytes(canonical_json_bytes(payload))


def _artifact_records(repo: GitRepository, allow: tuple[str, ...]) -> list[dict[str, Any]]:
    staged = set(repo.staged())
    observed = staged | set(repo.tracked_changes()) | set(repo.untracked())
    records: list[dict[str, Any]] = []
    for relative_path in sorted(observed & set(allow)):
        path = repo.root / relative_path
        if not path.is_file():
            continue
        worktree = path.read_bytes()
        record: dict[str, Any] = {
            "path": relative_path,
            "worktree": {"sha256": sha256_bytes(worktree), "bytes": len(worktree)},
            "index": None,
        }
        if relative_path in staged:
            index = repo.index_blob(relative_path)
            record["index"] = {"sha256": sha256_bytes(index), "bytes": len(index)}
        records.append(record)
    return records


def build_evidence_manifest(
    repo: GitRepository,
    allow: tuple[str, ...],
    forbid: tuple[str, ...] = (),
    *,
    run_id: str | None = None,
    timestamp_utc: str | None = None,
) -> tuple[Result, dict[str, Any]]:
    result = run_candidate(repo, allow, forbid)
    payload: dict[str, Any] = {
        "schema": MANIFEST_SCHEMA,
        "run_id": run_id or uuid.uuid4().hex,
        "timestamp_utc": timestamp_utc or utc_timestamp(),
        "repository": str(repo.root),
        "branch": repo.branch(),
        "head": repo.head(),
        "origin_main": repo.origin_main(),
        "command": "candidate",
        "allow": sorted(set(allow)),
        "forbid": sorted(set(forbid)),
        "artifacts": _artifact_records(repo, allow),
        "result": result.to_dict(),
    }
    return result, {"manifest_sha256": payload_sha256(payload), "payload": payload}


def write_manifest(path: Path, envelope: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(canonical_document_bytes(envelope))


def _embedded_evidence_coherent(payload: dict[str, Any]) -> bool:
    embedded = payload.get("result")
    if not isinstance(embedded, dict):
        return False
    proven = set(embedded.get("proven", []))
    not_proven = set(embedded.get("not_proven", []))
    authority = embedded.get("authority", {})
    return (
        not bool(proven & not_proven)
        and not bool(proven & FORBIDDEN_PROVEN_CLAIMS)
        and all(authority.get(k) == "NOT_GRANTED" for k in ("COMMIT", "PUSH", "PR", "MERGE"))
    )


def verify_evidence_manifest(repo: GitRepository, manifest_path: Path) -> Result:
    result = Result(command="verify-evidence", repository=str(repo.root))
    result.source_of_truth = repo.head()
    result.not_proven.extend(DEFAULT_NOT_PROVEN)
    result.not_proven.extend(
        [
            "semantic correctness of evidence content",
            "product certification or external-framework compliance",
            "human authorization for repository mutation",
        ]
    )

    try:
        raw = manifest_path.read_bytes()
        envelope = json.loads(raw.decode("utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        result.add("MANIFEST_PARSE", False, type(exc).__name__)
        return result

    result.add("MANIFEST_PARSE", isinstance(envelope, dict))
    if not isinstance(envelope, dict):
        return result

    result.add("CANONICAL_SERIALIZATION", raw == canonical_document_bytes(envelope))
    payload = envelope.get("payload")
    stored_hash = envelope.get("manifest_sha256")
    if not isinstance(payload, dict) or not isinstance(stored_hash, str):
        result.add("MANIFEST_STRUCTURE", False)
        return result

    result.add("MANIFEST_STRUCTURE", True)
    result.add("SCHEMA", payload.get("schema") == MANIFEST_SCHEMA, str(payload.get("schema")))
    result.add("MANIFEST_HASH", payload_sha256(payload) == stored_hash)
    result.add("REPOSITORY_IDENTITY", payload.get("repository") == str(repo.root), str(payload.get("repository")))
    result.add("BRANCH_IDENTITY", payload.get("branch") == repo.branch(), str(payload.get("branch")))
    result.add("HEAD_IDENTITY", payload.get("head") == repo.head(), str(payload.get("head")))
    result.add("ORIGIN_MAIN_IDENTITY", payload.get("origin_main") == repo.origin_main(), str(payload.get("origin_main")))

    failures: list[str] = []
    artifacts = payload.get("artifacts")
    if not isinstance(artifacts, list):
        result.add("ARTIFACT_BINDING", False, "artifacts is not a list")
    else:
        for record in artifacts:
            if not isinstance(record, dict):
                failures.append("<invalid-record>")
                continue
            rel = record.get("path")
            wt = record.get("worktree")
            if not isinstance(rel, str) or not isinstance(wt, dict):
                failures.append(str(rel))
                continue
            path = repo.root / rel
            if not path.is_file():
                failures.append(rel)
                continue
            current = path.read_bytes()
            if wt.get("sha256") != sha256_bytes(current) or wt.get("bytes") != len(current):
                failures.append(rel)
                continue
            idx = record.get("index")
            if idx is not None:
                if not isinstance(idx, dict):
                    failures.append(rel)
                    continue
                try:
                    current_index = repo.index_blob(rel)
                except Exception:
                    failures.append(rel)
                    continue
                if idx.get("sha256") != sha256_bytes(current_index) or idx.get("bytes") != len(current_index):
                    failures.append(rel)
        result.add("ARTIFACT_BINDING", not failures, f"checked={len(artifacts)},failures={len(failures)}")

    result.add("EVIDENCE_BOUNDARY", _embedded_evidence_coherent(payload))
    result.add(
        "AUTHORITY_BOUNDARY",
        all(v == "NOT_GRANTED" for v in result.authority.values()),
        "verification grants no repository mutation authority",
    )
    result.proven.extend(
        [
            f"manifest path is {manifest_path.resolve()}",
            f"manifest sha256 is {stored_hash}",
            f"current HEAD is {repo.head()}",
        ]
    )
    return result
