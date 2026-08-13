"""Tests for Guardian M13 physical hardware qualification planning and reporting."""

# Import json for persisted report validation.
import json

# Import tempfile for disposable report storage.
import tempfile

# Import unittest from the standard library.
import unittest

# Import mock helpers for process isolation.
from unittest.mock import patch

# Import Path for repository/report fixtures.
from pathlib import Path

# Import subprocess for CompletedProcess fixtures.
import subprocess

# Import the real guardianctl command grammar.
from guardianctl.cli import build_parser as build_guardianctl_parser

# Import the public M13 hardware-validation API.
from guardianctl.hardware_validation import (
    build_guardianctl_command,
    build_validation_steps,
    run_hardware_validation,
    write_hardware_validation_report,
)


# Verify M13 never issues machine-changing commands during default qualification.
class HardwareValidationTests(unittest.TestCase):
    """Exercise safe read-only plan construction and report persistence."""

    # Preserve the repository root resolved from this test file.
    REPOSITORY_ROOT = Path(__file__).resolve().parents[2]

    # Verify the plan excludes privileged or active control operations.
    def test_validation_plan_is_read_only(self) -> None:

        # Build the default physical qualification plan.
        steps = build_validation_steps(
            command_timeout_seconds=3.0,
            telemetry_count=3,
            telemetry_period_ms=500,
        )

        # Join arguments for simple forbidden-action assertions.
        commands = [
            " ".join(
                step.arguments
            )
            for step in steps
        ]

        # Define machine-mutating commands forbidden from the default plan.
        forbidden = (
            "baseline start",
            "baseline reset",
            "control arm",
            "control disarm",
            "control clear-fault",
            "security authenticate",
            "firmware upload",
        )

        # Require every forbidden operation to be absent.
        for operation in forbidden:

            # Check the complete immutable command plan.
            self.assertTrue(
                all(
                    operation not in command
                    for command in commands
                )
            )

    # Verify every generated step is accepted by the real guardianctl argument grammar.
    def test_every_validation_step_matches_guardianctl_cli(self) -> None:

        # Build the real public guardianctl parser.
        parser = build_guardianctl_parser()

        # Build the default physical qualification plan.
        steps = build_validation_steps(
            command_timeout_seconds=3.0,
            telemetry_count=3,
            telemetry_period_ms=500,
        )

        # Validate every generated subcommand against the public grammar.
        for step in steps:

            # Parse the exact global serial options and generated step arguments.
            namespace = parser.parse_args(
                [
                    "--serial-port",
                    "COM5",
                    "--baud",
                    "115200",
                    "--timeout",
                    "3.0",
                    "--json",
                    *step.arguments,
                ]
            )

            # Require the parser to identify one top-level command.
            self.assertIsNotNone(
                namespace.command
            )

    # Verify global serial options appear before the guardianctl subcommand.
    def test_guardianctl_command_orders_global_options_first(self) -> None:

        # Build one PING validation step.
        step = build_validation_steps(
            command_timeout_seconds=3.0,
            telemetry_count=1,
            telemetry_period_ms=500,
            include_telemetry=False,
        )[0]

        # Build the exact public guardianctl command.
        command = build_guardianctl_command(
            repository_root=self.REPOSITORY_ROOT,
            serial_port="COM5",
            baud_rate=115200,
            response_timeout_seconds=3.0,
            step=step,
        )

        # Locate the JSON global option.
        json_index = command.index(
            "--json"
        )

        # Locate the PING subcommand.
        ping_index = command.index(
            "ping"
        )

        # Require every global option to precede the subcommand.
        self.assertLess(
            json_index,
            ping_index,
        )

    # Verify successful isolated command results produce a passing report.
    @patch(
        "guardianctl.hardware_validation.subprocess.run"
    )
    def test_successful_commands_produce_passing_report(
        self,
        run_mock,
    ) -> None:

        # Return one successful guardianctl process result for every plan step.
        run_mock.return_value = subprocess.CompletedProcess(
            args=("guardianctl",),
            returncode=0,
            stdout='{"ok": true}\n',
            stderr="",
        )

        # Execute the read-only plan without telemetry to keep the test compact.
        report = run_hardware_validation(
            repository_root=self.REPOSITORY_ROOT,
            serial_port="COM5",
            include_telemetry=False,
        )

        # Require complete qualification success.
        self.assertTrue(
            report.passed
        )

        # Require every individual result to pass.
        self.assertTrue(
            all(
                result.passed
                for result in report.results
            )
        )

        # Require one process execution per immutable plan step.
        self.assertEqual(
            run_mock.call_count,
            len(
                report.results
            ),
        )

    # Verify a failed command is preserved in the final report.
    @patch(
        "guardianctl.hardware_validation.subprocess.run"
    )
    def test_failed_command_fails_complete_report(
        self,
        run_mock,
    ) -> None:

        # Return one ordinary guardianctl remote failure.
        run_mock.return_value = subprocess.CompletedProcess(
            args=("guardianctl",),
            returncode=1,
            stdout="",
            stderr="guardianctl: timeout\n",
        )

        # Execute the compact read-only plan.
        report = run_hardware_validation(
            repository_root=self.REPOSITORY_ROOT,
            serial_port="COM5",
            include_telemetry=False,
        )

        # Require complete qualification failure.
        self.assertFalse(
            report.passed
        )

        # Require every recorded result to preserve the failing return code.
        self.assertTrue(
            all(
                result.return_code == 1
                for result in report.results
            )
        )

    # Verify the JSON evidence artifact is stable and parseable.
    @patch(
        "guardianctl.hardware_validation.subprocess.run"
    )
    def test_report_writes_valid_json(
        self,
        run_mock,
    ) -> None:

        # Return successful guardianctl output for every step.
        run_mock.return_value = subprocess.CompletedProcess(
            args=("guardianctl",),
            returncode=0,
            stdout='{"ok": true}\n',
            stderr="",
        )

        # Execute the compact read-only plan.
        report = run_hardware_validation(
            repository_root=self.REPOSITORY_ROOT,
            serial_port="COM5",
            include_telemetry=False,
        )

        # Create one disposable output directory.
        with tempfile.TemporaryDirectory(
            prefix="guardian-m13-report-"
        ) as temporary_directory:

            # Define the report artifact path.
            output_path = (
                Path(temporary_directory)
                / "report.json"
            )

            # Persist the qualification artifact.
            write_hardware_validation_report(
                report,
                output_path,
            )

            # Parse the complete JSON document.
            payload = json.loads(
                output_path.read_text(
                    encoding="utf-8"
                )
            )

        # Require the frozen report schema.
        self.assertEqual(
            payload["schema_version"],
            1,
        )

        # Require selected physical port preservation.
        self.assertEqual(
            payload["serial_port"],
            "COM5",
        )

        # Require complete pass status.
        self.assertTrue(
            payload["passed"]
        )

        # Require at least one recorded qualification result.
        self.assertGreater(
            len(
                payload["results"]
            ),
            0,
        )


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
