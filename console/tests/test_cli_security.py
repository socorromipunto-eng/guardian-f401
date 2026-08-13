"""End-to-end tests for guardianctl M10 security CLI behavior."""

# Import io for in-memory terminal capture.
import io

# Import json for machine-readable status validation.
import json

# Import threading so the simulator can run during CLI execution.
import threading

# Import unittest from the Python standard library.
import unittest

# Import stdout/stderr redirection.
from contextlib import redirect_stderr, redirect_stdout

# Import the public guardianctl CLI boundary.
from guardianctl.cli import main

# Import the real secure-mode simulator endpoint.
from guardian_sim import GuardianDevice, GuardianTcpServer

# Import the public simulator demo key.
from guardian_sim.config import (
    DEFAULT_SECURITY_PSK_HEX,
    SimulatorConfig,
)


# Verify M10 through the public command-line interface.
class GuardianSecurityCliTests(unittest.TestCase):
    """Exercise authentication and protected baseline commands over real TCP."""

    # Start one secure-mode simulator before every test.
    def setUp(self) -> None:

        # Create secure simulator device.
        self.device = GuardianDevice(
            SimulatorConfig(
                security_enabled=True,
            )
        )

        # Bind to an ephemeral loopback port.
        self.server = GuardianTcpServer(
            ("127.0.0.1", 0),
            self.device,
        )

        # Run server in the background.
        self.server_thread = threading.Thread(
            target=self.server.serve_forever,
            daemon=True,
        )

        # Start accepting connections.
        self.server_thread.start()

        # Preserve selected port.
        _, self.port = self.server.server_address

    # Stop simulator after every test.
    def tearDown(self) -> None:

        # Stop server loop.
        self.server.shutdown()

        # Close listener.
        self.server.server_close()

        # Join background thread.
        self.server_thread.join(
            timeout=2.0
        )

    # Execute one CLI call with captured streams.
    def _run_cli(
        self,
        arguments: list[str],
    ) -> tuple[int, str, str]:
        """Return exit code, stdout and stderr."""

        # Create output buffers.
        stdout_buffer = io.StringIO()
        stderr_buffer = io.StringIO()

        # Capture terminal output.
        with redirect_stdout(stdout_buffer), redirect_stderr(stderr_buffer):

            # Execute CLI.
            exit_code = main(arguments)

        # Return process-like output.
        return (
            exit_code,
            stdout_buffer.getvalue(),
            stderr_buffer.getvalue(),
        )

    # Verify explicit authentication and protected baseline from CLI.
    def test_authenticate_and_secure_baseline(self) -> None:

        # Execute an explicit authentication probe.
        exit_code, stdout, stderr = self._run_cli(
            [
                "--host",
                "127.0.0.1",
                "--port",
                str(self.port),
                "--psk-hex",
                DEFAULT_SECURITY_PSK_HEX,
                "security",
                "authenticate",
            ]
        )

        # Require successful authentication.
        self.assertEqual(exit_code, 0)
        self.assertEqual(stderr, "")
        self.assertIn(
            "Authentication: SUCCESS",
            stdout,
        )

        # Execute protected baseline start in a new CLI process-like invocation.
        exit_code, stdout, stderr = self._run_cli(
            [
                "--host",
                "127.0.0.1",
                "--port",
                str(self.port),
                "--psk-hex",
                DEFAULT_SECURITY_PSK_HEX,
                "baseline",
                "start",
                "--samples",
                "16",
            ]
        )

        # Require protected baseline success.
        self.assertEqual(exit_code, 0)
        self.assertEqual(stderr, "")
        self.assertIn(
            "Baseline learning started",
            stdout,
        )

        # Read public JSON security diagnostics.
        exit_code, stdout, stderr = self._run_cli(
            [
                "--host",
                "127.0.0.1",
                "--port",
                str(self.port),
                "--json",
                "security",
                "status",
            ]
        )

        # Require status success.
        self.assertEqual(exit_code, 0)
        self.assertEqual(stderr, "")

        # Decode diagnostics.
        status = json.loads(stdout)

        # Require at least two completed authentication handshakes.
        self.assertGreaterEqual(
            status["auth_successes"],
            2,
        )


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
