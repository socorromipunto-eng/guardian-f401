#!/usr/bin/env python3
"""
Guardian F401 governed Cppcheck runner.

The runner analyzes first-party Guardian C implementation files only.
Vendor CMSIS/HAL implementation source remains outside the primary finding
domain, while required vendor headers are supplied as parsing context.

This runner does not claim that successful analyzer execution means zero
findings. Execution state and finding state are deliberately separate.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import subprocess
import sys
import xml.etree.ElementTree as ET


EXPECTED_CPPCHECK_VERSION = "Cppcheck 2.21.0"

FIRST_PARTY_ROOTS = (
    "firmware/Acquisition",
    "firmware/App",
    "firmware/Control",
    "firmware/DSP",
    "firmware/Firmware",
    "firmware/Health",
    "firmware/NodeLink",
    "firmware/Platform",
    "firmware/Protocol",
    "firmware/Security",
    "firmware/Telemetry",
)

VENDOR_INCLUDE_ROOTS = (
    "firmware/MDK-ARM/GuardianF401/Inc",
    "firmware/MDK-ARM/GuardianF401/Drivers/CMSIS/Include",
    "firmware/MDK-ARM/GuardianF401/Drivers/CMSIS/Device/ST/STM32F4xx/Include",
    "firmware/MDK-ARM/GuardianF401/Drivers/STM32F4xx_HAL_Driver/Inc",
    "firmware/MDK-ARM/GuardianF401/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy",
)

REQUIRED_DEFINES = (
    "STM32F401xE",
    "USE_HAL_DRIVER",
)

COMPILER_MODEL_NAME = "ARMCLANG_6_24"
COMPILER_MODEL_ARMCC_VERSION = "6240002"
COMPILER_MODEL_DEFINE = (
    f"__ARMCC_VERSION={COMPILER_MODEL_ARMCC_VERSION}"
)


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def enumerate_c_files(repo: pathlib.Path) -> list[pathlib.Path]:
    files: list[pathlib.Path] = []

    for rel_root in FIRST_PARTY_ROOTS:
        root = repo / rel_root
        if not root.is_dir():
            raise RuntimeError(f"required first-party root missing: {rel_root}")
        files.extend(
            path.resolve()
            for path in root.rglob("*.c")
            if path.is_file()
        )

    unique = sorted(set(files))

    if not unique:
        raise RuntimeError("analysis file set is empty")

    return unique


def first_party_include_dirs(repo: pathlib.Path) -> list[pathlib.Path]:
    dirs: set[pathlib.Path] = set()

    for rel_root in FIRST_PARTY_ROOTS:
        root = repo / rel_root
        for header in root.rglob("*.h"):
            if header.is_file():
                dirs.add(header.parent.resolve())

    return sorted(dirs)


def file_set_hash(
    repo: pathlib.Path,
    files: list[pathlib.Path],
) -> str:
    digest = hashlib.sha256()
    repo_resolved = repo.resolve()

    for path in files:
        relative = path.relative_to(repo_resolved).as_posix()
        digest.update(relative.encode("utf-8"))
        digest.update(b"\0")
        digest.update(bytes.fromhex(sha256_file(path)))
        digest.update(b"\0")

    return digest.hexdigest().upper()


def parse_findings(
    xml_path: pathlib.Path,
) -> list[dict[str, object]]:
    tree = ET.parse(xml_path)
    root = tree.getroot()

    findings: list[dict[str, object]] = []

    errors = root.find("errors")
    if errors is None:
        return findings

    for error in errors.findall("error"):

        locations = []

        for location in error.findall("location"):
            locations.append(
                {
                    "file": location.attrib.get("file"),
                    "line": location.attrib.get("line"),
                    "column": location.attrib.get("column"),
                }
            )

        findings.append(
            {
                "id": error.attrib.get("id"),
                "severity": error.attrib.get("severity"),
                "message": error.attrib.get("msg"),
                "verbose": error.attrib.get("verbose"),
                "cwe": error.attrib.get("cwe"),
                "locations": locations,
            }
        )

    return findings


def main() -> int:
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--repo",
        required=True,
    )

    parser.add_argument(
        "--cppcheck",
        required=True,
    )

    parser.add_argument(
        "--evidence-dir",
        required=True,
    )

    parser.add_argument(
        "--fail-on-findings",
        action="store_true",
    )

    args = parser.parse_args()

    repo = pathlib.Path(args.repo).resolve()
    cppcheck = pathlib.Path(args.cppcheck).resolve()
    evidence = pathlib.Path(args.evidence_dir).resolve()

    if not (repo / ".git").exists():
        raise RuntimeError("repo does not look like a Git worktree")

    if not cppcheck.is_file():
        raise RuntimeError("cppcheck executable missing")

    if evidence.exists():
        raise RuntimeError("evidence directory already exists")

    evidence.mkdir(parents=True)

    version = subprocess.run(
        [str(cppcheck), "--version"],
        check=False,
        capture_output=True,
        text=True,
    )

    if version.returncode != 0:
        raise RuntimeError("cppcheck --version failed")

    version_text = (
        version.stdout +
        version.stderr
    ).strip()

    if version_text != EXPECTED_CPPCHECK_VERSION:
        raise RuntimeError(
            f"unexpected cppcheck version: {version_text!r}"
        )

    c_files = enumerate_c_files(repo)

    first_party_includes = first_party_include_dirs(repo)

    vendor_includes: list[pathlib.Path] = []

    for rel in VENDOR_INCLUDE_ROOTS:
        path = (repo / rel).resolve()

        if not path.is_dir():
            raise RuntimeError(
                f"required include root missing: {rel}"
            )

        vendor_includes.append(path)

    raw_xml = evidence / "cppcheck.xml"
    summary_json = evidence / "summary.json"

    command = [
        str(cppcheck),
        "--xml",
        "--xml-version=2",
        "--enable=warning,style,performance,portability,information",
        "--inconclusive",
        "--force",
        "--error-exitcode=2",
        "-DSTM32F401xE",
        "-DUSE_HAL_DRIVER",
        f"-D{COMPILER_MODEL_DEFINE}",
    ]

    for include in first_party_includes:
        command.extend(
            [
                "-I",
                str(include),
            ]
        )

    for include in vendor_includes:
        command.extend(
            [
                "-I",
                str(include),
            ]
        )

    command.extend(
        str(path)
        for path in c_files
    )

    process = subprocess.run(
        command,
        cwd=str(repo),
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    raw_xml.write_text(
        process.stderr,
        encoding="utf-8",
    )

    findings = parse_findings(raw_xml)

    severity_counts: dict[str, int] = {}

    for finding in findings:
        severity = str(
            finding.get("severity") or "unknown"
        )

        severity_counts[severity] = severity_counts.get(
                severity,
                0,
            ) + 1

    summary = {

        "compiler_model": COMPILER_MODEL_NAME,

        "compiler_model_armcc_version": COMPILER_MODEL_ARMCC_VERSION,

        "compiler_model_define": COMPILER_MODEL_DEFINE,

        "suppression_count": 0,
        "schema": "guardian.static-analysis.cppcheck.v1",
        "tool": "cppcheck",
        "tool_version": version_text,
        "cppcheck_sha256": sha256_file(cppcheck),
        "analyzed_file_count": len(c_files),
        "analyzed_file_set_sha256": file_set_hash(
            repo,
            c_files,
        ),
        "vendor_implementation_analyzed": False,
        "target_defines": list(REQUIRED_DEFINES),
        "finding_count": len(findings),
        "finding_count_by_severity": severity_counts,
        "findings": findings,
        "analyzer_exit_code": process.returncode,
    }

    summary_json.write_text(
        json.dumps(
            summary,
            indent=2,
            sort_keys=True,
        ) + "\n",
        encoding="utf-8",
    )

    print(
        f"CPPCHECK_VERSION={version_text}"
    )

    print(
        "CPPCHECK_SHA256=" +
        summary["cppcheck_sha256"]
    )

    print(
        "ANALYZED_FILE_COUNT=" +
        str(len(c_files))
    )

    print(
        "ANALYZED_FILE_SET_SHA256=" +
        summary["analyzed_file_set_sha256"]
    )

    print(
        "FINDING_COUNT=" +
        str(len(findings))
    )

    print(
        "ANALYZER_EXIT_CODE=" +
        str(process.returncode)
    )

    print(
        "RAW_XML=" +
        str(raw_xml)
    )

    print(
        "SUMMARY_JSON=" +
        str(summary_json)
    )

    if process.returncode not in (0, 2):
        print(
            "STATIC_ANALYSIS_EXECUTION=FAIL"
        )
        return 3

    if (
        args.fail_on_findings and
        findings
    ):
        print(
            "STATIC_ANALYSIS_GATE=FAIL_FINDINGS_PRESENT"
        )
        return 2

    print(
        "STATIC_ANALYSIS_EXECUTION=PASS"
    )

    return 0


if __name__ == "__main__":
    sys.exit(main())
