"""TCP transport for the Guardian F401 software device simulator."""

# Import socket for bounded receive timeouts that allow telemetry polling.
import socket

# Import socketserver for a dependency-free local TCP transport.
import socketserver

# Import the shared incremental parser and frame encoder.
from guardian_protocol import IncrementalParser, encode_frame

# Import immutable simulator network configuration.
from .config import SimulatorConfig

# Import the transport-independent simulated device application.
from .device import GuardianDevice


# Handle one TCP client while keeping protocol parsing independent from packetization.
class GuardianRequestHandler(socketserver.BaseRequestHandler):
    """Receive requests and asynchronously emit M5 telemetry."""

    # Process one connected client until it disconnects.
    def handle(self) -> None:

        # Create independent parser state for this client connection.
        parser = IncrementalParser()

        # Create one opaque token that owns telemetry enabled by this connection.
        telemetry_owner = object()

        # Use a short timeout so telemetry can be emitted without incoming traffic.
        self.request.settimeout(0.02)

        # Guarantee telemetry ownership cleanup after every disconnect path.
        try:

            # Continue until the client closes the TCP stream.
            while True:

                # Default to no incoming bytes during this loop iteration.
                chunk = None

                # Attempt one bounded receive.
                try:

                    # Receive an arbitrary transport fragment.
                    chunk = self.request.recv(4096)
                except socket.timeout:

                    # A short timeout is expected while waiting to emit telemetry.
                    chunk = None

                # Stop cleanly after an orderly client disconnect.
                if chunk == b"":

                    # Exit through the ownership-cleanup finally block.
                    return

                # Process incoming bytes when this iteration received any.
                if chunk:

                    # Snapshot parser failures before consuming this block.
                    previous_errors = (
                        parser.stats.crc_errors
                        + parser.stats.oversize_errors
                        + parser.stats.protocol_errors
                    )

                    # Recover every complete valid frame from arbitrary TCP bytes.
                    frames = parser.feed(chunk)

                    # Snapshot parser failures after consuming this block.
                    current_errors = (
                        parser.stats.crc_errors
                        + parser.stats.oversize_errors
                        + parser.stats.protocol_errors
                    )

                    # Calculate new parser failures observed in this block.
                    new_errors = (
                        current_errors - previous_errors
                    )

                    # Add parser failures to device diagnostics.
                    if new_errors:

                        # Record failures that cannot safely produce correlated ERROR frames.
                        self.server.guardian_device.record_protocol_errors(
                            new_errors
                        )

                    # Process every validated request in original wire order.
                    for frame in frames:

                        # Execute command semantics with this connection's telemetry ownership.
                        response = self.server.guardian_device.process_frame(
                            frame,
                            telemetry_owner,
                        )

                        # Encode the response using the canonical protocol codec.
                        encoded_response = encode_frame(
                            response
                        )

                        # Send the complete response to the connected client.
                        self.request.sendall(
                            encoded_response
                        )

                # Ask whether this connection owns one due asynchronous sample.
                telemetry_frame = (
                    self.server.guardian_device.poll_telemetry(
                        telemetry_owner
                    )
                )

                # Send at most one telemetry frame per server loop iteration.
                if telemetry_frame is not None:

                    # Encode the asynchronous frame using the canonical codec.
                    encoded_telemetry = encode_frame(
                        telemetry_frame
                    )

                    # Send the complete telemetry frame.
                    self.request.sendall(
                        encoded_telemetry
                    )
        finally:

            # Disable telemetry if this connection owned the active subscription.
            self.server.guardian_device.release_telemetry(
                telemetry_owner
            )


# Provide a reusable TCP server class for CLI and integration tests.
class GuardianTcpServer(socketserver.ThreadingTCPServer):
    """Threaded development TCP transport for one GuardianDevice instance."""

    # Allow rapid restart during development.
    allow_reuse_address = True

    # Ensure client threads cannot keep the simulator process alive during shutdown.
    daemon_threads = True

    # Create the TCP listener and attach the device model.
    def __init__(
        self,
        server_address: tuple[str, int],
        guardian_device: GuardianDevice,
    ) -> None:

        # Preserve the shared simulated device application.
        self.guardian_device = guardian_device

        # Initialize the standard-library TCP server.
        super().__init__(
            server_address,
            GuardianRequestHandler,
        )


# Create and run one simulator until interrupted by the operator.
def run_server(
    config: SimulatorConfig | None = None,
) -> None:
    """Run the Guardian TCP simulator until KeyboardInterrupt."""

    # Use explicit caller configuration or safe loopback defaults.
    active_config = config or SimulatorConfig()

    # Create the transport-independent simulated device.
    device = GuardianDevice(active_config)

    # Create the TCP server using the configured endpoint.
    with GuardianTcpServer(
        (active_config.host, active_config.port),
        device,
    ) as server:

        # Read the actual listener endpoint.
        host, port = server.server_address

        # Print one concise startup message.
        print(
            f"Guardian F401 simulator listening on {host}:{port}"
        )

        # Print the supported synchronous command set.
        print(
            "Commands: PING, DEVICE_INFO, GET_STATUS, SET_TELEMETRY"
        )

        # Print the asynchronous channel.
        print(
            "Telemetry: MACHINE_TELEMETRY"
        )

        # Explain how to stop the foreground server.
        print("Press Ctrl+C to stop.")

        # Run client handling until operator interruption.
        try:

            # Enter the standard-library request loop.
            server.serve_forever()
        except KeyboardInterrupt:

            # Print deterministic shutdown text.
            print("\nGuardian F401 simulator stopped.")
