"""TCP request/response transport used by guardianctl M3."""

# Import socket for the dependency-free TCP development transport.
import socket

# Import the immutable configuration used by this transport.
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


# Send one Guardian request and wait for its correlated response.
class GuardianTcpTransport:
    """One-request-per-connection Guardian TCP development transport."""

    # Store immutable network configuration.
    def __init__(self, config: ClientConfig | None = None) -> None:

        # Use explicit caller configuration or safe local-development defaults.
        self._config = config or ClientConfig()

    # Expose immutable configuration for output formatting and tests.
    @property
    def config(self) -> ClientConfig:
        """Return immutable transport configuration."""

        # Return the configuration object without exposing mutable state.
        return self._config

    # Execute one bounded request/response exchange.
    def exchange(self, request: Frame) -> Frame:
        """Send *request* and return its correlated RESPONSE frame."""

        # Reject non-request frames before opening a network connection.
        if request.message_type != MessageType.REQUEST:

            # Raise a host contract error rather than transmitting invalid traffic.
            raise ProtocolClientError("transport exchange requires a REQUEST frame")

        # Encode the complete request before opening the socket.
        encoded_request = encode_frame(request)

        # Create independent parser state for this bounded connection.
        parser = IncrementalParser()

        # Build the configured remote endpoint tuple.
        endpoint = (self._config.host, self._config.port)

        # Convert connection failures into a stable guardianctl exception hierarchy.
        try:

            # Open a TCP connection using the configured bounded timeout.
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

                    # Receive an arbitrary stream fragment without assuming frame boundaries.
                    chunk = client.recv(4096)

                    # Treat an orderly disconnect before a response as a transport failure.
                    if not chunk:

                        # Raise a controlled operator-facing failure.
                        raise TransportError(
                            "device disconnected before returning a response"
                        )

                    # Recover every complete validated frame from this transport fragment.
                    frames = parser.feed(chunk)

                    # Inspect decoded frames in original wire order.
                    for response in frames:

                        # Ignore unrelated asynchronous traffic with a different sequence.
                        if response.sequence != request.sequence:

                            # Continue waiting for the request-correlated response.
                            continue

                        # Require the correlated response to preserve the request command.
                        if response.command != request.command:

                            # Reject contradictory correlation fields.
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

                        # Require a successful response message class for synchronous exchange.
                        if response.message_type != MessageType.RESPONSE:

                            # Reject event/telemetry traffic incorrectly correlated as a response.
                            raise ProtocolClientError(
                                (
                                    "unexpected correlated message type: "
                                    f"{response.message_type.name}"
                                )
                            )

                        # Return the completely validated correlated response.
                        return response

        # Preserve already normalized guardianctl exceptions without wrapping them again.
        except (TransportError, ProtocolClientError, RemoteDeviceError):

            # Re-raise the expected guardianctl exception unchanged.
            raise

        # Convert socket timeouts into a concise bounded-I/O diagnostic.
        except socket.timeout as exc:

            # Raise the stable guardianctl transport failure from the socket exception.
            raise TransportError(
                (
                    "timed out waiting for Guardian device at "
                    f"{self._config.host}:{self._config.port}"
                )
            ) from exc

        # Convert operating-system network errors into stable CLI diagnostics.
        except OSError as exc:

            # Raise a concise transport failure while preserving the original cause.
            raise TransportError(
                (
                    "cannot communicate with Guardian device at "
                    f"{self._config.host}:{self._config.port}: {exc}"
                )
            ) from exc
