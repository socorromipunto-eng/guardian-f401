"""Read-only Git repository observations."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import subprocess

from .errors import FailureClass, GovernanceToolError


@dataclass(slots=True, frozen=True)
class CommandOutput:
    stdout: str
    stderr: str
    returncode: int


class GitRepository:
    def __init__(self, root: Path, timeout_seconds: float = 15.0) -> None:
        self.root = root.resolve()
        self.timeout_seconds = timeout_seconds
        if not self.root.is_dir():
            raise GovernanceToolError(FailureClass.ENVIRONMENT, "REPO_NOT_FOUND", str(self.root))

    def git(self, *args: str, allow_failure: bool = False) -> CommandOutput:
        command = ["git", "-C", str(self.root), *args]
        try:
            completed = subprocess.run(
                command,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
                timeout=self.timeout_seconds,
                shell=False,
                check=False,
            )
        except FileNotFoundError as exc:
            raise GovernanceToolError(
                FailureClass.ENVIRONMENT, "GIT_NOT_FOUND", "git executable not found"
            ) from exc
        except subprocess.TimeoutExpired as exc:
            raise GovernanceToolError(
                FailureClass.ENVIRONMENT, "GIT_TIMEOUT", "git command timed out"
            ) from exc

        output = CommandOutput(
            completed.stdout.rstrip("\n"),
            completed.stderr.rstrip("\n"),
            completed.returncode,
        )
        if output.returncode != 0 and not allow_failure:
            raise GovernanceToolError(
                FailureClass.ENVIRONMENT,
                "GIT_COMMAND_FAILED",
                f"git {args!r} exited {output.returncode}: {output.stderr}",
            )
        return output

    def git_bytes(self, *args: str) -> bytes:
        command = ["git", "-C", str(self.root), *args]
        try:
            completed = subprocess.run(command, capture_output=True, text=False, timeout=self.timeout_seconds, shell=False, check=False)
        except FileNotFoundError as exc:
            raise GovernanceToolError(FailureClass.ENVIRONMENT, "GIT_NOT_FOUND", "git executable not found") from exc
        except subprocess.TimeoutExpired as exc:
            raise GovernanceToolError(FailureClass.ENVIRONMENT, "GIT_TIMEOUT", "git command timed out") from exc
        if completed.returncode != 0:
            stderr = completed.stderr.decode("utf-8", errors="replace").rstrip("\n")
            raise GovernanceToolError(FailureClass.ENVIRONMENT, "GIT_COMMAND_FAILED", f"git {args!r} exited {completed.returncode}: {stderr}")
        return completed.stdout

    def branch(self) -> str:
        return self.git("branch", "--show-current").stdout.strip()

    def head(self) -> str:
        return self.git("rev-parse", "HEAD").stdout.strip()

    def origin_main(self) -> str:
        return self.git("rev-parse", "origin/main").stdout.strip()

    def staged(self) -> list[str]:
        return [line for line in self.git("diff", "--cached", "--name-only").stdout.splitlines() if line]

    def tracked_changes(self) -> list[str]:
        return [line for line in self.git("diff", "--name-only").stdout.splitlines() if line]

    def untracked(self) -> list[str]:
        return [line for line in self.git("ls-files", "--others", "--exclude-standard").stdout.splitlines() if line]

    def is_inside_work_tree(self) -> bool:
        return self.git("rev-parse", "--is-inside-work-tree").stdout.strip() == "true"

    def merge_base(self, left: str, right: str) -> str:
        return self.git("merge-base", left, right).stdout.strip()

    def ahead_behind(self, left: str, right: str) -> tuple[int, int]:
        fields = self.git("rev-list", "--left-right", "--count", f"{left}...{right}").stdout.split()
        if len(fields) != 2:
            raise GovernanceToolError(
                FailureClass.HARNESS, "DIVERGENCE_PARSE", "unexpected rev-list output"
            )
        try:
            return int(fields[0]), int(fields[1])
        except ValueError as exc:
            raise GovernanceToolError(
                FailureClass.HARNESS, "DIVERGENCE_PARSE", "non-integer rev-list output"
            ) from exc

    def index_blob(self, relative_path: str) -> bytes:
        return self.git_bytes("show", f":{relative_path}")
