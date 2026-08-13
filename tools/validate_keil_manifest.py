"""Validate Guardian M13 Keil source/include manifests before opening uVision."""

# Import argparse for the dependency-free command-line grammar.
import argparse

# Import sys for conventional process exit handling.
import sys

# Import dataclass for immutable validation results.
from dataclasses import dataclass

# Import Path for repository-relative manifest resolution.
from pathlib import Path


# Store one manifest validation issue.
@dataclass(frozen=True, slots=True)
class ManifestIssue:
    """One missing, duplicate or unsafe Keil manifest entry."""

    # Store the issue class.
    kind: str

    # Store the affected manifest entry.
    entry: str

    # Store one concise diagnostic.
    message: str


# Read one comment-aware line manifest.
def read_manifest(
    path: Path,
) -> tuple[str, ...]:
    """Return non-empty, non-comment manifest entries in source order."""

    # Read UTF-8 manifest text.
    text = path.read_text(
        encoding="utf-8"
    )

    # Collect meaningful entries.
    entries: list[str] = []

    # Process every physical line independently.
    for line in text.splitlines():

        # Remove surrounding whitespace.
        stripped = line.strip()

        # Ignore blank lines and documentation comments.
        if (
            not stripped
            or stripped.startswith("#")
        ):

            # Continue with the next physical line.
            continue

        # Preserve the repository-relative entry.
        entries.append(
            stripped
        )

    # Return an immutable manifest.
    return tuple(
        entries
    )


# Validate source/include manifests against one repository checkout.
def validate_keil_manifests(
    repository_root: Path,
) -> tuple[ManifestIssue, ...]:
    """Return every M13 Keil manifest issue without modifying the repository."""

    # Resolve repository root.
    root = repository_root.resolve()

    # Resolve the source manifest.
    source_manifest = (
        root
        / "firmware"
        / "MDK-ARM"
        / "guardian-f401-keil-sources.txt"
    )

    # Resolve the include manifest.
    include_manifest = (
        root
        / "firmware"
        / "MDK-ARM"
        / "guardian-f401-keil-includes.txt"
    )

    # Require both manifest files.
    for manifest in (
        source_manifest,
        include_manifest,
    ):

        # Reject missing project metadata.
        if not manifest.is_file():

            # Return one precise missing-manifest issue immediately.
            return (
                ManifestIssue(
                    kind="missing_manifest",
                    entry=str(
                        manifest
                    ),
                    message="required Keil manifest file is missing",
                ),
            )

    # Read source entries.
    sources = read_manifest(
        source_manifest
    )

    # Read include entries.
    includes = read_manifest(
        include_manifest
    )

    # Collect all issues deterministically.
    issues: list[
        ManifestIssue
    ] = []

    # Detect duplicate source entries.
    seen_sources: set[str] = set()

    # Validate every source file.
    for entry in sources:

        # Normalize separators for duplicate detection.
        normalized = entry.replace(
            "\\",
            "/",
        )

        # Detect duplicate source ownership.
        if normalized in seen_sources:

            # Record duplicate source configuration.
            issues.append(
                ManifestIssue(
                    kind="duplicate_source",
                    entry=entry,
                    message="source appears more than once in the Keil manifest",
                )
            )
        else:

            # Preserve first source occurrence.
            seen_sources.add(
                normalized
            )

        # Resolve the source path.
        source_path = (
            root
            / Path(
                normalized
            )
        )

        # Require an existing regular C source.
        if (
            not source_path.is_file()
            or source_path.suffix.lower() != ".c"
        ):

            # Record missing/invalid source path.
            issues.append(
                ManifestIssue(
                    kind="invalid_source",
                    entry=entry,
                    message="source entry does not resolve to an existing .c file",
                )
            )

        # Prevent host-only tests from entering a physical Keil target.
        if "/Tests/" in (
            "/" +
            normalized
        ):

            # Record unsafe target contamination.
            issues.append(
                ManifestIssue(
                    kind="host_test_source",
                    entry=entry,
                    message="host test source must not be added to the physical Keil target",
                )
            )

        # Prevent the CMSIS compile stub from entering production firmware.
        if "CMSISStub" in normalized:

            # Record test-stub contamination.
            issues.append(
                ManifestIssue(
                    kind="cmsis_stub_source",
                    entry=entry,
                    message="CMSISStub is compile-test-only and must not enter the Keil target",
                )
            )

    # Detect duplicate include entries.
    seen_includes: set[str] = set()

    # Validate every include directory.
    for entry in includes:

        # Normalize separators.
        normalized = entry.replace(
            "\\",
            "/",
        )

        # Detect duplicate include configuration.
        if normalized in seen_includes:

            # Record duplicate include path.
            issues.append(
                ManifestIssue(
                    kind="duplicate_include",
                    entry=entry,
                    message="include path appears more than once in the Keil manifest",
                )
            )
        else:

            # Preserve first include occurrence.
            seen_includes.add(
                normalized
            )

        # Resolve the include directory.
        include_path = (
            root
            / Path(
                normalized
            )
        )

        # Require an existing directory.
        if not include_path.is_dir():

            # Record missing include path.
            issues.append(
                ManifestIssue(
                    kind="invalid_include",
                    entry=entry,
                    message="include entry does not resolve to an existing directory",
                )
            )

        # Prevent the test-only CMSIS stub from entering target includes.
        if "CMSISStub" in normalized:

            # Record unsafe include configuration.
            issues.append(
                ManifestIssue(
                    kind="cmsis_stub_include",
                    entry=entry,
                    message="CMSISStub must never be configured as a physical target include path",
                )
            )

    # Require the exact standalone reference main template.
    required_main = (
        "firmware/MDK-ARM/Templates/main_guardian.c"
    )

    # Record missing target entry point.
    if required_main not in seen_sources:

        # Add one missing-main issue.
        issues.append(
            ManifestIssue(
                kind="missing_main",
                entry=required_main,
                message="standalone M13 Keil target main template is missing",
            )
        )

    # Require both standalone IRQ wrapper translation units.
    for required_irq in (
        "firmware/Platform/STM32F401/Src/stm32f401_uart2_irq.c",
        "firmware/Platform/STM32F401/Src/stm32f401_acquisition_irq.c",
    ):

        # Record missing standalone IRQ ownership.
        if required_irq not in seen_sources:

            # Add one missing wrapper issue.
            issues.append(
                ManifestIssue(
                    kind="missing_irq_wrapper",
                    entry=required_irq,
                    message="standalone reference target requires this IRQ wrapper",
                )
            )

    # Return immutable issues in deterministic discovery order.
    return tuple(
        issues
    )


# Build the command-line parser.
def build_parser() -> argparse.ArgumentParser:
    """Return the M13 Keil manifest validation parser."""

    # Create the top-level parser.
    parser = argparse.ArgumentParser(
        description=(
            "Validate Guardian M13 Keil source/include manifests "
            "without requiring uVision."
        )
    )

    # Permit explicit repository root for CI or alternate checkouts.
    parser.add_argument(
        "--repository-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="Guardian repository root (default: inferred from tools directory)",
    )

    # Return the configured parser.
    return parser


# Execute manifest validation.
def main(
    argv: list[str] | None = None,
) -> int:
    """Validate M13 Keil metadata and print concise results."""

    # Parse caller arguments.
    args = build_parser().parse_args(
        argv
    )

    # Validate manifests without changing any files.
    try:

        # Collect every issue.
        issues = validate_keil_manifests(
            args.repository_root
        )
    except OSError as exc:

        # Report filesystem failure without traceback.
        print(
            f"keil-manifest: {exc}",
            file=sys.stderr,
        )

        # Return operational failure.
        return 2

    # Print every issue when validation fails.
    if issues:

        # Report one issue per line.
        for issue in issues:

            # Print stable issue classification.
            print(
                (
                    f"[FAIL] {issue.kind}: "
                    f"{issue.entry}: "
                    f"{issue.message}"
                ),
                file=sys.stderr,
            )

        # Return failed validation.
        return 1

    # Print one concise success line.
    print(
        "Guardian M13 Keil source/include manifest: PASS"
    )

    # Return conventional success.
    return 0


# Execute the validator when run directly.
if __name__ == "__main__":

    # Exit using the validation result.
    raise SystemExit(
        main()
    )
