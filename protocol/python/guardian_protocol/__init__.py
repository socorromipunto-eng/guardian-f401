"""Public Python API for Guardian Protocol v0.1."""

# Re-export protocol constants used by host applications and tests.
from .constants import HEADER_SIZE, MAGIC, MAX_FRAME_SIZE, MAX_PAYLOAD_SIZE, VERSION

# Re-export the CRC implementation for deterministic cross-language tests.
from .crc import crc32_ieee

# Re-export the public protocol enumerations.
from .enums import Command, ErrorCode, MessageType, ProtocolResult

# Re-export the frame representation and canonical codec.
from .frame import Frame, ProtocolDecodeError, decode_frame, encode_frame

# Re-export command-specific payload models and codecs.
from .payloads import (
    DeviceInfo,
    DeviceState,
    DeviceStatus,
    decode_device_info,
    decode_device_status,
    encode_device_info,
    encode_device_status,
)

# Re-export the incremental stream parser and its diagnostics.
from .parser import IncrementalParser, ParserStats




# Re-export M8 machine-health models and codecs.
from .health import (
    HEALTH_SCHEMA_VERSION,
    MAX_BASELINE_SAMPLES,
    MIN_BASELINE_SAMPLES,
    BaselineAction,
    BaselineControl,
    HealthState,
    HealthStatus,
    decode_baseline_control,
    decode_health_status,
    encode_baseline_control,
    encode_health_status,
)

# Re-export M7 DSP feature models and codecs.
from .dsp import (
    DSP_SCHEMA_VERSION,
    DspFeatures,
    decode_dsp_features,
    encode_dsp_features,
)

# Re-export M5 telemetry models, bounds and codecs.
from .telemetry import (
    DEFAULT_TELEMETRY_PERIOD_MS,
    MAX_TELEMETRY_PERIOD_MS,
    MIN_TELEMETRY_PERIOD_MS,
    MachineTelemetry,
    TelemetryConfig,
    decode_machine_telemetry,
    decode_telemetry_config,
    encode_machine_telemetry,
    encode_telemetry_config,
)

# Define the supported public import surface explicitly.
__all__ = [
    "BaselineAction",
    "BaselineControl",
    "Command",
    "DEFAULT_TELEMETRY_PERIOD_MS",
    "DSP_SCHEMA_VERSION",
    "DeviceInfo",
    "DeviceState",
    "DeviceStatus",
    "DspFeatures",
    "ErrorCode",
    "Frame",
    "HEALTH_SCHEMA_VERSION",
    "HEADER_SIZE",
    "HealthState",
    "HealthStatus",
    "IncrementalParser",
    "MAGIC",
    "MAX_BASELINE_SAMPLES",
    "MAX_FRAME_SIZE",
    "MAX_TELEMETRY_PERIOD_MS",
    "MAX_PAYLOAD_SIZE",
    "MIN_BASELINE_SAMPLES",
    "MessageType",
    "MIN_TELEMETRY_PERIOD_MS",
    "MachineTelemetry",
    "ParserStats",
    "ProtocolDecodeError",
    "ProtocolResult",
    "TelemetryConfig",
    "VERSION",
    "crc32_ieee",
    "decode_baseline_control",
    "decode_device_info",
    "decode_device_status",
    "decode_dsp_features",
    "decode_health_status",
    "decode_frame",
    "decode_machine_telemetry",
    "decode_telemetry_config",
    "encode_baseline_control",
    "encode_device_info",
    "encode_device_status",
    "encode_dsp_features",
    "encode_frame",
    "encode_health_status",
    "encode_machine_telemetry",
    "encode_telemetry_config",
]
