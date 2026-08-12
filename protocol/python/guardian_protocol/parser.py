"""Incremental stream parser for Guardian Protocol v0.1."""

# Import dataclass for lightweight parser diagnostic counters.
from dataclasses import dataclass

# Import protocol framing constants.
from .constants import CRC_SIZE, HEADER_SIZE, MAGIC, MAX_PAYLOAD_SIZE

# Import the complete-frame decoder and its structured failure type.
from .frame import Frame, ProtocolDecodeError, decode_frame

# Import result codes so parser diagnostics can classify failures.
from .enums import ProtocolResult


# Store observable parser counters without exposing parser internals.
@dataclass(slots=True)
class ParserStats:
    """Diagnostic counters for an incremental protocol parser."""

    # Count complete frames returned to the caller.
    frames_received: int = 0

    # Count complete candidates rejected because their CRC was incorrect.
    crc_errors: int = 0

    # Count candidates rejected because a declared payload exceeded the bound.
    oversize_errors: int = 0

    # Count all other complete-candidate protocol validation failures.
    protocol_errors: int = 0

    # Count bytes discarded while searching for frame synchronization.
    discarded_bytes: int = 0


# Parse arbitrary stream fragments without requiring message-aligned reads.
class IncrementalParser:
    """Recover Guardian Protocol frames from arbitrary byte chunks."""

    # Initialize an empty parser and its diagnostics.
    def __init__(self) -> None:

        # Store unconsumed stream bytes between feed calls.
        self._buffer = bytearray()

        # Expose cumulative diagnostics useful for telemetry and tests.
        self.stats = ParserStats()

    # Remove all buffered bytes without erasing lifetime diagnostic counters.
    def reset(self) -> None:

        # Clear only incomplete stream state.
        self._buffer.clear()

    # Consume an arbitrary byte block and return every complete valid frame.
    def feed(self, data: bytes) -> list[Frame]:
        """Consume *data* and return all newly completed valid frames."""

        # Append the new transport bytes to the incomplete stream buffer.
        self._buffer.extend(data)

        # Collect complete frames discovered during this feed operation.
        frames: list[Frame] = []

        # Continue while enough information exists to make parser progress.
        while self._buffer:

            # Search for the next complete two-byte magic sequence.
            magic_index = self._buffer.find(MAGIC)

            # Handle the case where no complete magic sequence exists yet.
            if magic_index < 0:

                # Preserve a trailing first-magic byte because the next feed may complete it.
                if self._buffer[-1:] == MAGIC[:1]:

                    # Count every byte before the preserved synchronization candidate.
                    self.stats.discarded_bytes += len(self._buffer) - 1

                    # Keep only the final possible synchronization byte.
                    del self._buffer[:-1]
                else:

                    # Count every buffered byte because none can begin a valid frame.
                    self.stats.discarded_bytes += len(self._buffer)

                    # Discard bytes that cannot contribute to a future frame.
                    self._buffer.clear()

                # Stop until additional transport bytes arrive.
                break

            # Discard noise that appears before the located magic sequence.
            if magic_index > 0:

                # Count the discarded unsynchronized prefix.
                self.stats.discarded_bytes += magic_index

                # Remove the unsynchronized prefix while preserving the magic bytes.
                del self._buffer[:magic_index]

            # Wait for the complete fixed header before reading its payload length.
            if len(self._buffer) < HEADER_SIZE:

                # Stop because the next feed call may complete the header.
                break

            # Decode the two-byte big-endian payload length directly from its fixed offset.
            payload_length = int.from_bytes(self._buffer[10:12], byteorder="big")

            # Reject a malicious or corrupted length before waiting for excessive input.
            if payload_length > MAX_PAYLOAD_SIZE:

                # Count the explicit payload-bound violation.
                self.stats.oversize_errors += 1

                # Drop only the first magic byte so the next search can resynchronize.
                del self._buffer[0]

                # Continue searching the remaining stream immediately.
                continue

            # Calculate the exact complete-frame size declared by the header.
            expected_size = HEADER_SIZE + payload_length + CRC_SIZE

            # Wait until the complete frame candidate is available.
            if len(self._buffer) < expected_size:

                # Stop because the next feed call may complete this candidate.
                break

            # Copy exactly one complete candidate out of the stream.
            candidate = bytes(self._buffer[:expected_size])

            # Remove the candidate before decoding so a bad frame cannot block later traffic.
            del self._buffer[:expected_size]

            # Validate the complete candidate using the canonical decoder.
            try:

                # Decode the candidate into an immutable high-level frame.
                frame = decode_frame(candidate)
            except ProtocolDecodeError as exc:

                # Check whether this candidate failed specifically because of CRC.
                if exc.result == ProtocolResult.CRC_MISMATCH:

                    # Count integrity failures separately for communication diagnostics.
                    self.stats.crc_errors += 1
                else:

                    # Count all other complete-candidate decoder failures.
                    self.stats.protocol_errors += 1

                # Skip the invalid candidate and continue with later buffered traffic.
                continue

            # Count the validated frame.
            self.stats.frames_received += 1

            # Return the validated frame to the caller after the loop finishes.
            frames.append(frame)

        # Return every valid frame discovered in this feed call.
        return frames
