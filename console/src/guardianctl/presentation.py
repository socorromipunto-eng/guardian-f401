"""Human-readable and JSON presentation helpers for guardianctl."""

# Import json for machine-readable output.
import json

# Import typed protocol models displayed by the CLI.
from guardian_protocol import DeviceInfo, DeviceStatus

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
