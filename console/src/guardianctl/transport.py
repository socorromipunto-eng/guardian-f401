"""Transport contracts and TCP request-response implementation for guardianctl."""

# Import socket for the dependency-free TCP development transport.
import socket

# Import Protocol for structural typing across TCP and serial transports.
from typing import Protocol

# Import immutable TCP configuration used by this transport.
from .config import ClientConfig

# Import host-level exceptions for controlled operator diagnostics.
from .errors import ProtocolClientError, RemoteDeviceError, TransportError

# Import canonical Guardian framing and parsing primitives.
from guardian_protocol import (
    Frame,
    IncrementalParser,
    MessageType,
    encode_frame,
)


# Define the minimal exchange contract consumed by GuardianClient.
class ExchangeTransport(Protocol):
    """Structural request-response transport contract."""

    # Return operator-facing endpoint text.
    @property
    def endpoint(self) -> str:
        """Return a stable endpoint description."""
        ...

    # Execute one synchronous Guardian request-response exchange.
    def exchange(self, request: Frame) -> Frame:
        """Return the correlated response for *request*."""
        ...


# Validate one decoded frame against synchronous request correlation rules.
def validate_correlated_response(
    request: Frame,
    response: Frame,
) -> Frame:
    """Return a valid synchronous response or raise a structured client error."""

    # Reject contradictory command correlation.
    if response.command != request.command:

        # Raise a precise host-side protocol diagnostic.
        raise ProtocolClientError(
            (
                "response command does not match request: "
                f"0x{response.command:02X} != "
                f"0x{request.command:02X}"
            )
        )

    # Convert remote ERROR frames into structured host exceptions.
    if response.message_type == MessageType.ERROR:

        # Require the frozen one-byte error payload.
        if len(response.payload) != 1:

            # Reject malformed remote error semantics.
            raise ProtocolClientError(
                "device ERROR frame must contain exactly one error byte"
            )

        # Raise a stable error while preserving command and error identifiers.
        raise RemoteDeviceError(
            command=response.command,
            error_code=response.payload[0],
        )

    # Require a successful synchronous response message class.
    if response.message_type != MessageType.RESPONSE:

        # Reject event or telemetry traffic correlated as a response.
        raise ProtocolClientError(
            (
                "unexpected correlated message type: "
                f"{response.message_type.name}"
            )
        )

    # Return the validated synchronous response.
    return response


# Send one Guardian request and wait for its correlated TCP response.
class GuardianTcpTransport:
    """One-request-per-connection Guardian TCP development transport."""

    # Store immutable network configuration.
    def __init__(self, config: ClientConfig | None = None) -> None:

        # Use explicit configuration or safe local-development defaults.
        self._config = config or ClientConfig()

    # Expose immutable configuration for tests and integration.
    @property
    def config(self) -> ClientConfig:
        """Return immutable transport configuration."""

        # Return configuration without exposing mutable state.
        return self._config

    # Expose one stable endpoint description required by ExchangeTransport.
    @property
    def endpoint(self) -> str:
        """Return the configured TCP endpoint."""

        # Delegate endpoint formatting to immutable configuration.
        return self._config.endpoint

    # Execute one bounded request-response exchange.
    def exchange(self, request: Frame) -> Frame:
        """Send *request* and return its correlated RESPONSE frame."""

        # Reject non-request frames before opening a connection.
        if request.message_type != MessageType.REQUEST:

            # Raise a host contract error instead of transmitting invalid traffic.
            raise ProtocolClientError("transport exchange requires a REQUEST frame")

        # Encode the complete request before opening the socket.
        encoded_request = encode_frame(request)

        # Create independent parser state for this connection.
        parser = IncrementalParser()

        # Build the configured remote endpoint tuple.
        endpoint = (self._config.host, self._config.port)

        # Normalize expected transport failures into guardianctl exceptions.
        try:

            # Open a TCP connection using the configured timeout.
            with socket.create_connection(
                endpoint,
                timeout=self._config.timeout_seconds,
            ) as client:

                # Apply the same bounded timeout to response reads.
                client.settimeout(self._config.timeout_seconds)

                # Send the complete Guardian request bytes.
                client.sendall(encoded_request)

                # Continue receiving until the correlated response is found.
                while True:

                    # Receive an arbitrary stream fragment.
                    chunk = client.recv(4096)

                    # Treat disconnect before a response as transport failure.
                    if not chunk:

                        # Raise a controlled operator-facing failure.
                        raise TransportError(
                            "device disconnected before returning a response"
                        )

                    # Recover every complete validated frame.
                    frames = parser.feed(chunk)

                    # Inspect frames in original wire order.
                    for response in frames:

                        # Ignore unrelated asynchronous traffic.
                        if response.sequence != request.sequence:

                            # Continue waiting for the correlated response.
                            continue

                        # Validate and return the correlated response.
                        return validate_correlated_response(
                            request,
                            response,
                        )

        # Preserve normalized guardianctl exceptions.
        except (TransportError, ProtocolClientError, RemoteDeviceError):

            # Re-raise unchanged.
            raise

        # Convert socket timeout into a concise diagnostic.
        except socket.timeout as exc:

            # Raise stable transport context.
            raise TransportError(
                f"timed out waiting for Guardian device at {self.endpoint}"
            ) from exc

        # Convert operating-system network failures into stable diagnostics.
        except OSError as exc:

            # Raise stable transport context.
            raise TransportError(
                f"cannot communicate with Guardian device at {self.endpoint}: {exc}"
            ) from exc
