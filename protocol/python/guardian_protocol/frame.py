"""Frame encoder and decoder for Guardian Protocol v0.1."""

# Import struct for deterministic binary packing and unpacking.
import struct

# Import dataclass for an immutable high-level frame representation.
from dataclasses import dataclass

# Import protocol constants used by every encoder and decoder operation.
from .constants import HEADER_SIZE, MAGIC, MAX_PAYLOAD_SIZE, SUPPORTED_FLAGS, VERSION

# Import the protocol CRC implementation.
from .crc import crc32_ieee

# Import message and result enumerations.
from .enums import MessageType, ProtocolResult


# Define the exact binary format of the fixed 12-byte header.
_HEADER_STRUCT = struct.Struct(">2sBBBBIH")

# Define the exact binary format of the four-byte CRC trailer.
_CRC_STRUCT = struct.Struct(">I")


# Represent a decoder failure with a machine-readable result code.
class ProtocolDecodeError(ValueError):
    """Raised when a byte sequence violates Guardian Protocol v0.1."""

    # Initialize the exception with a structured protocol result and message.
    def __init__(self, result: ProtocolResult, message: str) -> None:

        # Initialize the base ValueError with the human-readable message.
        super().__init__(message)

        # Preserve the structured result code for tests and parser diagnostics.
        self.result = result


# Represent a validated Guardian Protocol frame.
@dataclass(frozen=True, slots=True)
class Frame:
    """Validated in-memory representation of a Guardian Protocol frame."""

    # Store the wire-level message class.
    message_type: MessageType

    # Store the unsigned eight-bit command identifier.
    command: int

    # Store the unsigned 32-bit request correlation value.
    sequence: int

    # Store the command-specific payload bytes.
    payload: bytes = b""

    # Store protocol flags, which must remain zero in version 0.1.
    flags: int = SUPPORTED_FLAGS

    # Validate every field immediately after dataclass construction.
    def __post_init__(self) -> None:

        # Normalize or reject an invalid message type before encoding.
        if not isinstance(self.message_type, MessageType):

            # Convert integer-compatible values into the declared message enum.
            object.__setattr__(self, "message_type", MessageType(self.message_type))

        # Reject values that cannot fit into the one-byte command field.
        if not 0 <= self.command <= 0xFF:

            # Raise a precise validation error for the caller.
            raise ValueError("command must fit in an unsigned 8-bit field")

        # Reject values that cannot fit into the four-byte sequence field.
        if not 0 <= self.sequence <= 0xFFFFFFFF:

            # Raise a precise validation error for the caller.
            raise ValueError("sequence must fit in an unsigned 32-bit field")

        # Reject flags that are undefined by protocol version 0.1.
        if self.flags != SUPPORTED_FLAGS:

            # Raise a precise validation error for the caller.
            raise ValueError("Guardian Protocol v0.1 requires flags == 0")

        # Reject payloads that violate the embedded parser memory bound.
        if len(self.payload) > MAX_PAYLOAD_SIZE:

            # Raise a precise validation error for the caller.
            raise ValueError(f"payload cannot exceed {MAX_PAYLOAD_SIZE} bytes")


# Encode one validated high-level frame into its wire representation.
def encode_frame(frame: Frame) -> bytes:
    """Encode *frame* into Guardian Protocol v0.1 bytes."""

    # Copy the payload so mutable bytearray callers cannot change it mid-encode.
    payload = bytes(frame.payload)

    # Pack the fixed header in big-endian wire order.
    header = _HEADER_STRUCT.pack(
        MAGIC,
        VERSION,
        int(frame.message_type),
        frame.command,
        frame.flags,
        frame.sequence,
        len(payload),
    )

    # Concatenate the header and payload because both are protected by CRC32.
    protected_bytes = header + payload

    # Calculate the protocol CRC over every byte preceding the CRC field.
    crc = crc32_ieee(protected_bytes)

    # Encode the numeric CRC as four big-endian bytes.
    crc_bytes = _CRC_STRUCT.pack(crc)

    # Return the exact on-wire frame.
    return protected_bytes + crc_bytes


# Decode and validate exactly one complete Guardian Protocol frame.
def decode_frame(data: bytes) -> Frame:
    """Decode one complete Guardian Protocol v0.1 frame."""

    # Convert bytes-like input into immutable bytes for deterministic validation.
    encoded = bytes(data)

    # Calculate the smallest legal frame containing an empty payload.
    minimum_size = HEADER_SIZE + _CRC_STRUCT.size

    # Reject truncated input before attempting to unpack its header.
    if len(encoded) < minimum_size:

        # Raise a structured short-frame error.
        raise ProtocolDecodeError(
            ProtocolResult.FRAME_TOO_SHORT,
            f"frame requires at least {minimum_size} bytes",
        )

    # Unpack the fixed header from the start of the encoded frame.
    magic, version, message_type_raw, command, flags, sequence, payload_length = (
        _HEADER_STRUCT.unpack_from(encoded, 0)
    )

    # Reject input that is not synchronized to the Guardian magic bytes.
    if magic != MAGIC:

        # Raise a structured magic validation error.
        raise ProtocolDecodeError(
            ProtocolResult.INVALID_MAGIC,
            "invalid Guardian Protocol magic bytes",
        )

    # Reject protocol versions that this implementation does not understand.
    if version != VERSION:

        # Raise a structured version validation error.
        raise ProtocolDecodeError(
            ProtocolResult.UNSUPPORTED_VERSION,
            f"unsupported protocol version: {version}",
        )

    # Convert and validate the raw message type value.
    try:

        # Map the wire value into the published message-type registry.
        message_type = MessageType(message_type_raw)
    except ValueError as exc:

        # Raise a protocol-specific error while preserving the original cause.
        raise ProtocolDecodeError(
            ProtocolResult.UNSUPPORTED_MESSAGE_TYPE,
            f"unsupported message type: {message_type_raw}",
        ) from exc

    # Reject undefined flags so future semantics cannot be silently misinterpreted.
    if flags != SUPPORTED_FLAGS:

        # Raise a structured unsupported-flags error.
        raise ProtocolDecodeError(
            ProtocolResult.UNSUPPORTED_FLAGS,
            f"unsupported flags value: 0x{flags:02X}",
        )

    # Reject lengths that exceed the bounded embedded payload storage.
    if payload_length > MAX_PAYLOAD_SIZE:

        # Raise a structured payload-bound error.
        raise ProtocolDecodeError(
            ProtocolResult.PAYLOAD_TOO_LARGE,
            f"payload length exceeds {MAX_PAYLOAD_SIZE} bytes",
        )

    # Calculate the exact number of bytes required by the declared payload length.
    expected_size = HEADER_SIZE + payload_length + _CRC_STRUCT.size

    # Reject both truncated frames and frames containing trailing unframed bytes.
    if len(encoded) != expected_size:

        # Raise a structured size mismatch error.
        raise ProtocolDecodeError(
            ProtocolResult.LENGTH_MISMATCH,
            f"expected {expected_size} bytes but received {len(encoded)}",
        )

    # Find the first byte of the CRC trailer.
    crc_offset = HEADER_SIZE + payload_length

    # Decode the received big-endian CRC value.
    received_crc = _CRC_STRUCT.unpack_from(encoded, crc_offset)[0]

    # Calculate the CRC over the header and payload only.
    calculated_crc = crc32_ieee(encoded[:crc_offset])

    # Reject any frame whose protected bytes do not match its CRC.
    if received_crc != calculated_crc:

        # Raise a structured CRC mismatch error.
        raise ProtocolDecodeError(
            ProtocolResult.CRC_MISMATCH,
            (
                f"CRC mismatch: received 0x{received_crc:08X}, "
                f"calculated 0x{calculated_crc:08X}"
            ),
        )

    # Copy the variable payload out of the validated frame.
    payload = encoded[HEADER_SIZE:crc_offset]

    # Return an immutable validated frame object.
    return Frame(
        message_type=message_type,
        command=command,
        sequence=sequence,
        payload=payload,
        flags=flags,
    )
