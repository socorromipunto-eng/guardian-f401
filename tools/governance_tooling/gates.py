"""Read-only repository gates for Guardian governance tooling.

These functions adjudicate observable repository state. They never grant human
authority and never mutate repository state.
"""

from __future__ import annotations

from .evidence import DEFAULT_NOT_PROVEN
from .repository import GitRepository
from .result import Result


def _common_nonclaims(result: Result) -> None:
    result.not_proven.extend(DEFAULT_NOT_PROVEN)


def run_verify(repo: GitRepository) -> Result:
    """Verify repository identity, cleanliness, and relation to origin/main."""
    result = Result(command="verify", repository=str(repo.root))
    branch = repo.branch()
    head = repo.head()
    staged = repo.staged()
    tracked = repo.tracked_changes()
    untracked = repo.untracked()
    origin_main = repo.origin_main()
    origin_only, head_only = repo.ahead_behind(origin_main, head)

    result.source_of_truth = origin_main
    result.add("INSIDE_WORK_TREE", repo.is_inside_work_tree())
    result.add("BRANCH_PRESENT", bool(branch), branch or "DETACHED_HEAD")
    result.add("HEAD_AVAILABLE", len(head) == 40, head)
    result.add("ORIGIN_MAIN_AVAILABLE", len(origin_main) == 40, origin_main)
    result.add("STAGED_CLEAN", len(staged) == 0, f"count={len(staged)}")
    result.add("TRACKED_CLEAN", len(tracked) == 0, f"count={len(tracked)}")
    result.add("UNTRACKED_CLEAN", len(untracked) == 0, f"count={len(untracked)}")
    result.add("NOT_BEHIND_MAIN", origin_only == 0, f"behind={origin_only}")
    result.proven.extend(
        [
            f"current branch is {branch or 'DETACHED'}",
            f"HEAD is {head}",
            f"origin/main is {origin_main}",
            f"HEAD-only commit count is {head_only}",
            f"origin/main-only commit count is {origin_only}",
        ]
    )
    _common_nonclaims(result)
    return result


def run_precommit(repo: GitRepository) -> Result:
    """Adjudicate technical readiness for a separate human commit decision."""
    result = Result(command="precommit", repository=str(repo.root))
    branch = repo.branch()
    head = repo.head()
    staged = repo.staged()
    tracked = repo.tracked_changes()
    untracked = repo.untracked()
    diff_check = repo.git("diff", "--cached", "--check", allow_failure=True)

    result.source_of_truth = head
    result.add("INSIDE_WORK_TREE", repo.is_inside_work_tree())
    result.add("FEATURE_BRANCH", bool(branch) and branch != "main", branch or "DETACHED_HEAD")
    result.add("STAGED_PRESENT", len(staged) > 0, f"count={len(staged)}")
    result.add("UNSTAGED_TRACKED_CLEAN", len(tracked) == 0, f"count={len(tracked)}")
    result.add("UNTRACKED_CLEAN", len(untracked) == 0, f"count={len(untracked)}")
    result.add(
        "CACHED_DIFF_HYGIENE",
        diff_check.returncode == 0,
        diff_check.stderr or "git diff --cached --check",
    )
    result.proven.extend(
        [
            f"current branch is {branch or 'DETACHED'}",
            f"HEAD is {head}",
            f"staged file count is {len(staged)}",
            f"unstaged tracked file count is {len(tracked)}",
            f"untracked file count is {len(untracked)}",
        ]
    )
    _common_nonclaims(result)
    result.not_proven.append("human authorization to create a commit")
    result.not_proven.append("staged scope is semantically approved")
    return result


def run_prepr(repo: GitRepository) -> Result:
    """Adjudicate technical readiness for a separate human push/PR decision."""
    result = Result(command="prepr", repository=str(repo.root))
    branch = repo.branch()
    head = repo.head()
    staged = repo.staged()
    tracked = repo.tracked_changes()
    untracked = repo.untracked()
    origin_main = repo.origin_main()
    origin_only, head_only = repo.ahead_behind(origin_main, head)
    changed = [
        line
        for line in repo.git("diff", "--name-only", f"{origin_main}..{head}").stdout.splitlines()
        if line
    ]
    commit_count_text = repo.git("rev-list", "--count", f"{origin_main}..{head}").stdout.strip()
    try:
        commit_count = int(commit_count_text)
    except ValueError:
        commit_count = -1

    result.source_of_truth = origin_main
    result.add("INSIDE_WORK_TREE", repo.is_inside_work_tree())
    result.add("FEATURE_BRANCH", bool(branch) and branch != "main", branch or "DETACHED_HEAD")
    result.add("INDEX_CLEAN", len(staged) == 0, f"count={len(staged)}")
    result.add("TRACKED_CLEAN", len(tracked) == 0, f"count={len(tracked)}")
    result.add("UNTRACKED_CLEAN", len(untracked) == 0, f"count={len(untracked)}")
    result.add("NOT_BEHIND_MAIN", origin_only == 0, f"behind={origin_only}")
    result.add("AHEAD_OF_MAIN", head_only > 0, f"ahead={head_only}")
    result.add("COMMIT_COUNT_VALID", commit_count > 0, f"count={commit_count}")
    result.add("DIFF_SCOPE_PRESENT", len(changed) > 0, f"files={len(changed)}")
    result.proven.extend(
        [
            f"current branch is {branch or 'DETACHED'}",
            f"HEAD is {head}",
            f"origin/main is {origin_main}",
            f"feature commit count is {commit_count}",
            f"changed file count is {len(changed)}",
        ]
    )
    _common_nonclaims(result)
    result.not_proven.append("human authorization to push or create a pull request")
    result.not_proven.append("absence of an existing remote pull request")
    result.not_proven.append("pull-request CI success")
    return result
