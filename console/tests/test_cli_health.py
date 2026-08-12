"""End-to-end tests for guardianctl M8 baseline and health commands."""

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


# Verify complete CLI baseline and health behavior through real TCP framing.
class GuardianHealthCliTests(unittest.TestCase):
    """Exercise M8 through the public command-line interface."""

    # Start one ephemeral simulator before every test.
    def setUp(self) -> None:

        # Create independent simulated device state.
        self.device = GuardianDevice()

        # Bind a real loopback server to an operating-system-selected port.
        self.server = GuardianTcpServer(
            ("127.0.0.1", 0),
            self.device,
        )

        # Run the simulator request loop in a daemon thread.
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
        self.server_thread.join(timeout=2.0)

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

    # Verify baseline START followed by JSON health query.
    def test_baseline_start_and_health_json(self) -> None:

        # Execute explicit baseline start.
        exit_code, stdout, stderr = self._run_cli(
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

        # Require successful baseline command status.
        self.assertEqual(exit_code, 0)

        # Require no error output.
        self.assertEqual(stderr, "")

        # Require concise human acknowledgement.
        self.assertIn(
            "Baseline learning started",
            stdout,
        )

        # Query machine-readable health.
        exit_code, stdout, stderr = self._run_cli(
            [
                "--host",
                "127.0.0.1",
                "--port",
                str(self.port),
                "--json",
                "health",
            ]
        )

        # Require successful health command status.
        self.assertEqual(exit_code, 0)

        # Require no error output.
        self.assertEqual(stderr, "")

        # Decode the JSON object.
        health = json.loads(stdout)

        # Require simulator baseline completion.
        self.assertEqual(
            health["state"],
            "READY",
        )

        # Require the exact requested baseline sample count.
        self.assertEqual(
            health["baseline_samples"],
            16,
        )

        # Require neutral trained health.
        self.assertEqual(
            health["health_score"],
            1000,
        )


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
