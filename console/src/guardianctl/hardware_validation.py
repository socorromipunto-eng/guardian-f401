"""Guardian M13 read-only physical hardware qualification runner."""

# Import json for machine-readable qualification artifacts.
import json

# Import platform for host-environment metadata.
import platform

# Import subprocess for isolated guardianctl command execution.
import subprocess

# Import sys for the active Python executable and version metadata.
import sys

# Import time for monotonic step timing and UTC report timestamps.
import time

# Import dataclass for immutable qualification plan/results.
from dataclasses import asdict, dataclass

# Import datetime for ISO-8601 UTC report metadata.
from datetime import datetime, timezone

# Import Path for repository and report paths.
from pathlib import Path

# Import Sequence for typed command arguments.
from collections.abc import Sequence


# Store one read-only physical validation command.
@dataclass(frozen=True, slots=True)
class HardwareValidationStep:
    """One guardianctl command in the M13 read-only hardware qualification plan."""

    # Store a stable machine-readable step identifier.
    name: str

    # Store a short operator-facing purpose.
    description: str

    # Store arguments appended after the guardianctl global serial options.
    arguments: tuple[str, ...]

    # Store the complete process timeout for this step.
    timeout_seconds: float


# Store one executed validation result.
@dataclass(frozen=True, slots=True)
class HardwareValidationResult:
    """One completed M13 hardware qualification step."""

    # Store the stable plan identifier.
    name: str

    # Store the purpose copied from the plan.
    description: str

    # Store the exact process command for reproduction.
    command: tuple[str, ...]

    # Store conventional process return status.
    return_code: int

    # Store elapsed monotonic time in milliseconds.
    elapsed_ms: float

    # Store captured standard output.
    stdout: str

    # Store captured standard error.
    stderr: str

    # Store whether the step passed.
    passed: bool

    # Store whether the process exceeded its bounded timeout.
    timed_out: bool


# Store one complete physical hardware qualification artifact.
@dataclass(frozen=True, slots=True)
class HardwareValidationReport:
    """Complete M13 read-only physical hardware qualification report."""

    # Store report schema revision.
    schema_version: int

    # Store UTC generation timestamp.
    generated_at_utc: str

    # Store selected physical serial device.
    serial_port: str

    # Store selected physical UART baud rate.
    baud_rate: int

    # Store host operating-system metadata.
    host_platform: str

    # Store active Python runtime version.
    python_version: str

    # Store exact repository path used for guardianctl.
    repository_root: str

    # Store whether every executed read-only step passed.
    passed: bool

    # Store every executed step result in plan order.
    results: tuple[HardwareValidationResult, ...]


# Build the immutable read-only M13 validation plan.
def build_validation_steps(
    command_timeout_seconds: float,
    telemetry_count: int,
    telemetry_period_ms: int,
    include_telemetry: bool = True,
) -> tuple[HardwareValidationStep, ...]:
    """Return the fixed M13 read-only hardware qualification plan."""

    # Require a positive process/response timeout.
    if command_timeout_seconds <= 0.0:

        # Reject invalid qualification configuration.
        raise ValueError(
            "command_timeout_seconds must be positive"
        )

    # Require a positive bounded telemetry sample count.
    if not 1 <= telemetry_count <= 100:

        # Reject unbounded physical test duration.
        raise ValueError(
            "telemetry_count must be between 1 and 100"
        )

    # Match the M5 published telemetry rate bounds.
    if not 100 <= telemetry_period_ms <= 60000:

        # Reject unsupported device-side telemetry periods.
        raise ValueError(
            "telemetry_period_ms must be between 100 and 60000"
        )

    # Preserve one common timeout margin for ordinary request/response operations.
    ordinary_timeout = (
        command_timeout_seconds
        + 2.0
    )

    # Calculate a bounded telemetry process timeout from requested stream duration.
    telemetry_timeout = (
        command_timeout_seconds
        + (
            telemetry_count *
            telemetry_period_ms /
            1000.0
        )
        + 3.0
    )

    # Define safe read-only and passive-observation steps.
    steps = [
        HardwareValidationStep(
            name="ping",
            description="Verify physical UART framing and request/response correlation.",
            arguments=("ping",),
            timeout_seconds=ordinary_timeout,
        ),
        HardwareValidationStep(
            name="device_info",
            description="Read hardware identity and firmware version 0.13.x.",
            arguments=("info",),
            timeout_seconds=ordinary_timeout,
        ),
        HardwareValidationStep(
            name="device_status",
            description="Read runtime state and protocol counters.",
            arguments=("status",),
            timeout_seconds=ordinary_timeout,
        ),
        HardwareValidationStep(
            name="security_status",
            description="Read public authenticated-session provisioning diagnostics.",
            arguments=("security", "status"),
            timeout_seconds=ordinary_timeout,
        ),
        HardwareValidationStep(
            name="firmware_status",
            description="Read public signed-firmware lifecycle diagnostics.",
            arguments=("firmware", "status"),
            timeout_seconds=ordinary_timeout,
        ),
        HardwareValidationStep(
            name="control_status",
            description="Read supervisory-control state without issuing a control action.",
            arguments=("control", "status"),
            timeout_seconds=ordinary_timeout,
        ),
    ]

    # Add passive live telemetry before DSP/health reads so acquisition has observation time.
    if include_telemetry:

        # Append one bounded telemetry observation step.
        steps.append(
            HardwareValidationStep(
                name="telemetry",
                description="Observe bounded live acquisition telemetry without machine-control actions.",
                arguments=(
                    "telemetry",
                    "--period-ms",
                    str(
                        telemetry_period_ms
                    ),
                    "--count",
                    str(
                        telemetry_count
                    ),
                ),
                timeout_seconds=telemetry_timeout,
            )
        )

    # Read the latest DSP feature snapshot after acquisition observation.
    steps.append(
        HardwareValidationStep(
            name="dsp",
            description="Read the latest physical acquisition DSP feature snapshot.",
            arguments=("dsp",),
            timeout_seconds=ordinary_timeout,
        )
    )

    # Read the current machine-health model without baseline mutation.
    steps.append(
        HardwareValidationStep(
            name="health",
            description="Read the machine-health snapshot without starting or resetting a baseline.",
            arguments=("health",),
            timeout_seconds=ordinary_timeout,
        )
    )

    # Return an immutable plan.
    return tuple(
        steps
    )


# Build one exact guardianctl process command.
def build_guardianctl_command(
    repository_root: Path,
    serial_port: str,
    baud_rate: int,
    response_timeout_seconds: float,
    step: HardwareValidationStep,
) -> tuple[str, ...]:
    """Return the exact guardianctl process command for one validation step."""

    # Require a non-empty serial-device path.
    if not serial_port.strip():

        # Reject ambiguous physical endpoint selection.
        raise ValueError(
            "serial_port cannot be empty"
        )

    # Require a conventional positive UART baud rate.
    if baud_rate <= 0:

        # Reject invalid physical UART configuration.
        raise ValueError(
            "baud_rate must be positive"
        )

    # Resolve the repository-local public guardianctl runner.
    runner = (
        repository_root
        / "tools"
        / "guardianctl.py"
    )

    # Require the public runner before starting physical qualification.
    if not runner.is_file():

        # Report repository-layout failure clearly.
        raise FileNotFoundError(
            f"guardianctl runner not found: {runner}"
        )

    # Return the exact global options before the selected subcommand.
    return (
        sys.executable,
        str(
            runner
        ),
        "--serial-port",
        serial_port,
        "--baud",
        str(
            baud_rate
        ),
        "--timeout",
        str(
            response_timeout_seconds
        ),
        "--json",
        *step.arguments,
    )


# Execute one isolated validation step.
def run_validation_step(
    command: Sequence[str],
    step: HardwareValidationStep,
) -> HardwareValidationResult:
    """Execute one guardianctl validation command with captured output."""

    # Snapshot monotonic start time.
    started = time.perf_counter()

    # Execute the command with a hard process timeout.
    try:

        # Run without shell interpretation so port/path contents cannot become shell syntax.
        completed = subprocess.run(
            tuple(
                command
            ),
            text=True,
            capture_output=True,
            timeout=step.timeout_seconds,
            check=False,
        )
    except subprocess.TimeoutExpired as exc:

        # Calculate elapsed wall duration for the timed-out process.
        elapsed_ms = (
            time.perf_counter() -
            started
        ) * 1000.0

        # Normalize partial standard output into text.
        stdout = (
            exc.stdout.decode(
                errors="replace"
            )
            if isinstance(
                exc.stdout,
                bytes,
            )
            else (
                exc.stdout
                or ""
            )
        )

        # Normalize partial standard error into text.
        stderr = (
            exc.stderr.decode(
                errors="replace"
            )
            if isinstance(
                exc.stderr,
                bytes,
            )
            else (
                exc.stderr
                or ""
            )
        )

        # Return an explicit timeout failure result.
        return HardwareValidationResult(
            name=step.name,
            description=step.description,
            command=tuple(
                command
            ),
            return_code=124,
            elapsed_ms=elapsed_ms,
            stdout=stdout,
            stderr=stderr,
            passed=False,
            timed_out=True,
        )

    # Calculate elapsed process duration.
    elapsed_ms = (
        time.perf_counter() -
        started
    ) * 1000.0

    # Return the complete process result.
    return HardwareValidationResult(
        name=step.name,
        description=step.description,
        command=tuple(
            command
        ),
        return_code=completed.returncode,
        elapsed_ms=elapsed_ms,
        stdout=completed.stdout,
        stderr=completed.stderr,
        passed=(
            completed.returncode == 0
        ),
        timed_out=False,
    )


# Execute the complete M13 physical qualification plan.
def run_hardware_validation(
    repository_root: Path,
    serial_port: str,
    baud_rate: int = 115200,
    response_timeout_seconds: float = 3.0,
    telemetry_count: int = 3,
    telemetry_period_ms: int = 500,
    include_telemetry: bool = True,
) -> HardwareValidationReport:
    """Execute the complete read-only M13 physical UART qualification plan."""

    # Resolve the repository path for stable report metadata.
    resolved_root = repository_root.resolve()

    # Build the immutable read-only plan.
    steps = build_validation_steps(
        command_timeout_seconds=response_timeout_seconds,
        telemetry_count=telemetry_count,
        telemetry_period_ms=telemetry_period_ms,
        include_telemetry=include_telemetry,
    )

    # Collect results in exact execution order.
    results: list[
        HardwareValidationResult
    ] = []

    # Execute every plan step independently.
    for step in steps:

        # Build the exact public guardianctl command.
        command = build_guardianctl_command(
            repository_root=resolved_root,
            serial_port=serial_port,
            baud_rate=baud_rate,
            response_timeout_seconds=response_timeout_seconds,
            step=step,
        )

        # Execute and record the isolated result.
        results.append(
            run_validation_step(
                command,
                step,
            )
        )

    # Determine complete qualification outcome.
    passed = all(
        result.passed
        for result in results
    )

    # Return one immutable report artifact.
    return HardwareValidationReport(
        schema_version=1,
        generated_at_utc=datetime.now(
            timezone.utc
        ).isoformat(),
        serial_port=serial_port,
        baud_rate=baud_rate,
        host_platform=platform.platform(),
        python_version=platform.python_version(),
        repository_root=str(
            resolved_root
        ),
        passed=passed,
        results=tuple(
            results
        ),
    )


# Write one report with stable indentation and key ordering.
def write_hardware_validation_report(
    report: HardwareValidationReport,
    output_path: Path,
) -> None:
    """Write one M13 hardware qualification JSON artifact."""

    # Resolve parent storage.
    parent = output_path.parent

    # Create missing report directories.
    parent.mkdir(
        parents=True,
        exist_ok=True,
    )

    # Convert nested dataclasses into JSON-compatible dictionaries/lists.
    payload = asdict(
        report
    )

    # Write stable UTF-8 JSON with one final newline.
    output_path.write_text(
        json.dumps(
            payload,
            indent=2,
            sort_keys=True,
        )
        + "\n",
        encoding="utf-8",
    )
