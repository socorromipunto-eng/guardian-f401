"""Enumerations shared by Guardian Protocol v0.1 host tools."""

# Import IntEnum so protocol values remain usable as integers on the wire.
from enum import IntEnum


# Define every message class accepted by the v0.1 decoder.
class MessageType(IntEnum):
    """Guardian Protocol message classes."""

    # Identify a host-to-device operation request.
    REQUEST = 0x01

    # Identify a successful device response.
    RESPONSE = 0x02

    # Identify an asynchronous device event.
    EVENT = 0x03

    # Identify an error associated with a request or protocol condition.
    ERROR = 0x04

    # Identify an asynchronous telemetry sample.
    TELEMETRY = 0x05


# Define command identifiers published by the v0.1 command registry.
class Command(IntEnum):
    """Guardian Protocol command identifiers."""

    # Verify that the remote endpoint can decode and answer protocol traffic.
    PING = 0x01

    # Request immutable device and firmware metadata.
    DEVICE_INFO = 0x02

    # Request the current runtime state.
    GET_STATUS = 0x10


# Define error identifiers carried by ERROR frame payloads.
class ErrorCode(IntEnum):
    """Guardian Protocol application and protocol errors."""

    # Report a frame that cannot be interpreted safely.
    MALFORMED_FRAME = 0x01

    # Report a protocol version that the receiver does not implement.
    UNSUPPORTED_VERSION = 0x02

    # Report a message type that the receiver does not implement.
    UNSUPPORTED_MESSAGE_TYPE = 0x03

    # Report flags that are not defined by the active protocol version.
    UNSUPPORTED_FLAGS = 0x04

    # Report a command identifier that the application does not implement.
    UNKNOWN_COMMAND = 0x05

    # Report a payload that violates command-specific requirements.
    INVALID_PAYLOAD = 0x06

    # Report an unexpected internal device failure.
    INTERNAL_ERROR = 0x07

    # Report a valid operation that cannot currently execute.
    BUSY = 0x08

    # Reserve an authorization failure for the later security milestone.
    UNAUTHORIZED = 0x09

    # Reserve replay detection for the later security milestone.
    REPLAY_DETECTED = 0x0A


# Define structured decoder outcomes for diagnostics and parser statistics.
class ProtocolResult(IntEnum):
    """Local decoder result codes."""

    # Indicate that a frame was decoded successfully.
    OK = 0

    # Indicate that a supplied frame is shorter than the minimum frame size.
    FRAME_TOO_SHORT = 1

    # Indicate that the synchronization bytes are incorrect.
    INVALID_MAGIC = 2

    # Indicate that the encoded protocol version is unsupported.
    UNSUPPORTED_VERSION = 3

    # Indicate that the encoded message type is unsupported.
    UNSUPPORTED_MESSAGE_TYPE = 4

    # Indicate that reserved flag bits are set.
    UNSUPPORTED_FLAGS = 5

    # Indicate that the declared payload exceeds the protocol bound.
    PAYLOAD_TOO_LARGE = 6

    # Indicate that the encoded frame size does not match its declared payload.
    LENGTH_MISMATCH = 7

    # Indicate that the received CRC does not match the calculated CRC.
    CRC_MISMATCH = 8
