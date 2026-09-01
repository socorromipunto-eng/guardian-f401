from __future__ import annotations

import argparse
import csv
import hashlib
import io
import subprocess
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class BlobEntry:
    path: str
    blob_sha: str
    size: int
    sha256: str
    ref_commit: str


def _git(repo: Path, *args: str, binary: bool = False):
    cp = subprocess.run(
        ["git", "-C", str(repo), *args],
        capture_output=True,
        text=not binary,
        shell=False,
        check=False,
    )
    if cp.returncode != 0:
        stderr = cp.stderr.decode(errors="replace") if binary else cp.stderr
        raise ValueError(f"git {' '.join(args)} failed: {stderr.strip()}")
    return cp.stdout


def resolve_commit(repo: Path, ref: str) -> str:
    return str(_git(repo, "rev-parse", f"{ref}^{{commit}}")).strip()


def tracked_blobs(repo: Path, ref: str) -> list[tuple[str, str]]:
    raw = _git(repo, "ls-tree", "-r", "-z", "--full-tree", ref, binary=True)
    rows: list[tuple[str, str]] = []
    for record in raw.split(b"\0"):
        if not record:
            continue
        meta, path_b = record.split(b"\t", 1)
        mode, obj_type, sha = meta.decode("ascii").split(" ", 2)
        if obj_type != "blob":
            continue
        path = path_b.decode("utf-8", errors="strict")
        rows.append((path, sha))
    rows.sort(key=lambda x: x[0])
    return rows


def blob_bytes(repo: Path, sha: str) -> bytes:
    return bytes(_git(repo, "cat-file", "blob", sha, binary=True))


def build_entries(repo: Path, ref: str) -> list[BlobEntry]:
    commit = resolve_commit(repo, ref)
    entries: list[BlobEntry] = []
    for path, blob_sha in tracked_blobs(repo, ref):
        data = blob_bytes(repo, blob_sha)
        entries.append(
            BlobEntry(
                path=path,
                blob_sha=blob_sha,
                size=len(data),
                sha256=hashlib.sha256(data).hexdigest().upper(),
                ref_commit=commit,
            )
        )
    return entries


def render_csv(entries: list[BlobEntry]) -> str:
    out = io.StringIO(newline="")
    writer = csv.writer(out, lineterminator="\n")
    writer.writerow(["RelativePath", "SHA256", "Bytes", "GitBlobSHA", "RefCommit"])
    for e in entries:
        writer.writerow([e.path, e.sha256, str(e.size), e.blob_sha, e.ref_commit])
    return out.getvalue()


def parse_manifest(path: Path) -> list[BlobEntry]:
    with path.open("r", encoding="utf-8", newline="") as fh:
        reader = csv.DictReader(fh)
        expected = ["RelativePath", "SHA256", "Bytes", "GitBlobSHA", "RefCommit"]
        if reader.fieldnames != expected:
            raise ValueError(f"unexpected manifest columns: {reader.fieldnames!r}")
        rows: list[BlobEntry] = []
        for row in reader:
            rows.append(
                BlobEntry(
                    path=row["RelativePath"],
                    sha256=row["SHA256"].upper(),
                    size=int(row["Bytes"]),
                    blob_sha=row["GitBlobSHA"],
                    ref_commit=row["RefCommit"],
                )
            )
    return rows


def verify(repo: Path, ref: str, manifest: Path) -> list[str]:
    errors: list[str] = []
    actual = build_entries(repo, ref)
    recorded = parse_manifest(manifest)

    actual_by_path = {e.path: e for e in actual}
    recorded_by_path = {e.path: e for e in recorded}

    if len(recorded_by_path) != len(recorded):
        errors.append("manifest contains duplicate RelativePath values")

    missing = sorted(set(actual_by_path) - set(recorded_by_path))
    unexpected = sorted(set(recorded_by_path) - set(actual_by_path))
    for p in missing:
        errors.append(f"missing manifest row: {p}")
    for p in unexpected:
        errors.append(f"unexpected manifest row: {p}")

    for path in sorted(set(actual_by_path) & set(recorded_by_path)):
        a = actual_by_path[path]
        r = recorded_by_path[path]
        if a.sha256 != r.sha256:
            errors.append(f"SHA256 mismatch: {path}")
        if a.size != r.size:
            errors.append(f"byte-count mismatch: {path}")
        if a.blob_sha != r.blob_sha:
            errors.append(f"Git blob mismatch: {path}")
        if a.ref_commit != r.ref_commit:
            errors.append(f"ref-commit mismatch: {path}")

    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description="Deterministic Guardian F401 Git-blob release manifest")
    parser.add_argument("--repo", default=".")
    sub = parser.add_subparsers(dest="command", required=True)

    gen = sub.add_parser("generate")
    gen.add_argument("--ref", required=True)
    gen.add_argument("--output", required=True)

    ver = sub.add_parser("verify")
    ver.add_argument("--ref", required=True)
    ver.add_argument("--manifest", required=True)

    args = parser.parse_args()
    repo = Path(args.repo).resolve()

    try:
        if args.command == "generate":
            entries = build_entries(repo, args.ref)
            output = Path(args.output)
            if not output.is_absolute():
                output = repo / output
            output.parent.mkdir(parents=True, exist_ok=True)
            rendered = render_csv(entries)
            output.write_text(rendered, encoding="utf-8", newline="\n")
            print("GUARDIAN_RELEASE_MANIFEST_V1")
            print(f"COMMAND=generate")
            print(f"REF={args.ref}")
            print(f"REF_COMMIT={resolve_commit(repo, args.ref)}")
            print(f"FILE_COUNT={len(entries)}")
            print(f"MANIFEST={output}")
            print(f"MANIFEST_SHA256={hashlib.sha256(rendered.encode('utf-8')).hexdigest().upper()}")
            print("BYTE_SOURCE=GIT_BLOB")
            print("FINAL=PASS")
            return 0

        manifest = Path(args.manifest)
        if not manifest.is_absolute():
            manifest = repo / manifest
        errors = verify(repo, args.ref, manifest)
        print("GUARDIAN_RELEASE_MANIFEST_V1")
        print("COMMAND=verify")
        print(f"REF={args.ref}")
        print(f"REF_COMMIT={resolve_commit(repo, args.ref)}")
        print(f"ERROR_COUNT={len(errors)}")
        for err in errors:
            print("ERROR=" + err)
        print("BYTE_SOURCE=GIT_BLOB")
        print("FINAL=" + ("PASS" if not errors else "FAIL"))
        return 0 if not errors else 1
    except (ValueError, OSError) as exc:
        print("GUARDIAN_RELEASE_MANIFEST_V1")
        print("FINAL=FAIL")
        print("ERROR=" + str(exc))
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
