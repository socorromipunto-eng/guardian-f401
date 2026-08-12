"""End-to-end TCP tests for the Guardian F401 M2 device simulator."""

# Import socket for a real loopback client connection.
import socket

# Import threading so the development server can run during each test.
import threading

# Import unittest from the Python standard library.
import unittest

# Import protocol objects used by the simulated host client.
from guardian_protocol import (
    Command,
    Frame,
    IncrementalParser,
    MessageType,
    decode_device_info,
    encode_frame,
)

# Import the M2 simulator application and reusable TCP server.
from guardian_sim import GuardianDevice, GuardianTcpServer


# Verify the complete byte-stream path over a real local TCP socket.
class GuardianTcpServerTests(unittest.TestCase):
    """Exercise parser, device dispatcher, encoder and TCP transport together."""

    # Start one ephemeral loopback simulator before every test.
    def setUp(self) -> None:

        # Create a fresh simulated device with independent counters.
        self.device = GuardianDevice()

        # Bind to loopback and port zero so the operating system selects a free test port.
        self.server = GuardianTcpServer(
            ("127.0.0.1", 0),
            self.device,
        )

        # Create a daemon thread running the standard-library server loop.
        self.server_thread = threading.Thread(
            target=self.server.serve_forever,
            daemon=True,
        )

        # Start accepting test-client connections.
        self.server_thread.start()

    # Stop the ephemeral simulator after every test.
    def tearDown(self) -> None:

        # Stop the server request loop.
        self.server.shutdown()

        # Close the listening socket.
        self.server.server_close()

        # Wait briefly for the server thread to terminate cleanly.
        self.server_thread.join(timeout=2.0)

    # Receive exactly one Guardian frame without assuming TCP packet boundaries.
    def _receive_frame(self, client: socket.socket) -> Frame:

        # Create independent host-side stream-parser state.
        parser = IncrementalParser()

        # Continue receiving until one valid complete response is decoded.
        while True:

            # Receive an arbitrary response chunk from the TCP stream.
            chunk = client.recv(4096)

            # Treat an unexpected disconnect as an immediate test failure.
            self.assertTrue(chunk, "simulator disconnected before returning a frame")

            # Feed the arbitrary chunk into the same transport-independent parser.
            frames = parser.feed(chunk)

            # Return the first decoded response when one becomes available.
            if frames:

                # Return exactly the first response frame for this request helper.
                return frames[0]

    # Verify a request split into one-byte TCP writes still produces a valid PONG.
    def test_fragmented_ping_round_trip(self) -> None:

        # Build the canonical M2 PING request.
        request = encode_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.PING,
                sequence=0x01020304,
            )
        )

        # Read the actual ephemeral server endpoint.
        address = self.server.server_address

        # Connect a real loopback TCP client with a deterministic timeout.
        with socket.create_connection(address, timeout=2.0) as client:

            # Apply a receive timeout so a failed server cannot hang the test suite.
            client.settimeout(2.0)

            # Send every encoded byte in a separate TCP write to force fragmentation.
            for byte in request:

                # Send exactly one protocol byte.
                client.sendall(bytes((byte,)))

            # Receive and decode one complete response frame.
            response = self._receive_frame(client)

        # Require a successful PING response after extreme transport fragmentation.
        self.assertEqual(response.message_type, MessageType.RESPONSE)

        # Require request/response sequence correlation.
        self.assertEqual(response.sequence, 0x01020304)

        # Require the frozen PONG payload.
        self.assertEqual(response.payload, b"PONG")

    # Verify a damaged frame is discarded while the next valid request still succeeds.
    def test_crc_error_recovers_for_next_request(self) -> None:

        # Encode a request that will be intentionally damaged.
        damaged = bytearray(
            encode_frame(
                Frame(
                    message_type=MessageType.REQUEST,
                    command=Command.PING,
                    sequence=1,
                )
            )
        )

        # Corrupt the final CRC byte without changing the declared frame length.
        damaged[-1] ^= 0xFF

        # Encode a valid metadata request immediately after the damaged frame.
        valid = encode_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.DEVICE_INFO,
                sequence=2,
            )
        )

        # Read the actual ephemeral server endpoint.
        address = self.server.server_address

        # Connect one real loopback TCP client.
        with socket.create_connection(address, timeout=2.0) as client:

            # Apply a receive timeout so parser recovery failures cannot hang tests.
            client.settimeout(2.0)

            # Send both frames in one continuous byte stream.
            client.sendall(bytes(damaged) + valid)

            # Receive the response to the surviving valid request.
            response = self._receive_frame(client)

        # Require the parser to discard the damaged request and answer the second request.
        self.assertEqual(response.command, Command.DEVICE_INFO)

        # Require sequence correlation to identify the second request.
        self.assertEqual(response.sequence, 2)

        # Decode the returned metadata to prove the complete command path executed.
        info = decode_device_info(response.payload)

        # Require the expected simulated model identity.
        self.assertEqual(info.model, "Guardian-F401-SIM")

        # Require the device diagnostics to record the discarded corrupted frame.
        self.assertEqual(self.device.snapshot_status().protocol_errors, 1)


# Execute the tests when this file is run directly.
if __name__ == "__main__":

    # Start the standard-library unit test runner with verbose output.
    unittest.main(verbosity=2)
