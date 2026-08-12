"""Unit tests for the Guardian Protocol v0.1 incremental stream parser."""

# Import unittest from the Python standard library.
import unittest

# Import the public protocol objects required by parser tests.
from guardian_protocol import Command, Frame, IncrementalParser, MessageType, encode_frame


# Verify stream fragmentation, noise recovery and multi-frame behavior.
class IncrementalParserTests(unittest.TestCase):
    """Exercise the stateful transport-independent stream parser."""

    # Verify that every possible byte boundary can split a valid frame safely.
    def test_fragmented_frame_is_reassembled(self) -> None:

        # Create one valid binary frame.
        encoded = encode_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.GET_STATUS,
                sequence=0x10203040,
            )
        )

        # Test every split position inside the frame.
        for split_index in range(1, len(encoded)):

            # Create a fresh parser so every split case starts identically.
            parser = IncrementalParser()

            # Feed the first incomplete portion and require no premature frame.
            first_result = parser.feed(encoded[:split_index])

            # Feed the remaining bytes and capture the completed frame.
            second_result = parser.feed(encoded[split_index:])

            # Require the first fragment to remain incomplete.
            self.assertEqual(first_result, [])

            # Require exactly one completed frame after the second fragment.
            self.assertEqual(len(second_result), 1)

            # Require the request sequence to survive stream reassembly.
            self.assertEqual(second_result[0].sequence, 0x10203040)

    # Verify that unrelated bytes before a frame do not prevent synchronization.
    def test_noise_before_magic_is_discarded(self) -> None:

        # Create a valid PING request.
        encoded = encode_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.PING,
                sequence=9,
            )
        )

        # Create a new incremental parser.
        parser = IncrementalParser()

        # Feed unsynchronized noise followed by one complete frame.
        frames = parser.feed(b"\x00\xFFNOISE" + encoded)

        # Require the valid frame to be recovered.
        self.assertEqual(len(frames), 1)

        # Require the parser to report discarded unsynchronized bytes.
        self.assertGreater(parser.stats.discarded_bytes, 0)

    # Verify that one transport read may safely contain several complete frames.
    def test_multiple_frames_in_one_feed(self) -> None:

        # Encode the first independent request.
        first = encode_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.PING,
                sequence=1,
            )
        )

        # Encode the second independent request.
        second = encode_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.DEVICE_INFO,
                sequence=2,
            )
        )

        # Create a parser with no previous stream state.
        parser = IncrementalParser()

        # Feed both frames as one transport block.
        frames = parser.feed(first + second)

        # Require both complete frames to be returned.
        self.assertEqual(len(frames), 2)

        # Require ordering to match the original transport byte order.
        self.assertEqual([frame.sequence for frame in frames], [1, 2])

    # Verify that one damaged frame does not prevent a later frame from decoding.
    def test_crc_error_recovers_for_next_frame(self) -> None:

        # Encode a frame that will be intentionally corrupted.
        damaged = bytearray(
            encode_frame(
                Frame(
                    message_type=MessageType.REQUEST,
                    command=Command.PING,
                    sequence=1,
                )
            )
        )

        # Corrupt the CRC trailer without changing the header or payload length.
        damaged[-1] ^= 0xFF

        # Encode a second valid request immediately after the damaged frame.
        valid = encode_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.GET_STATUS,
                sequence=2,
            )
        )

        # Create a fresh parser.
        parser = IncrementalParser()

        # Feed the damaged and valid frames in one stream block.
        frames = parser.feed(bytes(damaged) + valid)

        # Require the damaged frame to be discarded and the second frame to survive.
        self.assertEqual(len(frames), 1)

        # Require the valid frame sequence to identify the surviving request.
        self.assertEqual(frames[0].sequence, 2)

        # Require an explicit CRC diagnostic counter increment.
        self.assertEqual(parser.stats.crc_errors, 1)

    # Verify that impossible payload lengths are rejected before large buffering occurs.
    def test_oversized_declared_payload_is_rejected(self) -> None:

        # Build only a header containing the correct synchronization bytes.
        malicious_header = bytearray.fromhex("474601010100000000010101")

        # Create a fresh parser.
        parser = IncrementalParser()

        # Feed the header declaring a 257-byte payload, which exceeds the v0.1 bound.
        frames = parser.feed(malicious_header)

        # Require no application frame to escape from the malformed input.
        self.assertEqual(frames, [])

        # Require the explicit bounded-length diagnostic to increment.
        self.assertEqual(parser.stats.oversize_errors, 1)


# Execute the tests when this file is run directly.
if __name__ == "__main__":

    # Start the standard-library unit test runner with verbose output.
    unittest.main(verbosity=2)
