"""Human-readable and JSON presentation helpers for guardianctl."""

# Import json for machine-readable CLI output without external dependencies.
import json

# Import typed protocol models displayed by the CLI.
from guardian_protocol import DeviceInfo, DeviceStatus

# Import the typed successful PING result.
from .client import PingResult

# Import immutable host connection configuration.
from .config import ClientConfig


# Format unsigned 32-bit device identifiers consistently.
def format_device_id(device_id: int) -> str:
    """Return a fixed-width hexadecimal Guardian device identifier."""

    # Render the identifier using eight uppercase hexadecimal digits.
    return f"{device_id:08X}"


# Convert whole uptime seconds into a compact human-readable duration.
def format_uptime(total_seconds: int) -> str:
    """Return uptime using days plus HH:MM:SS when required."""

    # Split the complete duration into whole days and remaining seconds.
    days, remaining_seconds = divmod(total_seconds, 86400)

    # Split the remaining duration into whole hours and remaining seconds.
    hours, remaining_seconds = divmod(remaining_seconds, 3600)

    # Split the remaining duration into minutes and seconds.
    minutes, seconds = divmod(remaining_seconds, 60)

    # Include the day component only when uptime has crossed one day.
    if days:

        # Return the extended duration representation.
        return f"{days}d {hours:02d}:{minutes:02d}:{seconds:02d}"

    # Return the compact sub-day duration representation.
    return f"{hours:02d}:{minutes:02d}:{seconds:02d}"


# Render one successful PING result for a human operator.
def render_ping_text(result: PingResult, config: ClientConfig) -> str:
    """Return human-readable PING output."""

    # Join stable output lines into one terminal-ready text block.
    return "\n".join(
        (
            "Guardian device reachable",
            f"Endpoint: {config.host}:{config.port}",
            f"Reply: {result.reply}",
            f"Latency: {result.latency_ms:.2f} ms",
        )
    )


# Render one successful PING result for scripting and automation.
def render_ping_json(result: PingResult, config: ClientConfig) -> str:
    """Return JSON PING output."""

    # Serialize a stable machine-readable object with sorted keys.
    return json.dumps(
        {
            "endpoint": f"{config.host}:{config.port}",
            "latency_ms": round(result.latency_ms, 3),
            "reachable": True,
            "reply": result.reply,
        },
        sort_keys=True,
    )


# Render immutable device metadata for a human operator.
def render_info_text(info: DeviceInfo) -> str:
    """Return human-readable DEVICE_INFO output."""

    # Join stable metadata lines into one terminal-ready text block.
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


# Render immutable device metadata for scripts and automation.
def render_info_json(info: DeviceInfo) -> str:
    """Return JSON DEVICE_INFO output."""

    # Serialize a stable machine-readable metadata object.
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


# Render one runtime status snapshot for a human operator.
def render_status_text(status: DeviceStatus) -> str:
    """Return human-readable GET_STATUS output."""

    # Join stable runtime diagnostic lines into one terminal-ready text block.
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


# Render one runtime status snapshot for scripts and automation.
def render_status_json(status: DeviceStatus) -> str:
    """Return JSON GET_STATUS output."""

    # Serialize a stable machine-readable runtime diagnostic object.
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
