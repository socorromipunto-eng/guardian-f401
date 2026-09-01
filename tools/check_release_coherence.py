from __future__ import annotations

import argparse
import re
from pathlib import Path


def read(path: Path) -> str:
    if not path.is_file():
        raise ValueError(f"missing required file: {path}")
    return path.read_text(encoding="utf-8")


def check(repo: Path) -> list[str]:
    errors: list[str] = []
    try:
        version = read(repo / "VERSION").strip()
        readme = read(repo / "README.md")
        citation = read(repo / "CITATION.cff")
        changelog = read(repo / "CHANGELOG.md")
        closure = read(repo / "docs/governance/M15-Governance-Closure.md")
        evidence = read(repo / f"docs/release-evidence-v{version}.md")
    except ValueError as exc:
        return [str(exc)]

    if not re.fullmatch(r"\d+\.\d+\.\d+", version):
        return [f"invalid VERSION: {version!r}"]

    doi_m = re.search(r"Zenodo version DOI:\s*`([^`]+)`", evidence)
    concept_m = re.search(r"Zenodo concept DOI:\s*`([^`]+)`", evidence)
    date_m = re.search(r"(?m)^Publication date:\s*(\d{4}-\d{2}-\d{2})$", evidence)
    if not doi_m: errors.append("release evidence lacks Zenodo version DOI")
    if not concept_m: errors.append("release evidence lacks Zenodo concept DOI")
    if not date_m: errors.append("release evidence lacks publication date")
    if errors:
        return errors

    doi = doi_m.group(1)
    concept = concept_m.group(1)
    date = date_m.group(1)

    for marker in [
        f"Project release: v{version}",
        f"Firmware semantic version: {version}",
        f"releases/tag/v{version}",
        f"docs/release-evidence-v{version}.md",
        f"https://doi.org/{doi}",
        f"https://doi.org/{concept}",
        "M15 governance / semantic assurance foundation",
    ]:
        if marker not in readme:
            errors.append("README missing: " + marker)

    if "The current M14 feature branch" in readme:
        errors.append("README retains stale M14 feature-branch statement")

    if f'version: "{version}"' not in citation:
        errors.append("CITATION version mismatch")
    if f'date-released: "{date}"' not in citation:
        errors.append("CITATION release date mismatch")
    if doi not in citation:
        errors.append("CITATION lacks version DOI")
    if concept not in citation:
        errors.append("CITATION lacks concept DOI")

    if f"## [{version}] - {date}" not in changelog:
        errors.append("CHANGELOG lacks current version/date")

    if "Status: PUBLISHED / BOUNDED" not in closure:
        errors.append("M15 closure is not PUBLISHED / BOUNDED")
    if doi not in closure or concept not in closure:
        errors.append("M15 closure lacks published DOI identity")

    return errors


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--repo", default=".")
    args = p.parse_args()
    repo = Path(args.repo).resolve()
    errors = check(repo)
    print("GUARDIAN_RELEASE_COHERENCE_V1")
    print(f"ERROR_COUNT={len(errors)}")
    for e in errors:
        print("ERROR=" + e)
    print("FINAL=" + ("PASS" if not errors else "FAIL"))
    return 0 if not errors else 1


if __name__ == "__main__":
    raise SystemExit(main())
