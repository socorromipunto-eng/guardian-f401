from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[2]
VALIDATOR_PATH = ROOT / "tools" / "validate_project_state_v2.py"
SCHEMA_PATH = ROOT / "governance" / "schemas" / "project-state-v2.schema.json"

spec = importlib.util.spec_from_file_location("validate_project_state_v2", VALIDATOR_PATH)
assert spec and spec.loader
validator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(validator)


def _git(repo: Path, *args: str) -> str:
    cp = subprocess.run(
        ["git", "-C", str(repo), *args],
        capture_output=True,
        text=True,
        encoding="utf-8",
        shell=False,
        check=True,
    )
    return cp.stdout.strip()


def _write(path: Path, obj: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(obj, indent=2) + "\n", encoding="utf-8")


def _sha_bytes(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest().upper()


def _make_repo(tmp_path: Path) -> tuple[Path, str, str, Path]:
    repo = tmp_path / "repo"
    repo.mkdir()
    subprocess.run(["git", "init"], cwd=repo, check=True, capture_output=True)
    subprocess.run(["git", "config", "user.email", "test@example.com"], cwd=repo, check=True)
    subprocess.run(["git", "config", "user.name", "Guardian Test"], cwd=repo, check=True)

    reg_schema = {
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "$id": "https://example.test/schema/register/v1",
        "type": "object",
        "additionalProperties": False,
        "required": ["schema_version", "value"],
        "properties": {
            "schema_version": {"const": "1.0.0"},
            "value": {"type": "integer"},
        },
    }
    register = {"schema_version": "1.0.0", "value": 7}
    schema_rel = "governance/schemas/example-register.schema.json"
    reg_rel = "governance/example-register.json"
    _write(repo / schema_rel, reg_schema)
    _write(repo / reg_rel, register)
    subprocess.run(["git", "add", "."], cwd=repo, check=True)
    subprocess.run(["git", "commit", "-m", "payload"], cwd=repo, check=True, capture_output=True)

    payload_commit = _git(repo, "rev-parse", "HEAD")
    payload_tree = _git(repo, "rev-parse", f"{payload_commit}^{{tree}}")
    schema_raw = subprocess.run(
        ["git", "-C", str(repo), "show", f"{payload_commit}:{schema_rel}"],
        capture_output=True,
        check=True,
    ).stdout
    reg_raw = subprocess.run(
        ["git", "-C", str(repo), "show", f"{payload_commit}:{reg_rel}"],
        capture_output=True,
        check=True,
    ).stdout

    project_state = {
        "schema_version": "2.0.0",
        "id": "guardian:project-state:current",
        "binding_model": "PREDECESSOR_PAYLOAD_ATTESTATION",
        "source_commit": payload_commit,
        "source_tree": payload_tree,
        "registers": [
            {
                "path": reg_rel,
                "schema_id": reg_schema["$id"],
                "schema_version": "1.0.0",
                "schema_sha256": _sha_bytes(schema_raw),
                "register_sha256": _sha_bytes(reg_raw),
            }
        ],
    }
    project_state_path = repo / "candidate-project-state-v2.json"
    _write(project_state_path, project_state)
    return repo, payload_commit, payload_tree, project_state_path


def test_v2_schema_is_separate_and_versioned():
    data = json.loads(SCHEMA_PATH.read_text(encoding="utf-8"))
    assert data["$id"] == "https://guardian-f401.dev/schema/governance/project-state/v2"
    assert data["properties"]["schema_version"]["const"] == "2.0.0"
    assert data["properties"]["binding_model"]["const"] == "PREDECESSOR_PAYLOAD_ATTESTATION"


def test_v1_schema_is_not_rewritten():
    v1 = ROOT / "governance" / "schemas" / "project-state.schema.json"
    assert v1.is_file()
    data = json.loads(v1.read_text(encoding="utf-8"))
    assert data["properties"]["schema_version"]["const"] == "1.0.0"


def test_authority_is_never_granted():
    assert all(value == "NOT_GRANTED" for value in validator.AUTHORITY.values())


def test_duplicate_json_keys_fail_closed(tmp_path: Path):
    path = tmp_path / "dup.json"
    path.write_text('{"a":1,"a":2}\n', encoding="utf-8")
    with pytest.raises(validator.ValidationError):
        validator.load_strict_json(path)


def test_bom_fails_closed(tmp_path: Path):
    path = tmp_path / "bom.json"
    path.write_bytes(b"\xef\xbb\xbf{}")
    with pytest.raises(validator.ValidationError):
        validator.load_strict_json(path)


def test_unsafe_path_fails_closed():
    with pytest.raises(validator.ValidationError):
        validator._assert_repo_relative_json_path("../escape.json")


def test_schema_requires_separate_schema_and_register_hashes():
    data = json.loads(SCHEMA_PATH.read_text(encoding="utf-8"))
    required = set(data["$defs"]["register"]["required"])
    assert "schema_sha256" in required
    assert "register_sha256" in required


def test_schema_version_is_per_register_not_fixed_to_v1():
    data = json.loads(SCHEMA_PATH.read_text(encoding="utf-8"))
    reg = data["$defs"]["register"]["properties"]["schema_version"]
    assert "const" not in reg
    assert reg["pattern"] == r"^[0-9]+\.[0-9]+\.[0-9]+$"


def test_immutable_git_object_validation_passes(tmp_path: Path):
    repo, payload_commit, _, project_state_path = _make_repo(tmp_path)
    result = validator.validate(repo, project_state_path, SCHEMA_PATH)
    assert result["final"] == "PASS"
    assert result["source_commit"] == payload_commit
    assert result["validation_source"] == "IMMUTABLE_GIT_OBJECTS"


def test_worktree_mutation_does_not_change_commit_bound_validation(tmp_path: Path):
    repo, _, _, project_state_path = _make_repo(tmp_path)
    (repo / "governance/example-register.json").write_text(
        json.dumps({"schema_version": "1.0.0", "value": 999}) + "\n",
        encoding="utf-8",
    )
    result = validator.validate(repo, project_state_path, SCHEMA_PATH)
    assert result["final"] == "PASS"


def test_source_tree_mismatch_fails_closed(tmp_path: Path):
    repo, _, _, project_state_path = _make_repo(tmp_path)
    data = json.loads(project_state_path.read_text(encoding="utf-8"))
    data["source_tree"] = "0" * 40
    _write(project_state_path, data)
    with pytest.raises(validator.ValidationError, match="source_tree mismatch"):
        validator.validate(repo, project_state_path, SCHEMA_PATH)


def test_self_referential_materialization_binding_fails_closed(tmp_path: Path):
    repo, payload_commit, _, project_state_path = _make_repo(tmp_path)
    with pytest.raises(validator.ValidationError, match="self-referential"):
        validator.validate(
            repo,
            project_state_path,
            SCHEMA_PATH,
            materialization_commit=payload_commit,
        )


def test_payload_must_be_ancestor_of_materialization_commit(tmp_path: Path):
    repo, payload_commit, _, project_state_path = _make_repo(tmp_path)
    (repo / "marker.txt").write_text("materialization\n", encoding="utf-8")
    subprocess.run(["git", "add", "marker.txt"], cwd=repo, check=True)
    subprocess.run(["git", "commit", "-m", "materialization"], cwd=repo, check=True, capture_output=True)
    materialization_commit = _git(repo, "rev-parse", "HEAD")
    result = validator.validate(
        repo,
        project_state_path,
        SCHEMA_PATH,
        materialization_commit=materialization_commit,
    )
    assert result["final"] == "PASS"
    assert result["source_commit"] == payload_commit
    assert result["materialization_commit"] == materialization_commit


def test_project_state_instance_migration_not_materialized_by_stage_a():
    current = json.loads((ROOT / "governance" / "project-state.json").read_text(encoding="utf-8"))
    assert current["schema_version"] == "1.0.0"
