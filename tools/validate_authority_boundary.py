#!/usr/bin/env python3
"""Validate the Guardian G15-05 authority ownership boundary.

This validator enforces repository-level ownership rules derived from the
frozen G15-05 Authority Reachability Model.

It does not prove functional safety, physical actuation safety, or complete
C-language reachability. It establishes mechanically checked repository
invariants for specifically protected Guardian authority surfaces.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


POLICY_RELATIVE_PATH = Path(
    "assurance/policies/m15-authority-boundary.json"
)

PRODUCTION_SUFFIXES = {".c", ".h"}


@dataclass(frozen=True)
class Violation:
    rule_id: str
    path: str
    line: int
    message: str
    text: str


class PolicyError(RuntimeError):
    pass


def repository_root() -> Path:
    return Path(__file__).resolve().parents[1]


def load_policy(root: Path) -> dict:
    path = root / POLICY_RELATIVE_PATH

    try:
        raw = path.read_text(encoding="utf-8-sig")
        policy = json.loads(raw)
    except FileNotFoundError as exc:
        raise PolicyError(f"policy missing: {path}") from exc
    except json.JSONDecodeError as exc:
        raise PolicyError(f"invalid policy JSON: {exc}") from exc

    if policy.get("policy_id") != "G15-05-AUTHORITY-BOUNDARY":
        raise PolicyError("unexpected policy_id")

    if policy.get("schema_version") != 1:
        raise PolicyError("unsupported schema_version")

    return policy


def normalized_relative(root: Path, path: Path) -> str:
    return path.resolve().relative_to(root.resolve()).as_posix()


def production_files(root: Path) -> Iterable[Path]:
    firmware = root / "firmware"

    for path in sorted(firmware.rglob("*")):
        if not path.is_file():
            continue

        if path.suffix.lower() not in PRODUCTION_SUFFIXES:
            continue

        relative = normalized_relative(root, path)

        if "/Tests/" in f"/{relative}":
            continue

        yield path


def strip_comments_and_literals(text: str) -> str:
    """Remove comments and string/character literal contents while preserving lines."""

    out: list[str] = []
    index = 0
    length = len(text)

    while index < length:
        current = text[index]
        following = text[index + 1] if index + 1 < length else ""

        if current == "/" and following == "/":
            out.extend((" ", " "))
            index += 2

            while index < length and text[index] != "\n":
                out.append(" ")
                index += 1

            continue

        if current == "/" and following == "*":
            out.extend((" ", " "))
            index += 2

            while index < length:
                if (
                    text[index] == "*"
                    and index + 1 < length
                    and text[index + 1] == "/"
                ):
                    out.extend((" ", " "))
                    index += 2
                    break

                if text[index] == "\n":
                    out.append("\n")
                else:
                    out.append(" ")

                index += 1

            continue

        if current in ('"', "'"):
            quote = current
            out.append(" ")
            index += 1
            escaped = False

            while index < length:
                char = text[index]

                if char == "\n":
                    out.append("\n")
                else:
                    out.append(" ")

                if escaped:
                    escaped = False
                elif char == "\\":
                    escaped = True
                elif char == quote:
                    index += 1
                    break

                index += 1

            continue

        out.append(current)
        index += 1

    return "".join(out)


def normalized_source(path: Path) -> str:
    raw = path.read_text(encoding="utf-8", errors="strict")
    return strip_comments_and_literals(raw)


def line_number_for_offset(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def source_excerpt(text: str, offset: int) -> str:
    start = text.rfind("\n", 0, offset) + 1
    end = text.find("\n", offset)

    if end == -1:
        end = len(text)

    return text[start:end].strip()


def structural_matches(path: Path, pattern: re.Pattern[str]):
    text = normalized_source(path)

    for match in pattern.finditer(text):
        yield (
            line_number_for_offset(text, match.start()),
            source_excerpt(text, match.start()),
            match,
        )


def callback_aliases(path: Path) -> dict[str, tuple[int, str]]:
    """Find local identifiers initialized from a Guardian output.apply field."""

    text = normalized_source(path)

    pattern = re.compile(
        r"\b([A-Za-z_][A-Za-z0-9_]*)\s*=\s*"
        r"(?:[A-Za-z_][A-Za-z0-9_]*\s*(?:->|\.)\s*)+"
        r"output\s*\.\s*apply\b",
        re.S,
    )

    aliases: dict[str, tuple[int, str]] = {}

    for match in pattern.finditer(text):
        aliases[match.group(1)] = (
            line_number_for_offset(text, match.start()),
            source_excerpt(text, match.start()),
        )

    return aliases


def callback_alias_invocations(path: Path):
    text = normalized_source(path)
    aliases = callback_aliases(path)

    for name, origin in aliases.items():
        call_pattern = re.compile(
            r"\b" + re.escape(name) + r"\s*\(",
            re.S,
        )

        for match in call_pattern.finditer(text):
            if match.start() <= 0:
                continue

            line_number = line_number_for_offset(text, match.start())
            excerpt = source_excerpt(text, match.start())

            yield name, origin, line_number, excerpt


def rule_by_id(policy: dict, rule_id: str) -> dict:
    for rule in policy.get("rules", []):
        if rule.get("id") == rule_id:
            return rule

    raise PolicyError(f"required rule missing: {rule_id}")


def allowed(rule: dict) -> set[str]:
    return {
        str(value).replace("\\", "/")
        for value in rule.get("allowed_files", [])
    }


def validate_run_permit_owner(
    root: Path,
    files: list[Path],
    policy: dict,
) -> list[Violation]:
    rule = rule_by_id(policy, "AUTH-001")
    allowed_files = allowed(rule)

    pattern = re.compile(
        r"(?:->|\.)\s*status\s*\.\s*run_permit\s*=",
        re.S,
    )

    violations: list[Violation] = []

    for path in files:
        relative = normalized_relative(root, path)

        for line_number, line, _match in structural_matches(path, pattern):
            if relative not in allowed_files:
                violations.append(
                    Violation(
                        rule_id="AUTH-001",
                        path=relative,
                        line=line_number,
                        message=(
                            "canonical run-permit mutation outside "
                            "Guardian Control"
                        ),
                        text=line,
                    )
                )

    return violations


def validate_output_invocation_owner(
    root: Path,
    files: list[Path],
    policy: dict,
) -> list[Violation]:
    rule = rule_by_id(policy, "AUTH-002")
    allowed_files = allowed(rule)

    direct_pattern = re.compile(
        r"(?:->|\.)\s*output\s*\.\s*apply\s*\(",
        re.S,
    )

    violations: list[Violation] = []

    for path in files:
        relative = normalized_relative(root, path)

        for line_number, line, _match in structural_matches(
            path,
            direct_pattern,
        ):
            if relative not in allowed_files:
                violations.append(
                    Violation(
                        rule_id="AUTH-002",
                        path=relative,
                        line=line_number,
                        message=(
                            "authority-output callback invocation outside "
                            "Guardian Control"
                        ),
                        text=line,
                    )
                )

        if relative not in allowed_files:
            for (
                alias_name,
                _origin,
                line_number,
                line,
            ) in callback_alias_invocations(path):
                violations.append(
                    Violation(
                        rule_id="AUTH-002",
                        path=relative,
                        line=line_number,
                        message=(
                            "aliased authority-output callback invocation "
                            f"outside Guardian Control: {alias_name}"
                        ),
                        text=line,
                    )
                )

    return violations


def validate_output_configuration_path(
    root: Path,
    files: list[Path],
    policy: dict,
) -> list[Violation]:
    rule = rule_by_id(policy, "AUTH-003")
    allowed_files = allowed(rule)

    pattern = re.compile(
        r"\bguardian_control_configure_output\s*\(",
        re.S,
    )

    violations: list[Violation] = []

    for path in files:
        if path.suffix.lower() != ".c":
            continue

        relative = normalized_relative(root, path)

        for line_number, line, _match in structural_matches(path, pattern):
            if relative not in allowed_files:
                violations.append(
                    Violation(
                        rule_id="AUTH-003",
                        path=relative,
                        line=line_number,
                        message=(
                            "direct Guardian Control output configuration "
                            "outside approved path"
                        ),
                        text=line,
                    )
                )

    return violations


def validate_heap_absence(
    root: Path,
    files: list[Path],
    policy: dict,
) -> list[Violation]:
    rule_by_id(policy, "AUTH-004")

    pattern = re.compile(
        r"\b(malloc|calloc|realloc|free)\s*\(",
        re.S,
    )

    violations: list[Violation] = []

    for path in files:
        relative = normalized_relative(root, path)

        for line_number, line, match in structural_matches(path, pattern):
            violations.append(
                Violation(
                    rule_id="AUTH-004",
                    path=relative,
                    line=line_number,
                    message=(
                        "runtime heap call prohibited: "
                        + match.group(1)
                    ),
                    text=line,
                )
            )

    return violations


def validate_required_paths(root: Path, policy: dict) -> list[Violation]:
    authority = policy.get("authority", {})

    required = [
        authority.get("control_implementation"),
        authority.get("control_public_header"),
        authority.get("embedded_link_implementation"),
        authority.get("application_implementation"),
    ]

    violations: list[Violation] = []

    for relative in required:
        if not relative:
            raise PolicyError("required authority path missing from policy")

        if not (root / relative).is_file():
            violations.append(
                Violation(
                    rule_id="POLICY-PATH",
                    path=str(relative),
                    line=0,
                    message="required authority path does not exist",
                    text="",
                )
            )

    return violations


def run_validation(root: Path, policy: dict) -> list[Violation]:
    files = list(production_files(root))

    violations: list[Violation] = []

    violations.extend(validate_required_paths(root, policy))
    violations.extend(validate_run_permit_owner(root, files, policy))
    violations.extend(validate_output_invocation_owner(root, files, policy))
    violations.extend(validate_output_configuration_path(root, files, policy))
    violations.extend(validate_heap_absence(root, files, policy))

    return violations


def print_violation(violation: Violation) -> None:
    location = violation.path

    if violation.line:
        location = f"{location}:{violation.line}"

    print(
        f"FAIL {violation.rule_id}: "
        f"{location}: {violation.message}"
    )

    if violation.text:
        print(f"  {violation.text}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate Guardian G15-05 authority boundary."
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=None,
        help="Repository root; defaults to validator repository.",
    )
    args = parser.parse_args()

    root = (
        args.root.resolve()
        if args.root is not None
        else repository_root()
    )

    try:
        policy = load_policy(root)
        violations = run_validation(root, policy)
    except (PolicyError, UnicodeDecodeError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2

    if violations:
        print(
            f"FAILED: {len(violations)} "
            "authority-boundary violation(s)."
        )

        for violation in violations:
            print_violation(violation)

        return 1

    print("PASS: G15-05 authority boundary")
    print("PASS: canonical run-permit write ownership")
    print("PASS: authority-output callback invocation ownership")
    print("PASS: approved output-configuration path")
    print("PASS: production firmware runtime-heap prohibition")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())