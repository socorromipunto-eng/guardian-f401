"""Wire-level constants for Guardian Protocol v0.1."""

# Define the two synchronization bytes used to locate frame boundaries.
MAGIC = b"GF"

# Define the protocol version encoded in every v0.1 frame.
VERSION = 0x01

# Define the fixed number of bytes before the variable payload.
HEADER_SIZE = 12

# Define the number of bytes used by the CRC32 trailer.
CRC_SIZE = 4

# Bound externally supplied payloads so parsers never allocate unbounded memory.
MAX_PAYLOAD_SIZE = 256

# Define the largest legal v0.1 frame.
MAX_FRAME_SIZE = HEADER_SIZE + MAX_PAYLOAD_SIZE + CRC_SIZE

# Define the only flags value accepted by protocol version 0.1.
SUPPORTED_FLAGS = 0x00
