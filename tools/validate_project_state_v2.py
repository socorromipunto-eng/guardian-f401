#!/usr/bin/env python3
"""Read-only validator for Guardian project-state v2 records.

Final validation is commit-bound. Register and schema bytes are read from the
immutable Git objects addressed by project_state.source_commit, never silently
substituted from the current worktree.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
from typing import Any

try:
    import jsonschema
except ImportError as exc:
    raise SystemExit(
        "jsonschema is required; install governance/requirements-governance.in"
    ) from exc

SCHEMA_VERSION = "2.0.0"
SCHEMA_ID = "https://guardian-f401.dev/schema/governance/project-state/v2"
BINDING_MODEL = "PREDECESSOR_PAYLOAD_ATTESTATION"
HEX40 = re.compile(r"^[0-9a-f]{40}$")
HEX64_UPPER = re.compile(r"^[0-9A-F]{64}$")

AUTHORITY = {
    "COMMIT": "NOT_GRANTED",
    "PUSH": "NOT_GRANTED",
    "PR": "NOT_GRANTED",
    "MERGE": "NOT_GRANTED",
    "ACTUATION": "NOT_GRANTED",
    "NODE_SUPERVISOR_ARCHITECTURE": "NOT_GRANTED",
    "NODE_SUPERVISOR_IMPLEMENTATION": "NOT_GRANTED",
}


class ValidationError(RuntimeError):
    pass


def _strict_object(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    out: dict[str, Any] = {}
    for key, value in pairs:
        if key in out:
            raise ValidationError(f"duplicate JSON object key: {key}")
        out[key] = value
    return out


def parse_strict_json_bytes(raw: bytes, subject: str) -> Any:
    if raw.startswith(b"\xef\xbb\xbf"):
        raise ValidationError(f"BOM prohibited: {subject}")
    if b"\x00" in raw:
        raise ValidationError(f"NUL byte prohibited: {subject}")
    try:
        text = raw.decode("utf-8", errors="strict")
    except UnicodeDecodeError as exc:
        raise ValidationError(f"invalid UTF-8: {subject}") from exc
    try:
        return json.loads(
            text,
            object_pairs_hook=_strict_object,
            parse_constant=lambda value: (_ for _ in ()).throw(
                ValidationError(f"non-standard JSON number: {value}")
            ),
        )
    except json.JSONDecodeError as exc:
        raise ValidationError(f"malformed JSON: {subject}: {exc}") from exc


def load_strict_json(path: Path) -> Any:
    return parse_strict_json_bytes(path.read_bytes(), str(path))


def sha256_bytes(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest().upper()


def _run_git(repo: Path, *args: str, allow_failure: bool = False) -> bytes:
    cp = subprocess.run(
        ["git", "-C", str(repo), *args],
        capture_output=True,
        text=False,
        shell=False,
        check=False,
    )
    if cp.returncode != 0 and not allow_failure:
        stderr = cp.stderr.decode("utf-8", errors="replace").strip()
        raise ValidationError(f"git {' '.join(args)} failed: {stderr}")
    return cp.stdout


def git_text(repo: Path, *args: str) -> str:
    return _run_git(repo, *args).decode("utf-8", errors="strict").strip()


def git_blob(repo: Path, commit: str, path: str) -> bytes:
    return _run_git(repo, "show", f"{commit}:{path}")


def git_tree(repo: Path, commit: str) -> str:
    return git_text(repo, "rev-parse", f"{commit}^{{tree}}")


def git_commit_exists(repo: Path, commit: str) -> None:
    cp = subprocess.run(
        ["git", "-C", str(repo), "cat-file", "-e", f"{commit}^{{commit}}"],
        capture_output=True,
        text=False,
        shell=False,
        check=False,
    )
    if cp.returncode != 0:
        raise ValidationError(f"commit does not exist: {commit}")


def git_is_ancestor(repo: Path, ancestor: str, descendant: str) -> bool:
    cp = subprocess.run(
        ["git", "-C", str(repo), "merge-base", "--is-ancestor", ancestor, descendant],
        capture_output=True,
        text=False,
        shell=False,
        check=False,
    )
    if cp.returncode == 0:
        return True
    if cp.returncode == 1:
        return False
    raise ValidationError("git merge-base --is-ancestor failed")


def list_schema_paths(repo: Path, commit: str) -> list[str]:
    raw = _run_git(
        repo,
        "ls-tree",
        "-r",
        "--name-only",
        commit,
        "governance/schemas",
    )
    paths = raw.decode("utf-8", errors="strict").splitlines()
    return sorted(
        p for p in paths
        if p.startswith("governance/schemas/") and p.endswith(".json")
    )


def _assert_repo_relative_json_path(value: str) -> None:
    if "\\" in value or value.startswith("/") or re.match(r"^[A-Za-z]:", value):
        raise ValidationError(f"unsafe register path: {value}")
    parts = value.split("/")
    if any(part in {"", ".", ".."} for part in parts):
        raise ValidationError(f"unsafe register path: {value}")
    if not value.startswith("governance/") or not value.endswith(".json"):
        raise ValidationError(f"register path outside governance JSON scope: {value}")


def _schema_declared_version(schema: Any) -> str | None:
    if not isinstance(schema, dict):
        return None
    props = schema.get("properties")
    if not isinstance(props, dict):
        return None
    sv = props.get("schema_version")
    if not isinstance(sv, dict):
        return None
    value = sv.get("const")
    return value if isinstance(value, str) else None


def _resolve_schema(
    repo: Path,
    source_commit: str,
    schema_id: str,
    schema_version: str,
) -> tuple[str, bytes, dict[str, Any]]:
    matches: list[tuple[str, bytes, dict[str, Any]]] = []
    for path in list_schema_paths(repo, source_commit):
        raw = git_blob(repo, source_commit, path)
        try:
            parsed = parse_strict_json_bytes(raw, f"{source_commit}:{path}")
        except ValidationError:
            continue
        if not isinstance(parsed, dict):
            continue
        if parsed.get("$id") != schema_id:
            continue
        if _schema_declared_version(parsed) != schema_version:
            continue
        matches.append((path, raw, parsed))
    if len(matches) != 1:
        raise ValidationError(
            "expected exactly one schema for "
            f"{schema_id}@{schema_version}; found {len(matches)}"
        )
    return matches[0]


def validate(
    repo: Path,
    project_state: Path,
    schema_path: Path,
    materialization_commit: str | None = None,
) -> dict[str, Any]:
    repo = repo.resolve()
    schema = load_strict_json(schema_path)
    instance = load_strict_json(project_state)

    if schema.get("$id") != SCHEMA_ID:
        raise ValidationError("project-state v2 schema id mismatch")

    jsonschema.Draft202012Validator.check_schema(schema)
    errors = sorted(
        jsonschema.Draft202012Validator(schema).iter_errors(instance),
        key=lambda e: list(e.absolute_path),
    )
    if errors:
        first = errors[0]
        location = "/".join(str(p) for p in first.absolute_path) or "<root>"
        raise ValidationError(f"schema validation failed at {location}: {first.message}")

    if instance.get("schema_version") != SCHEMA_VERSION:
        raise ValidationError("unsupported project-state schema_version")
    if instance.get("binding_model") != BINDING_MODEL:
        raise ValidationError("unsupported project-state binding_model")

    source_commit = instance["source_commit"]
    source_tree = instance["source_tree"]
    if not HEX40.fullmatch(source_commit):
        raise ValidationError("source_commit malformed")
    if not HEX40.fullmatch(source_tree):
        raise ValidationError("source_tree malformed")

    git_commit_exists(repo, source_commit)
    actual_tree = git_tree(repo, source_commit)
    if actual_tree != source_tree:
        raise ValidationError(
            f"source_tree mismatch: declared {source_tree}, actual {actual_tree}"
        )

    if materialization_commit is not None:
        if not HEX40.fullmatch(materialization_commit):
            raise ValidationError("materialization_commit malformed")
        git_commit_exists(repo, materialization_commit)
        if materialization_commit == source_commit:
            raise ValidationError("self-referential commit binding prohibited")
        if not git_is_ancestor(repo, source_commit, materialization_commit):
            raise ValidationError(
                "source_commit must be an ancestor of materialization_commit"
            )

    seen_paths: set[str] = set()

    for item in instance["registers"]:
        path = item["path"]
        _assert_repo_relative_json_path(path)
        if path in seen_paths:
            raise ValidationError(f"duplicate register path: {path}")
        seen_paths.add(path)

        if not HEX64_UPPER.fullmatch(item["schema_sha256"]):
            raise ValidationError(f"schema_sha256 malformed for {path}")
        if not HEX64_UPPER.fullmatch(item["register_sha256"]):
            raise ValidationError(f"register_sha256 malformed for {path}")

        register_raw = git_blob(repo, source_commit, path)
        if sha256_bytes(register_raw) != item["register_sha256"]:
            raise ValidationError(f"register hash mismatch: {path}")

        register_data = parse_strict_json_bytes(
            register_raw, f"{source_commit}:{path}"
        )
        if (
            not isinstance(register_data, dict)
            or register_data.get("schema_version") != item["schema_version"]
        ):
            raise ValidationError(
                f"register declared schema_version mismatch: {path}"
            )

        schema_file, schema_raw, target_schema = _resolve_schema(
            repo,
            source_commit,
            item["schema_id"],
            item["schema_version"],
        )
        if sha256_bytes(schema_raw) != item["schema_sha256"]:
            raise ValidationError(f"schema hash mismatch: {schema_file}")

        jsonschema.Draft202012Validator.check_schema(target_schema)
        target_errors = list(
            jsonschema.Draft202012Validator(target_schema).iter_errors(register_data)
        )
        if target_errors:
            raise ValidationError(
                f"register does not validate against bound schema: {path}"
            )

    return {
        "schema": "guardian.project-state.validation.v2",
        "binding_model": BINDING_MODEL,
        "project_state": str(project_state),
        "schema_path": str(schema_path),
        "source_commit": source_commit,
        "source_tree": source_tree,
        "materialization_commit": materialization_commit,
        "register_count": len(instance["registers"]),
        "validation_source": "IMMUTABLE_GIT_OBJECTS",
        "authority": dict(AUTHORITY),
        "nonclaims": [
            "schema validity is not semantic truth",
            "semantic validity is not governance approval",
            "validation does not grant repository mutation authority",
            "validation does not grant NodeSupervisor authority",
            "validation does not grant physical actuation authority",
            "governance completeness is not demonstrated by this validator",
        ],
        "final": "PASS",
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", default=".")
    parser.add_argument("--project-state", required=True)
    parser.add_argument(
        "--schema",
        default="governance/schemas/project-state-v2.schema.json",
    )
    parser.add_argument(
        "--materialization-commit",
        default=None,
        help="Optional commit containing the materialized project-state instance",
    )
    args = parser.parse_args()

    repo = Path(args.repo).resolve()
    project_state = Path(args.project_state)
    if not project_state.is_absolute():
        project_state = (repo / project_state).resolve()
    schema_path = Path(args.schema)
    if not schema_path.is_absolute():
        schema_path = (repo / schema_path).resolve()

    materialization_commit = args.materialization_commit
    if materialization_commit == "HEAD":
        materialization_commit = git_text(repo, "rev-parse", "HEAD")

    try:
        result = validate(
            repo,
            project_state,
            schema_path,
            materialization_commit=materialization_commit,
        )
    except (OSError, ValidationError, jsonschema.SchemaError) as exc:
        result = {
            "schema": "guardian.project-state.validation.v2",
            "authority": dict(AUTHORITY),
            "final": "FAIL",
            "failure": str(exc),
        }
        print(json.dumps(result, indent=2, sort_keys=True))
        return 1

    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
