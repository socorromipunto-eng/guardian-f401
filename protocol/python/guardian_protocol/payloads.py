"""Command-specific binary payload codecs for Guardian Protocol v0.1."""

# Import struct for deterministic big-endian payload serialization.
import struct

# Import dataclass for immutable decoded payload models.
from dataclasses import dataclass

# Import IntEnum so device-state values remain directly serializable.
from enum import IntEnum


# Define the first payload schema version used by M2 command responses.
_PAYLOAD_SCHEMA_VERSION = 0x01

# Define the maximum UTF-8 model-name size accepted by the v0.1 device-info payload.
_MAX_MODEL_NAME_SIZE = 32

# Define the fixed portion of the device-information payload.
_DEVICE_INFO_PREFIX = struct.Struct(">BBBBIB")

# Define the complete fixed-width runtime-status payload.
_DEVICE_STATUS_STRUCT = struct.Struct(">BBIIIIB")


# Define device runtime states exposed by GET_STATUS.
class DeviceState(IntEnum):
    """Runtime states published by Guardian-compatible devices."""

    # Identify early device initialization before normal services are available.
    BOOT = 0x00

    # Identify an initialized device waiting for an operation.
    IDLE = 0x01

    # Identify a device actively controlling or monitoring a process.
    RUNNING = 0x02

    # Identify operation with a non-fatal degraded condition.
    DEGRADED = 0x03

    # Identify a latched fault requiring explicit recovery policy.
    FAULT = 0x04


# Represent decoded immutable device-identification information.
@dataclass(frozen=True, slots=True)
class DeviceInfo:
    """Decoded DEVICE_INFO response payload."""

    # Store the simulated or physical device model name.
    model: str

    # Store the firmware semantic-version major component.
    firmware_major: int

    # Store the firmware semantic-version minor component.
    firmware_minor: int

    # Store the firmware semantic-version patch component.
    firmware_patch: int

    # Store a stable unsigned 32-bit device identifier.
    device_id: int


# Represent decoded immutable runtime status information.
@dataclass(frozen=True, slots=True)
class DeviceStatus:
    """Decoded GET_STATUS response payload."""

    # Store the current deterministic application state.
    state: DeviceState

    # Store monotonic device uptime in whole seconds.
    uptime_seconds: int

    # Store the number of valid request frames accepted by the device.
    rx_frames: int

    # Store the number of response or error frames created by the device.
    tx_frames: int

    # Store framing, CRC and semantic protocol failures observed by the device.
    protocol_errors: int

    # Store the most recent Guardian application/protocol error code.
    last_error: int


# Validate an integer before packing it into an unsigned eight-bit field.
def _require_u8(value: int, field_name: str) -> int:
    """Return *value* after validating unsigned 8-bit range."""

    # Reject values that cannot be represented in one protocol byte.
    if not 0 <= value <= 0xFF:

        # Raise a field-specific diagnostic for the caller.
        raise ValueError(f"{field_name} must fit in an unsigned 8-bit field")

    # Return the validated value unchanged.
    return value


# Validate an integer before packing it into an unsigned 32-bit field.
def _require_u32(value: int, field_name: str) -> int:
    """Return *value* after validating unsigned 32-bit range."""

    # Reject values that cannot be represented in four protocol bytes.
    if not 0 <= value <= 0xFFFFFFFF:

        # Raise a field-specific diagnostic for the caller.
        raise ValueError(f"{field_name} must fit in an unsigned 32-bit field")

    # Return the validated value unchanged.
    return value


# Encode the deterministic DEVICE_INFO response payload.
def encode_device_info(info: DeviceInfo) -> bytes:
    """Encode one DEVICE_INFO response payload."""

    # Encode the model using UTF-8 so host tools have an explicit text encoding.
    model_bytes = info.model.encode("utf-8")

    # Reject names that would violate the command-specific payload bound.
    if len(model_bytes) > _MAX_MODEL_NAME_SIZE:

        # Raise a precise diagnostic before constructing wire bytes.
        raise ValueError(
            f"model cannot exceed {_MAX_MODEL_NAME_SIZE} UTF-8 bytes"
        )

    # Reject an empty model because it would provide no useful device identity.
    if not model_bytes:

        # Raise a precise diagnostic before constructing wire bytes.
        raise ValueError("model cannot be empty")

    # Validate the firmware major component before binary packing.
    firmware_major = _require_u8(info.firmware_major, "firmware_major")

    # Validate the firmware minor component before binary packing.
    firmware_minor = _require_u8(info.firmware_minor, "firmware_minor")

    # Validate the firmware patch component before binary packing.
    firmware_patch = _require_u8(info.firmware_patch, "firmware_patch")

    # Validate the device identifier before binary packing.
    device_id = _require_u32(info.device_id, "device_id")

    # Pack the fixed prefix in Guardian big-endian wire order.
    prefix = _DEVICE_INFO_PREFIX.pack(
        _PAYLOAD_SCHEMA_VERSION,
        firmware_major,
        firmware_minor,
        firmware_patch,
        device_id,
        len(model_bytes),
    )

    # Append the variable UTF-8 model bytes after the validated prefix.
    return prefix + model_bytes


# Decode and validate one DEVICE_INFO response payload.
def decode_device_info(payload: bytes) -> DeviceInfo:
    """Decode one DEVICE_INFO response payload."""

    # Convert bytes-like input into immutable bytes for deterministic validation.
    encoded = bytes(payload)

    # Reject input shorter than the fixed prefix plus one required model byte.
    if len(encoded) < (_DEVICE_INFO_PREFIX.size + 1):

        # Raise a precise command-payload length error.
        raise ValueError("DEVICE_INFO payload is too short")

    # Decode the fixed payload prefix.
    (
        schema_version,
        firmware_major,
        firmware_minor,
        firmware_patch,
        device_id,
        model_length,
    ) = _DEVICE_INFO_PREFIX.unpack_from(encoded, 0)

    # Reject payload schema revisions not implemented by this decoder.
    if schema_version != _PAYLOAD_SCHEMA_VERSION:

        # Raise a precise schema compatibility error.
        raise ValueError(
            f"unsupported DEVICE_INFO schema version: {schema_version}"
        )

    # Reject zero-length model names.
    if model_length == 0:

        # Raise a precise semantic payload error.
        raise ValueError("DEVICE_INFO model length cannot be zero")

    # Reject model lengths that violate the published command bound.
    if model_length > _MAX_MODEL_NAME_SIZE:

        # Raise a precise semantic payload error.
        raise ValueError("DEVICE_INFO model length exceeds protocol bound")

    # Calculate the only legal complete payload length.
    expected_size = _DEVICE_INFO_PREFIX.size + model_length

    # Reject truncated payloads and payloads containing undocumented trailing bytes.
    if len(encoded) != expected_size:

        # Raise a deterministic size mismatch error.
        raise ValueError(
            f"DEVICE_INFO expected {expected_size} bytes, received {len(encoded)}"
        )

    # Extract exactly the declared UTF-8 model bytes.
    model_bytes = encoded[_DEVICE_INFO_PREFIX.size:expected_size]

    # Decode the model using the explicit protocol text encoding.
    model = model_bytes.decode("utf-8")

    # Return the validated immutable device-information model.
    return DeviceInfo(
        model=model,
        firmware_major=firmware_major,
        firmware_minor=firmware_minor,
        firmware_patch=firmware_patch,
        device_id=device_id,
    )


# Encode one deterministic GET_STATUS response payload.
def encode_device_status(status: DeviceStatus) -> bytes:
    """Encode one GET_STATUS response payload."""

    # Convert or validate the runtime state against the published state registry.
    state = DeviceState(status.state)

    # Validate uptime before binary packing.
    uptime_seconds = _require_u32(status.uptime_seconds, "uptime_seconds")

    # Validate the received-frame counter before binary packing.
    rx_frames = _require_u32(status.rx_frames, "rx_frames")

    # Validate the transmitted-frame counter before binary packing.
    tx_frames = _require_u32(status.tx_frames, "tx_frames")

    # Validate the protocol-error counter before binary packing.
    protocol_errors = _require_u32(
        status.protocol_errors,
        "protocol_errors",
    )

    # Validate the last error identifier before binary packing.
    last_error = _require_u8(status.last_error, "last_error")

    # Pack every status field in one fixed-width deterministic payload.
    return _DEVICE_STATUS_STRUCT.pack(
        _PAYLOAD_SCHEMA_VERSION,
        int(state),
        uptime_seconds,
        rx_frames,
        tx_frames,
        protocol_errors,
        last_error,
    )


# Decode and validate one GET_STATUS response payload.
def decode_device_status(payload: bytes) -> DeviceStatus:
    """Decode one GET_STATUS response payload."""

    # Convert bytes-like input into immutable bytes for deterministic validation.
    encoded = bytes(payload)

    # Reject any payload whose size differs from the frozen status schema.
    if len(encoded) != _DEVICE_STATUS_STRUCT.size:

        # Raise a deterministic size mismatch error.
        raise ValueError(
            (
                f"GET_STATUS expected {_DEVICE_STATUS_STRUCT.size} bytes, "
                f"received {len(encoded)}"
            )
        )

    # Decode every fixed-width status field in big-endian wire order.
    (
        schema_version,
        state_raw,
        uptime_seconds,
        rx_frames,
        tx_frames,
        protocol_errors,
        last_error,
    ) = _DEVICE_STATUS_STRUCT.unpack(encoded)

    # Reject payload schema revisions not implemented by this decoder.
    if schema_version != _PAYLOAD_SCHEMA_VERSION:

        # Raise a precise schema compatibility error.
        raise ValueError(
            f"unsupported GET_STATUS schema version: {schema_version}"
        )

    # Convert and validate the raw device-state identifier.
    state = DeviceState(state_raw)

    # Return the validated immutable runtime-status model.
    return DeviceStatus(
        state=state,
        uptime_seconds=uptime_seconds,
        rx_frames=rx_frames,
        tx_frames=tx_frames,
        protocol_errors=protocol_errors,
        last_error=last_error,
    )
