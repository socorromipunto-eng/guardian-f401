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
    SecurityStatus,
    decode_device_info,
    decode_baseline_control,
    decode_control_command_result,
    decode_control_status,
    decode_device_status,
    decode_dsp_features,
    decode_health_status,
    decode_security_status,
    encode_baseline_control,
    encode_control_command,
)

# Import immutable M10 host security configuration.
from .config import SecurityClientConfig

# Import host-side protocol contract errors.
from .errors import ProtocolClientError

# Import the M10 authenticated-session manager.
from .security_client import GuardianSecuritySession

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
        security_config: SecurityClientConfig | None = None,
    ) -> None:

        # Use explicit transport or default local TCP.
        self._transport = transport or GuardianTcpTransport()

        # Use explicit sequence allocator or start from one.
        self._sequence_manager = sequence_manager or SequenceManager()

        # Create M10 authenticated-session state only when credentials were supplied.
        self._security_session = (
            GuardianSecuritySession(
                transport=self._transport,
                sequence_manager=self._sequence_manager,
                config=security_config,
            )
            if security_config is not None
            else None
        )

    # Expose configured exchange transport.
    @property
    def transport(self) -> ExchangeTransport:
        """Return the configured synchronous transport."""

        # Return transport without changing state.
        return self._transport

    # Read public M10 security/session diagnostics without authentication.
    def security_status(self) -> SecurityStatus:
        """Execute GET_SECURITY_STATUS and decode its fixed public payload."""

        # Allocate request correlation sequence.
        sequence = self._sequence_manager.next()

        # Build the empty security-status request.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.GET_SECURITY_STATUS,
            sequence=sequence,
        )

        # Execute ordinary read-only transport exchange.
        response = self._transport.exchange(request)

        # Decode fixed M10 public diagnostics.
        return decode_security_status(
            response.payload
        )

    # Explicitly establish a new authenticated M10 session.
    def authenticate_security(self):
        """Authenticate using configured M10 credentials and return session metadata."""

        # Require credentials for an explicit handshake.
        if self._security_session is None:

            # Reject missing host provisioning before transport side effects.
            raise ProtocolClientError(
                "M10 security credentials are not configured"
            )

        # Establish a fresh challenge-response session.
        return self._security_session.authenticate()

    # Execute one privileged command through M10 when credentials are configured.
    def _privileged_exchange(
        self,
        command: Command,
        payload: bytes,
    ) -> Frame:
        """Use SECURE_COMMAND when configured, otherwise preserve legacy direct mode."""

        # Use authenticated, authorized and anti-replay-protected wrapping when available.
        if self._security_session is not None:

            # Execute one M10 protected operation.
            return self._security_session.exchange(
                int(command),
                payload,
            )

        # Preserve M8/M9 compatibility with explicitly insecure simulator mode.
        sequence = self._sequence_manager.next()

        # Build the legacy direct privileged request.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=command,
            sequence=sequence,
            payload=payload,
        )

        # Execute the legacy direct exchange.
        return self._transport.exchange(
            request
        )

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

        # Execute BASELINE_CONTROL through M10 when credentials are configured.
        response = self._privileged_exchange(
            Command.BASELINE_CONTROL,
            payload,
        )

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

        # Execute BASELINE_CONTROL through M10 when credentials are configured.
        response = self._privileged_exchange(
            Command.BASELINE_CONTROL,
            payload,
        )

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

        # Execute CONTROL_COMMAND through M10 when credentials are configured.
        response = self._privileged_exchange(
            Command.CONTROL_COMMAND,
            payload,
        )

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
