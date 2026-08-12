"""Machine-health payload codecs for Guardian M8."""

# Import struct for deterministic big-endian wire serialization.
import struct

# Import dataclass for immutable protocol models.
from dataclasses import dataclass

# Import IntEnum for wire-compatible lifecycle and action values.
from enum import IntEnum


# Define the first machine-health schema revision.
HEALTH_SCHEMA_VERSION = 0x01

# Define the bounded explicit baseline sample policy.
MIN_BASELINE_SAMPLES = 16

# Define the maximum explicit runtime baseline sample count.
MAX_BASELINE_SAMPLES = 1024

# Define the exact fixed M8 health-status layout.
_HEALTH_STATUS_STRUCT = struct.Struct(">BBHHHHHBBHIHHIHHHH")

# Define the exact fixed baseline-control layout.
_BASELINE_CONTROL_STRUCT = struct.Struct(">BBH")


# Define M8 model lifecycle and anomaly states.
class HealthState(IntEnum):
    """Guardian M8 machine-health states."""

    # No explicit baseline has been requested.
    UNTRAINED = 0

    # A bounded baseline is currently being learned.
    LEARNING = 1

    # A trained baseline exists and current input is normal.
    READY = 2

    # Persistent warning-level deviation is active.
    WARNING = 3

    # Persistent alarm-level deviation is active.
    ALARM = 4


# Define explicit baseline lifecycle actions.
class BaselineAction(IntEnum):
    """Guardian M8 baseline-control operations."""

    # Start a fresh baseline learning session.
    START = 1

    # Erase the runtime baseline and return to UNTRAINED.
    RESET = 2


# Store one immutable baseline-control request or response.
@dataclass(frozen=True, slots=True)
class BaselineControl:
    """Guardian M8 baseline-control payload."""

    # Store the explicit lifecycle action.
    action: BaselineAction

    # Store the bounded target sample count for START.
    target_samples: int


# Store one immutable host-visible health snapshot.
@dataclass(frozen=True, slots=True)
class HealthStatus:
    """Decoded Guardian M8 machine-health status."""

    # Store model lifecycle or anomaly state.
    state: HealthState

    # Store accepted baseline sample count.
    baseline_samples: int

    # Store requested baseline sample target.
    baseline_target: int

    # Store bounded anomaly severity from 0 through 1000.
    anomaly_score: int

    # Store inverse health score from 1000 through 0.
    health_score: int

    # Store largest weighted normalized deviation multiplied by 1000.
    max_deviation_milli: int

    # Store feature identifier responsible for the largest deviation.
    dominant_feature: int

    # Store current consecutive warning/alarm deviation count.
    consecutive_anomalous: int

    # Store baseline and input-quality flags.
    quality_flags: int

    # Store M7 block sequence associated with the current score.
    block_sequence: int

    # Store current vibration AC RMS in milli-g.
    current_rms_mg: int

    # Store current crest factor multiplied by 1000.
    current_crest_factor_milli: int

    # Store current dominant frequency in hundredths of one hertz.
    current_dominant_frequency_centi_hz: int

    # Store learned RMS baseline mean in milli-g.
    baseline_rms_mean_mg: int

    # Store learned effective RMS standard deviation in milli-g.
    baseline_rms_std_mg: int

    # Store one bit per warning-threshold feature.
    exceeded_feature_mask: int

    # Store saturated rejected-input count.
    rejected_inputs: int


# Validate one unsigned integer against a field maximum.
def _require_unsigned(
    value: int,
    maximum: int,
    field_name: str,
) -> int:
    """Return *value* after bounded validation."""

    # Reject values outside the published field width.
    if not 0 <= value <= maximum:

        # Raise a precise field-specific caller error.
        raise ValueError(
            f"{field_name} must be between 0 and {maximum}"
        )

    # Return the validated integer.
    return value


# Encode one fixed baseline-control payload.
def encode_baseline_control(
    control: BaselineControl,
) -> bytes:
    """Encode one Guardian M8 baseline-control payload."""

    # Normalize action into the published enum.
    try:

        # Convert integers or enum values into BaselineAction.
        action = BaselineAction(control.action)
    except ValueError as exc:

        # Reject undefined baseline actions.
        raise ValueError(
            f"unsupported baseline action: {control.action}"
        ) from exc

    # Apply action-specific target semantics.
    if action == BaselineAction.START:

        # Enforce the bounded baseline sample policy.
        if not MIN_BASELINE_SAMPLES <= control.target_samples <= MAX_BASELINE_SAMPLES:

            # Reject a target that firmware would reject.
            raise ValueError(
                (
                    "baseline target must be between "
                    f"{MIN_BASELINE_SAMPLES} and {MAX_BASELINE_SAMPLES}"
                )
            )

        # Preserve the validated start target.
        target_samples = control.target_samples
    else:

        # Require RESET to carry zero target samples.
        if control.target_samples != 0:

            # Reject contradictory reset semantics.
            raise ValueError(
                "baseline reset target_samples must be zero"
            )

        # Normalize RESET target.
        target_samples = 0

    # Pack the fixed four-byte payload.
    return _BASELINE_CONTROL_STRUCT.pack(
        HEALTH_SCHEMA_VERSION,
        int(action),
        target_samples,
    )


# Decode one fixed baseline-control payload.
def decode_baseline_control(
    payload: bytes,
) -> BaselineControl:
    """Decode one Guardian M8 baseline-control payload."""

    # Convert any bytes-like input into immutable bytes.
    encoded = bytes(payload)

    # Require the exact fixed payload size.
    if len(encoded) != _BASELINE_CONTROL_STRUCT.size:

        # Reject truncated or trailing bytes.
        raise ValueError(
            (
                "BASELINE_CONTROL expected "
                f"{_BASELINE_CONTROL_STRUCT.size} bytes, "
                f"received {len(encoded)}"
            )
        )

    # Decode the complete big-endian payload.
    schema_version, action_value, target_samples = (
        _BASELINE_CONTROL_STRUCT.unpack(encoded)
    )

    # Require schema revision one.
    if schema_version != HEALTH_SCHEMA_VERSION:

        # Reject unknown semantics.
        raise ValueError(
            f"unsupported health schema version: {schema_version}"
        )

    # Require a defined baseline action.
    try:

        # Convert wire value into the published enum.
        action = BaselineAction(action_value)
    except ValueError as exc:

        # Reject unknown actions.
        raise ValueError(
            f"unsupported baseline action: {action_value}"
        ) from exc

    # Reuse encoder validation so request and response semantics stay identical.
    control = BaselineControl(
        action=action,
        target_samples=target_samples,
    )

    # Validate the decoded action-specific target.
    encode_baseline_control(control)

    # Return the immutable validated model.
    return control


# Encode one fixed M8 health-status payload.
def encode_health_status(
    status: HealthStatus,
) -> bytes:
    """Encode one Guardian M8 health-status payload."""

    # Normalize model state into the published enum.
    try:

        # Convert integer or enum input into HealthState.
        state = HealthState(status.state)
    except ValueError as exc:

        # Reject unsupported states.
        raise ValueError(
            f"unsupported health state: {status.state}"
        ) from exc

    # Pack every validated fixed-width health field.
    return _HEALTH_STATUS_STRUCT.pack(
        HEALTH_SCHEMA_VERSION,
        int(state),
        _require_unsigned(status.baseline_samples, 0xFFFF, "baseline_samples"),
        _require_unsigned(status.baseline_target, 0xFFFF, "baseline_target"),
        _require_unsigned(status.anomaly_score, 1000, "anomaly_score"),
        _require_unsigned(status.health_score, 1000, "health_score"),
        _require_unsigned(
            status.max_deviation_milli,
            0xFFFF,
            "max_deviation_milli",
        ),
        _require_unsigned(status.dominant_feature, 0xFF, "dominant_feature"),
        _require_unsigned(
            status.consecutive_anomalous,
            0xFF,
            "consecutive_anomalous",
        ),
        _require_unsigned(status.quality_flags, 0xFFFF, "quality_flags"),
        _require_unsigned(status.block_sequence, 0xFFFFFFFF, "block_sequence"),
        _require_unsigned(status.current_rms_mg, 0xFFFF, "current_rms_mg"),
        _require_unsigned(
            status.current_crest_factor_milli,
            0xFFFF,
            "current_crest_factor_milli",
        ),
        _require_unsigned(
            status.current_dominant_frequency_centi_hz,
            0xFFFFFFFF,
            "current_dominant_frequency_centi_hz",
        ),
        _require_unsigned(
            status.baseline_rms_mean_mg,
            0xFFFF,
            "baseline_rms_mean_mg",
        ),
        _require_unsigned(
            status.baseline_rms_std_mg,
            0xFFFF,
            "baseline_rms_std_mg",
        ),
        _require_unsigned(
            status.exceeded_feature_mask,
            0xFFFF,
            "exceeded_feature_mask",
        ),
        _require_unsigned(status.rejected_inputs, 0xFFFF, "rejected_inputs"),
    )


# Decode one fixed M8 health-status payload.
def decode_health_status(
    payload: bytes,
) -> HealthStatus:
    """Decode one Guardian M8 health-status payload."""

    # Convert any bytes-like input into immutable bytes.
    encoded = bytes(payload)

    # Require the exact fixed schema size.
    if len(encoded) != _HEALTH_STATUS_STRUCT.size:

        # Reject truncated or trailing bytes.
        raise ValueError(
            (
                "GET_HEALTH_STATUS expected "
                f"{_HEALTH_STATUS_STRUCT.size} bytes, "
                f"received {len(encoded)}"
            )
        )

    # Decode the complete big-endian payload.
    (
        schema_version,
        state_value,
        baseline_samples,
        baseline_target,
        anomaly_score,
        health_score,
        max_deviation_milli,
        dominant_feature,
        consecutive_anomalous,
        quality_flags,
        block_sequence,
        current_rms_mg,
        current_crest_factor_milli,
        current_dominant_frequency_centi_hz,
        baseline_rms_mean_mg,
        baseline_rms_std_mg,
        exceeded_feature_mask,
        rejected_inputs,
    ) = _HEALTH_STATUS_STRUCT.unpack(encoded)

    # Require schema revision one.
    if schema_version != HEALTH_SCHEMA_VERSION:

        # Reject unknown semantics.
        raise ValueError(
            f"unsupported health schema version: {schema_version}"
        )

    # Require a published health state.
    try:

        # Convert the wire state into the published enum.
        state = HealthState(state_value)
    except ValueError as exc:

        # Reject unknown health states.
        raise ValueError(
            f"unsupported health state: {state_value}"
        ) from exc

    # Reject impossible bounded score values.
    if anomaly_score > 1000 or health_score > 1000:

        # Protect callers from corrupted semantic data.
        raise ValueError(
            "health scores must be between 0 and 1000"
        )

    # Return the immutable decoded snapshot.
    return HealthStatus(
        state=state,
        baseline_samples=baseline_samples,
        baseline_target=baseline_target,
        anomaly_score=anomaly_score,
        health_score=health_score,
        max_deviation_milli=max_deviation_milli,
        dominant_feature=dominant_feature,
        consecutive_anomalous=consecutive_anomalous,
        quality_flags=quality_flags,
        block_sequence=block_sequence,
        current_rms_mg=current_rms_mg,
        current_crest_factor_milli=current_crest_factor_milli,
        current_dominant_frequency_centi_hz=current_dominant_frequency_centi_hz,
        baseline_rms_mean_mg=baseline_rms_mean_mg,
        baseline_rms_std_mg=baseline_rms_std_mg,
        exceeded_feature_mask=exceeded_feature_mask,
        rejected_inputs=rejected_inputs,
    )
