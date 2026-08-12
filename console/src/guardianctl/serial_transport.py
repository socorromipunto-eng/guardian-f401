"""Physical serial request-response transport for guardianctl M4."""

# Import time for a total transaction deadline.
import time

# Import Any and Callable so tests can inject a serial-compatible fake.
from typing import Any, Callable

# Import immutable physical serial configuration.
from .config import SerialConfig

# Import host-level exceptions for controlled operator diagnostics.
from .errors import ProtocolClientError, RemoteDeviceError, TransportError

# Import the shared response-correlation validator.
from .transport import validate_correlated_response

# Import canonical Guardian framing and parsing primitives.
from guardian_protocol import (
    Frame,
    IncrementalParser,
    MessageType,
    encode_frame,
)


# Define the callable signature used to create a serial-compatible object.
SerialFactory = Callable[[SerialConfig], Any]


# Create a real pyserial connection lazily.
def open_serial_port(config: SerialConfig) -> Any:
    """Return one configured pyserial Serial object."""

    # Import pyserial only when physical UART is requested.
    try:

        # Import the external package under its canonical module name.
        import serial
    except ImportError as exc:

        # Explain the optional dependency without a traceback.
        raise TransportError(
            "serial transport requires pyserial: python -m pip install pyserial"
        ) from exc

    # Open an 8-N-1 serial port using bounded reads.
    return serial.Serial(
        port=config.port,
        baudrate=config.baud_rate,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=min(0.05, config.timeout_seconds),
        write_timeout=config.timeout_seconds,
    )


# Exchange Guardian frames with a physical STM32 through USB-to-UART.
class GuardianSerialTransport:
    """Bounded Guardian Protocol serial transport."""

    # Store immutable serial configuration and an injectable factory.
    def __init__(
        self,
        config: SerialConfig,
        serial_factory: SerialFactory | None = None,
    ) -> None:

        # Preserve validated physical endpoint configuration.
        self._config = config

        # Use a test fake or lazily open pyserial.
        self._serial_factory = serial_factory or open_serial_port

    # Expose immutable serial configuration.
    @property
    def config(self) -> SerialConfig:
        """Return immutable physical serial configuration."""

        # Return configuration without exposing mutable state.
        return self._config

    # Expose one stable endpoint description.
    @property
    def endpoint(self) -> str:
        """Return the configured physical UART endpoint."""

        # Delegate endpoint formatting to immutable configuration.
        return self._config.endpoint

    # Execute one bounded physical UART request-response exchange.
    def exchange(self, request: Frame) -> Frame:
        """Send *request* over UART and return its correlated response."""

        # Reject non-request frames before opening the serial port.
        if request.message_type != MessageType.REQUEST:

            # Raise a host contract error instead of transmitting invalid traffic.
            raise ProtocolClientError("transport exchange requires a REQUEST frame")

        # Encode the complete request using the canonical codec.
        encoded_request = encode_frame(request)

        # Create independent parser state for this transaction.
        parser = IncrementalParser()

        # Calculate an absolute transaction deadline.
        deadline = time.monotonic() + self._config.timeout_seconds

        # Normalize serial failures into guardianctl exceptions.
        try:

            # Open the physical serial endpoint.
            with self._serial_factory(self._config) as serial_port:

                # Discover optional stale-input clearing support.
                reset_input = getattr(
                    serial_port,
                    "reset_input_buffer",
                    None,
                )

                # Clear stale bytes when supported.
                if callable(reset_input):

                    # Remove old uncorrelated bytes.
                    reset_input()

                # Write the complete request.
                bytes_written = serial_port.write(encoded_request)

                # Reject partial request transmission.
                if bytes_written != len(encoded_request):

                    # Raise a stable atomic-frame diagnostic.
                    raise TransportError(
                        (
                            "serial transport wrote "
                            f"{bytes_written}/{len(encoded_request)} request bytes"
                        )
                    )

                # Discover optional host-output flush support.
                flush = getattr(serial_port, "flush", None)

                # Flush when supported.
                if callable(flush):

                    # Complete host-side buffering.
                    flush()

                # Continue bounded reads until the response arrives.
                while time.monotonic() < deadline:

                    # Request a modest response fragment.
                    chunk = serial_port.read(64)

                    # Continue after a short empty read.
                    if not chunk:

                        # Retry until the total deadline.
                        continue

                    # Recover complete frames from arbitrary fragmentation.
                    frames = parser.feed(chunk)

                    # Inspect validated frames in wire order.
                    for response in frames:

                        # Ignore unrelated sequence values.
                        if response.sequence != request.sequence:

                            # Continue waiting for the correlated response.
                            continue

                        # Validate and return the correlated response.
                        return validate_correlated_response(
                            request,
                            response,
                        )

                # Report a total-transaction timeout.
                raise TransportError(
                    f"timed out waiting for Guardian device at {self.endpoint}"
                )

        # Preserve normalized guardianctl exceptions.
        except (TransportError, ProtocolClientError, RemoteDeviceError):

            # Re-raise unchanged.
            raise

        # Convert serial-library or operating-system failures.
        except Exception as exc:

            # Raise stable endpoint context.
            raise TransportError(
                f"cannot communicate with Guardian device at {self.endpoint}: {exc}"
            ) from exc
