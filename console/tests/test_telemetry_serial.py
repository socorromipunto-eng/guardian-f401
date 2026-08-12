"""Tests for persistent guardianctl telemetry over a serial-compatible fake."""

# Import time so the fake device can schedule real M5 samples.
import time

# Import unittest from the Python standard library.
import unittest

# Import protocol parser and encoder.
from guardian_protocol import IncrementalParser, encode_frame

# Import physical telemetry host components.
from guardianctl import SerialConfig, TelemetryMonitor

# Import the transport-independent simulated device.
from guardian_sim import GuardianDevice


# Emulate the pyserial subset required by the persistent telemetry monitor.
class TelemetryFakeSerial:
    """In-memory serial endpoint that emits due Guardian telemetry."""

    # Create one fake serial endpoint around a GuardianDevice.
    def __init__(self, device: GuardianDevice) -> None:

        # Preserve the simulated remote device.
        self._device = device

        # Create parser state for host-written Guardian frames.
        self._request_parser = IncrementalParser()

        # Store response and telemetry bytes waiting for host reads.
        self._response = bytearray()

    # Enter the serial-compatible context manager.
    def __enter__(self) -> "TelemetryFakeSerial":

        # Return this in-memory endpoint.
        return self

    # Exit the serial-compatible context manager.
    def __exit__(self, exc_type, exc_value, traceback) -> None:

        # No external resource cleanup is required.
        return None

    # Clear stale device-to-host bytes before subscription.
    def reset_input_buffer(self) -> None:

        # Remove pending response bytes.
        self._response.clear()

    # Accept host request bytes.
    def write(self, data: bytes) -> int:

        # Recover complete requests from arbitrary serial writes.
        frames = self._request_parser.feed(data)

        # Process every decoded request.
        for frame in frames:

            # Execute real simulated-device command semantics.
            response = self._device.process_frame(frame)

            # Queue the canonical encoded response.
            self._response.extend(
                encode_frame(response)
            )

        # Report complete request acceptance.
        return len(data)

    # Provide pyserial-compatible flush behavior.
    def flush(self) -> None:

        # No extra host-side buffering exists.
        return None

    # Return response bytes or one due asynchronous telemetry frame.
    def read(self, size: int) -> bytes:

        # Ask the remote device whether one telemetry sample is due.
        telemetry = self._device.poll_telemetry()

        # Queue a due asynchronous frame behind any earlier response bytes.
        if telemetry is not None:

            # Append canonical telemetry bytes.
            self._response.extend(
                encode_frame(telemetry)
            )

        # Emulate a short physical read timeout when no data is available.
        if not self._response:

            # Sleep briefly so the test does not busy-spin.
            time.sleep(0.005)

            # Return an empty serial read.
            return b""

        # Select at most the requested byte count.
        count = min(
            size,
            len(self._response),
        )

        # Copy the selected fragment.
        chunk = bytes(
            self._response[:count]
        )

        # Remove bytes returned to the host.
        del self._response[:count]

        # Return the bounded serial fragment.
        return chunk


# Verify the persistent physical telemetry abstraction without attached hardware.
class GuardianSerialTelemetryTests(unittest.TestCase):
    """Exercise M5 telemetry through a serial-compatible endpoint."""

    # Verify two physical-style samples are streamed and decoded.
    def test_serial_telemetry_stream(self) -> None:

        # Create one deterministic remote Guardian application.
        device = GuardianDevice()

        # Configure one fake physical serial endpoint.
        config = SerialConfig(
            port="COM_TELEMETRY_TEST",
            baud_rate=115200,
            timeout_seconds=1.0,
        )

        # Create the persistent monitor with an injected serial factory.
        monitor = TelemetryMonitor(
            config,
            serial_factory=lambda selected: TelemetryFakeSerial(
                device
            ),
        )

        # Store live decoded records.
        records = []

        # Stream two samples through the physical serial abstraction.
        delivered = monitor.stream_samples(
            period_ms=100,
            count=2,
            consumer=records.append,
        )

        # Require both requested samples.
        self.assertEqual(delivered, 2)

        # Require increasing independent telemetry sequences.
        self.assertEqual(
            [record.sequence for record in records],
            [1, 2],
        )


# Execute the tests when this file is run directly.
if __name__ == "__main__":

    # Start the standard-library test runner.
    unittest.main(verbosity=2)
