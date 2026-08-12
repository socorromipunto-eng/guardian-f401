"""Telemetry payload codecs for Guardian Protocol v0.1 M5."""

# Import struct for deterministic big-endian binary serialization.
import struct

# Import dataclass for immutable decoded protocol models.
from dataclasses import dataclass

# Import the published device-state registry used by telemetry samples.
from .payloads import DeviceState


# Define the first telemetry payload schema version.
TELEMETRY_SCHEMA_VERSION = 0x01

# Define the minimum allowed asynchronous telemetry period.
MIN_TELEMETRY_PERIOD_MS = 100

# Define the maximum allowed asynchronous telemetry period.
MAX_TELEMETRY_PERIOD_MS = 60000

# Define the default asynchronous telemetry period.
DEFAULT_TELEMETRY_PERIOD_MS = 1000

# Define the fixed SET_TELEMETRY request and response payload.
_TELEMETRY_CONFIG_STRUCT = struct.Struct(">BBH")

# Define the fixed MACHINE_TELEMETRY payload.
_MACHINE_TELEMETRY_STRUCT = struct.Struct(">BBIhHHHHH")


# Store immutable telemetry stream configuration.
@dataclass(frozen=True, slots=True)
class TelemetryConfig:
    """Decoded SET_TELEMETRY request or response payload."""

    # Store whether asynchronous transmission is enabled.
    enabled: bool

    # Store the bounded telemetry period in milliseconds.
    period_ms: int = DEFAULT_TELEMETRY_PERIOD_MS


# Store one immutable machine telemetry sample.
@dataclass(frozen=True, slots=True)
class MachineTelemetry:
    """Decoded MACHINE_TELEMETRY payload."""

    # Store the application state at sample emission.
    state: DeviceState

    # Store the device monotonic timestamp modulo 2^32 milliseconds.
    timestamp_ms: int

    # Store temperature in signed hundredths of one degree Celsius.
    temperature_centi_c: int

    # Store RMS vibration magnitude in milli-g.
    vibration_mg_rms: int

    # Store machine current in milliamperes.
    current_ma: int

    # Store shaft speed in revolutions per minute.
    rpm: int

    # Store measured supply voltage in millivolts.
    supply_mv: int

    # Store application-defined sample status flags.
    status_flags: int


# Validate an unsigned sixteen-bit integer.
def _require_u16(value: int, field_name: str) -> int:
    """Return *value* after validating unsigned 16-bit range."""

    # Reject values that cannot fit in the published field.
    if not 0 <= value <= 0xFFFF:

        # Raise a field-specific caller diagnostic.
        raise ValueError(f"{field_name} must fit in an unsigned 16-bit field")

    # Return the validated integer.
    return value


# Validate an unsigned 32-bit integer.
def _require_u32(value: int, field_name: str) -> int:
    """Return *value* after validating unsigned 32-bit range."""

    # Reject values that cannot fit in the published field.
    if not 0 <= value <= 0xFFFFFFFF:

        # Raise a field-specific caller diagnostic.
        raise ValueError(f"{field_name} must fit in an unsigned 32-bit field")

    # Return the validated integer.
    return value


# Validate a signed 16-bit integer.
def _require_i16(value: int, field_name: str) -> int:
    """Return *value* after validating signed 16-bit range."""

    # Reject values that cannot fit in the published field.
    if not -0x8000 <= value <= 0x7FFF:

        # Raise a field-specific caller diagnostic.
        raise ValueError(f"{field_name} must fit in a signed 16-bit field")

    # Return the validated integer.
    return value


# Validate the published telemetry period bounds.
def _require_period(period_ms: int) -> int:
    """Return *period_ms* after validating M5 rate limits."""

    # Reject periods that could flood the command transport.
    if not MIN_TELEMETRY_PERIOD_MS <= period_ms <= MAX_TELEMETRY_PERIOD_MS:

        # Raise a precise command-payload diagnostic.
        raise ValueError(
            (
                "period_ms must be between "
                f"{MIN_TELEMETRY_PERIOD_MS} and "
                f"{MAX_TELEMETRY_PERIOD_MS}"
            )
        )

    # Return the validated period.
    return period_ms


# Encode one SET_TELEMETRY command payload.
def encode_telemetry_config(config: TelemetryConfig) -> bytes:
    """Encode one telemetry configuration payload."""

    # Validate the requested rate before creating wire bytes.
    period_ms = _require_period(config.period_ms)

    # Convert the boolean state into the frozen one-byte representation.
    enabled = 1 if config.enabled else 0

    # Pack the complete configuration payload in Guardian big-endian order.
    return _TELEMETRY_CONFIG_STRUCT.pack(
        TELEMETRY_SCHEMA_VERSION,
        enabled,
        period_ms,
    )


# Decode one SET_TELEMETRY command payload.
def decode_telemetry_config(payload: bytes) -> TelemetryConfig:
    """Decode one telemetry configuration payload."""

    # Convert bytes-like input into immutable bytes.
    encoded = bytes(payload)

    # Require the exact frozen payload size.
    if len(encoded) != _TELEMETRY_CONFIG_STRUCT.size:

        # Reject truncated or undocumented trailing bytes.
        raise ValueError(
            (
                f"SET_TELEMETRY expected {_TELEMETRY_CONFIG_STRUCT.size} "
                f"bytes, received {len(encoded)}"
            )
        )

    # Decode the complete fixed-width payload.
    schema_version, enabled_raw, period_ms = (
        _TELEMETRY_CONFIG_STRUCT.unpack(encoded)
    )

    # Reject unsupported future payload schemas.
    if schema_version != TELEMETRY_SCHEMA_VERSION:

        # Raise a precise compatibility diagnostic.
        raise ValueError(
            f"unsupported telemetry config schema version: {schema_version}"
        )

    # Reject enabled values outside the frozen Boolean encoding.
    if enabled_raw not in (0, 1):

        # Raise a precise semantic payload diagnostic.
        raise ValueError("telemetry enabled field must be 0 or 1")

    # Validate the published telemetry rate bound.
    period_ms = _require_period(period_ms)

    # Return the immutable decoded configuration.
    return TelemetryConfig(
        enabled=bool(enabled_raw),
        period_ms=period_ms,
    )


# Encode one MACHINE_TELEMETRY sample payload.
def encode_machine_telemetry(sample: MachineTelemetry) -> bytes:
    """Encode one machine telemetry payload."""

    # Convert and validate the application state.
    state = DeviceState(sample.state)

    # Validate the monotonic timestamp.
    timestamp_ms = _require_u32(
        sample.timestamp_ms,
        "timestamp_ms",
    )

    # Validate signed temperature representation.
    temperature_centi_c = _require_i16(
        sample.temperature_centi_c,
        "temperature_centi_c",
    )

    # Validate vibration magnitude.
    vibration_mg_rms = _require_u16(
        sample.vibration_mg_rms,
        "vibration_mg_rms",
    )

    # Validate machine current.
    current_ma = _require_u16(
        sample.current_ma,
        "current_ma",
    )

    # Validate shaft speed.
    rpm = _require_u16(
        sample.rpm,
        "rpm",
    )

    # Validate supply voltage.
    supply_mv = _require_u16(
        sample.supply_mv,
        "supply_mv",
    )

    # Validate sample status flags.
    status_flags = _require_u16(
        sample.status_flags,
        "status_flags",
    )

    # Pack the complete fixed-width sample.
    return _MACHINE_TELEMETRY_STRUCT.pack(
        TELEMETRY_SCHEMA_VERSION,
        int(state),
        timestamp_ms,
        temperature_centi_c,
        vibration_mg_rms,
        current_ma,
        rpm,
        supply_mv,
        status_flags,
    )


# Decode one MACHINE_TELEMETRY sample payload.
def decode_machine_telemetry(payload: bytes) -> MachineTelemetry:
    """Decode one machine telemetry payload."""

    # Convert bytes-like input into immutable bytes.
    encoded = bytes(payload)

    # Require the exact frozen sample size.
    if len(encoded) != _MACHINE_TELEMETRY_STRUCT.size:

        # Reject truncated or undocumented trailing bytes.
        raise ValueError(
            (
                f"MACHINE_TELEMETRY expected "
                f"{_MACHINE_TELEMETRY_STRUCT.size} bytes, "
                f"received {len(encoded)}"
            )
        )

    # Decode every fixed-width sample field.
    (
        schema_version,
        state_raw,
        timestamp_ms,
        temperature_centi_c,
        vibration_mg_rms,
        current_ma,
        rpm,
        supply_mv,
        status_flags,
    ) = _MACHINE_TELEMETRY_STRUCT.unpack(encoded)

    # Reject unsupported future payload schemas.
    if schema_version != TELEMETRY_SCHEMA_VERSION:

        # Raise a precise compatibility diagnostic.
        raise ValueError(
            f"unsupported machine telemetry schema version: {schema_version}"
        )

    # Convert and validate the application state identifier.
    state = DeviceState(state_raw)

    # Return one immutable decoded sample.
    return MachineTelemetry(
        state=state,
        timestamp_ms=timestamp_ms,
        temperature_centi_c=temperature_centi_c,
        vibration_mg_rms=vibration_mg_rms,
        current_ma=current_ma,
        rpm=rpm,
        supply_mv=supply_mv,
        status_flags=status_flags,
    )
