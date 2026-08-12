"""Public Python API for Guardian Protocol v0.1."""

# Re-export protocol constants used by host applications and tests.
from .constants import HEADER_SIZE, MAGIC, MAX_FRAME_SIZE, MAX_PAYLOAD_SIZE, VERSION

# Re-export the CRC implementation for deterministic cross-language tests.
from .crc import crc32_ieee

# Re-export the public protocol enumerations.
from .enums import Command, ErrorCode, MessageType, ProtocolResult

# Re-export the frame representation and canonical codec.
from .frame import Frame, ProtocolDecodeError, decode_frame, encode_frame

# Re-export the incremental stream parser and its diagnostics.
from .parser import IncrementalParser, ParserStats

# Define the supported public import surface explicitly.
__all__ = [
    "Command",
    "ErrorCode",
    "Frame",
    "HEADER_SIZE",
    "IncrementalParser",
    "MAGIC",
    "MAX_FRAME_SIZE",
    "MAX_PAYLOAD_SIZE",
    "MessageType",
    "ParserStats",
    "ProtocolDecodeError",
    "ProtocolResult",
    "VERSION",
    "crc32_ieee",
    "decode_frame",
    "encode_frame",
]
