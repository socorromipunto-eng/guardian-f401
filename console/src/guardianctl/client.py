"""High-level Guardian host client independent from CLI presentation."""

# Import high-resolution timing for PING latency measurement.
import time

# Import dataclass for immutable PING results.
from dataclasses import dataclass

# Import shared command and payload codecs.
from guardian_protocol import (
    BaselineAction,
    BaselineControl,
    Command,
    ControlAction,
    ControlCommand,
    ControlCommandResult,
    ControlStatus,
    DeviceInfo,
    DeviceStatus,
    DspFeatures,
    Frame,
    HealthStatus,
    MessageType,
    decode_device_info,
    decode_baseline_control,
    decode_control_command_result,
    decode_control_status,
    decode_device_status,
    decode_dsp_features,
    decode_health_status,
    encode_baseline_control,
    encode_control_command,
)

# Import host-side protocol contract errors.
from .errors import ProtocolClientError

# Import request sequence allocation.
from .sequence import SequenceManager

# Import transport contract and default TCP transport.
from .transport import ExchangeTransport, GuardianTcpTransport


# Represent one successful PING result.
@dataclass(frozen=True, slots=True)
class PingResult:
    """Successful Guardian PING result."""

    # Store response payload text.
    reply: str

    # Store measured latency in milliseconds.
    latency_ms: float


# Expose typed Guardian operations to CLI and future GUI callers.
class GuardianClient:
    """High-level synchronous Guardian device client."""

    # Create a client from an explicit transport or TCP defaults.
    def __init__(
        self,
        transport: ExchangeTransport | None = None,
        sequence_manager: SequenceManager | None = None,
    ) -> None:

        # Use explicit transport or default local TCP.
        self._transport = transport or GuardianTcpTransport()

        # Use explicit sequence allocator or start from one.
        self._sequence_manager = sequence_manager or SequenceManager()

    # Expose configured exchange transport.
    @property
    def transport(self) -> ExchangeTransport:
        """Return the configured synchronous transport."""

        # Return transport without changing state.
        return self._transport

    # Verify connectivity and measure response latency.
    def ping(self) -> PingResult:
        """Execute PING and return reply text plus measured latency."""

        # Allocate request correlation sequence.
        sequence = self._sequence_manager.next()

        # Build the PING request.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.PING,
            sequence=sequence,
        )

        # Capture start time.
        started_at = time.perf_counter()

        # Execute transport exchange.
        response = self._transport.exchange(request)

        # Calculate end-to-end latency.
        latency_ms = (time.perf_counter() - started_at) * 1000.0

        # Require frozen PONG semantics.
        if response.payload != b"PONG":

            # Reject semantically invalid response.
            raise ProtocolClientError(
                f"PING returned unexpected payload: {response.payload!r}"
            )

        # Return typed result.
        return PingResult(
            reply="PONG",
            latency_ms=latency_ms,
        )

    # Read immutable device metadata.
    def device_info(self) -> DeviceInfo:
        """Execute DEVICE_INFO and decode its binary payload."""

        # Allocate request correlation sequence.
        sequence = self._sequence_manager.next()

        # Build the metadata request.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.DEVICE_INFO,
            sequence=sequence,
        )

        # Execute transport exchange.
        response = self._transport.exchange(request)

        # Decode command-specific payload.
        return decode_device_info(response.payload)

    # Read one runtime diagnostic snapshot.
    def status(self) -> DeviceStatus:
        """Execute GET_STATUS and decode its binary payload."""

        # Allocate request correlation sequence.
        sequence = self._sequence_manager.next()

        # Build the status request.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.GET_STATUS,
            sequence=sequence,
        )

        # Execute transport exchange.
        response = self._transport.exchange(request)

        # Decode fixed-width status payload.
        return decode_device_status(response.payload)


    # Read the latest firmware DSP and spectral feature snapshot.
    def dsp_features(self) -> DspFeatures:
        """Execute GET_DSP_FEATURES and decode its fixed M7 payload."""

        # Allocate one request correlation sequence.
        sequence = self._sequence_manager.next()

        # Build the empty GET_DSP_FEATURES request.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.GET_DSP_FEATURES,
            sequence=sequence,
        )

        # Execute one synchronous transport exchange.
        response = self._transport.exchange(request)

        # Decode the fixed M7 feature payload.
        return decode_dsp_features(response.payload)


    # Read the current M8 machine-health snapshot.
    def health_status(self) -> HealthStatus:
        """Execute GET_HEALTH_STATUS and decode its fixed M8 payload."""

        # Allocate one request correlation sequence.
        sequence = self._sequence_manager.next()

        # Build the empty health-status request.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.GET_HEALTH_STATUS,
            sequence=sequence,
        )

        # Execute one synchronous transport exchange.
        response = self._transport.exchange(request)

        # Decode the fixed M8 health payload.
        return decode_health_status(response.payload)

    # Start a fresh bounded M8 baseline learning session.
    def start_baseline(
        self,
        target_samples: int,
    ) -> BaselineControl:
        """Execute BASELINE_CONTROL START and return normalized configuration."""

        # Build and validate the shared baseline-control payload.
        control = BaselineControl(
            action=BaselineAction.START,
            target_samples=target_samples,
        )

        # Encode before transport side effects so invalid targets fail locally.
        payload = encode_baseline_control(control)

        # Allocate one request correlation sequence.
        sequence = self._sequence_manager.next()

        # Build the baseline-start request.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.BASELINE_CONTROL,
            sequence=sequence,
            payload=payload,
        )

        # Execute one synchronous transport exchange.
        response = self._transport.exchange(request)

        # Decode and validate the device-normalized response.
        normalized = decode_baseline_control(
            response.payload
        )

        # Require the response to preserve START semantics.
        if normalized != control:

            # Reject a contradictory remote baseline configuration.
            raise ProtocolClientError(
                "device returned an unexpected baseline configuration"
            )

        # Return the normalized active configuration.
        return normalized

    # Erase the runtime M8 baseline.
    def reset_baseline(self) -> BaselineControl:
        """Execute BASELINE_CONTROL RESET."""

        # Build the frozen reset payload.
        control = BaselineControl(
            action=BaselineAction.RESET,
            target_samples=0,
        )

        # Encode the reset request.
        payload = encode_baseline_control(control)

        # Allocate one request correlation sequence.
        sequence = self._sequence_manager.next()

        # Build the baseline-reset request.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.BASELINE_CONTROL,
            sequence=sequence,
            payload=payload,
        )

        # Execute one synchronous transport exchange.
        response = self._transport.exchange(request)

        # Decode the normalized response.
        normalized = decode_baseline_control(
            response.payload
        )

        # Require exact RESET acknowledgement.
        if normalized != control:

            # Reject contradictory remote semantics.
            raise ProtocolClientError(
                "device did not acknowledge baseline reset"
            )

        # Return the normalized reset response.
        return normalized

    # Read the current M9 supervisory-control snapshot.
    def control_status(self) -> ControlStatus:
        """Execute GET_CONTROL_STATUS and decode its fixed M9 payload."""

        # Allocate one request correlation sequence.
        sequence = self._sequence_manager.next()

        # Build the empty control-status request.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.GET_CONTROL_STATUS,
            sequence=sequence,
        )

        # Execute one synchronous transport exchange.
        response = self._transport.exchange(request)

        # Decode the fixed M9 control payload.
        return decode_control_status(response.payload)

    # Execute one M9 host supervisory action.
    def control_action(
        self,
        action: ControlAction,
    ) -> ControlCommandResult:
        """Execute one safety-gated CONTROL_COMMAND."""

        # Build the immutable shared command model.
        command = ControlCommand(
            action=action
        )

        # Validate and encode before transport side effects.
        payload = encode_control_command(
            command
        )

        # Allocate one request correlation sequence.
        sequence = self._sequence_manager.next()

        # Build the fixed control request.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.CONTROL_COMMAND,
            sequence=sequence,
            payload=payload,
        )

        # Execute one synchronous transport exchange.
        response = self._transport.exchange(request)

        # Decode the normalized successful command result.
        result = decode_control_command_result(
            response.payload
        )

        # Require the device to acknowledge the exact requested action.
        if result.action != ControlAction(action):

            # Reject contradictory remote control semantics.
            raise ProtocolClientError(
                "device returned an unexpected control action"
            )

        # Return the typed normalized result.
        return result
