"""Deterministic Guardian Protocol device model with M5 telemetry."""

# Import time for monotonic simulated-device uptime and telemetry scheduling.
import time

# Import RLock so counters and telemetry configuration remain coherent.
from threading import RLock

# Import shared Guardian Protocol models and codecs.
from guardian_protocol import (
    Command,
    DeviceInfo,
    DeviceState,
    DeviceStatus,
    ErrorCode,
    Frame,
    MachineTelemetry,
    MessageType,
    TelemetryConfig,
    decode_telemetry_config,
    encode_device_info,
    encode_device_status,
    encode_machine_telemetry,
    encode_telemetry_config,
)

# Import immutable simulator identity configuration.
from .config import SimulatorConfig


# Implement command behavior independently from TCP packet boundaries.
class GuardianDevice:
    """In-memory Guardian-compatible device application model."""

    # Create one simulated device with deterministic identity and counters.
    def __init__(self, config: SimulatorConfig | None = None) -> None:

        # Use explicit caller configuration or safe loopback defaults.
        self._config = config or SimulatorConfig()

        # Record a monotonic starting point for uptime and synthetic telemetry.
        self._started_at = time.monotonic()

        # Protect mutable counters, state and telemetry scheduling.
        self._lock = RLock()

        # Start the simulated application in its initialized idle state.
        self._state = DeviceState.IDLE

        # Start without accepted request traffic.
        self._rx_frames = 0

        # Start without generated response or telemetry traffic.
        self._tx_frames = 0

        # Start without observed protocol failures.
        self._protocol_errors = 0

        # Start without a Guardian application or protocol error code.
        self._last_error = 0

        # Start asynchronous telemetry disabled.
        self._telemetry_enabled = False

        # Start with the documented safe one-second period.
        self._telemetry_period_ms = 1000

        # Start without a scheduled emission deadline.
        self._telemetry_next_due = 0.0

        # Reserve sequence zero and start asynchronous telemetry at one.
        self._telemetry_sequence = 1

        # Start without an active transport session owning telemetry delivery.
        self._telemetry_owner: object | None = None

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

            # Raise a caller error rather than corrupting diagnostics.
            raise ValueError("protocol error count cannot be negative")

        # Serialize the counter update with command processing.
        with self._lock:

            # Saturate the public 32-bit counter instead of wrapping.
            self._protocol_errors = min(
                0xFFFFFFFF,
                self._protocol_errors + count,
            )

    # Build an immutable status snapshot.
    def snapshot_status(self) -> DeviceStatus:
        """Return the current simulated device status."""

        # Serialize the multi-field snapshot for internal coherence.
        with self._lock:

            # Calculate whole monotonic uptime seconds.
            uptime_seconds = int(
                time.monotonic() - self._started_at
            )

            # Saturate uptime to the published unsigned 32-bit field.
            uptime_seconds = min(
                0xFFFFFFFF,
                uptime_seconds,
            )

            # Return one immutable protocol-level status object.
            return DeviceStatus(
                state=self._state,
                uptime_seconds=uptime_seconds,
                rx_frames=self._rx_frames,
                tx_frames=self._tx_frames,
                protocol_errors=self._protocol_errors,
                last_error=self._last_error,
            )

    # Process one structurally valid request at the application boundary.
    def process_frame(
        self,
        frame: Frame,
        telemetry_owner: object | None = None,
    ) -> Frame:
        """Process one validated frame and return one response or error frame."""

        # Serialize request processing and telemetry configuration.
        with self._lock:

            # Count every structurally valid frame presented to the application.
            self._rx_frames = min(
                0xFFFFFFFF,
                self._rx_frames + 1,
            )

            # Reject non-request traffic at the device command boundary.
            if frame.message_type != MessageType.REQUEST:

                # Return a deterministic semantic error.
                return self._make_error(
                    frame,
                    ErrorCode.MALFORMED_FRAME,
                )

            # Dispatch PING.
            if frame.command == int(Command.PING):

                # Require the frozen empty request payload.
                if frame.payload:

                    # Reject undefined PING payload bytes.
                    return self._make_error(
                        frame,
                        ErrorCode.INVALID_PAYLOAD,
                    )

                # Return the frozen PONG payload.
                return self._make_response(
                    frame,
                    b"PONG",
                )

            # Dispatch DEVICE_INFO.
            if frame.command == int(Command.DEVICE_INFO):

                # Require the frozen empty request payload.
                if frame.payload:

                    # Reject undefined metadata request bytes.
                    return self._make_error(
                        frame,
                        ErrorCode.INVALID_PAYLOAD,
                    )

                # Build immutable simulated device metadata.
                info = DeviceInfo(
                    model=self._config.model,
                    firmware_major=self._config.firmware_major,
                    firmware_minor=self._config.firmware_minor,
                    firmware_patch=self._config.firmware_patch,
                    device_id=self._config.device_id,
                )

                # Serialize metadata using the shared protocol codec.
                payload = encode_device_info(info)

                # Return the correlated response.
                return self._make_response(
                    frame,
                    payload,
                )

            # Dispatch GET_STATUS.
            if frame.command == int(Command.GET_STATUS):

                # Require the frozen empty request payload.
                if frame.payload:

                    # Reject undefined status request bytes.
                    return self._make_error(
                        frame,
                        ErrorCode.INVALID_PAYLOAD,
                    )

                # Build a coherent runtime snapshot.
                status = self.snapshot_status()

                # Serialize the fixed-width runtime payload.
                payload = encode_device_status(status)

                # Return the correlated response.
                return self._make_response(
                    frame,
                    payload,
                )

            # Dispatch M5 telemetry configuration.
            if frame.command == int(Command.SET_TELEMETRY):

                # Use the device object as the direct-call owner when no transport token exists.
                owner = (
                    self
                    if telemetry_owner is None
                    else telemetry_owner
                )

                # Decode and validate the frozen configuration payload.
                try:

                    # Decode shared protocol configuration semantics.
                    telemetry_config = decode_telemetry_config(
                        frame.payload
                    )
                except ValueError:

                    # Reject malformed or out-of-policy configuration.
                    return self._make_error(
                        frame,
                        ErrorCode.INVALID_PAYLOAD,
                    )

                # Reject a second transport attempting to take an active subscription.
                if (
                    telemetry_config.enabled
                    and self._telemetry_enabled
                    and self._telemetry_owner is not None
                    and self._telemetry_owner is not owner
                ):

                    # Preserve deterministic single-subscriber delivery.
                    return self._make_error(
                        frame,
                        ErrorCode.BUSY,
                    )

                # Reject another transport attempting to disable someone else's subscription.
                if (
                    not telemetry_config.enabled
                    and self._telemetry_owner is not None
                    and self._telemetry_owner is not owner
                ):

                    # Preserve ownership until the active transport releases it.
                    return self._make_error(
                        frame,
                        ErrorCode.BUSY,
                    )

                # Apply the normalized telemetry enabled state.
                self._telemetry_enabled = telemetry_config.enabled

                # Apply the validated telemetry period.
                self._telemetry_period_ms = telemetry_config.period_ms

                # Assign or release the single active telemetry owner.
                self._telemetry_owner = (
                    owner
                    if telemetry_config.enabled
                    else None
                )

                # Restart scheduling from the current monotonic instant.
                self._telemetry_next_due = (
                    time.monotonic()
                    + (telemetry_config.period_ms / 1000.0)
                )

                # Echo the normalized active configuration.
                return self._make_response(
                    frame,
                    encode_telemetry_config(telemetry_config),
                )

            # Reject command identifiers not implemented by the simulator.
            return self._make_error(
                frame,
                ErrorCode.UNKNOWN_COMMAND,
            )

    # Create one due deterministic synthetic telemetry frame.
    def poll_telemetry(
        self,
        telemetry_owner: object | None = None,
    ) -> Frame | None:
        """Return one due telemetry frame only to the active subscription owner."""

        # Normalize direct-call ownership for unit tests and serial fakes.
        owner = (
            self
            if telemetry_owner is None
            else telemetry_owner
        )

        # Serialize telemetry scheduling and transmitted-frame diagnostics.
        with self._lock:

            # Do not emit while asynchronous telemetry is disabled.
            if not self._telemetry_enabled:

                # Report that no frame is currently due.
                return None

            # Deliver asynchronous samples only to the transport that enabled them.
            if self._telemetry_owner is not owner:

                # Prevent another concurrent simulator client from stealing a sample.
                return None

            # Capture one monotonic timestamp for scheduling and sample generation.
            now = time.monotonic()

            # Wait until the configured period has elapsed.
            if now < self._telemetry_next_due:

                # Report that no frame is currently due.
                return None

            # Schedule from the current instant to avoid catch-up bursts after stalls.
            self._telemetry_next_due = (
                now + (self._telemetry_period_ms / 1000.0)
            )

            # Calculate the simulated device timestamp modulo 2^32 milliseconds.
            timestamp_ms = int(
                (now - self._started_at) * 1000.0
            ) & 0xFFFFFFFF

            # Build deterministic synthetic values for transport testing only.
            sample = MachineTelemetry(
                state=self._state,
                timestamp_ms=timestamp_ms,
                temperature_centi_c=2500 + ((timestamp_ms // 100) % 40),
                vibration_mg_rms=30 + ((timestamp_ms // 50) % 20),
                current_ma=420 + ((timestamp_ms // 200) % 50),
                rpm=1450 + ((timestamp_ms // 100) % 30),
                supply_mv=3300,
                status_flags=0,
            )

            # Preserve the independent asynchronous telemetry sequence.
            sequence = self._telemetry_sequence

            # Wrap the telemetry sequence to one so zero remains reserved.
            if self._telemetry_sequence == 0xFFFFFFFF:

                # Restart the non-zero sequence space.
                self._telemetry_sequence = 1
            else:

                # Advance monotonically for the next asynchronous frame.
                self._telemetry_sequence += 1

            # Count the generated telemetry frame as transmitted device output.
            self._tx_frames = min(
                0xFFFFFFFF,
                self._tx_frames + 1,
            )

            # Return one canonical asynchronous telemetry frame.
            return Frame(
                message_type=MessageType.TELEMETRY,
                command=Command.MACHINE_TELEMETRY,
                sequence=sequence,
                payload=encode_machine_telemetry(sample),
            )

    # Release telemetry when the owning transport disconnects unexpectedly.
    def release_telemetry(
        self,
        telemetry_owner: object,
    ) -> None:
        """Disable telemetry only when *telemetry_owner* owns the active stream."""

        # Serialize ownership changes with command and scheduling operations.
        with self._lock:

            # Ignore disconnects from transports that do not own telemetry.
            if self._telemetry_owner is not telemetry_owner:

                # Return without affecting the active subscriber.
                return

            # Disable asynchronous telemetry after owner disconnect.
            self._telemetry_enabled = False

            # Release the transport ownership token.
            self._telemetry_owner = None

            # Clear the pending schedule deadline.
            self._telemetry_next_due = 0.0

    # Create one successful response while maintaining diagnostics.
    def _make_response(
        self,
        request: Frame,
        payload: bytes,
    ) -> Frame:
        """Create a successful response correlated to *request*."""

        # Increment the transmitted-frame counter.
        self._tx_frames = min(
            0xFFFFFFFF,
            self._tx_frames + 1,
        )

        # Return an immutable correlated response.
        return Frame(
            message_type=MessageType.RESPONSE,
            command=request.command,
            sequence=request.sequence,
            payload=payload,
        )

    # Create one error response while maintaining diagnostics.
    def _make_error(
        self,
        request: Frame,
        error: ErrorCode,
    ) -> Frame:
        """Create an ERROR frame correlated to *request*."""

        # Store the latest application or protocol error identifier.
        self._last_error = int(error)

        # Increment the protocol error diagnostic counter.
        self._protocol_errors = min(
            0xFFFFFFFF,
            self._protocol_errors + 1,
        )

        # Increment transmitted-frame count because an error is device output.
        self._tx_frames = min(
            0xFFFFFFFF,
            self._tx_frames + 1,
        )

        # Return an immutable one-byte error response.
        return Frame(
            message_type=MessageType.ERROR,
            command=request.command,
            sequence=request.sequence,
            payload=bytes((int(error),)),
        )
