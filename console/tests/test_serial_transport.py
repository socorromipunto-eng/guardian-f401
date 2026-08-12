"""Tests for guardianctl physical serial transport."""

# Import unittest from the standard library.
import unittest

# Import protocol parser and encoder.
from guardian_protocol import IncrementalParser, encode_frame

# Import physical transport components.
from guardianctl import (
    GuardianClient,
    GuardianSerialTransport,
    SerialConfig,
)

# Import the transport-independent simulated device.
from guardian_sim import GuardianDevice


# Emulate the subset of pyserial used by GuardianSerialTransport.
class FakeSerial:
    """In-memory serial object backed by GuardianDevice."""

    # Create one fake serial endpoint.
    def __init__(self, device: GuardianDevice) -> None:

        # Preserve remote device.
        self._device = device

        # Create request parser.
        self._request_parser = IncrementalParser()

        # Store pending response bytes.
        self._response = bytearray()

    # Enter context manager.
    def __enter__(self) -> "FakeSerial":

        # Return the serial-compatible object.
        return self

    # Exit context manager.
    def __exit__(self, exc_type, exc_value, traceback) -> None:

        # No external resource cleanup is required.
        return None

    # Clear stale response bytes.
    def reset_input_buffer(self) -> None:

        # Remove pending bytes.
        self._response.clear()

    # Accept host serial bytes.
    def write(self, data: bytes) -> int:

        # Parse host bytes.
        frames = self._request_parser.feed(data)

        # Process every decoded request.
        for frame in frames:

            # Execute device semantics.
            response = self._device.process_frame(frame)

            # Queue encoded response.
            self._response.extend(encode_frame(response))

        # Report complete write.
        return len(data)

    # Provide pyserial-compatible flush.
    def flush(self) -> None:

        # No extra buffering exists.
        return None

    # Return a bounded response fragment.
    def read(self, size: int) -> bytes:

        # Return empty when nothing is queued.
        if not self._response:

            # Emulate a short timeout.
            return b""

        # Select a bounded fragment.
        count = min(size, len(self._response))

        # Copy fragment.
        chunk = bytes(self._response[:count])

        # Remove returned bytes.
        del self._response[:count]

        # Return fragment.
        return chunk


# Verify physical serial host behavior without hardware.
class GuardianSerialTransportTests(unittest.TestCase):
    """Exercise the UART host path using an in-memory serial fake."""

    # Create fresh remote state before every test.
    def setUp(self) -> None:

        # Create simulated remote device.
        self.device = GuardianDevice()

        # Configure fake physical endpoint.
        self.config = SerialConfig(
            port="COM_TEST",
            baud_rate=115200,
            timeout_seconds=0.25,
        )

        # Create serial factory.
        self.serial_factory = lambda config: FakeSerial(self.device)

        # Create physical transport.
        self.transport = GuardianSerialTransport(
            self.config,
            serial_factory=self.serial_factory,
        )

        # Create high-level client.
        self.client = GuardianClient(transport=self.transport)

    # Verify PING.
    def test_serial_ping(self) -> None:

        # Execute PING.
        result = self.client.ping()

        # Require PONG.
        self.assertEqual(result.reply, "PONG")

        # Require physical endpoint formatting.
        self.assertEqual(self.transport.endpoint, "COM_TEST@115200")

    # Verify DEVICE_INFO.
    def test_serial_device_info(self) -> None:

        # Execute metadata request.
        info = self.client.device_info()

        # Require model.
        self.assertEqual(info.model, "Guardian-F401-SIM")

        # Require device ID.
        self.assertEqual(info.device_id, 0xF4010001)

    # Verify GET_STATUS.
    def test_serial_status(self) -> None:

        # Execute status request.
        status = self.client.status()

        # Require at least the current RX request.
        self.assertGreaterEqual(status.rx_frames, 1)


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
