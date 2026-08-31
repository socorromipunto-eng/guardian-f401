"""Read-only command-line interface for Guardian F401 governance tooling."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

from .candidate import run_candidate
from .evidence import DEFAULT_NOT_PROVEN
from .evidence_manifest import build_evidence_manifest, verify_evidence_manifest, write_manifest
from .errors import FailureClass, GovernanceToolError
from .gates import run_precommit, run_prepr, run_verify
from .repository import GitRepository
from .result import Result


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="guardian-governance",
        description="Read-only Guardian F401 repository governance verifier",
    )
    parser.add_argument("--repo", default=".", help="Repository root (default: current directory)")
    parser.add_argument("--format", choices=("text", "json"), default="text")
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("inspect", help="Observe repository identity and working-tree state")
    subparsers.add_parser("baseline", help="Observe relation between local HEAD and origin/main")
    subparsers.add_parser("verify", help="Gate repository identity and cleanliness")
    subparsers.add_parser("precommit", help="Gate technical readiness for human commit decision")
    subparsers.add_parser("prepr", help="Gate technical readiness for human push/PR decision")

    candidate = subparsers.add_parser("candidate", help="Gate exact candidate scope and identity")
    candidate.add_argument("--allow", action="append", required=True)
    candidate.add_argument("--forbid", action="append", default=[])

    evidence = subparsers.add_parser("evidence", help="Write canonical evidence outside repository")
    evidence.add_argument("--allow", action="append", required=True)
    evidence.add_argument("--forbid", action="append", default=[])
    evidence.add_argument("--output", required=True)
    evidence.add_argument("--run-id", default=None)

    verify_evidence = subparsers.add_parser("verify-evidence", help="Verify evidence manifest")
    verify_evidence.add_argument("--manifest", required=True)
    return parser


def run_inspect(repo: GitRepository) -> Result:
    result = Result(command="inspect", repository=str(repo.root))
    branch = repo.branch()
    head = repo.head()
    staged = repo.staged()
    tracked = repo.tracked_changes()
    untracked = repo.untracked()
    result.source_of_truth = head
    result.add("INSIDE_WORK_TREE", repo.is_inside_work_tree())
    result.add("BRANCH_OBSERVED", True, branch or "DETACHED_HEAD")
    result.add("HEAD_OBSERVED", len(head) == 40, head)
    result.add("STAGED_STATE_OBSERVED", True, f"count={len(staged)}")
    result.add("TRACKED_STATE_OBSERVED", True, f"count={len(tracked)}")
    result.add("UNTRACKED_STATE_OBSERVED", True, f"count={len(untracked)}")
    result.proven.extend(
        [
            f"current branch is {branch or 'DETACHED'}",
            f"HEAD is {head}",
            f"staged file count is {len(staged)}",
            f"tracked change count is {len(tracked)}",
            f"untracked file count is {len(untracked)}",
        ]
    )
    result.not_proven.extend(DEFAULT_NOT_PROVEN)
    return result


def run_baseline(repo: GitRepository) -> Result:
    result = Result(command="baseline", repository=str(repo.root))
    head = repo.head()
    origin_main = repo.origin_main()
    merge_base = repo.merge_base(head, origin_main)
    origin_only, head_only = repo.ahead_behind(origin_main, head)
    result.source_of_truth = origin_main
    result.add("INSIDE_WORK_TREE", repo.is_inside_work_tree())
    result.add("HEAD_AVAILABLE", len(head) == 40, head)
    result.add("ORIGIN_MAIN_AVAILABLE", len(origin_main) == 40, origin_main)
    result.add("MERGE_BASE_AVAILABLE", len(merge_base) == 40, merge_base)
    result.proven.extend(
        [
            f"HEAD is {head}",
            f"origin/main is {origin_main}",
            f"merge base is {merge_base}",
            f"origin/main-only commit count is {origin_only}",
            f"HEAD-only commit count is {head_only}",
        ]
    )
    result.not_proven.extend(DEFAULT_NOT_PROVEN)
    return result


def _outside_repository(repo: GitRepository, path: Path) -> bool:
    try:
        path.resolve().relative_to(repo.root)
        return False
    except ValueError:
        return True


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        repo = GitRepository(Path(args.repo))
        if args.command == "candidate":
            result = run_candidate(repo, tuple(args.allow), tuple(args.forbid))
        elif args.command == "evidence":
            output = Path(args.output).resolve()
            if not _outside_repository(repo, output):
                raise GovernanceToolError(
                    FailureClass.AUTHORITY,
                    "EVIDENCE_OUTPUT_INSIDE_REPOSITORY",
                    "evidence output must be outside the repository",
                )
            result, envelope = build_evidence_manifest(
                repo, tuple(args.allow), tuple(args.forbid), run_id=args.run_id
            )
            write_manifest(output, envelope)
            if args.format == "json":
                print(json.dumps({
                    "result": result.to_dict(),
                    "manifest_path": str(output),
                    "manifest_sha256": envelope["manifest_sha256"],
                }, indent=2, sort_keys=True))
            else:
                print(result.to_text())
                print(f"EVIDENCE_MANIFEST_PATH={output}")
                print(f"EVIDENCE_MANIFEST_SHA256={envelope['manifest_sha256']}")
            return 0 if result.final == "PASS" else 1
        elif args.command == "verify-evidence":
            result = verify_evidence_manifest(repo, Path(args.manifest).resolve())
        else:
            runners = {
                "inspect": run_inspect,
                "baseline": run_baseline,
                "verify": run_verify,
                "precommit": run_precommit,
                "prepr": run_prepr,
            }
            result = runners[args.command](repo)
    except GovernanceToolError as exc:
        print(str(exc), file=sys.stderr)
        return 2

    print(result.to_json() if args.format == "json" else result.to_text())
    return 0 if result.final == "PASS" else 1
