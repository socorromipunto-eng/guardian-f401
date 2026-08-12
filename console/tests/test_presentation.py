"""Unit tests for guardianctl presentation helpers."""

# Import json for machine-readable output validation.
import json

# Import unittest from the standard library.
import unittest

# Import typed protocol models.
from guardian_protocol import DeviceInfo, DeviceState, DeviceStatus

# Import typed PING result.
from guardianctl.client import PingResult

# Import presentation functions.
from guardianctl.presentation import (
    format_uptime,
    render_info_json,
    render_ping_text,
    render_status_text,
)


# Verify stable console formatting.
class PresentationTests(unittest.TestCase):
    """Exercise guardianctl presentation behavior."""

    # Verify short uptime formatting.
    def test_format_uptime_under_one_day(self) -> None:

        # Require HH:MM:SS output.
        self.assertEqual(format_uptime(3661), "01:01:01")

    # Verify extended uptime formatting.
    def test_format_uptime_with_days(self) -> None:

        # Require day plus HH:MM:SS output.
        self.assertEqual(format_uptime(90061), "1d 01:01:01")

    # Verify PING output contains the supplied endpoint.
    def test_ping_text_contains_endpoint(self) -> None:

        # Render deterministic PING output.
        output = render_ping_text(
            PingResult(reply="PONG", latency_ms=1.25),
            "COM5@115200",
        )

        # Require physical endpoint text.
        self.assertIn("COM5@115200", output)

        # Require PONG.
        self.assertIn("PONG", output)

    # Verify metadata JSON.
    def test_info_json_is_machine_readable(self) -> None:

        # Render deterministic metadata.
        output = render_info_json(
            DeviceInfo(
                model="Guardian-F401-SIM",
                firmware_major=0,
                firmware_minor=2,
                firmware_patch=0,
                device_id=0xF4010001,
            )
        )

        # Decode generated JSON.
        decoded = json.loads(output)

        # Require model identity.
        self.assertEqual(decoded["model"], "Guardian-F401-SIM")

        # Require fixed-width device ID.
        self.assertEqual(decoded["device_id"], "F4010001")

    # Verify status text.
    def test_status_text_contains_runtime_diagnostics(self) -> None:

        # Render deterministic status.
        output = render_status_text(
            DeviceStatus(
                state=DeviceState.IDLE,
                uptime_seconds=10,
                rx_frames=4,
                tx_frames=3,
                protocol_errors=1,
                last_error=6,
            )
        )

        # Require state.
        self.assertIn("State: IDLE", output)

        # Require protocol error count.
        self.assertIn("Protocol errors: 1", output)


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
