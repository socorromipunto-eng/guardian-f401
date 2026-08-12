"""Persistent host-side telemetry streaming for Guardian F401 M5."""

# Import socket for persistent TCP telemetry sessions.
import socket

# Import time for bounded response and sample deadlines.
import time

# Import dataclass for immutable decoded telemetry records.
from dataclasses import dataclass

# Import Callable and Protocol for consumer and byte-stream contracts.
from typing import Any, Callable, Protocol

# Import immutable TCP and serial endpoint configuration.
from .config import ClientConfig, SerialConfig

# Import normalized guardianctl exceptions.
from .errors import (
    ProtocolClientError,
    RemoteDeviceError,
    TransportError,
)

# Import the request sequence allocator.
from .sequence import SequenceManager

# Import the reusable physical serial opener.
from .serial_transport import SerialFactory, open_serial_port

# Import the shared synchronous response validator.
from .transport import validate_correlated_response

# Import canonical Guardian protocol models and codecs.
from guardian_protocol import (
    Command,
    Frame,
    IncrementalParser,
    MachineTelemetry,
    MessageType,
    TelemetryConfig,
    decode_machine_telemetry,
    decode_telemetry_config,
    encode_frame,
    encode_telemetry_config,
)


# Store one decoded asynchronous frame with its wire-level sequence.
@dataclass(frozen=True, slots=True)
class TelemetryRecord:
    """One validated asynchronous machine telemetry record."""

    # Store the independent telemetry frame sequence.
    sequence: int

    # Store the decoded command-specific machine sample.
    sample: MachineTelemetry


# Define the byte-stream behavior required by the telemetry session.
class _ByteStream(Protocol):
    """Minimal persistent byte-stream contract."""

    # Write one complete Guardian frame.
    def write(self, data: bytes) -> int:
        """Return the number of accepted bytes."""
        ...

    # Read up to the requested number of bytes.
    def read(self, size: int) -> bytes:
        """Return zero or more received bytes."""
        ...


# Implement a persistent bounded TCP byte stream.
class _TcpByteStream:
    """Persistent TCP stream used by M5 telemetry."""

    # Store immutable network configuration.
    def __init__(self, config: ClientConfig) -> None:

        # Preserve the validated endpoint configuration.
        self._config = config

        # Start without an open socket.
        self._socket: socket.socket | None = None

    # Open the TCP endpoint.
    def __enter__(self) -> "_TcpByteStream":

        # Convert connection failures into stable guardianctl diagnostics.
        try:

            # Open the configured TCP endpoint.
            self._socket = socket.create_connection(
                (self._config.host, self._config.port),
                timeout=self._config.timeout_seconds,
            )

            # Use short reads so the total telemetry deadline remains responsive.
            self._socket.settimeout(
                min(0.05, self._config.timeout_seconds)
            )

            # Return the opened persistent stream.
            return self
        except OSError as exc:

            # Raise a stable endpoint-specific failure.
            raise TransportError(
                (
                    "cannot open Guardian telemetry stream at "
                    f"{self._config.endpoint}: {exc}"
                )
            ) from exc

    # Close the TCP endpoint.
    def __exit__(
        self,
        exc_type: Any,
        exc_value: Any,
        traceback: Any,
    ) -> None:

        # Close the socket only when it was opened successfully.
        if self._socket is not None:

            # Release the operating-system socket.
            self._socket.close()

            # Remove the closed socket reference.
            self._socket = None

    # Send one complete Guardian frame.
    def write(self, data: bytes) -> int:
        """Send all bytes and return the accepted byte count."""

        # Reject use before the context manager opens the socket.
        if self._socket is None:

            # Raise a stable host-side transport error.
            raise TransportError("TCP telemetry stream is not open")

        # Convert operating-system send failures into stable diagnostics.
        try:

            # Send the complete encoded frame.
            self._socket.sendall(data)

            # Report atomic acceptance of all requested bytes.
            return len(data)
        except OSError as exc:

            # Raise a stable endpoint-specific failure.
            raise TransportError(
                f"telemetry write failed at {self._config.endpoint}: {exc}"
            ) from exc

    # Read one bounded TCP fragment.
    def read(self, size: int) -> bytes:
        """Return one bounded TCP fragment."""

        # Reject use before the context manager opens the socket.
        if self._socket is None:

            # Raise a stable host-side transport error.
            raise TransportError("TCP telemetry stream is not open")

        # Convert normal short socket timeouts into an empty polling result.
        try:

            # Receive up to the requested number of bytes.
            return self._socket.recv(size)
        except socket.timeout:

            # Return no bytes so the total deadline can be checked.
            return b""
        except OSError as exc:

            # Raise a stable endpoint-specific failure.
            raise TransportError(
                f"telemetry read failed at {self._config.endpoint}: {exc}"
            ) from exc


# Implement a persistent bounded physical serial byte stream.
class _SerialByteStream:
    """Persistent serial stream used by M5 telemetry."""

    # Store immutable serial configuration and an injectable factory.
    def __init__(
        self,
        config: SerialConfig,
        serial_factory: SerialFactory,
    ) -> None:

        # Preserve the validated physical endpoint configuration.
        self._config = config

        # Preserve the real or test serial object factory.
        self._serial_factory = serial_factory

        # Start without an open serial object.
        self._serial_port: Any = None

        # Track whether the returned object entered its own context manager.
        self._entered_context = False

    # Open the serial endpoint.
    def __enter__(self) -> "_SerialByteStream":

        # Convert serial-library failures into stable guardianctl diagnostics.
        try:

            # Create the serial-compatible endpoint object.
            serial_port = self._serial_factory(self._config)

            # Discover context-manager support.
            enter = getattr(serial_port, "__enter__", None)

            # Enter the serial object's own context manager when supported.
            if callable(enter):

                # Preserve the object returned by its context manager.
                self._serial_port = enter()

                # Remember that __exit__ must be used later.
                self._entered_context = True
            else:

                # Use the factory object directly.
                self._serial_port = serial_port

                # Remember that a direct close is required later.
                self._entered_context = False

            # Discover optional stale-input clearing support.
            reset_input = getattr(
                self._serial_port,
                "reset_input_buffer",
                None,
            )

            # Clear stale bytes when supported.
            if callable(reset_input):

                # Remove old uncorrelated data before subscription.
                reset_input()

            # Return the opened persistent stream.
            return self
        except TransportError:

            # Preserve already normalized guardianctl errors.
            raise
        except Exception as exc:

            # Raise stable physical endpoint context.
            raise TransportError(
                (
                    "cannot open Guardian telemetry stream at "
                    f"{self._config.endpoint}: {exc}"
                )
            ) from exc

    # Close the serial endpoint.
    def __exit__(
        self,
        exc_type: Any,
        exc_value: Any,
        traceback: Any,
    ) -> None:

        # Ignore cleanup when endpoint creation failed.
        if self._serial_port is None:

            # Return without touching missing endpoint state.
            return

        # Use the serial object's context-manager exit when it was entered.
        if self._entered_context:

            # Discover the matching context-manager exit method.
            exit_method = getattr(
                self._serial_port,
                "__exit__",
                None,
            )

            # Invoke it only when available.
            if callable(exit_method):

                # Forward the original context exit information.
                exit_method(
                    exc_type,
                    exc_value,
                    traceback,
                )
        else:

            # Discover direct close support.
            close = getattr(
                self._serial_port,
                "close",
                None,
            )

            # Close directly when supported.
            if callable(close):

                # Release the serial endpoint.
                close()

        # Remove the closed endpoint reference.
        self._serial_port = None

    # Write one complete encoded Guardian frame.
    def write(self, data: bytes) -> int:
        """Write one complete frame and return the accepted byte count."""

        # Reject use before opening the serial endpoint.
        if self._serial_port is None:

            # Raise a stable host-side transport error.
            raise TransportError("serial telemetry stream is not open")

        # Convert serial-library failures into stable diagnostics.
        try:

            # Write the complete encoded frame.
            bytes_written = self._serial_port.write(data)

            # Discover optional host-output flush support.
            flush = getattr(
                self._serial_port,
                "flush",
                None,
            )

            # Flush host buffering when supported.
            if callable(flush):

                # Complete host-side buffered transmission.
                flush()

            # Return the serial implementation's accepted byte count.
            return int(bytes_written)
        except Exception as exc:

            # Raise stable physical endpoint context.
            raise TransportError(
                f"telemetry write failed at {self._config.endpoint}: {exc}"
            ) from exc

    # Read one bounded serial fragment.
    def read(self, size: int) -> bytes:
        """Return one bounded serial fragment."""

        # Reject use before opening the serial endpoint.
        if self._serial_port is None:

            # Raise a stable host-side transport error.
            raise TransportError("serial telemetry stream is not open")

        # Convert serial-library failures into stable diagnostics.
        try:

            # Return the serial implementation's bounded read result.
            return bytes(self._serial_port.read(size))
        except Exception as exc:

            # Raise stable physical endpoint context.
            raise TransportError(
                f"telemetry read failed at {self._config.endpoint}: {exc}"
            ) from exc


# Manage one bounded asynchronous telemetry subscription.
class TelemetryMonitor:
    """Enable, receive and disable Guardian machine telemetry."""

    # Store endpoint configuration and an optional injected serial factory.
    def __init__(
        self,
        config: ClientConfig | SerialConfig,
        serial_factory: SerialFactory | None = None,
    ) -> None:

        # Preserve the validated TCP or serial endpoint configuration.
        self._config = config

        # Use an injected test factory or the real lazy pyserial opener.
        self._serial_factory = serial_factory or open_serial_port

        # Allocate independent request sequences for subscription control.
        self._sequence_manager = SequenceManager()

    # Return operator-facing endpoint text.
    @property
    def endpoint(self) -> str:
        """Return the configured telemetry endpoint."""

        # Delegate formatting to immutable endpoint configuration.
        return self._config.endpoint

    # Create the correct persistent byte stream for the endpoint type.
    def _create_stream(
        self,
    ) -> _TcpByteStream | _SerialByteStream:

        # Select physical serial mode from configuration type.
        if isinstance(self._config, SerialConfig):

            # Return the persistent physical serial stream.
            return _SerialByteStream(
                self._config,
                self._serial_factory,
            )

        # Return the persistent TCP development stream.
        return _TcpByteStream(self._config)

    # Send one complete Guardian request atomically.
    @staticmethod
    def _send_request(
        stream: _ByteStream,
        request: Frame,
    ) -> None:

        # Encode the complete request using the canonical protocol codec.
        encoded = encode_frame(request)

        # Write the complete encoded request.
        written = stream.write(encoded)

        # Reject partial writes because protocol frame atomicity was lost.
        if written != len(encoded):

            # Raise a stable transport diagnostic.
            raise TransportError(
                f"telemetry stream wrote {written}/{len(encoded)} bytes"
            )

    # Return the next decoded frame before a monotonic deadline.
    @staticmethod
    def _next_frame(
        stream: _ByteStream,
        parser: IncrementalParser,
        pending: list[Frame],
        deadline: float,
    ) -> Frame:

        # Continue until one decoded frame is available or the deadline expires.
        while time.monotonic() < deadline:

            # Consume already decoded frames before reading more bytes.
            if pending:

                # Preserve original wire order.
                return pending.pop(0)

            # Read one bounded byte-stream fragment.
            chunk = stream.read(128)

            # Treat an orderly TCP disconnect as an immediate failure.
            if chunk == b"":

                # Continue short polling until the deadline because serial uses empty reads.
                continue

            # Decode every complete frame from arbitrary fragmentation.
            pending.extend(parser.feed(chunk))

        # Report a bounded telemetry stream timeout.
        raise TransportError("timed out waiting for Guardian telemetry data")

    # Wait for one request-correlated synchronous control response.
    @classmethod
    def _wait_for_response(
        cls,
        stream: _ByteStream,
        parser: IncrementalParser,
        pending: list[Frame],
        request: Frame,
        timeout_seconds: float,
    ) -> Frame:

        # Calculate one absolute control-response deadline.
        deadline = time.monotonic() + timeout_seconds

        # Continue until the request-correlated response arrives.
        while True:

            # Read the next decoded frame.
            response = cls._next_frame(
                stream,
                parser,
                pending,
                deadline,
            )

            # Ignore unrelated asynchronous sequence values.
            if response.sequence != request.sequence:

                # Continue waiting for the correlated control response.
                continue

            # Validate command, response class and remote ERROR semantics.
            return validate_correlated_response(
                request,
                response,
            )

    # Stream a bounded number of live telemetry samples to a consumer callback.
    def stream_samples(
        self,
        period_ms: int,
        count: int,
        consumer: Callable[[TelemetryRecord], None],
    ) -> int:
        """Stream *count* live samples and return the delivered sample count."""

        # Reject empty or unbounded accidental sample requests.
        if not 1 <= count <= 100000:

            # Raise a precise CLI/API validation error.
            raise ValueError("count must be between 1 and 100000")

        # Build and validate the requested active configuration.
        enable_config = TelemetryConfig(
            enabled=True,
            period_ms=period_ms,
        )

        # Encode once so period policy is validated before opening the endpoint.
        enable_payload = encode_telemetry_config(
            enable_config
        )

        # Build the matching disabled configuration for deterministic cleanup.
        disable_config = TelemetryConfig(
            enabled=False,
            period_ms=period_ms,
        )

        # Encode the cleanup configuration before endpoint side effects.
        disable_payload = encode_telemetry_config(
            disable_config
        )

        # Create the selected persistent TCP or serial stream.
        stream = self._create_stream()

        # Create independent parser state for the persistent session.
        parser = IncrementalParser()

        # Preserve decoded frames that arrive together in one transport fragment.
        pending: list[Frame] = []

        # Track delivered telemetry samples.
        delivered = 0

        # Track whether the enable request was acknowledged successfully.
        enabled = False

        # Open the persistent endpoint.
        with stream:

            # Allocate one control request sequence.
            enable_sequence = self._sequence_manager.next()

            # Build the SET_TELEMETRY enable request.
            enable_request = Frame(
                message_type=MessageType.REQUEST,
                command=Command.SET_TELEMETRY,
                sequence=enable_sequence,
                payload=enable_payload,
            )

            # Send the complete subscription request.
            self._send_request(
                stream,
                enable_request,
            )

            # Wait for the correlated normalized configuration response.
            enable_response = self._wait_for_response(
                stream,
                parser,
                pending,
                enable_request,
                self._config.timeout_seconds,
            )

            # Decode and verify the normalized active configuration.
            active_config = decode_telemetry_config(
                enable_response.payload
            )

            # Require the device to acknowledge telemetry enabled.
            if active_config != enable_config:

                # Reject contradictory subscription state.
                raise ProtocolClientError(
                    "device returned an unexpected telemetry configuration"
                )

            # Mark the subscription active only after acknowledgement.
            enabled = True

            # Calculate a per-sample timeout that scales with the requested period.
            sample_timeout = max(
                self._config.timeout_seconds,
                (period_ms / 1000.0) * 3.0 + 0.1,
            )

            # Ensure cleanup is attempted after consumer or transport failures.
            try:

                # Continue until the requested sample count has been delivered.
                while delivered < count:

                    # Calculate one absolute sample deadline.
                    deadline = time.monotonic() + sample_timeout

                    # Read the next decoded frame.
                    frame = self._next_frame(
                        stream,
                        parser,
                        pending,
                        deadline,
                    )

                    # Ignore non-telemetry traffic unrelated to this stream.
                    if frame.message_type != MessageType.TELEMETRY:

                        # Continue waiting for an asynchronous sample.
                        continue

                    # Ignore telemetry channels not implemented by this monitor.
                    if frame.command != int(Command.MACHINE_TELEMETRY):

                        # Continue waiting for the machine sample channel.
                        continue

                    # Decode and validate the fixed M5 telemetry payload.
                    sample = decode_machine_telemetry(
                        frame.payload
                    )

                    # Build one immutable record preserving the wire sequence.
                    record = TelemetryRecord(
                        sequence=frame.sequence,
                        sample=sample,
                    )

                    # Deliver the live sample immediately to the caller.
                    consumer(record)

                    # Count one successfully decoded and delivered sample.
                    delivered += 1
            finally:

                # Attempt deterministic remote cleanup only after enable acknowledgement.
                if enabled:

                    # Allocate one independent disable request sequence.
                    disable_sequence = self._sequence_manager.next()

                    # Build the SET_TELEMETRY disable request.
                    disable_request = Frame(
                        message_type=MessageType.REQUEST,
                        command=Command.SET_TELEMETRY,
                        sequence=disable_sequence,
                        payload=disable_payload,
                    )

                    # Attempt best-effort cleanup without masking an earlier failure.
                    try:

                        # Send the complete disable request.
                        self._send_request(
                            stream,
                            disable_request,
                        )

                        # Wait briefly for the normalized disabled acknowledgement.
                        disable_response = self._wait_for_response(
                            stream,
                            parser,
                            pending,
                            disable_request,
                            self._config.timeout_seconds,
                        )

                        # Decode the disabled configuration response.
                        normalized_disabled = decode_telemetry_config(
                            disable_response.payload
                        )

                        # Reject contradictory cleanup only during otherwise successful flow.
                        if normalized_disabled != disable_config:

                            # Raise a host protocol contract failure.
                            raise ProtocolClientError(
                                "device did not acknowledge telemetry disable"
                            )
                    except (
                        TransportError,
                        ProtocolClientError,
                        RemoteDeviceError,
                    ):

                        # Preserve the original active-stream exception when one is already propagating.
                        if delivered < count:

                            # Ignore cleanup failure because an earlier stream failure is primary.
                            pass
                        else:

                            # Re-raise cleanup failure after an otherwise successful stream.
                            raise

        # Return the number of live samples delivered to the caller.
        return delivered
