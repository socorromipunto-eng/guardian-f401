"""Run deterministic Guardian M11 robustness campaigns without package installation."""

# Import argparse for the dependency-free campaign CLI.
import argparse

# Import sys so repository-local source roots can be added to module search paths.
import sys

# Import Path for platform-independent repository path construction.
from pathlib import Path


# Resolve the repository root from this tools script location.
_REPOSITORY_ROOT = Path(__file__).resolve().parents[1]

# Resolve the Guardian Protocol Python package directory.
_PROTOCOL_SOURCE = _REPOSITORY_ROOT / "protocol" / "python"

# Resolve the Guardian simulator source directory.
_SIMULATOR_SOURCE = _REPOSITORY_ROOT / "simulator" / "src"

# Resolve the M11 robustness package directory.
_ROBUSTNESS_SOURCE = _REPOSITORY_ROOT / "fuzz" / "python"

# Add the protocol package before importing local modules.
sys.path.insert(
    0,
    str(_PROTOCOL_SOURCE),
)

# Add the simulator package before importing local modules.
sys.path.insert(
    0,
    str(_SIMULATOR_SOURCE),
)

# Add the M11 robustness package before importing its API.
sys.path.insert(
    0,
    str(_ROBUSTNESS_SOURCE),
)

# Import the campaign API only after repository-local paths are available.
from guardian_robustness import run_all_campaigns


# Parse command-line options.
def build_parser() -> argparse.ArgumentParser:
    """Return the M11 robustness campaign argument parser."""

    # Create the top-level parser.
    parser = argparse.ArgumentParser(
        description=(
            "Run deterministic Guardian parser, security and "
            "fault-injection robustness campaigns."
        )
    )

    # Configure reproducible campaign seed.
    parser.add_argument(
        "--seed",
        type=lambda value: int(value, 0),
        default=0xC0FFEE11,
        help=(
            "master integer seed; decimal or 0x-prefixed hexadecimal "
            "(default: 0xC0FFEE11)"
        ),
    )

    # Configure iteration count per mutation campaign.
    parser.add_argument(
        "--iterations",
        type=int,
        default=1000,
        help=(
            "iterations for parser and secure-tamper campaigns "
            "(default: 1000)"
        ),
    )

    # Return the configured parser.
    return parser


# Run the complete campaign.
def main(
    argv: list[str] | None = None,
) -> int:
    """Execute M11 robustness campaigns and print a reproducible summary."""

    # Parse caller arguments.
    args = build_parser().parse_args(
        argv
    )

    # Reject invalid campaign size before test execution.
    if args.iterations <= 0:

        # Ask argparse to report a conventional CLI configuration error.
        raise SystemExit(
            "--iterations must be positive"
        )

    # Execute every deterministic M11 campaign.
    report = run_all_campaigns(
        iterations=args.iterations,
        seed=args.seed,
    )

    # Print one stable success banner.
    print(
        "Guardian M11 robustness campaign: PASS"
    )

    # Print the exact master seed for reproduction.
    print(
        f"Seed: 0x{report.seed:08X}"
    )

    # Print the requested iteration count.
    print(
        f"Iterations per mutation campaign: {report.iterations}"
    )

    # Print parser mutation coverage.
    print(
        (
            "Parser recovery: "
            f"{report.parser_recoveries}/{report.parser_cases}"
        )
    )

    # Print secure-envelope fail-closed coverage.
    print(
        (
            "Secure tamper rejection: "
            f"{report.security_tamper_rejections}/"
            f"{report.security_tamper_cases}"
        )
    )

    # Print explicit replay rejection coverage.
    print(
        f"Exact replay rejections: {report.replay_rejections}"
    )

    # Print valid-MAC counter-gap coverage.
    print(
        f"Counter-gap rejections: {report.counter_gap_rejections}"
    )

    # Return conventional success.
    return 0


# Execute the campaign when run directly.
if __name__ == "__main__":

    # Exit using the campaign result.
    raise SystemExit(
        main()
    )
