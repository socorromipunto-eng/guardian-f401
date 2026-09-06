#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path

H40 = re.compile(r"^[0-9a-f]{40}$")
H64 = re.compile(r"^[0-9a-fA-F]{64}$")

CLASSES = {
    "ADR", "ARCHITECTURE", "SPECIFICATION", "GOVERNANCE",
    "EVIDENCE", "OPERATIONAL", "INFORMATIONAL",
}
LIFECYCLE = {
    "DRAFT", "CANDIDATE", "APPROVED",
    "SUPERSEDED", "DEPRECATED", "WITHDRAWN",
}
AUTHORITY = {"NONE", "AUTHORITATIVE"}
SCOPE_DIMS = (
    "component",
    "chip_family",
    "hardware_revision",
    "firmware_version",
    "protocol_version",
    "semantic_profile",
    "system_role",
)
NON_NORMATIVE_CLASSES = {"EVIDENCE", "INFORMATIONAL"}

SELF_LIFECYCLE_ALIASES = {
    "DRAFT": "DRAFT",
    "CANDIDATE": "CANDIDATE",
    "APPROVED": "APPROVED",
    "APPROVED - HUMAN ADJUDICATED": "APPROVED",
    "ACCEPTED": "APPROVED",
    "SUPERSEDED": "SUPERSEDED",
    "DEPRECATED": "DEPRECATED",
    "WITHDRAWN": "WITHDRAWN",
}

def normalize_self_lifecycle(value: str) -> str | None:
    normalized = re.sub(r"\s+", " ", value.strip()).upper()
    return SELF_LIFECYCLE_ALIASES.get(normalized)

def document_self_lifecycle(blob: bytes) -> tuple[str | None, str | None]:
    try:
        text = blob.decode("utf-8")
    except UnicodeDecodeError:
        return None, "SELF_STATUS_UTF8_INVALID"

    lines = text.splitlines()
    recognized = []

    for index, line in enumerate(lines[:80]):
        inline = re.fullmatch(
            r"\s*(?:\*\*)?Status(?:\*\*)?\s*:\s*(.+?)\s*",
            line,
            flags=re.IGNORECASE,
        )
        if inline:
            value = normalize_self_lifecycle(inline.group(1))
            if value is not None:
                recognized.append(value)
            continue

        section = re.fullmatch(
            r"\s*#{1,6}\s+Status\s*",
            line,
            flags=re.IGNORECASE,
        )
        if section:
            for candidate in lines[index + 1 : min(index + 8, len(lines))]:
                stripped = candidate.strip()
                if not stripped:
                    continue
                if stripped.startswith("#"):
                    break
                value = normalize_self_lifecycle(stripped)
                if value is not None:
                    recognized.append(value)
                break

    distinct = sorted(set(recognized))
    if len(distinct) > 1:
        return None, "SELF_STATUS_CONFLICT:" + "|".join(distinct)
    if len(distinct) == 1:
        return distinct[0], None
    return None, None

def digest_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()

def digest_file(path: Path) -> str:
    return digest_bytes(path.read_bytes())

def bad(reason: str) -> int:
    print("DOCUMENT_REGISTER_VALIDATION=FAIL")
    print("REASON=" + reason)
    return 2

def git_blob(repo: Path, commit: str, path: str) -> bytes | None:
    cp = subprocess.run(
        ["git", "-C", str(repo), "show", f"{commit}:{path}"],
        capture_output=True,
        shell=False,
    )
    if cp.returncode != 0:
        return None
    return cp.stdout

def valid_selector(value) -> bool:
    if not isinstance(value, list) or not value:
        return False
    if len(set(value)) != len(value):
        return False
    if "*" in value and len(value) != 1:
        return False
    return all(
        isinstance(x, str)
        and x
        and re.fullmatch(r"[A-Za-z0-9._:+*-]+", x) is not None
        for x in value
    )

def selectors_overlap(a: list[str] | None, b: list[str] | None) -> bool:
    if a is None or b is None:
        return True
    if a == ["*"] or b == ["*"]:
        return True
    return bool(set(a) & set(b))

def scopes_overlap(a: dict, b: dict) -> bool:
    return all(selectors_overlap(a.get(dim), b.get(dim)) for dim in SCOPE_DIMS)

def validate_v1(repo: Path, data: dict) -> int:
    if set(data.keys()) != {"schema_version", "documents"}:
        return bad("V1_TOP_LEVEL_FIELDS_INVALID")
    if not isinstance(data["documents"], list):
        return bad("DOCUMENTS_NOT_LIST")

    paths = set()
    ids = set()
    for entry in data["documents"]:
        if not isinstance(entry, dict) or "sha256" not in entry or "path" not in entry:
            return bad("MALFORMED_DOCUMENT_ENTRY")
        path = str(entry["path"])
        doc_id = entry.get("id")
        if path in paths:
            return bad("DUPLICATE_DOCUMENT_PATH:" + path)
        paths.add(path)
        if doc_id is not None:
            if doc_id in ids:
                return bad("DUPLICATE_DOCUMENT_ID:" + str(doc_id))
            ids.add(doc_id)
        if not H64.fullmatch(str(entry["sha256"])):
            return bad("INVALID_SHA256:" + path)
        file_path = repo / path
        if not file_path.is_file():
            return bad("DOCUMENT_MISSING:" + path)
        if digest_file(file_path) != str(entry["sha256"]).upper():
            return bad("DOCUMENT_HASH_MISMATCH:" + path)

    print("DOCUMENT_REGISTER_SCHEMA=1.0.0")
    return 0

def validate_scope(scope, key: str) -> str | None:
    if not isinstance(scope, dict):
        return "SCOPE_NOT_OBJECT:" + key
    if "component" not in scope:
        return "SCOPE_COMPONENT_REQUIRED:" + key
    unknown = set(scope) - set(SCOPE_DIMS)
    if unknown:
        return "UNKNOWN_SCOPE_DIMENSION:" + key + ":" + ",".join(sorted(unknown))
    for dim, value in scope.items():
        if not valid_selector(value):
            return "INVALID_SCOPE_SELECTOR:" + key + ":" + dim
    return None

def validate_v2(repo: Path, data: dict) -> int:
    if set(data.keys()) != {"schema_version", "controlled_scope", "documents"}:
        return bad("V2_TOP_LEVEL_FIELDS_INVALID")

    controlled = data["controlled_scope"]
    if not isinstance(controlled, dict):
        return bad("CONTROLLED_SCOPE_NOT_OBJECT")
    if set(controlled.keys()) != {"id", "description", "completeness_status"}:
        return bad("CONTROLLED_SCOPE_FIELDS_INVALID")
    if not isinstance(controlled["id"], str) or not controlled["id"]:
        return bad("CONTROLLED_SCOPE_ID_INVALID")
    if not isinstance(controlled["description"], str) or not controlled["description"]:
        return bad("CONTROLLED_SCOPE_DESCRIPTION_INVALID")
    if controlled["completeness_status"] not in {"NOT_DEMONSTRATED", "CONFIRMED"}:
        return bad("CONTROLLED_SCOPE_COMPLETENESS_INVALID")

    docs = data["documents"]
    if not isinstance(docs, list):
        return bad("DOCUMENTS_NOT_LIST")

    required = {
        "id", "document_class", "version", "path", "sha256", "source_commit",
        "lifecycle_status", "authority_status", "authority_domain",
        "scope", "supersedes",
    }
    optional = {"effective_from", "notes"}

    by_key = {}
    paths = set()

    for entry in docs:
        if not isinstance(entry, dict):
            return bad("DOCUMENT_ENTRY_NOT_OBJECT")
        keys = set(entry)
        if not required.issubset(keys):
            return bad("MISSING_V2_DOCUMENT_FIELD")
        if keys - required - optional:
            return bad("UNKNOWN_V2_DOCUMENT_FIELD:" + ",".join(sorted(keys - required - optional)))

        doc_id = entry["id"]
        version = entry["version"]
        key = f"{doc_id}@{version}"

        if not isinstance(doc_id, str) or not doc_id:
            return bad("DOCUMENT_ID_INVALID")
        if not isinstance(version, str) or not version:
            return bad("DOCUMENT_VERSION_INVALID")
        if key in by_key:
            return bad("DUPLICATE_DOCUMENT_ID_VERSION:" + key)
        by_key[key] = entry

        path = entry["path"]
        if not isinstance(path, str) or not path:
            return bad("DOCUMENT_PATH_INVALID:" + key)
        if path in paths:
            return bad("DUPLICATE_DOCUMENT_PATH:" + path)
        paths.add(path)

        if entry["document_class"] not in CLASSES:
            return bad("UNKNOWN_DOCUMENT_CLASS:" + key)
        if entry["lifecycle_status"] not in LIFECYCLE:
            return bad("UNKNOWN_LIFECYCLE_STATUS:" + key)
        if entry["authority_status"] not in AUTHORITY:
            return bad("UNKNOWN_AUTHORITY_STATUS:" + key)
        if not isinstance(entry["authority_domain"], str) or not entry["authority_domain"]:
            return bad("AUTHORITY_DOMAIN_INVALID:" + key)

        if entry["document_class"] in NON_NORMATIVE_CLASSES and entry["authority_status"] != "NONE":
            return bad("NON_NORMATIVE_CLASS_CANNOT_BE_AUTHORITATIVE:" + key)

        if entry["lifecycle_status"] in {"DRAFT", "CANDIDATE"} and entry["authority_status"] != "NONE":
            return bad("UNAPPROVED_DOCUMENT_CANNOT_BE_AUTHORITATIVE:" + key)

        if not H64.fullmatch(str(entry["sha256"])):
            return bad("INVALID_SHA256:" + key)

        commit = entry["source_commit"]
        if not isinstance(commit, str) or not H40.fullmatch(commit):
            return bad("INVALID_SOURCE_COMMIT:" + key)

        scope_error = validate_scope(entry["scope"], key)
        if scope_error:
            return bad(scope_error)

        supersedes = entry["supersedes"]
        if not isinstance(supersedes, list):
            return bad("SUPERSEDES_NOT_LIST:" + key)

        refs = []
        for ref in supersedes:
            if not isinstance(ref, dict) or set(ref.keys()) != {"id", "version"}:
                return bad("MALFORMED_SUPERSEDES_REF:" + key)
            if not isinstance(ref["id"], str) or not ref["id"]:
                return bad("SUPERSEDES_ID_INVALID:" + key)
            if not isinstance(ref["version"], str) or not ref["version"]:
                return bad("SUPERSEDES_VERSION_INVALID:" + key)
            ref_key = f'{ref["id"]}@{ref["version"]}'
            if ref_key == key:
                return bad("SUPERSESSION_SELF_LOOP:" + key)
            refs.append(ref_key)
        if len(refs) != len(set(refs)):
            return bad("DUPLICATE_SUPERSEDES_REF:" + key)

        blob = git_blob(repo, commit, path)
        if blob is None:
            return bad("SOURCE_COMMIT_PATH_MISSING:" + key)
        registered_digest = str(entry["sha256"]).upper()
        if digest_bytes(blob) != registered_digest:
            return bad("SOURCE_COMMIT_HASH_MISMATCH:" + key)

        # Canonical current-repository coherence is evaluated against the Git
        # blob at HEAD, not raw checkout bytes. This avoids CRLF/LF checkout
        # transformations changing document identity across platforms.
        head_blob = git_blob(repo, "HEAD", path)
        if head_blob is None:
            return bad("CURRENT_HEAD_PATH_MISSING:" + key)
        if digest_bytes(head_blob) != registered_digest:
            return bad("CURRENT_HEAD_BLOB_HASH_MISMATCH:" + key)

        self_lifecycle, self_status_error = document_self_lifecycle(blob)
        if self_status_error is not None:
            return bad(self_status_error + ":" + key)
        if (
            self_lifecycle is not None
            and self_lifecycle != entry["lifecycle_status"]
        ):
            return bad(
                "STATUS_METADATA_DRIFT:"
                + key
                + ":"
                + self_lifecycle
                + "!="
                + entry["lifecycle_status"]
            )

        # Dirty tracked content is a separate repository-state condition.
        dirty = subprocess.run(
            ["git", "-C", str(repo), "diff", "--quiet", "HEAD", "--", path],
            capture_output=True,
            shell=False,
        )
        if dirty.returncode not in (0, 1):
            return bad("CURRENT_WORKTREE_STATE_UNRESOLVABLE:" + key)
        if dirty.returncode == 1:
            return bad("CURRENT_WORKTREE_DOCUMENT_DIRTY:" + key)

    graph = {}
    for key, entry in by_key.items():
        refs = [f'{r["id"]}@{r["version"]}' for r in entry["supersedes"]]
        for ref in refs:
            if ref not in by_key:
                return bad("SUPERSEDES_UNKNOWN_PREDECESSOR:" + key + "->" + ref)
        graph[key] = refs

    visiting = set()
    visited = set()

    def visit(node: str) -> bool:
        if node in visiting:
            return False
        if node in visited:
            return True
        visiting.add(node)
        for nxt in graph.get(node, []):
            if not visit(nxt):
                return False
        visiting.remove(node)
        visited.add(node)
        return True

    for node in graph:
        if not visit(node):
            return bad("SUPERSESSION_CYCLE:" + node)

    current = [
        (key, entry)
        for key, entry in by_key.items()
        if entry["lifecycle_status"] == "APPROVED"
        and entry["authority_status"] == "AUTHORITATIVE"
    ]
    for i in range(len(current)):
        key_a, a = current[i]
        for j in range(i + 1, len(current)):
            key_b, b = current[j]
            if a["authority_domain"] != b["authority_domain"]:
                continue
            if scopes_overlap(a["scope"], b["scope"]):
                return bad("AMBIGUOUS_CURRENT_AUTHORITY:" + key_a + "|" + key_b)

    print("DOCUMENT_REGISTER_SCHEMA=2.0.0")
    print("DOCUMENT_REGISTER_V2_STRUCTURE=PASS")
    print("DOCUMENT_REGISTER_V2_SOURCE_COMMIT_BINDING=PASS")
    print("DOCUMENT_REGISTER_V2_SUPERSESSION_GRAPH=PASS")
    print("DOCUMENT_REGISTER_V2_AUTHORITY_RESOLUTION=PASS")
    print("CONTROLLED_DOCUMENT_COMPLETENESS=" + controlled["completeness_status"])
    return 0

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", default=".")
    args = parser.parse_args()
    repo = Path(args.repo).resolve()

    try:
        data = json.loads(
            (repo / "governance/document-register.json").read_text(encoding="utf-8")
        )
    except Exception as exc:
        return bad("PARSE:" + str(exc))

    version = data.get("schema_version")
    if version == "1.0.0":
        rc = validate_v1(repo, data)
    elif version == "2.0.0":
        rc = validate_v2(repo, data)
    else:
        return bad("UNSUPPORTED_SCHEMA_VERSION:" + str(version))

    if rc != 0:
        return rc

    print("DOCUMENT_REGISTER_VALIDATION=PASS")
    print("REGISTERED_DOCUMENT_INTEGRITY=PASS")
    print("REGISTERED_DOES_NOT_IMPLY_APPROVED=PASS")
    print("REGISTERED_DOES_NOT_IMPLY_IMPLEMENTED=PASS")
    print("REGISTERED_DOES_NOT_IMPLY_VALIDATED=PASS")
    print("REGISTERED_DOES_NOT_IMPLY_EVIDENCE_PRESENT=PASS")
    if version == "1.0.0":
        print("CONTROLLED_DOCUMENT_COMPLETENESS=NOT_DEMONSTRATED")
    return 0

if __name__ == "__main__":
    sys.exit(main())
