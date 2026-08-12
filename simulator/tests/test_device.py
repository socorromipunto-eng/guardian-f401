"""Unit tests for the transport-independent Guardian simulated device."""

# Import unittest from the Python standard library.
import unittest

# Import protocol models and payload decoders used to validate device behavior.
from guardian_protocol import (
    Command,
    DeviceState,
    ErrorCode,
    Frame,
    MessageType,
    decode_device_info,
    decode_device_status,
)

# Import the simulator application model under test.
from guardian_sim import GuardianDevice


# Verify deterministic command handling without involving any socket transport.
class GuardianDeviceTests(unittest.TestCase):
    """Exercise the M2 GuardianDevice command dispatcher."""

    # Create a fresh simulated device before every independent test.
    def setUp(self) -> None:

        # Avoid counter or state leakage between tests.
        self.device = GuardianDevice()

    # Verify the first complete M2 request/response contract.
    def test_ping_returns_pong(self) -> None:

        # Build one valid empty-payload PING request.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.PING,
            sequence=100,
        )

        # Process the validated request at the application boundary.
        response = self.device.process_frame(request)

        # Require a successful response message class.
        self.assertEqual(response.message_type, MessageType.RESPONSE)

        # Require response/request command correlation.
        self.assertEqual(response.command, Command.PING)

        # Require response/request sequence correlation.
        self.assertEqual(response.sequence, 100)

        # Require the frozen M2 PING response payload.
        self.assertEqual(response.payload, b"PONG")

    # Verify command payload requirements are enforced before application behavior executes.
    def test_ping_rejects_unexpected_payload(self) -> None:

        # Build a PING request containing undefined command payload bytes.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.PING,
            sequence=101,
            payload=b"unexpected",
        )

        # Process the semantically invalid request.
        response = self.device.process_frame(request)

        # Require a protocol/application ERROR response.
        self.assertEqual(response.message_type, MessageType.ERROR)

        # Require the frozen one-byte invalid-payload error identifier.
        self.assertEqual(response.payload, bytes((ErrorCode.INVALID_PAYLOAD,)))

    # Verify that DEVICE_INFO returns parseable deterministic binary metadata.
    def test_device_info_returns_metadata(self) -> None:

        # Build one valid metadata request.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.DEVICE_INFO,
            sequence=102,
        )

        # Process the metadata request.
        response = self.device.process_frame(request)

        # Decode the command-specific binary payload using the shared protocol codec.
        info = decode_device_info(response.payload)

        # Require an identity that clearly identifies software simulation.
        self.assertEqual(info.model, "Guardian-F401-SIM")

        # Require the M2 simulator firmware version.
        self.assertEqual(
            (
                info.firmware_major,
                info.firmware_minor,
                info.firmware_patch,
            ),
            (0, 2, 0),
        )

        # Require the published default simulator device identifier.
        self.assertEqual(info.device_id, 0xF4010001)

    # Verify that GET_STATUS exposes coherent protocol diagnostics.
    def test_get_status_returns_runtime_snapshot(self) -> None:

        # First execute PING so the runtime counters become observable.
        self.device.process_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.PING,
                sequence=1,
            )
        )

        # Build one valid runtime-status request.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.GET_STATUS,
            sequence=2,
        )

        # Process the runtime-status request.
        response = self.device.process_frame(request)

        # Decode the deterministic fixed-width runtime payload.
        status = decode_device_status(response.payload)

        # Require the simulator to start in the initialized IDLE state.
        self.assertEqual(status.state, DeviceState.IDLE)

        # Require both accepted requests to be reflected in the receive counter.
        self.assertEqual(status.rx_frames, 2)

        # Require the earlier PING response to appear in the pre-response snapshot.
        self.assertEqual(status.tx_frames, 1)

        # Require no protocol failures during valid command traffic.
        self.assertEqual(status.protocol_errors, 0)

    # Verify unknown command identifiers fail closed.
    def test_unknown_command_returns_error(self) -> None:

        # Build one structurally valid request containing an unpublished command identifier.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=0xFE,
            sequence=103,
        )

        # Process the unknown request.
        response = self.device.process_frame(request)

        # Require an error message class instead of executing undefined behavior.
        self.assertEqual(response.message_type, MessageType.ERROR)

        # Require the published UNKNOWN_COMMAND error code.
        self.assertEqual(response.payload, bytes((ErrorCode.UNKNOWN_COMMAND,)))

    # Verify device command processing rejects incoming response/event traffic.
    def test_non_request_frame_is_rejected(self) -> None:

        # Build a structurally valid frame with an invalid direction for device command dispatch.
        frame = Frame(
            message_type=MessageType.RESPONSE,
            command=Command.PING,
            sequence=104,
            payload=b"PONG",
        )

        # Present the semantically invalid frame to the simulated device.
        response = self.device.process_frame(frame)

        # Require an error frame instead of treating response traffic as a command.
        self.assertEqual(response.message_type, MessageType.ERROR)

        # Require the current semantic malformed-frame diagnostic.
        self.assertEqual(response.payload, bytes((ErrorCode.MALFORMED_FRAME,)))


# Execute the tests when this file is run directly.
if __name__ == "__main__":

    # Start the standard-library unit test runner with verbose output.
    unittest.main(verbosity=2)
