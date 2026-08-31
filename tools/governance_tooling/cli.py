"""Read-only command-line interface for Guardian F401 governance tooling."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys

from .evidence import DEFAULT_NOT_PROVEN
from .errors import GovernanceToolError
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
    subparsers.add_parser("inspect", help="Inspect repository identity and cleanliness")
    subparsers.add_parser("baseline", help="Compare local HEAD with origin/main")
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
    result.add("BRANCH_PRESENT", bool(branch), branch or "DETACHED_HEAD")
    result.add("STAGED_CLEAN", len(staged) == 0, f"count={len(staged)}")
    result.add("TRACKED_CLEAN", len(tracked) == 0, f"count={len(tracked)}")
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


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        repo = GitRepository(Path(args.repo))
        result = run_inspect(repo) if args.command == "inspect" else run_baseline(repo)
    except GovernanceToolError as exc:
        print(str(exc), file=sys.stderr)
        return 2

    print(result.to_json() if args.format == "json" else result.to_text())
    return 0 if result.final == "PASS" else 1
