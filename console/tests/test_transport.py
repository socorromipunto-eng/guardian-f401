"""Integration tests for guardianctl against the real M2 loopback simulator."""

# Import unittest from the Python standard library.
import unittest

# Import the typed protocol models used to validate client results.
from guardian_protocol import DeviceState, ErrorCode, Frame, MessageType

# Import guardianctl components under integration test.
from guardianctl import (
    ClientConfig,
    GuardianClient,
    GuardianTcpTransport,
    RemoteDeviceError,
)

# Import the M2 simulator used as the real remote endpoint.
from guardian_sim import GuardianDevice, GuardianTcpServer

# Import threading so the simulator can run during each test.
import threading


# Verify the complete host-to-simulator request/response path.
class GuardianClientIntegrationTests(unittest.TestCase):
    """Exercise guardianctl against a real GuardianTcpServer."""

    # Start one fresh simulator on an ephemeral port before every test.
    def setUp(self) -> None:

        # Create an independent simulated device.
        self.device = GuardianDevice()

        # Create a loopback TCP server on an operating-system-selected free port.
        self.server = GuardianTcpServer(
            ("127.0.0.1", 0),
            self.device,
        )

        # Run the simulator request loop in a daemon test thread.
        self.server_thread = threading.Thread(
            target=self.server.serve_forever,
            daemon=True,
        )

        # Start accepting guardianctl connections.
        self.server_thread.start()

        # Read the actual ephemeral test endpoint.
        host, port = self.server.server_address

        # Configure guardianctl to connect to the test simulator.
        self.config = ClientConfig(
            host=host,
            port=port,
            timeout_seconds=2.0,
        )

        # Build the synchronous TCP transport.
        self.transport = GuardianTcpTransport(self.config)

        # Build the typed high-level client.
        self.client = GuardianClient(transport=self.transport)

    # Stop the simulator cleanly after every test.
    def tearDown(self) -> None:

        # Stop the server request loop.
        self.server.shutdown()

        # Close the listening socket.
        self.server.server_close()

        # Wait briefly for the server thread to finish.
        self.server_thread.join(timeout=2.0)

    # Verify guardianctl PING against the real simulator transport.
    def test_ping_round_trip(self) -> None:

        # Execute the high-level PING operation.
        result = self.client.ping()

        # Require the frozen PONG payload.
        self.assertEqual(result.reply, "PONG")

        # Require a non-negative measured latency.
        self.assertGreaterEqual(result.latency_ms, 0.0)

    # Verify guardianctl metadata decoding against the real simulator.
    def test_device_info_round_trip(self) -> None:

        # Execute the high-level metadata operation.
        info = self.client.device_info()

        # Require the expected simulator identity.
        self.assertEqual(info.model, "Guardian-F401-SIM")

        # Require the expected simulated firmware version.
        self.assertEqual(
            (
                info.firmware_major,
                info.firmware_minor,
                info.firmware_patch,
            ),
            (0, 2, 0),
        )

    # Verify guardianctl runtime-status decoding against the real simulator.
    def test_status_round_trip(self) -> None:

        # Execute the high-level status operation.
        status = self.client.status()

        # Require the initialized simulator application state.
        self.assertEqual(status.state, DeviceState.IDLE)

        # Require the request to be counted by the remote device.
        self.assertGreaterEqual(status.rx_frames, 1)

    # Verify sequence allocation progresses between independent high-level commands.
    def test_multiple_commands_share_sequence_manager(self) -> None:

        # Execute one PING request.
        self.client.ping()

        # Execute one metadata request using the same client instance.
        info = self.client.device_info()

        # Require the second request to complete successfully.
        self.assertEqual(info.device_id, 0xF4010001)


    # Verify the transport converts a remote ERROR frame into a structured host exception.
    def test_unknown_command_becomes_remote_device_error(self) -> None:

        # Build one structurally valid request containing an unpublished command.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=0xFE,
            sequence=77,
        )

        # Capture the structured guardianctl remote-device failure.
        with self.assertRaises(RemoteDeviceError) as context:

            # Send the unknown command through the real M2 simulator.
            self.transport.exchange(request)

        # Require the original command identifier to remain available to callers.
        self.assertEqual(context.exception.command, 0xFE)

        # Require the published UNKNOWN_COMMAND wire error value.
        self.assertEqual(
            context.exception.error_code,
            int(ErrorCode.UNKNOWN_COMMAND),
        )


# Execute the tests when this file is run directly.
if __name__ == "__main__":

    # Start the standard-library unit test runner with verbose output.
    unittest.main(verbosity=2)
