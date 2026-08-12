"""End-to-end tests for the guardianctl telemetry command."""

# Import io for in-memory terminal capture.
import io

# Import json for JSON Lines validation.
import json

# Import threading so the simulator can run during the CLI test.
import threading

# Import unittest from the Python standard library.
import unittest

# Import stdout and stderr redirection helpers.
from contextlib import redirect_stderr, redirect_stdout

# Import the guardianctl CLI boundary.
from guardianctl.cli import main

# Import the real Guardian simulator endpoint.
from guardian_sim import GuardianDevice, GuardianTcpServer


# Verify the complete CLI-to-simulator asynchronous telemetry path.
class GuardianTelemetryCliTests(unittest.TestCase):
    """Exercise live JSON telemetry through the public CLI boundary."""

    # Start one ephemeral simulator before every test.
    def setUp(self) -> None:

        # Create one independent simulated Guardian device.
        self.device = GuardianDevice()

        # Bind the simulator to a free loopback TCP port.
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

        # Preserve the operating-system-selected test port.
        _, self.port = self.server.server_address

    # Stop the simulator after every test.
    def tearDown(self) -> None:

        # Stop the server request loop.
        self.server.shutdown()

        # Close the listening socket.
        self.server.server_close()

        # Wait briefly for background cleanup.
        self.server_thread.join(timeout=2.0)

    # Verify two JSON Lines samples are printed live.
    def test_telemetry_json_command(self) -> None:

        # Create an in-memory stdout buffer.
        stdout_buffer = io.StringIO()

        # Create an in-memory stderr buffer.
        stderr_buffer = io.StringIO()

        # Capture both terminal streams during command execution.
        with redirect_stdout(stdout_buffer), redirect_stderr(stderr_buffer):

            # Execute the public CLI telemetry command.
            exit_code = main(
                [
                    "--host",
                    "127.0.0.1",
                    "--port",
                    str(self.port),
                    "--json",
                    "telemetry",
                    "--period-ms",
                    "100",
                    "--count",
                    "2",
                ]
            )

        # Require successful process status.
        self.assertEqual(exit_code, 0)

        # Require no operator-facing error output.
        self.assertEqual(stderr_buffer.getvalue(), "")

        # Split the streaming JSON Lines output.
        lines = [
            line
            for line in stdout_buffer.getvalue().splitlines()
            if line
        ]

        # Require exactly the requested sample count.
        self.assertEqual(len(lines), 2)

        # Decode the first live JSON sample.
        first = json.loads(lines[0])

        # Decode the second live JSON sample.
        second = json.loads(lines[1])

        # Require independent telemetry sequence progression.
        self.assertEqual(
            [first["sequence"], second["sequence"]],
            [1, 2],
        )

        # Require deterministic synthetic simulator supply voltage.
        self.assertEqual(first["supply_mv"], 3300)


# Execute the tests when this file is run directly.
if __name__ == "__main__":

    # Start the standard-library test runner.
    unittest.main(verbosity=2)
