"""Supervisory-control payload codecs for Guardian M9."""

# Import struct for deterministic big-endian wire serialization.
import struct

# Import dataclass for immutable protocol models.
from dataclasses import dataclass

# Import IntEnum for wire-compatible control values.
from enum import IntEnum

# Import the shared M8 health state enum.
from .health import HealthState


# Define the first supervisory-control schema revision.
CONTROL_SCHEMA_VERSION = 0x01

# Define the exact fixed GET_CONTROL_STATUS payload layout.
_CONTROL_STATUS_STRUCT = struct.Struct(">BBBBBBBBHHHHIIBBH")

# Define the exact fixed CONTROL_COMMAND request layout.
_CONTROL_COMMAND_REQUEST_STRUCT = struct.Struct(">BB")

# Define the exact fixed successful CONTROL_COMMAND response layout.
_CONTROL_COMMAND_RESPONSE_STRUCT = struct.Struct(">BBBB")


# Define M9 supervisory states.
class ControlState(IntEnum):
    """Guardian M9 supervisory-control states."""

    # Keep supervision disabled and output safe.
    DISABLED = 0

    # Wait for a local-only run request after safe arming.
    ARMED = 1

    # Publish an active logical run permit under normal health.
    ACTIVE = 2

    # Keep active permit while health reports WARNING.
    DEGRADED = 3

    # Force safe output until explicit fault reset.
    FAULT_LATCHED = 4


# Define host actions that never directly assert machine run request.
class ControlAction(IntEnum):
    """Guardian M9 host supervisory actions."""

    # Arm supervision after all safe-entry conditions pass.
    ARM = 1

    # Disable supervision and force safe output.
    DISARM = 2

    # Clear faults only after explicit safe recovery conditions pass.
    CLEAR_FAULT = 3


# Store one immutable host control command.
@dataclass(frozen=True, slots=True)
class ControlCommand:
    """Guardian M9 supervisory-control command."""

    # Store the explicit host supervisory action.
    action: ControlAction


# Store one immutable successful command acknowledgement.
@dataclass(frozen=True, slots=True)
class ControlCommandResult:
    """Guardian M9 normalized control-command response."""

    # Store the action executed by firmware.
    action: ControlAction

    # Store the resulting supervisory state.
    state: ControlState

    # Store the resulting logical run permit.
    run_permit: bool


# Store one immutable host-visible control snapshot.
@dataclass(frozen=True, slots=True)
class ControlStatus:
    """Decoded Guardian M9 supervisory-control status."""

    # Store current supervisory state.
    state: ControlState

    # Store whether supervision is armed.
    supervision_enabled: bool

    # Store the local-only machine run request.
    local_run_request: bool

    # Store the currently applied logical run permit.
    run_permit: bool

    # Store local safety-interlock state.
    interlock_closed: bool

    # Store the latest M8 health state.
    health_state: HealthState

    # Store whether a board/application output callback is installed.
    output_available: bool

    # Store latched faults requiring explicit reset.
    latched_faults: int

    # Store faults active under current input conditions.
    active_faults: int

    # Store current M8 health score.
    health_score: int

    # Store current M8 anomaly score.
    anomaly_score: int

    # Store monotonic supervisory state-transition count.
    transition_count: int

    # Store monotonic newly latched fault episode count.
    fault_latch_count: int

    # Store the action or automatic reason for the latest transition.
    last_transition_reason: int


# Validate one zero-or-one wire boolean.
def _decode_bool(
    value: int,
    field_name: str,
) -> bool:
    """Decode one canonical Guardian wire boolean."""

    # Require exactly zero or one.
    if value not in (0, 1):

        # Reject ambiguous wire semantics.
        raise ValueError(
            f"{field_name} must be encoded as 0 or 1"
        )

    # Convert the validated value into bool.
    return bool(value)


# Validate one bounded unsigned integer.
def _require_unsigned(
    value: int,
    maximum: int,
    field_name: str,
) -> int:
    """Return *value* after unsigned range validation."""

    # Reject values outside the published field width.
    if not 0 <= value <= maximum:

        # Raise a precise field-specific caller error.
        raise ValueError(
            f"{field_name} must be between 0 and {maximum}"
        )

    # Return the validated integer.
    return value


# Encode one M9 CONTROL_COMMAND request.
def encode_control_command(
    command: ControlCommand,
) -> bytes:
    """Encode one fixed Guardian M9 control request."""

    # Normalize the action into the published enum.
    try:

        # Convert integers or enum values into ControlAction.
        action = ControlAction(command.action)
    except ValueError as exc:

        # Reject undefined host control actions.
        raise ValueError(
            f"unsupported control action: {command.action}"
        ) from exc

    # Pack schema and action only.
    return _CONTROL_COMMAND_REQUEST_STRUCT.pack(
        CONTROL_SCHEMA_VERSION,
        int(action),
    )


# Decode one M9 CONTROL_COMMAND request.
def decode_control_command(
    payload: bytes,
) -> ControlCommand:
    """Decode one fixed Guardian M9 control request."""

    # Convert any bytes-like input into immutable bytes.
    encoded = bytes(payload)

    # Require the exact fixed request size.
    if len(encoded) != _CONTROL_COMMAND_REQUEST_STRUCT.size:

        # Reject truncated or trailing bytes.
        raise ValueError(
            (
                "CONTROL_COMMAND request expected "
                f"{_CONTROL_COMMAND_REQUEST_STRUCT.size} bytes, "
                f"received {len(encoded)}"
            )
        )

    # Decode schema and action.
    schema_version, action_value = (
        _CONTROL_COMMAND_REQUEST_STRUCT.unpack(encoded)
    )

    # Require schema revision one.
    if schema_version != CONTROL_SCHEMA_VERSION:

        # Reject unknown request semantics.
        raise ValueError(
            f"unsupported control schema version: {schema_version}"
        )

    # Require a published host action.
    try:

        # Convert the action wire value.
        action = ControlAction(action_value)
    except ValueError as exc:

        # Reject undefined actions.
        raise ValueError(
            f"unsupported control action: {action_value}"
        ) from exc

    # Return the immutable validated command.
    return ControlCommand(
        action=action
    )


# Encode one successful M9 CONTROL_COMMAND response.
def encode_control_command_result(
    result: ControlCommandResult,
) -> bytes:
    """Encode one fixed successful Guardian M9 control response."""

    # Normalize the executed action.
    try:

        # Convert integer or enum input into ControlAction.
        action = ControlAction(result.action)
    except ValueError as exc:

        # Reject undefined actions.
        raise ValueError(
            f"unsupported control action: {result.action}"
        ) from exc

    # Normalize the resulting state.
    try:

        # Convert integer or enum input into ControlState.
        state = ControlState(result.state)
    except ValueError as exc:

        # Reject undefined states.
        raise ValueError(
            f"unsupported control state: {result.state}"
        ) from exc

    # Pack schema, action, resulting state and canonical run permit.
    return _CONTROL_COMMAND_RESPONSE_STRUCT.pack(
        CONTROL_SCHEMA_VERSION,
        int(action),
        int(state),
        int(bool(result.run_permit)),
    )


# Decode one successful M9 CONTROL_COMMAND response.
def decode_control_command_result(
    payload: bytes,
) -> ControlCommandResult:
    """Decode one fixed successful Guardian M9 control response."""

    # Convert any bytes-like input into immutable bytes.
    encoded = bytes(payload)

    # Require the exact fixed successful response size.
    if len(encoded) != _CONTROL_COMMAND_RESPONSE_STRUCT.size:

        # Reject truncated or undocumented trailing bytes.
        raise ValueError(
            (
                "CONTROL_COMMAND response expected "
                f"{_CONTROL_COMMAND_RESPONSE_STRUCT.size} bytes, "
                f"received {len(encoded)}"
            )
        )

    # Decode the complete response.
    schema_version, action_value, state_value, run_permit_value = (
        _CONTROL_COMMAND_RESPONSE_STRUCT.unpack(encoded)
    )

    # Require schema revision one.
    if schema_version != CONTROL_SCHEMA_VERSION:

        # Reject unknown semantics.
        raise ValueError(
            f"unsupported control schema version: {schema_version}"
        )

    # Require a published action.
    try:

        # Convert the action wire value.
        action = ControlAction(action_value)
    except ValueError as exc:

        # Reject undefined action values.
        raise ValueError(
            f"unsupported control action: {action_value}"
        ) from exc

    # Require a published resulting state.
    try:

        # Convert the state wire value.
        state = ControlState(state_value)
    except ValueError as exc:

        # Reject undefined state values.
        raise ValueError(
            f"unsupported control state: {state_value}"
        ) from exc

    # Decode canonical logical run permit.
    run_permit = _decode_bool(
        run_permit_value,
        "run_permit",
    )

    # Return the immutable normalized acknowledgement.
    return ControlCommandResult(
        action=action,
        state=state,
        run_permit=run_permit,
    )


# Encode one fixed M9 control-status payload.
def encode_control_status(
    status: ControlStatus,
) -> bytes:
    """Encode one Guardian M9 control-status payload."""

    # Normalize control state.
    try:

        # Convert integer or enum input into ControlState.
        state = ControlState(status.state)
    except ValueError as exc:

        # Reject unsupported control states.
        raise ValueError(
            f"unsupported control state: {status.state}"
        ) from exc

    # Normalize health state.
    try:

        # Convert integer or enum input into HealthState.
        health_state = HealthState(status.health_state)
    except ValueError as exc:

        # Reject unsupported M8 state.
        raise ValueError(
            f"unsupported health state: {status.health_state}"
        ) from exc

    # Pack the exact fixed 28-byte schema.
    return _CONTROL_STATUS_STRUCT.pack(
        CONTROL_SCHEMA_VERSION,
        int(state),
        int(bool(status.supervision_enabled)),
        int(bool(status.local_run_request)),
        int(bool(status.run_permit)),
        int(bool(status.interlock_closed)),
        int(health_state),
        int(bool(status.output_available)),
        _require_unsigned(status.latched_faults, 0xFFFF, "latched_faults"),
        _require_unsigned(status.active_faults, 0xFFFF, "active_faults"),
        _require_unsigned(status.health_score, 1000, "health_score"),
        _require_unsigned(status.anomaly_score, 1000, "anomaly_score"),
        _require_unsigned(
            status.transition_count,
            0xFFFFFFFF,
            "transition_count",
        ),
        _require_unsigned(
            status.fault_latch_count,
            0xFFFFFFFF,
            "fault_latch_count",
        ),
        _require_unsigned(
            status.last_transition_reason,
            0xFF,
            "last_transition_reason",
        ),
        0,
        0,
    )


# Decode one fixed M9 control-status payload.
def decode_control_status(
    payload: bytes,
) -> ControlStatus:
    """Decode one Guardian M9 control-status payload."""

    # Convert any bytes-like input into immutable bytes.
    encoded = bytes(payload)

    # Require the exact fixed schema size.
    if len(encoded) != _CONTROL_STATUS_STRUCT.size:

        # Reject truncated or trailing bytes.
        raise ValueError(
            (
                "GET_CONTROL_STATUS expected "
                f"{_CONTROL_STATUS_STRUCT.size} bytes, "
                f"received {len(encoded)}"
            )
        )

    # Decode the complete big-endian payload.
    (
        schema_version,
        state_value,
        supervision_enabled_value,
        local_run_request_value,
        run_permit_value,
        interlock_closed_value,
        health_state_value,
        output_available_value,
        latched_faults,
        active_faults,
        health_score,
        anomaly_score,
        transition_count,
        fault_latch_count,
        last_transition_reason,
        reserved,
        reserved2,
    ) = _CONTROL_STATUS_STRUCT.unpack(encoded)

    # Require schema revision one.
    if schema_version != CONTROL_SCHEMA_VERSION:

        # Reject unknown semantics.
        raise ValueError(
            f"unsupported control schema version: {schema_version}"
        )

    # Require reserved fields to remain zero in schema v1.
    if reserved != 0 or reserved2 != 0:

        # Reject undefined schema-v1 semantics.
        raise ValueError(
            "control status reserved fields must be zero"
        )

    # Require a published control state.
    try:

        # Convert the wire state.
        state = ControlState(state_value)
    except ValueError as exc:

        # Reject unknown control states.
        raise ValueError(
            f"unsupported control state: {state_value}"
        ) from exc

    # Require a published health state.
    try:

        # Convert the M8 wire state.
        health_state = HealthState(health_state_value)
    except ValueError as exc:

        # Reject unknown health states.
        raise ValueError(
            f"unsupported health state: {health_state_value}"
        ) from exc

    # Reject impossible bounded score values.
    if health_score > 1000 or anomaly_score > 1000:

        # Protect callers from corrupted semantic data.
        raise ValueError(
            "control health/anomaly scores must be between 0 and 1000"
        )

    # Return the immutable decoded control snapshot.
    return ControlStatus(
        state=state,
        supervision_enabled=_decode_bool(
            supervision_enabled_value,
            "supervision_enabled",
        ),
        local_run_request=_decode_bool(
            local_run_request_value,
            "local_run_request",
        ),
        run_permit=_decode_bool(
            run_permit_value,
            "run_permit",
        ),
        interlock_closed=_decode_bool(
            interlock_closed_value,
            "interlock_closed",
        ),
        health_state=health_state,
        output_available=_decode_bool(
            output_available_value,
            "output_available",
        ),
        latched_faults=latched_faults,
        active_faults=active_faults,
        health_score=health_score,
        anomaly_score=anomaly_score,
        transition_count=transition_count,
        fault_latch_count=fault_latch_count,
        last_transition_reason=last_transition_reason,
    )
