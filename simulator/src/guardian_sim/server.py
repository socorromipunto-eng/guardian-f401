"""TCP transport for the Guardian F401 software device simulator."""

# Import socketserver for a dependency-free TCP development transport.
import socketserver

# Import the shared incremental parser and frame encoder.
from guardian_protocol import IncrementalParser, encode_frame

# Import immutable simulator network configuration.
from .config import SimulatorConfig

# Import the transport-independent simulated device application.
from .device import GuardianDevice


# Handle one TCP client while keeping protocol parsing independent from socket packetization.
class GuardianRequestHandler(socketserver.BaseRequestHandler):
    """Receive arbitrary TCP chunks and return Guardian Protocol responses."""

    # Process one connected client until it disconnects.
    def handle(self) -> None:

        # Create independent stream-parser state for this client connection.
        parser = IncrementalParser()

        # Continue receiving until the client closes the TCP stream.
        while True:

            # Receive an arbitrary transport chunk without assuming frame boundaries.
            chunk = self.request.recv(4096)

            # Stop cleanly after an orderly client disconnect.
            if not chunk:

                # Exit the connection handler.
                return

            # Snapshot parser error counters before consuming this transport block.
            previous_errors = (
                parser.stats.crc_errors
                + parser.stats.oversize_errors
                + parser.stats.protocol_errors
            )

            # Recover every complete valid Guardian frame from this arbitrary byte chunk.
            frames = parser.feed(chunk)

            # Snapshot parser error counters after consuming this transport block.
            current_errors = (
                parser.stats.crc_errors
                + parser.stats.oversize_errors
                + parser.stats.protocol_errors
            )

            # Calculate new parser failures observed while processing this chunk.
            new_errors = current_errors - previous_errors

            # Add transport/parser failures to device diagnostics when any occurred.
            if new_errors:

                # Record failures that could not safely produce request-correlated ERROR frames.
                self.server.guardian_device.record_protocol_errors(new_errors)

            # Process every validated request in original transport order.
            for frame in frames:

                # Execute command semantics independently from the TCP transport.
                response = self.server.guardian_device.process_frame(frame)

                # Serialize the response using the canonical Guardian Protocol encoder.
                encoded_response = encode_frame(response)

                # Send the complete encoded frame to the connected client.
                self.request.sendall(encoded_response)


# Provide a reusable TCP server class for the CLI and integration tests.
class GuardianTcpServer(socketserver.ThreadingTCPServer):
    """Threaded development TCP transport for one GuardianDevice instance."""

    # Allow rapid restart during development without waiting for socket timeout expiration.
    allow_reuse_address = True

    # Ensure client handler threads cannot keep the simulator process alive during shutdown.
    daemon_threads = True

    # Create the TCP listener and attach the transport-independent device model.
    def __init__(
        self,
        server_address: tuple[str, int],
        guardian_device: GuardianDevice,
    ) -> None:

        # Preserve the application model so request handlers share one simulated device.
        self.guardian_device = guardian_device

        # Initialize the standard-library TCP server with the Guardian request handler.
        super().__init__(
            server_address,
            GuardianRequestHandler,
        )


# Create and run one simulator until interrupted by the operator.
def run_server(config: SimulatorConfig | None = None) -> None:
    """Run the Guardian TCP simulator until KeyboardInterrupt."""

    # Use explicit caller configuration or safe loopback defaults.
    active_config = config or SimulatorConfig()

    # Create the transport-independent simulated device application.
    device = GuardianDevice(active_config)

    # Create the TCP server using the configured loopback endpoint.
    with GuardianTcpServer(
        (active_config.host, active_config.port),
        device,
    ) as server:

        # Read the actual listener endpoint because port zero may request an ephemeral test port.
        host, port = server.server_address

        # Print one concise startup message for a human operator.
        print(f"Guardian F401 simulator listening on {host}:{port}")

        # Print the supported M2 command set.
        print("Commands: PING, DEVICE_INFO, GET_STATUS")

        # Explain how to stop the foreground development server.
        print("Press Ctrl+C to stop.")

        # Run client handling until the operator interrupts the process.
        try:

            # Enter the standard-library request loop.
            server.serve_forever()
        except KeyboardInterrupt:

            # Print a deterministic shutdown message after operator interruption.
            print("\nGuardian F401 simulator stopped.")
