"""Unit tests for the Guardian Protocol v0.1 Python frame codec."""

# Import unittest from the Python standard library.
import unittest

# Import the public protocol API under test.
from guardian_protocol import (
    Command,
    Frame,
    MessageType,
    ProtocolDecodeError,
    ProtocolResult,
    crc32_ieee,
    decode_frame,
    encode_frame,
)


# Verify deterministic CRC and binary frame behavior.
class FrameCodecTests(unittest.TestCase):
    """Exercise Guardian Protocol frame encoding and decoding."""

    # Verify the standard CRC32 reference string before protocol-specific vectors.
    def test_crc32_reference_vector(self) -> None:

        # Calculate the canonical IEEE CRC32 reference value.
        actual_crc = crc32_ieee(b"123456789")

        # Verify the implementation against the published CRC32 check value.
        self.assertEqual(actual_crc, 0xCBF43926)

    # Verify that Python produces the canonical empty-payload PING request.
    def test_ping_request_matches_canonical_vector(self) -> None:

        # Build the canonical PING request object.
        frame = Frame(
            message_type=MessageType.REQUEST,
            command=Command.PING,
            sequence=1,
        )

        # Encode the request into its deterministic byte representation.
        encoded = encode_frame(frame)

        # Define the byte-for-byte protocol vector shared with the C test.
        expected = bytes.fromhex("47460101010000000001000034025e68")

        # Require exact cross-language wire compatibility.
        self.assertEqual(encoded, expected)

    # Verify that Python produces the canonical PONG response.
    def test_pong_response_matches_canonical_vector(self) -> None:

        # Build the canonical successful PING response.
        frame = Frame(
            message_type=MessageType.RESPONSE,
            command=Command.PING,
            sequence=1,
            payload=b"PONG",
        )

        # Encode the response into its deterministic byte representation.
        encoded = encode_frame(frame)

        # Define the byte-for-byte response vector shared with the specification.
        expected = bytes.fromhex(
            "474601020100000000010004504f4e47d4b52c94"
        )

        # Require exact wire compatibility with the protocol specification.
        self.assertEqual(encoded, expected)

    # Verify that a valid frame survives an encode/decode round trip.
    def test_round_trip_preserves_fields(self) -> None:

        # Construct a frame containing a non-empty binary payload.
        original = Frame(
            message_type=MessageType.RESPONSE,
            command=Command.GET_STATUS,
            sequence=0x11223344,
            payload=b"\x00\x01\xFE\xFF",
        )

        # Encode and immediately decode the complete frame.
        decoded = decode_frame(encode_frame(original))

        # Require the decoded frame to match the original high-level value.
        self.assertEqual(decoded, original)

    # Verify that payload corruption cannot pass the CRC validation step.
    def test_crc_mismatch_is_rejected(self) -> None:

        # Encode a valid frame first.
        encoded = bytearray(
            encode_frame(
                Frame(
                    message_type=MessageType.RESPONSE,
                    command=Command.PING,
                    sequence=7,
                    payload=b"PONG",
                )
            )
        )

        # Corrupt one payload byte without updating the CRC trailer.
        encoded[12] ^= 0x01

        # Capture the structured decode failure.
        with self.assertRaises(ProtocolDecodeError) as context:

            # Attempt to decode the corrupted frame.
            decode_frame(encoded)

        # Require the decoder to identify this failure specifically as CRC corruption.
        self.assertEqual(context.exception.result, ProtocolResult.CRC_MISMATCH)

    # Verify that undefined v0.1 flags cannot be silently accepted.
    def test_nonzero_flags_are_rejected(self) -> None:

        # Start from the canonical PING vector so only the flag byte needs mutation.
        encoded = bytearray.fromhex("47460101010000000001000034025e68")

        # Set an undefined flag bit.
        encoded[5] = 0x01

        # Recalculate the CRC so this test isolates semantic flag validation.
        crc = crc32_ieee(bytes(encoded[:-4]))

        # Replace the trailer with a correct CRC for the mutated header.
        encoded[-4:] = crc.to_bytes(4, byteorder="big")

        # Capture the structured semantic decode failure.
        with self.assertRaises(ProtocolDecodeError) as context:

            # Attempt to decode the structurally intact but unsupported frame.
            decode_frame(encoded)

        # Require the exact unsupported-flags diagnostic.
        self.assertEqual(context.exception.result, ProtocolResult.UNSUPPORTED_FLAGS)


# Execute the tests when this file is run directly.
if __name__ == "__main__":

    # Start the standard-library unit test runner with verbose output.
    unittest.main(verbosity=2)
