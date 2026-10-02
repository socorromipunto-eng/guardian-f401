from __future__ import annotations

import argparse
import re
from datetime import date
from pathlib import Path


CANDIDATE = "PREPUBLICATION_CANDIDATE"
PUBLISHED = "POST-PUBLICATION EVIDENCE RECORD"
FOUNDATION = "0.15.0"


def read(path: Path) -> str:
    if not path.is_file():
        raise ValueError(f"missing required file: {path}")
    return path.read_text(encoding="utf-8")


def field(text: str, label: str) -> str:
    values = re.findall(r"(?m)^(?:- )?" + re.escape(label) + r":\s*([^\r\n]+)$", text)
    if len(values) != 1:
        raise ValueError(f"expected one {label} field; found {len(values)}")
    return values[0].strip().strip("`")


def phase(evidence: str, version: str) -> str:
    # Legacy v0.15.0 fixtures predate the explicit status field.
    if not re.search(r"(?m)^Status:", evidence) and version == FOUNDATION:
        return PUBLISHED
    value = field(evidence, "Status")
    if value not in (CANDIDATE, PUBLISHED):
        raise ValueError(f"unknown publication state: {value!r}")
    return value


def doi(value: str) -> bool:
    return re.fullmatch(r"10\.\d{4,9}/[A-Za-z0-9._;()/:-]+", value) is not None


def markers(text: str, required: list[str], label: str, errors: list[str]) -> None:
    for marker in required:
        if marker not in text:
            errors.append(f"{label} missing: {marker}")


def check(repo: Path, *, require_published: bool = False) -> list[str]:
    errors: list[str] = []
    try:
        version = read(repo / "VERSION").strip()
        if not re.fullmatch(r"\d+\.\d+\.\d+", version):
            return [f"invalid VERSION: {version!r}"]
        readme = read(repo / "README.md")
        citation = read(repo / "CITATION.cff")
        changelog = read(repo / "CHANGELOG.md")
        closure = read(repo / "docs/governance/M15-Governance-Closure.md")
        evidence = read(repo / f"docs/release-evidence-v{version}.md")
        state = phase(evidence, version)
        historical = evidence if version == FOUNDATION else read(repo / f"docs/release-evidence-v{FOUNDATION}.md")
        if phase(historical, FOUNDATION) != PUBLISHED:
            errors.append("historical foundation evidence is not published")
        historical_doi = field(historical, "Zenodo version DOI")
        historical_concept = field(historical, "Zenodo concept DOI")
        concept = field(evidence, "Zenodo concept DOI")
    except (ValueError, UnicodeError) as exc:
        return [str(exc)]

    if not doi(concept) or not doi(historical_doi) or not doi(historical_concept):
        errors.append("invalid DOI syntax")
    if concept != historical_concept:
        errors.append("concept DOI differs from historical release family")
    if "Status: PUBLISHED / BOUNDED" not in closure:
        errors.append("M15 closure is not PUBLISHED / BOUNDED")
    if historical_doi not in closure or historical_concept not in closure:
        errors.append("M15 closure lacks its historical published DOI identity")

    markers(readme, [f"Project release: v{version}", f"Firmware semantic version: {version}",
                    f"releases/tag/v{version}", f"docs/release-evidence-v{version}.md",
                    f"https://doi.org/{concept}", "M15 governance / semantic assurance foundation"], "README", errors)
    if "The current M14 feature branch" in readme:
        errors.append("README retains stale M14 feature-branch statement")
    if re.findall(r'(?m)^version:\s*"([^"\r\n]+)"\s*$', citation) != [version]:
        errors.append("CITATION version mismatch or duplicate")

    if state == CANDIDATE:
        if require_published:
            errors.append("published evidence required; candidate is not published")
        required = {
            "Planned tag": f"v{version}", "Publication performed": "NO",
            "Version DOI allocation": "PENDING", "Publication date allocation": "PENDING",
            "PhysicalHardwareValidation": "PENDING", "TargetGeometryQualification": "NOT_DEMONSTRATED",
            "RuntimeStackSufficiency": "NOT_DEMONSTRATED", "StorageOwnershipQualification": "NOT_DEMONSTRATED",
            "BackupChipDeployment": "NOT_DEMONSTRATED", "ProductionSecurityQualification": "NOT_DEMONSTRATED",
            "CertificationClaimAuthorized": "NO",
        }
        for label, expected in required.items():
            try:
                if field(evidence, label) != expected:
                    errors.append(f"candidate {label} must be {expected}")
            except ValueError as exc:
                errors.append(str(exc))
        if re.search(r"(?m)^(?:- )?(?:Zenodo version DOI|Publication date):", evidence):
            errors.append("candidate carries premature DOI/date publication fields")
        if re.search(r"(?m)^date-released:", citation):
            errors.append("candidate CITATION carries a release date")
        # The only permitted DOI identifier for a candidate is the concept DOI.
        citation_dois = re.findall(r'(?m)^\s+value:\s*"(10\.[^"\r\n]+)"\s*$', citation)
        if citation_dois != [concept]:
            errors.append("candidate CITATION must contain only the concept DOI")
        if not re.search(r'(?m)^message: "[^"\r\n]*prepublication candidate[^"\r\n]*"$', citation):
            errors.append("candidate CITATION lacks explicit candidate message")
        markers(readme, [f"Publication state: {CANDIDATE}", f"Last published release: v{FOUNDATION}",
                         f"Planned GitHub release: https://github.com/socorromipunto-eng/guardian-f401/releases/tag/v{version}"], "README", errors)
        if re.search(r"(?m)^GitHub release:.*releases/tag/v" + re.escape(version) + r"\s*$", readme):
            errors.append("candidate README claims current published release")
        if f"## [{version}] - Unreleased" not in changelog:
            errors.append("candidate CHANGELOG lacks Unreleased entry")
        if re.search(r"(?m)^## \[" + re.escape(version) + r"\] - \d{4}-\d{2}-\d{2}$", changelog):
            errors.append("candidate CHANGELOG carries a publication date")
    else:
        try:
            current_doi = field(evidence, "Zenodo version DOI")
            publication_date = field(evidence, "Publication date")
            if not doi(current_doi):
                errors.append("invalid published version DOI")
            date.fromisoformat(publication_date)
        except ValueError as exc:
            errors.append(str(exc))
            return errors
        if version != FOUNDATION and current_doi == historical_doi:
            errors.append("new release reuses historical version DOI")
        markers(readme, [f"https://doi.org/{current_doi}"], "README", errors)
        if f'date-released: "{publication_date}"' not in citation:
            errors.append("CITATION release date mismatch")
        if current_doi not in citation or concept not in citation:
            errors.append("CITATION lacks published DOI identity")
        if f"## [{version}] - {publication_date}" not in changelog:
            errors.append("CHANGELOG lacks current version/date")
        if CANDIDATE in readme or "prepublication candidate" in citation or f"## [{version}] - Unreleased" in changelog:
            errors.append("published metadata retains candidate state")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", default=".")
    parser.add_argument("--require-published", action="store_true")
    args = parser.parse_args()
    repo = Path(args.repo).resolve()
    errors = check(repo, require_published=args.require_published)
    try:
        version = read(repo / "VERSION").strip()
        state = phase(read(repo / f"docs/release-evidence-v{version}.md"), version)
    except (ValueError, UnicodeError):
        state = "UNKNOWN"
    print("GUARDIAN_RELEASE_COHERENCE_V2")
    print(f"PUBLICATION_STATE={state}")
    print("PUBLICATION_PROVEN_BY_THIS_CHECK=NO")
    print("HARDWARE_QUALIFICATION_PROVEN_BY_THIS_CHECK=NO")
    print(f"ERROR_COUNT={len(errors)}")
    for error in errors:
        print("ERROR=" + error)
    print("FINAL=" + ("PASS" if not errors else "FAIL"))
    return 0 if not errors else 1


if __name__ == "__main__":
    raise SystemExit(main())
