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







# Re-export M12 secure firmware lifecycle models and codecs.
from .firmware import (
    FIRMWARE_CHUNK_MAX_DATA,
    FIRMWARE_MAX_SIGNATURE_SIZE,
    FIRMWARE_PACKAGE_MAGIC,
    FIRMWARE_SCHEMA_VERSION,
    FIRMWARE_SIGNATURE_DEMO_HMAC_SHA256,
    FIRMWARE_SIGNATURE_ED25519,
    FIRMWARE_SIGNED_DOMAIN,
    FIRMWARE_SIGNED_MANIFEST_SIZE,
    FirmwareChunk,
    FirmwareFailureCode,
    FirmwareLifecycleState,
    FirmwareManifest,
    FirmwarePackage,
    FirmwareSignatureAlgorithm,
    FirmwareStatus,
    canonical_firmware_manifest,
    decode_firmware_action,
    decode_firmware_chunk,
    decode_firmware_manifest,
    decode_firmware_package,
    decode_firmware_status,
    demo_sign_firmware_manifest,
    demo_verify_firmware_manifest,
    encode_firmware_action,
    encode_firmware_chunk,
    encode_firmware_manifest,
    encode_firmware_package,
    encode_firmware_status,
    validate_firmware_manifest,
)

# Re-export M10 authenticated-session models and codecs.
from .security import (
    DEFAULT_SECURITY_SESSION_TIMEOUT_SECONDS,
    MAX_SECURE_REQUEST_INNER_PAYLOAD,
    MAX_SECURE_RESPONSE_INNER_PAYLOAD,
    SECURITY_NONCE_SIZE,
    SECURITY_PSK_SIZE,
    SECURITY_SCHEMA_VERSION,
    SECURITY_SESSION_KEY_SIZE,
    SECURITY_TAG_SIZE,
    AuthBegin,
    AuthChallenge,
    AuthFinish,
    AuthenticatedSession,
    SecureRequest,
    SecureResponse,
    SecurityRole,
    SecurityStatus,
    compute_client_proof,
    compute_server_proof,
    decode_auth_begin,
    decode_auth_challenge,
    decode_auth_finish,
    decode_authenticated_session,
    decode_secure_request,
    decode_secure_response,
    decode_security_status,
    derive_session_key,
    encode_auth_begin,
    encode_auth_challenge,
    encode_auth_finish,
    encode_authenticated_session,
    encode_secure_request,
    encode_secure_response,
    encode_security_status,
    validate_psk,
)

# Re-export M9 supervisory-control models and codecs.
from .control import (
    CONTROL_SCHEMA_VERSION,
    ControlAction,
    ControlCommand,
    ControlCommandResult,
    ControlState,
    ControlStatus,
    decode_control_command,
    decode_control_command_result,
    decode_control_status,
    encode_control_command,
    encode_control_command_result,
    encode_control_status,
)

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
    "FIRMWARE_CHUNK_MAX_DATA",
    "FIRMWARE_MAX_SIGNATURE_SIZE",
    "FIRMWARE_PACKAGE_MAGIC",
    "FIRMWARE_SCHEMA_VERSION",
    "FIRMWARE_SIGNATURE_DEMO_HMAC_SHA256",
    "FIRMWARE_SIGNATURE_ED25519",
    "FIRMWARE_SIGNED_DOMAIN",
    "FIRMWARE_SIGNED_MANIFEST_SIZE",
    "FirmwareChunk",
    "FirmwareFailureCode",
    "FirmwareLifecycleState",
    "FirmwareManifest",
    "FirmwarePackage",
    "FirmwareSignatureAlgorithm",
    "FirmwareStatus",
    "canonical_firmware_manifest",
    "decode_firmware_action",
    "decode_firmware_chunk",
    "decode_firmware_manifest",
    "decode_firmware_package",
    "decode_firmware_status",
    "demo_sign_firmware_manifest",
    "demo_verify_firmware_manifest",
    "encode_firmware_action",
    "encode_firmware_chunk",
    "encode_firmware_manifest",
    "encode_firmware_package",
    "encode_firmware_status",
    "validate_firmware_manifest",
    "AuthBegin",
    "AuthChallenge",
    "AuthFinish",
    "AuthenticatedSession",
    "BaselineAction",
    "BaselineControl",
    "CONTROL_SCHEMA_VERSION",
    "Command",
    "ControlAction",
    "ControlCommand",
    "ControlCommandResult",
    "ControlState",
    "ControlStatus",
    "DEFAULT_TELEMETRY_PERIOD_MS",
    "DSP_SCHEMA_VERSION",
    "DeviceInfo",
    "DeviceState",
    "DeviceStatus",
    "DspFeatures",
    "ErrorCode",
    "Frame",
    "HEALTH_SCHEMA_VERSION",
    "DEFAULT_SECURITY_SESSION_TIMEOUT_SECONDS",
    "HEADER_SIZE",
    "HealthState",
    "HealthStatus",
    "IncrementalParser",
    "MAGIC",
    "MAX_BASELINE_SAMPLES",
    "MAX_SECURE_REQUEST_INNER_PAYLOAD",
    "MAX_SECURE_RESPONSE_INNER_PAYLOAD",
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
    "compute_client_proof",
    "compute_server_proof",
    "decode_auth_begin",
    "decode_auth_challenge",
    "decode_auth_finish",
    "decode_authenticated_session",
    "decode_baseline_control",
    "decode_control_command",
    "decode_control_command_result",
    "decode_control_status",
    "decode_device_info",
    "decode_device_status",
    "decode_dsp_features",
    "decode_secure_request",
    "decode_secure_response",
    "decode_security_status",
    "derive_session_key",
    "decode_health_status",
    "decode_frame",
    "decode_machine_telemetry",
    "decode_telemetry_config",
    "encode_auth_begin",
    "encode_auth_challenge",
    "encode_auth_finish",
    "encode_authenticated_session",
    "encode_baseline_control",
    "encode_control_command",
    "encode_control_command_result",
    "encode_control_status",
    "encode_device_info",
    "encode_device_status",
    "encode_dsp_features",
    "encode_frame",
    "encode_health_status",
    "encode_secure_request",
    "encode_secure_response",
    "encode_security_status",
    "validate_psk",
    "encode_machine_telemetry",
    "encode_telemetry_config",
]
