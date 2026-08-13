"""Run Guardian M13 read-only physical hardware qualification."""

# Import argparse for the dependency-free command-line grammar.
import argparse

# Import sys so repository-local modules can be loaded.
import sys

# Import Path for repository/report paths.
from pathlib import Path


# Resolve the repository root from this tools script.
_REPOSITORY_ROOT = Path(__file__).resolve().parents[1]

# Resolve guardianctl source.
_CONSOLE_SOURCE = (
    _REPOSITORY_ROOT
    / "console"
    / "src"
)

# Add guardianctl source before importing the M13 validation engine.
sys.path.insert(
    0,
    str(
        _CONSOLE_SOURCE
    ),
)

# Import the read-only physical qualification engine.
from guardianctl.hardware_validation import (
    run_hardware_validation,
    write_hardware_validation_report,
)


# Build the M13 hardware-validation command-line parser.
def build_parser() -> argparse.ArgumentParser:
    """Return the M13 physical qualification argument parser."""

    # Create the top-level parser.
    parser = argparse.ArgumentParser(
        description=(
            "Run read-only Guardian STM32F401 hardware qualification "
            "over the physical serial transport."
        )
    )

    # Require the operating-system serial port.
    parser.add_argument(
        "--serial-port",
        required=True,
        help="physical UART port, for example COM5 or /dev/ttyUSB0",
    )

    # Configure the physical UART baud rate.
    parser.add_argument(
        "--baud",
        type=int,
        default=115200,
        help="physical UART baud rate (default: 115200)",
    )

    # Configure per-request guardianctl response timeout.
    parser.add_argument(
        "--timeout",
        type=float,
        default=3.0,
        help="guardianctl response timeout in seconds (default: 3.0)",
    )

    # Configure passive telemetry observation count.
    parser.add_argument(
        "--telemetry-count",
        type=int,
        default=3,
        help="number of passive telemetry samples (default: 3)",
    )

    # Configure passive telemetry period.
    parser.add_argument(
        "--telemetry-period-ms",
        type=int,
        default=500,
        help="telemetry observation period in milliseconds (default: 500)",
    )

    # Permit qualification without telemetry when analog hardware is intentionally absent.
    parser.add_argument(
        "--skip-telemetry",
        action="store_true",
        help="skip the passive live telemetry observation step",
    )

    # Configure the JSON report path.
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("hardware-validation.json"),
        help="JSON qualification report path (default: hardware-validation.json)",
    )

    # Return the complete parser.
    return parser


# Execute physical qualification.
def main(
    argv: list[str] | None = None,
) -> int:
    """Run read-only physical hardware qualification and persist its report."""

    # Parse explicit or process arguments.
    args = build_parser().parse_args(
        argv
    )

    # Run the complete read-only plan.
    try:

        # Execute physical validation over the public serial transport.
        report = run_hardware_validation(
            repository_root=_REPOSITORY_ROOT,
            serial_port=args.serial_port,
            baud_rate=args.baud,
            response_timeout_seconds=args.timeout,
            telemetry_count=args.telemetry_count,
            telemetry_period_ms=args.telemetry_period_ms,
            include_telemetry=(
                not args.skip_telemetry
            ),
        )
    except (
        FileNotFoundError,
        ValueError,
    ) as exc:

        # Print concise local configuration failure without traceback.
        print(
            f"hardware-validation: {exc}",
            file=sys.stderr,
        )

        # Return conventional CLI configuration failure.
        return 2

    # Persist the complete reproducible report.
    write_hardware_validation_report(
        report,
        args.output,
    )

    # Print one result line per read-only validation step.
    for result in report.results:

        # Select a concise terminal status.
        status = (
            "PASS"
            if result.passed
            else "FAIL"
        )

        # Print the stable step identifier and elapsed time.
        print(
            (
                f"[{status}] {result.name}: "
                f"{result.elapsed_ms:.1f} ms"
            )
        )

        # Print captured failure diagnostics only when a step failed.
        if not result.passed:

            # Print standard output when available.
            if result.stdout.strip():

                # Prefix captured output for diagnosis.
                print(
                    result.stdout.rstrip()
                )

            # Print standard error when available.
            if result.stderr.strip():

                # Preserve guardianctl error text.
                print(
                    result.stderr.rstrip(),
                    file=sys.stderr,
                )

    # Print the generated evidence artifact path.
    print(
        f"Report: {args.output}"
    )

    # Print the final qualification result.
    print(
        (
            "Hardware qualification: PASS"
            if report.passed
            else "Hardware qualification: FAIL"
        )
    )

    # Return failure when any physical validation step failed.
    return (
        0
        if report.passed
        else 1
    )


# Execute the tool when run directly.
if __name__ == "__main__":

    # Exit using the qualification result.
    raise SystemExit(
        main()
    )
