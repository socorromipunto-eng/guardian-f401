"""Structured, deterministic result representation."""

from __future__ import annotations

from dataclasses import asdict, dataclass, field
import json
from typing import Any


@dataclass(slots=True)
class Check:
    name: str
    status: str
    detail: str = ""


@dataclass(slots=True)
class Result:
    command: str
    repository: str
    source_of_truth: str | None = None
    checks: list[Check] = field(default_factory=list)
    proven: list[str] = field(default_factory=list)
    not_proven: list[str] = field(default_factory=list)
    authority: dict[str, str] = field(
        default_factory=lambda: {
            "COMMIT": "NOT_GRANTED",
            "PUSH": "NOT_GRANTED",
            "PR": "NOT_GRANTED",
            "MERGE": "NOT_GRANTED",
        }
    )

    @property
    def final(self) -> str:
        return "PASS" if all(check.status == "PASS" for check in self.checks) else "FAIL"

    def add(self, name: str, passed: bool, detail: str = "") -> None:
        self.checks.append(Check(name, "PASS" if passed else "FAIL", detail))

    def to_dict(self) -> dict[str, Any]:
        data = asdict(self)
        data["final"] = self.final
        return data

    def to_json(self) -> str:
        return json.dumps(self.to_dict(), indent=2, sort_keys=True)

    def to_text(self) -> str:
        lines = [
            "GUARDIAN_GOVERNANCE_TOOLING_RESULT_V1",
            f"COMMAND={self.command}",
            f"REPOSITORY={self.repository}",
        ]
        if self.source_of_truth:
            lines.append(f"SOURCE_OF_TRUTH={self.source_of_truth}")
        for check in self.checks:
            suffix = f" ({check.detail})" if check.detail else ""
            lines.append(f"CHECK_{check.name}={check.status}{suffix}")
        lines.extend(f"PROVEN={item}" for item in self.proven)
        lines.extend(f"NOT_PROVEN={item}" for item in self.not_proven)
        lines.extend(f"AUTHORITY_{key}={value}" for key, value in self.authority.items())
        lines.append(f"FINAL={self.final}")
        return "\n".join(lines)
