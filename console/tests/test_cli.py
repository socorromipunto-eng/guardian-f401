"""End-to-end tests for the guardianctl CLI boundary."""

# Import io for in-memory stdout/stderr capture.
import io

# Import json to validate machine-readable command output.
import json

# Import threading so the simulator can run during each CLI test.
import threading

# Import unittest from the Python standard library.
import unittest

# Import redirect helpers for stable CLI output assertions.
from contextlib import redirect_stderr, redirect_stdout

# Import the CLI boundary under test.
from guardianctl.cli import main

# Import the real M2 simulator endpoint used by CLI integration tests.
from guardian_sim import GuardianDevice, GuardianTcpServer


# Verify guardianctl command parsing, network exchange and presentation together.
class GuardianCliTests(unittest.TestCase):
    """Exercise the complete M3 CLI against the M2 simulator."""

    # Start one fresh loopback simulator before every test.
    def setUp(self) -> None:

        # Create an independent simulated device.
        self.device = GuardianDevice()

        # Bind the simulator to an operating-system-selected local port.
        self.server = GuardianTcpServer(
            ("127.0.0.1", 0),
            self.device,
        )

        # Run the simulator in a background daemon thread.
        self.server_thread = threading.Thread(
            target=self.server.serve_forever,
            daemon=True,
        )

        # Start accepting test connections.
        self.server_thread.start()

        # Preserve the actual ephemeral port for CLI arguments.
        _, self.port = self.server.server_address

    # Stop the loopback simulator after every test.
    def tearDown(self) -> None:

        # Stop the request loop.
        self.server.shutdown()

        # Close the listening socket.
        self.server.server_close()

        # Wait briefly for background cleanup.
        self.server_thread.join(timeout=2.0)

    # Execute one CLI command while capturing its stdout and stderr streams.
    def _run_cli(self, *arguments: str) -> tuple[int, str, str]:

        # Create an in-memory stdout buffer.
        stdout_buffer = io.StringIO()

        # Create an in-memory stderr buffer.
        stderr_buffer = io.StringIO()

        # Redirect both terminal streams during CLI execution.
        with redirect_stdout(stdout_buffer), redirect_stderr(stderr_buffer):

            # Execute guardianctl against the ephemeral simulator endpoint.
            exit_code = main(
                [
                    "--host",
                    "127.0.0.1",
                    "--port",
                    str(self.port),
                    *arguments,
                ]
            )

        # Return process status plus captured output streams.
        return (
            exit_code,
            stdout_buffer.getvalue(),
            stderr_buffer.getvalue(),
        )

    # Verify human-readable PING output.
    def test_ping_command(self) -> None:

        # Execute the real PING CLI path.
        exit_code, stdout, stderr = self._run_cli("ping")

        # Require successful process status.
        self.assertEqual(exit_code, 0)

        # Require no error output.
        self.assertEqual(stderr, "")

        # Require the frozen PONG response in human output.
        self.assertIn("Reply: PONG", stdout)

    # Verify machine-readable device information.
    def test_info_json_command(self) -> None:

        # Execute the real metadata CLI path in JSON mode.
        exit_code, stdout, stderr = self._run_cli("--json", "info")

        # Require successful process status.
        self.assertEqual(exit_code, 0)

        # Require no error output.
        self.assertEqual(stderr, "")

        # Decode the generated machine-readable output.
        payload = json.loads(stdout)

        # Require the expected simulator identity.
        self.assertEqual(payload["model"], "Guardian-F401-SIM")

    # Verify machine-readable runtime status.
    def test_status_json_command(self) -> None:

        # Execute the real status CLI path in JSON mode.
        exit_code, stdout, stderr = self._run_cli("--json", "status")

        # Require successful process status.
        self.assertEqual(exit_code, 0)

        # Require no error output.
        self.assertEqual(stderr, "")

        # Decode the generated machine-readable output.
        payload = json.loads(stdout)

        # Require the initialized device state.
        self.assertEqual(payload["state"], "IDLE")


# Execute the tests when this file is run directly.
if __name__ == "__main__":

    # Start the standard-library unit test runner with verbose output.
    unittest.main(verbosity=2)
