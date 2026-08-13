"""Integration tests for guardianctl M10 authenticated privileged operations."""

# Import threading so the real simulator server can run during each test.
import threading

# Import unittest from the Python standard library.
import unittest

# Import shared M10 role and M9 control state.
from guardian_protocol import (
    ControlAction,
    ControlState,
    SecurityRole,
)

# Import high-level guardianctl components.
from guardianctl import (
    ClientConfig,
    GuardianClient,
    GuardianTcpTransport,
    ProtocolClientError,
    SecurityClientConfig,
)

# Import the real simulator endpoint and secure-mode configuration.
from guardian_sim import GuardianDevice, GuardianTcpServer

# Import the intentionally public demo key.
from guardian_sim.config import (
    DEFAULT_SECURITY_PSK_HEX,
    SimulatorConfig,
)


# Verify complete host authentication through real TCP exchanges.
class GuardianSecurityClientTests(unittest.TestCase):
    """Exercise M10 handshake and protected commands over loopback TCP."""

    # Start one secure-mode simulator before every test.
    def setUp(self) -> None:

        # Create secure-mode simulator configuration.
        simulator_config = SimulatorConfig(
            security_enabled=True,
        )

        # Create independent device state.
        self.device = GuardianDevice(
            simulator_config
        )

        # Bind a real loopback server to an operating-system-selected port.
        self.server = GuardianTcpServer(
            ("127.0.0.1", 0),
            self.device,
        )

        # Run the server in a daemon thread.
        self.server_thread = threading.Thread(
            target=self.server.serve_forever,
            daemon=True,
        )

        # Start accepting host connections.
        self.server_thread.start()

        # Read the actual bound endpoint.
        host, port = self.server.server_address

        # Build real TCP configuration.
        self.transport = GuardianTcpTransport(
            ClientConfig(
                host=host,
                port=port,
                timeout_seconds=1.0,
            )
        )

        # Preserve the public demo key.
        self.psk = bytes.fromhex(
            DEFAULT_SECURITY_PSK_HEX
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

    # Verify OPERATOR credentials transparently protect baseline and control commands.
    def test_operator_baseline_and_control(self) -> None:

        # Create one security-aware high-level client.
        client = GuardianClient(
            transport=self.transport,
            security_config=SecurityClientConfig(
                psk=self.psk,
                role=SecurityRole.OPERATOR,
            ),
        )

        # Start baseline through AUTH + SECURE_COMMAND.
        baseline = client.start_baseline(
            16
        )

        # Require the normalized baseline target.
        self.assertEqual(
            baseline.target_samples,
            16,
        )

        # Arm M9 through a protected CONTROL_COMMAND.
        control = client.control_action(
            ControlAction.ARM
        )

        # Require safe ARMED state.
        self.assertEqual(
            control.state,
            ControlState.ARMED,
        )

        # Require remote ARM not to assert run permit.
        self.assertFalse(
            control.run_permit
        )

        # Read public security diagnostics.
        security = client.security_status()

        # Require at least one successful authenticated session.
        self.assertGreaterEqual(
            security.auth_successes,
            1,
        )

        # Require an active session.
        self.assertTrue(
            security.active
        )

    # Verify a wrong PSK is rejected by server-proof verification.
    def test_wrong_psk_fails_authentication(self) -> None:

        # Create a client with an intentionally wrong 256-bit key.
        client = GuardianClient(
            transport=self.transport,
            security_config=SecurityClientConfig(
                psk=bytes([0xA5]) * 32,
                role=SecurityRole.OPERATOR,
            ),
        )

        # Require authentication failure before privileged command execution.
        with self.assertRaises(ProtocolClientError):

            # Attempt protected baseline mutation.
            client.start_baseline(
                16
            )

    # Verify OBSERVER authenticates but receives an authenticated authorization error.
    def test_observer_cannot_mutate_baseline(self) -> None:

        # Create an observer-role client using the correct PSK.
        client = GuardianClient(
            transport=self.transport,
            security_config=SecurityClientConfig(
                psk=self.psk,
                role=SecurityRole.OBSERVER,
            ),
        )

        # Import the existing structured remote error lazily for assertion clarity.
        from guardianctl import RemoteDeviceError

        # Require protected authorization rejection.
        with self.assertRaises(RemoteDeviceError) as context:

            # Attempt operator-only baseline mutation.
            client.start_baseline(
                16
            )

        # Require the activated M10 UNAUTHORIZED code.
        self.assertEqual(
            context.exception.error_code,
            0x09,
        )


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
