"""Integration tests for guardianctl M8 baseline and health operations."""

# Import threading so the simulator can run during each test.
import threading

# Import unittest from the Python standard library.
import unittest

# Import typed M8 protocol states.
from guardian_protocol import (
    BaselineAction,
    HealthState,
)

# Import high-level host components.
from guardianctl import (
    ClientConfig,
    GuardianClient,
    GuardianTcpTransport,
)

# Import the real loopback simulator endpoint.
from guardian_sim import GuardianDevice, GuardianTcpServer


# Verify host M8 operations through actual framed TCP exchanges.
class GuardianHealthClientTests(unittest.TestCase):
    """Exercise baseline and health commands end to end."""

    # Start one ephemeral simulator before every test.
    def setUp(self) -> None:

        # Create independent simulated device state.
        self.device = GuardianDevice()

        # Bind a real loopback server to an operating-system-selected port.
        self.server = GuardianTcpServer(
            ("127.0.0.1", 0),
            self.device,
        )

        # Run the simulator in a daemon thread.
        self.server_thread = threading.Thread(
            target=self.server.serve_forever,
            daemon=True,
        )

        # Start accepting host connections.
        self.server_thread.start()

        # Read the actual bound endpoint.
        host, port = self.server.server_address

        # Configure the real host TCP transport.
        config = ClientConfig(
            host=host,
            port=port,
            timeout_seconds=1.0,
        )

        # Create the typed high-level client.
        self.client = GuardianClient(
            transport=GuardianTcpTransport(
                config
            )
        )

    # Stop the simulator after every test.
    def tearDown(self) -> None:

        # Stop the server request loop.
        self.server.shutdown()

        # Close the listening socket.
        self.server.server_close()

        # Wait briefly for background cleanup.
        self.server_thread.join(
            timeout=2.0
        )

    # Verify baseline START and health query through separate TCP exchanges.
    def test_start_baseline_and_read_health(self) -> None:

        # Start a bounded simulator baseline.
        control = self.client.start_baseline(
            16
        )

        # Require normalized START acknowledgement.
        self.assertEqual(
            control.action,
            BaselineAction.START,
        )

        # Query the trained health model.
        health = self.client.health_status()

        # Require deterministic simulator baseline completion.
        self.assertEqual(
            health.state,
            HealthState.READY,
        )

        # Require the exact accepted baseline count.
        self.assertEqual(
            health.baseline_samples,
            16,
        )

        # Require a neutral trained health score.
        self.assertEqual(
            health.health_score,
            1000,
        )

    # Verify host-side baseline target validation occurs before transport.
    def test_rejects_small_baseline_target(self) -> None:

        # Require the shared codec to reject an unsafe tiny baseline.
        with self.assertRaises(ValueError):

            # Attempt a target below the published minimum.
            self.client.start_baseline(
                15
            )

    # Verify RESET returns the device to UNTRAINED.
    def test_reset_baseline(self) -> None:

        # Start one complete baseline.
        self.client.start_baseline(
            16
        )

        # Reset the runtime model.
        control = self.client.reset_baseline()

        # Require normalized RESET acknowledgement.
        self.assertEqual(
            control.action,
            BaselineAction.RESET,
        )

        # Query post-reset state.
        health = self.client.health_status()

        # Require explicit UNTRAINED state.
        self.assertEqual(
            health.state,
            HealthState.UNTRAINED,
        )


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
