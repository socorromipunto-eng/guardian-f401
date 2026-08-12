"""High-level Guardian host client independent from CLI presentation."""

# Import monotonic high-resolution timing for PING latency measurement.
import time

# Import dataclass for immutable PING results.
from dataclasses import dataclass

# Import shared command and payload codecs from the protocol package.
from guardian_protocol import (
    Command,
    DeviceInfo,
    DeviceStatus,
    Frame,
    MessageType,
    decode_device_info,
    decode_device_status,
)

# Import host-side protocol contract errors.
from .errors import ProtocolClientError

# Import the thread-safe request sequence allocator.
from .sequence import SequenceManager

# Import the default TCP transport.
from .transport import GuardianTcpTransport


# Represent the complete result of one successful PING operation.
@dataclass(frozen=True, slots=True)
class PingResult:
    """Successful Guardian PING result."""

    # Store the response payload text.
    reply: str

    # Store measured request/response latency in milliseconds.
    latency_ms: float


# Expose typed Guardian operations to CLI and future GUI callers.
class GuardianClient:
    """High-level synchronous Guardian device client."""

    # Create a client from an explicit transport or development defaults.
    def __init__(
        self,
        transport: GuardianTcpTransport | None = None,
        sequence_manager: SequenceManager | None = None,
    ) -> None:

        # Use the explicit transport or create the default local TCP transport.
        self._transport = transport or GuardianTcpTransport()

        # Use the explicit sequence allocator or start from one.
        self._sequence_manager = sequence_manager or SequenceManager()

    # Expose transport configuration for operator-facing endpoint output.
    @property
    def transport(self) -> GuardianTcpTransport:
        """Return the configured synchronous transport."""

        # Return the transport object without changing its state.
        return self._transport

    # Verify Guardian connectivity and measure end-to-end response latency.
    def ping(self) -> PingResult:
        """Execute PING and return reply text plus measured latency."""

        # Allocate one unique request correlation sequence.
        sequence = self._sequence_manager.next()

        # Build the frozen empty-payload PING request.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.PING,
            sequence=sequence,
        )

        # Capture a monotonic timestamp immediately before transport exchange.
        started_at = time.perf_counter()

        # Execute the bounded request/response exchange.
        response = self._transport.exchange(request)

        # Calculate end-to-end latency after a validated response arrives.
        latency_ms = (time.perf_counter() - started_at) * 1000.0

        # Require the frozen PING response payload.
        if response.payload != b"PONG":

            # Reject a semantically invalid response even when framing is correct.
            raise ProtocolClientError(
                f"PING returned unexpected payload: {response.payload!r}"
            )

        # Return immutable typed PING information.
        return PingResult(
            reply="PONG",
            latency_ms=latency_ms,
        )

    # Read immutable device and firmware metadata.
    def device_info(self) -> DeviceInfo:
        """Execute DEVICE_INFO and decode its binary payload."""

        # Allocate one unique request correlation sequence.
        sequence = self._sequence_manager.next()

        # Build the frozen empty-payload metadata request.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.DEVICE_INFO,
            sequence=sequence,
        )

        # Execute the bounded request/response exchange.
        response = self._transport.exchange(request)

        # Decode and validate the command-specific binary metadata payload.
        return decode_device_info(response.payload)

    # Read one coherent runtime diagnostic snapshot.
    def status(self) -> DeviceStatus:
        """Execute GET_STATUS and decode its binary payload."""

        # Allocate one unique request correlation sequence.
        sequence = self._sequence_manager.next()

        # Build the frozen empty-payload status request.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.GET_STATUS,
            sequence=sequence,
        )

        # Execute the bounded request/response exchange.
        response = self._transport.exchange(request)

        # Decode and validate the fixed-width runtime status payload.
        return decode_device_status(response.payload)
