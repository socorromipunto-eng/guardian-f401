"""End-to-end tests for guardianctl M9 supervisory-control commands."""

# Import io for in-memory terminal capture.
import io

# Import json for machine-readable output validation.
import json

# Import threading so the simulator can run during CLI execution.
import threading

# Import unittest from the Python standard library.
import unittest

# Import stdout and stderr redirection helpers.
from contextlib import redirect_stderr, redirect_stdout

# Import the public guardianctl CLI boundary.
from guardianctl.cli import main

# Import the real simulator endpoint.
from guardian_sim import GuardianDevice, GuardianTcpServer


# Verify M9 behavior through the public command-line interface.
class GuardianControlCliTests(unittest.TestCase):
    """Exercise baseline, ARM and control status through real TCP framing."""

    # Start one ephemeral simulator before every test.
    def setUp(self) -> None:

        # Create independent simulated device state.
        self.device = GuardianDevice()

        # Bind a real loopback server to an operating-system-selected port.
        self.server = GuardianTcpServer(
            ("127.0.0.1", 0),
            self.device,
        )

        # Run the request loop in a daemon thread.
        self.server_thread = threading.Thread(
            target=self.server.serve_forever,
            daemon=True,
        )

        # Start accepting CLI connections.
        self.server_thread.start()

        # Preserve the selected port.
        _, self.port = self.server.server_address

    # Stop the simulator after every test.
    def tearDown(self) -> None:

        # Stop the request loop.
        self.server.shutdown()

        # Close the listening socket.
        self.server.server_close()

        # Wait briefly for background cleanup.
        self.server_thread.join(
            timeout=2.0
        )

    # Execute one CLI invocation with captured terminal streams.
    def _run_cli(
        self,
        arguments: list[str],
    ) -> tuple[int, str, str]:
        """Return exit code, stdout and stderr for one CLI invocation."""

        # Create an in-memory stdout buffer.
        stdout_buffer = io.StringIO()

        # Create an in-memory stderr buffer.
        stderr_buffer = io.StringIO()

        # Capture both terminal streams.
        with redirect_stdout(stdout_buffer), redirect_stderr(stderr_buffer):

            # Execute the public CLI.
            exit_code = main(arguments)

        # Return captured process-like results.
        return (
            exit_code,
            stdout_buffer.getvalue(),
            stderr_buffer.getvalue(),
        )

    # Verify baseline, ARM and JSON control status.
    def test_baseline_arm_and_control_status_json(self) -> None:

        # Start a complete deterministic simulator baseline.
        exit_code, _, stderr = self._run_cli(
            [
                "--host",
                "127.0.0.1",
                "--port",
                str(self.port),
                "baseline",
                "start",
                "--samples",
                "16",
            ]
        )

        # Require baseline success.
        self.assertEqual(exit_code, 0)

        # Require no baseline error output.
        self.assertEqual(stderr, "")

        # Arm M9 supervision.
        exit_code, stdout, stderr = self._run_cli(
            [
                "--host",
                "127.0.0.1",
                "--port",
                str(self.port),
                "control",
                "arm",
            ]
        )

        # Require ARM success.
        self.assertEqual(exit_code, 0)

        # Require no ARM error output.
        self.assertEqual(stderr, "")

        # Require the human result to expose ARMED.
        self.assertIn(
            "Control state: ARMED",
            stdout,
        )

        # Query machine-readable control status.
        exit_code, stdout, stderr = self._run_cli(
            [
                "--host",
                "127.0.0.1",
                "--port",
                str(self.port),
                "--json",
                "control",
                "status",
            ]
        )

        # Require status success.
        self.assertEqual(exit_code, 0)

        # Require no status error output.
        self.assertEqual(stderr, "")

        # Decode the JSON status object.
        status = json.loads(stdout)

        # Require ARMED state.
        self.assertEqual(
            status["state"],
            "ARMED",
        )

        # Require remote ARM to remain safe-off.
        self.assertFalse(
            status["run_permit"]
        )

        # Require the local interlock to be closed in the simulator demo.
        self.assertTrue(
            status["interlock_closed"]
        )


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
