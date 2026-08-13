"""Integration tests for guardianctl M9 supervisory-control operations."""

# Import threading so the simulator can run during each test.
import threading

# Import unittest from the Python standard library.
import unittest

# Import shared M9 states and actions.
from guardian_protocol import (
    ControlAction,
    ControlState,
)

# Import high-level host components.
from guardianctl import (
    ClientConfig,
    GuardianClient,
    GuardianTcpTransport,
)

# Import the real loopback simulator endpoint.
from guardian_sim import GuardianDevice, GuardianTcpServer


# Verify M9 host operations through actual framed TCP exchanges.
class GuardianControlClientTests(unittest.TestCase):
    """Exercise control status and actions end to end."""

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

        # Stop the request loop.
        self.server.shutdown()

        # Close the listening socket.
        self.server.server_close()

        # Wait briefly for background cleanup.
        self.server_thread.join(
            timeout=2.0
        )

    # Verify baseline + ARM + status through separate real TCP exchanges.
    def test_start_baseline_arm_and_read_status(self) -> None:

        # Complete a deterministic healthy simulator baseline.
        self.client.start_baseline(
            16
        )

        # Arm supervision.
        result = self.client.control_action(
            ControlAction.ARM
        )

        # Require successful safe ARMED state.
        self.assertEqual(
            result.state,
            ControlState.ARMED,
        )

        # Require remote ARM not to assert run permit.
        self.assertFalse(
            result.run_permit
        )

        # Read current M9 status.
        status = self.client.control_status()

        # Require supervision to remain armed.
        self.assertTrue(
            status.supervision_enabled
        )

        # Require output safe while no local run request exists.
        self.assertFalse(
            status.run_permit
        )

    # Verify DISARM is always safe.
    def test_disarm_after_arm(self) -> None:

        # Complete baseline.
        self.client.start_baseline(
            16
        )

        # Arm supervision.
        self.client.control_action(
            ControlAction.ARM
        )

        # Disarm supervision.
        result = self.client.control_action(
            ControlAction.DISARM
        )

        # Require disabled state.
        self.assertEqual(
            result.state,
            ControlState.DISABLED,
        )

        # Require safe-off.
        self.assertFalse(
            result.run_permit
        )


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
