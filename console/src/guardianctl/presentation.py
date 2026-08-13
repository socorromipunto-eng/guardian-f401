"""Human-readable and JSON presentation helpers for guardianctl."""

# Import json for machine-readable output.
import json

# Import typed protocol models displayed by the CLI.
from guardian_protocol import (
    BaselineControl,
    ControlCommandResult,
    ControlStatus,
    DeviceInfo,
    DeviceStatus,
    DspFeatures,
    HealthStatus,
    AuthenticatedSession,
    SecurityStatus,
)

# Import typed PING result.
from .client import PingResult

# Import the decoded asynchronous telemetry record.
from .telemetry_client import TelemetryRecord


# Format unsigned 32-bit device identifiers consistently.
def format_device_id(device_id: int) -> str:
    """Return a fixed-width hexadecimal Guardian device identifier."""

    # Render eight uppercase hexadecimal digits.
    return f"{device_id:08X}"


# Convert uptime seconds into a compact duration.
def format_uptime(total_seconds: int) -> str:
    """Return uptime using days plus HH:MM:SS when required."""

    # Split into days and remaining seconds.
    days, remaining_seconds = divmod(total_seconds, 86400)

    # Split into hours and remaining seconds.
    hours, remaining_seconds = divmod(remaining_seconds, 3600)

    # Split into minutes and seconds.
    minutes, seconds = divmod(remaining_seconds, 60)

    # Include days only when needed.
    if days:

        # Return extended duration.
        return f"{days}d {hours:02d}:{minutes:02d}:{seconds:02d}"

    # Return sub-day duration.
    return f"{hours:02d}:{minutes:02d}:{seconds:02d}"


# Render one successful PING result for a human.
def render_ping_text(result: PingResult, endpoint: str) -> str:
    """Return human-readable PING output."""

    # Join stable output lines.
    return "\n".join(
        (
            "Guardian device reachable",
            f"Endpoint: {endpoint}",
            f"Reply: {result.reply}",
            f"Latency: {result.latency_ms:.2f} ms",
        )
    )


# Render one successful PING result as JSON.
def render_ping_json(result: PingResult, endpoint: str) -> str:
    """Return JSON PING output."""

    # Serialize stable machine-readable output.
    return json.dumps(
        {
            "endpoint": endpoint,
            "latency_ms": round(result.latency_ms, 3),
            "reachable": True,
            "reply": result.reply,
        },
        sort_keys=True,
    )


# Render immutable device metadata for a human.
def render_info_text(info: DeviceInfo) -> str:
    """Return human-readable DEVICE_INFO output."""

    # Join stable metadata lines.
    return "\n".join(
        (
            f"Model: {info.model}",
            (
                "Firmware: "
                f"{info.firmware_major}."
                f"{info.firmware_minor}."
                f"{info.firmware_patch}"
            ),
            f"Device ID: {format_device_id(info.device_id)}",
            "Protocol: 0.1",
        )
    )


# Render immutable device metadata as JSON.
def render_info_json(info: DeviceInfo) -> str:
    """Return JSON DEVICE_INFO output."""

    # Serialize stable machine-readable metadata.
    return json.dumps(
        {
            "device_id": format_device_id(info.device_id),
            "firmware": (
                f"{info.firmware_major}."
                f"{info.firmware_minor}."
                f"{info.firmware_patch}"
            ),
            "model": info.model,
            "protocol": "0.1",
        },
        sort_keys=True,
    )


# Render runtime status for a human.
def render_status_text(status: DeviceStatus) -> str:
    """Return human-readable GET_STATUS output."""

    # Join stable diagnostic lines.
    return "\n".join(
        (
            f"State: {status.state.name}",
            f"Uptime: {format_uptime(status.uptime_seconds)}",
            f"RX frames: {status.rx_frames}",
            f"TX frames: {status.tx_frames}",
            f"Protocol errors: {status.protocol_errors}",
            f"Last error: 0x{status.last_error:02X}",
        )
    )


# Render runtime status as JSON.
def render_status_json(status: DeviceStatus) -> str:
    """Return JSON GET_STATUS output."""

    # Serialize stable machine-readable status.
    return json.dumps(
        {
            "last_error": status.last_error,
            "protocol_errors": status.protocol_errors,
            "rx_frames": status.rx_frames,
            "state": status.state.name,
            "tx_frames": status.tx_frames,
            "uptime_seconds": status.uptime_seconds,
        },
        sort_keys=True,
    )


# Render one live telemetry record for a human operator.
def render_telemetry_text(record: TelemetryRecord) -> str:
    """Return one compact human-readable telemetry line."""

    # Convert signed hundredths of a degree into Celsius for display only.
    temperature_c = record.sample.temperature_centi_c / 100.0

    # Return one terminal-friendly live sample line.
    return (
        f"[{record.sequence:08d}] "
        f"t={record.sample.timestamp_ms} ms "
        f"state={record.sample.state.name} "
        f"temp={temperature_c:.2f} C "
        f"vib={record.sample.vibration_mg_rms} mgRMS "
        f"current={record.sample.current_ma} mA "
        f"rpm={record.sample.rpm} "
        f"supply={record.sample.supply_mv} mV "
        f"flags=0x{record.sample.status_flags:04X}"
    )


# Render one live telemetry record as one JSON Lines object.
def render_telemetry_json(record: TelemetryRecord) -> str:
    """Return one machine-readable JSON telemetry line."""

    # Serialize stable telemetry fields for streaming automation.
    return json.dumps(
        {
            "current_ma": record.sample.current_ma,
            "rpm": record.sample.rpm,
            "sequence": record.sequence,
            "state": record.sample.state.name,
            "status_flags": record.sample.status_flags,
            "supply_mv": record.sample.supply_mv,
            "temperature_centi_c": record.sample.temperature_centi_c,
            "timestamp_ms": record.sample.timestamp_ms,
            "vibration_mg_rms": record.sample.vibration_mg_rms,
        },
        sort_keys=True,
    )


# Render one M7 DSP feature snapshot for a human operator.
def render_dsp_text(features: DspFeatures) -> str:
    """Return human-readable DSP feature output."""

    # Convert fixed-point frequency fields into hertz.
    dominant_hz = features.dominant_frequency_centi_hz / 100.0

    # Convert fixed-point spectral centroid into hertz.
    centroid_hz = features.spectral_centroid_centi_hz / 100.0

    # Convert fixed-point crest factor into a decimal ratio.
    crest_factor = features.crest_factor_milli / 1000.0

    # Return stable diagnostic lines.
    return "\n".join(
        (
            f"Block: {features.block_sequence}",
            f"Sample rate: {features.sample_rate_hz} Hz",
            f"RMS: {features.rms_mg} mg",
            f"Peak: {features.peak_mg} mg",
            f"Crest factor: {crest_factor:.3f}",
            f"Dominant frequency: {dominant_hz:.2f} Hz",
            f"Dominant peak: {features.dominant_peak_mg} mg",
            f"Spectral centroid: {centroid_hz:.2f} Hz",
            (
                "Band energy: "
                f"low={features.low_band_permille}‰ "
                f"mid={features.mid_band_permille}‰ "
                f"high={features.high_band_permille}‰"
            ),
            (
                "Acquisition flags: "
                f"0x{features.acquisition_status_flags:04X}"
            ),
        )
    )


# Render one M7 DSP feature snapshot as JSON.
def render_dsp_json(features: DspFeatures) -> str:
    """Return machine-readable DSP feature output."""

    # Serialize stable field names for scripts and dashboards.
    return json.dumps(
        {
            "acquisition_status_flags": features.acquisition_status_flags,
            "block_sequence": features.block_sequence,
            "crest_factor_milli": features.crest_factor_milli,
            "dominant_frequency_centi_hz": features.dominant_frequency_centi_hz,
            "dominant_peak_mg": features.dominant_peak_mg,
            "high_band_permille": features.high_band_permille,
            "low_band_permille": features.low_band_permille,
            "mid_band_permille": features.mid_band_permille,
            "peak_mg": features.peak_mg,
            "rms_mg": features.rms_mg,
            "sample_rate_hz": features.sample_rate_hz,
            "spectral_centroid_centi_hz": features.spectral_centroid_centi_hz,
        },
        sort_keys=True,
    )


# Map M8 feature identifiers to stable operator-facing names.
_HEALTH_FEATURE_NAMES = {
    0: "rms",
    1: "crest_factor",
    2: "dominant_frequency",
    3: "spectral_centroid",
    4: "low_band",
    5: "mid_band",
    6: "high_band",
    0xFF: "none",
}


# Render one M8 machine-health snapshot for a human operator.
def render_health_text(status: HealthStatus) -> str:
    """Return human-readable M8 machine-health output."""

    # Resolve the dominant feature identifier safely.
    dominant_feature = _HEALTH_FEATURE_NAMES.get(
        status.dominant_feature,
        f"unknown({status.dominant_feature})",
    )

    # Convert fixed-point dominant frequency into hertz.
    dominant_hz = (
        status.current_dominant_frequency_centi_hz
        / 100.0
    )

    # Convert fixed-point crest factor into a ratio.
    crest_factor = (
        status.current_crest_factor_milli
        / 1000.0
    )

    # Convert fixed-point maximum deviation into weighted sigma-like units.
    maximum_deviation = (
        status.max_deviation_milli
        / 1000.0
    )

    # Return stable health diagnostic lines.
    return "\n".join(
        (
            f"Health state: {status.state.name}",
            (
                "Baseline: "
                f"{status.baseline_samples}/"
                f"{status.baseline_target} samples"
            ),
            f"Health score: {status.health_score}/1000",
            f"Anomaly score: {status.anomaly_score}/1000",
            f"Max deviation: {maximum_deviation:.3f}",
            f"Dominant anomaly feature: {dominant_feature}",
            f"Current RMS: {status.current_rms_mg} mg",
            f"Current crest factor: {crest_factor:.3f}",
            f"Current dominant frequency: {dominant_hz:.2f} Hz",
            (
                "Baseline RMS: "
                f"{status.baseline_rms_mean_mg} "
                f"+/- {status.baseline_rms_std_mg} mg"
            ),
            (
                "Exceeded feature mask: "
                f"0x{status.exceeded_feature_mask:04X}"
            ),
            (
                "Quality flags: "
                f"0x{status.quality_flags:04X}"
            ),
            f"Rejected inputs: {status.rejected_inputs}",
        )
    )


# Render one M8 machine-health snapshot as JSON.
def render_health_json(status: HealthStatus) -> str:
    """Return machine-readable M8 machine-health output."""

    # Serialize stable field names for dashboards and automation.
    return json.dumps(
        {
            "anomaly_score": status.anomaly_score,
            "baseline_rms_mean_mg": status.baseline_rms_mean_mg,
            "baseline_rms_std_mg": status.baseline_rms_std_mg,
            "baseline_samples": status.baseline_samples,
            "baseline_target": status.baseline_target,
            "block_sequence": status.block_sequence,
            "consecutive_anomalous": status.consecutive_anomalous,
            "current_crest_factor_milli": (
                status.current_crest_factor_milli
            ),
            "current_dominant_frequency_centi_hz": (
                status.current_dominant_frequency_centi_hz
            ),
            "current_rms_mg": status.current_rms_mg,
            "dominant_feature": status.dominant_feature,
            "exceeded_feature_mask": status.exceeded_feature_mask,
            "health_score": status.health_score,
            "max_deviation_milli": status.max_deviation_milli,
            "quality_flags": status.quality_flags,
            "rejected_inputs": status.rejected_inputs,
            "state": status.state.name,
        },
        sort_keys=True,
    )


# Render one normalized baseline-control acknowledgement for a human.
def render_baseline_text(control: BaselineControl) -> str:
    """Return human-readable baseline-control acknowledgement."""

    # Render START with its explicit target.
    if control.action.name == "START":

        # Return the active bounded baseline target.
        return (
            "Baseline learning started: "
            f"target={control.target_samples} samples"
        )

    # Render RESET without an irrelevant target.
    return "Baseline reset: model is UNTRAINED"


# Render one normalized baseline-control acknowledgement as JSON.
def render_baseline_json(control: BaselineControl) -> str:
    """Return machine-readable baseline-control acknowledgement."""

    # Serialize the normalized device response.
    return json.dumps(
        {
            "action": control.action.name,
            "target_samples": control.target_samples,
        },
        sort_keys=True,
    )


# Map M9 fault bits to stable human-readable names.
_CONTROL_FAULT_NAMES = (
    (0x0001, "HEALTH_NOT_READY"),
    (0x0002, "HEALTH_ALARM"),
    (0x0004, "INTERLOCK_OPEN"),
    (0x0008, "OUTPUT_FAILURE"),
    (0x0010, "OUTPUT_UNAVAILABLE"),
)


# Format one M9 control fault mask for human output.
def _format_control_faults(mask: int) -> str:
    """Return a compact symbolic M9 fault list."""

    # Collect every published fault bit currently set.
    names = [
        name
        for bit, name in _CONTROL_FAULT_NAMES
        if mask & bit
    ]

    # Return NONE when no published fault bit is active.
    if not names:

        # Publish an explicit clean state.
        return "NONE"

    # Join stable symbolic fault names.
    return ",".join(names)


# Render one M9 control snapshot for a human operator.
def render_control_status_text(status: ControlStatus) -> str:
    """Return human-readable M9 supervisory-control output."""

    # Return stable operator-facing control lines.
    return "\n".join(
        (
            f"Control state: {status.state.name}",
            (
                "Supervision enabled: "
                f"{'yes' if status.supervision_enabled else 'no'}"
            ),
            (
                "Local run request: "
                f"{'yes' if status.local_run_request else 'no'}"
            ),
            (
                "Run permit: "
                f"{'ON' if status.run_permit else 'SAFE-OFF'}"
            ),
            (
                "Interlock: "
                f"{'closed' if status.interlock_closed else 'OPEN'}"
            ),
            f"Health state: {status.health_state.name}",
            f"Health score: {status.health_score}/1000",
            f"Anomaly score: {status.anomaly_score}/1000",
            (
                "Latched faults: "
                f"0x{status.latched_faults:04X} "
                f"({_format_control_faults(status.latched_faults)})"
            ),
            (
                "Active faults: "
                f"0x{status.active_faults:04X} "
                f"({_format_control_faults(status.active_faults)})"
            ),
            f"Transitions: {status.transition_count}",
            f"Fault latch episodes: {status.fault_latch_count}",
            (
                "Last transition reason: "
                f"0x{status.last_transition_reason:02X}"
            ),
        )
    )


# Render one M9 control snapshot as JSON.
def render_control_status_json(status: ControlStatus) -> str:
    """Return machine-readable M9 supervisory-control output."""

    # Serialize stable fields for dashboards and automation.
    return json.dumps(
        {
            "active_faults": status.active_faults,
            "anomaly_score": status.anomaly_score,
            "fault_latch_count": status.fault_latch_count,
            "health_score": status.health_score,
            "health_state": status.health_state.name,
            "interlock_closed": status.interlock_closed,
            "last_transition_reason": status.last_transition_reason,
            "latched_faults": status.latched_faults,
            "local_run_request": status.local_run_request,
            "output_available": status.output_available,
            "run_permit": status.run_permit,
            "state": status.state.name,
            "supervision_enabled": status.supervision_enabled,
            "transition_count": status.transition_count,
        },
        sort_keys=True,
    )


# Render one successful M9 host control action for a human operator.
def render_control_result_text(
    result: ControlCommandResult,
) -> str:
    """Return human-readable M9 control action acknowledgement."""

    # Return the normalized device result.
    return "\n".join(
        (
            f"Action: {result.action.name}",
            f"Control state: {result.state.name}",
            (
                "Run permit: "
                f"{'ON' if result.run_permit else 'SAFE-OFF'}"
            ),
        )
    )


# Render one successful M9 host control action as JSON.
def render_control_result_json(
    result: ControlCommandResult,
) -> str:
    """Return machine-readable M9 control action acknowledgement."""

    # Serialize the normalized result.
    return json.dumps(
        {
            "action": result.action.name,
            "run_permit": result.run_permit,
            "state": result.state.name,
        },
        sort_keys=True,
    )


# Render one public M10 security-status snapshot for a human operator.
def render_security_status_text(
    status: SecurityStatus,
) -> str:
    """Return human-readable M10 security diagnostics."""

    # Return stable security diagnostic lines.
    return "\n".join(
        (
            (
                "Security provisioned: "
                f"{'yes' if status.configured else 'no'}"
            ),
            (
                "Authenticated session: "
                f"{'active' if status.active else 'none'}"
            ),
            f"Active role: {status.active_role.name}",
            f"Session ID: 0x{status.session_id:08X}",
            f"Next secure counter: {status.next_counter}",
            f"Session timeout: {status.timeout_seconds} s",
            f"Remaining lifetime: {status.remaining_seconds} s",
            f"Authentication successes: {status.auth_successes}",
            f"Authentication failures: {status.auth_failures}",
            f"Replay rejections: {status.replay_rejections}",
            (
                "Authorization rejections: "
                f"{status.unauthorized_rejections}"
            ),
        )
    )


# Render one public M10 security-status snapshot as JSON.
def render_security_status_json(
    status: SecurityStatus,
) -> str:
    """Return machine-readable M10 security diagnostics."""

    # Serialize stable fields without secret material.
    return json.dumps(
        {
            "active": status.active,
            "active_role": status.active_role.name,
            "auth_failures": status.auth_failures,
            "auth_successes": status.auth_successes,
            "configured": status.configured,
            "next_counter": status.next_counter,
            "remaining_seconds": status.remaining_seconds,
            "replay_rejections": status.replay_rejections,
            "session_id": status.session_id,
            "timeout_seconds": status.timeout_seconds,
            "unauthorized_rejections": status.unauthorized_rejections,
        },
        sort_keys=True,
    )


# Render one successful M10 authentication for a human operator.
def render_authenticated_session_text(
    session: AuthenticatedSession,
) -> str:
    """Return human-readable M10 session establishment output."""

    # Return concise authenticated-session metadata.
    return "\n".join(
        (
            "Authentication: SUCCESS",
            f"Role: {session.role.name}",
            f"Session ID: 0x{session.session_id:08X}",
            f"Timeout: {session.timeout_seconds} s",
        )
    )


# Render one successful M10 authentication as JSON.
def render_authenticated_session_json(
    session: AuthenticatedSession,
) -> str:
    """Return machine-readable M10 session establishment output."""

    # Serialize public session metadata only.
    return json.dumps(
        {
            "authenticated": True,
            "role": session.role.name,
            "session_id": session.session_id,
            "timeout_seconds": session.timeout_seconds,
        },
        sort_keys=True,
    )
