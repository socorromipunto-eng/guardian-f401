"""Deterministic Guardian Protocol device model used by the M2 simulator."""

# Import time for monotonic simulated-device uptime.
import time

# Import RLock so counters remain coherent if several simulator clients are used later.
from threading import RLock

# Import the protocol models and codecs shared with future host tools.
from guardian_protocol import (
    Command,
    DeviceInfo,
    DeviceState,
    DeviceStatus,
    ErrorCode,
    Frame,
    MessageType,
    encode_device_info,
    encode_device_status,
)

# Import immutable simulator identity configuration.
from .config import SimulatorConfig


# Implement command behavior independently from TCP transport details.
class GuardianDevice:
    """In-memory Guardian-compatible device application model."""

    # Create one simulated device with deterministic identity and counters.
    def __init__(self, config: SimulatorConfig | None = None) -> None:

        # Use explicit caller configuration or safe loopback defaults.
        self._config = config or SimulatorConfig()

        # Record a monotonic starting point so wall-clock changes cannot corrupt uptime.
        self._started_at = time.monotonic()

        # Protect mutable counters and state from future concurrent client handlers.
        self._lock = RLock()

        # Start the simulated application in its initialized idle state.
        self._state = DeviceState.IDLE

        # Start without accepted request traffic.
        self._rx_frames = 0

        # Start without generated response traffic.
        self._tx_frames = 0

        # Start without observed framing or semantic protocol failures.
        self._protocol_errors = 0

        # Start without a Guardian application/protocol error code.
        self._last_error = 0

    # Expose immutable configuration for diagnostics and tests.
    @property
    def config(self) -> SimulatorConfig:
        """Return immutable simulator configuration."""

        # Return the already immutable configuration object.
        return self._config

    # Record parser-level failures that could not produce a trustworthy request frame.
    def record_protocol_errors(self, count: int) -> None:
        """Add parser-level failures to device diagnostics."""

        # Reject negative diagnostic deltas because counters must be monotonic.
        if count < 0:

            # Raise a caller error rather than corrupting diagnostic state.
            raise ValueError("protocol error count cannot be negative")

        # Serialize the counter update with command processing.
        with self._lock:

            # Saturate the public 32-bit counter instead of wrapping unexpectedly.
            self._protocol_errors = min(
                0xFFFFFFFF,
                self._protocol_errors + count,
            )

    # Build an immutable status snapshot without exposing mutable internal fields.
    def snapshot_status(self) -> DeviceStatus:
        """Return the current simulated device status."""

        # Serialize the multi-field snapshot so counters remain internally coherent.
        with self._lock:

            # Calculate whole monotonic uptime seconds.
            uptime_seconds = int(time.monotonic() - self._started_at)

            # Saturate uptime to the published unsigned 32-bit payload field.
            uptime_seconds = min(0xFFFFFFFF, uptime_seconds)

            # Return one immutable protocol-level status object.
            return DeviceStatus(
                state=self._state,
                uptime_seconds=uptime_seconds,
                rx_frames=self._rx_frames,
                tx_frames=self._tx_frames,
                protocol_errors=self._protocol_errors,
                last_error=self._last_error,
            )

    # Process one structurally valid protocol frame at the application boundary.
    def process_frame(self, frame: Frame) -> Frame:
        """Process one validated frame and return one response or error frame."""

        # Serialize request processing so counters and future state transitions remain atomic.
        with self._lock:

            # Count every structurally valid frame presented to the device application.
            self._rx_frames = min(0xFFFFFFFF, self._rx_frames + 1)

            # Reject non-request traffic at the device command boundary.
            if frame.message_type != MessageType.REQUEST:

                # Return a deterministic semantic error without executing a command.
                return self._make_error(
                    frame,
                    ErrorCode.MALFORMED_FRAME,
                )

            # Dispatch the published PING command.
            if frame.command == int(Command.PING):

                # Require the frozen empty request payload.
                if frame.payload:

                    # Reject command payload bytes that are undefined by v0.1.
                    return self._make_error(
                        frame,
                        ErrorCode.INVALID_PAYLOAD,
                    )

                # Return the frozen ASCII PONG response payload.
                return self._make_response(
                    frame,
                    b"PONG",
                )

            # Dispatch the published DEVICE_INFO command.
            if frame.command == int(Command.DEVICE_INFO):

                # Require the frozen empty request payload.
                if frame.payload:

                    # Reject command payload bytes that are undefined by v0.1.
                    return self._make_error(
                        frame,
                        ErrorCode.INVALID_PAYLOAD,
                    )

                # Build immutable protocol-level device metadata.
                info = DeviceInfo(
                    model=self._config.model,
                    firmware_major=self._config.firmware_major,
                    firmware_minor=self._config.firmware_minor,
                    firmware_patch=self._config.firmware_patch,
                    device_id=self._config.device_id,
                )

                # Serialize metadata using the shared command-payload codec.
                payload = encode_device_info(info)

                # Return one successful response correlated to the request sequence.
                return self._make_response(
                    frame,
                    payload,
                )

            # Dispatch the published GET_STATUS command.
            if frame.command == int(Command.GET_STATUS):

                # Require the frozen empty request payload.
                if frame.payload:

                    # Reject command payload bytes that are undefined by v0.1.
                    return self._make_error(
                        frame,
                        ErrorCode.INVALID_PAYLOAD,
                    )

                # Build a coherent immutable runtime snapshot while the device lock is held.
                status = self.snapshot_status()

                # Serialize the runtime snapshot using the shared command-payload codec.
                payload = encode_device_status(status)

                # Return one successful response correlated to the request sequence.
                return self._make_response(
                    frame,
                    payload,
                )

            # Reject command identifiers not published by the current device implementation.
            return self._make_error(
                frame,
                ErrorCode.UNKNOWN_COMMAND,
            )

    # Create one successful response while maintaining diagnostic counters.
    def _make_response(self, request: Frame, payload: bytes) -> Frame:
        """Create a successful response correlated to *request*."""

        # Increment the transmitted-frame counter before returning application output.
        self._tx_frames = min(0xFFFFFFFF, self._tx_frames + 1)

        # Return an immutable protocol frame with request correlation preserved.
        return Frame(
            message_type=MessageType.RESPONSE,
            command=request.command,
            sequence=request.sequence,
            payload=payload,
        )

    # Create one error response while maintaining diagnostics.
    def _make_error(self, request: Frame, error: ErrorCode) -> Frame:
        """Create an ERROR frame correlated to *request*."""

        # Store the latest application/protocol error identifier.
        self._last_error = int(error)

        # Increment the protocol error diagnostic counter.
        self._protocol_errors = min(
            0xFFFFFFFF,
            self._protocol_errors + 1,
        )

        # Increment the transmitted-frame counter because an error is still device output.
        self._tx_frames = min(0xFFFFFFFF, self._tx_frames + 1)

        # Return an immutable one-byte error payload correlated to the original request.
        return Frame(
            message_type=MessageType.ERROR,
            command=request.command,
            sequence=request.sequence,
            payload=bytes((int(error),)),
        )
