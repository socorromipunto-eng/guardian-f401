"""Human-readable and JSON presentation helpers for guardianctl."""

# Import json for machine-readable output.
import json

# Import typed protocol models displayed by the CLI.
from guardian_protocol import DeviceInfo, DeviceStatus

# Import typed PING result.
from .client import PingResult


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
