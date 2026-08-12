"""Unit tests for guardianctl text and JSON presentation helpers."""

# Import json so generated machine-readable output can be validated structurally.
import json

# Import unittest from the Python standard library.
import unittest

# Import shared typed protocol models used by presentation helpers.
from guardian_protocol import DeviceInfo, DeviceState, DeviceStatus

# Import the typed PING result.
from guardianctl.client import PingResult

# Import the default host configuration.
from guardianctl.config import ClientConfig

# Import presentation functions under test.
from guardianctl.presentation import (
    format_uptime,
    render_info_json,
    render_ping_text,
    render_status_text,
)


# Verify stable human and machine-readable console formatting.
class PresentationTests(unittest.TestCase):
    """Exercise guardianctl presentation behavior."""

    # Verify short uptime formatting.
    def test_format_uptime_under_one_day(self) -> None:

        # Require deterministic zero-padded HH:MM:SS output.
        self.assertEqual(format_uptime(3661), "01:01:01")

    # Verify extended uptime formatting.
    def test_format_uptime_with_days(self) -> None:

        # Require deterministic day plus HH:MM:SS output.
        self.assertEqual(format_uptime(90061), "1d 01:01:01")

    # Verify human-readable PING output contains the endpoint and latency.
    def test_ping_text_contains_endpoint(self) -> None:

        # Render one deterministic PING result.
        output = render_ping_text(
            PingResult(reply="PONG", latency_ms=1.25),
            ClientConfig(),
        )

        # Require the safe default development endpoint.
        self.assertIn("127.0.0.1:9401", output)

        # Require the frozen PONG response text.
        self.assertIn("PONG", output)

    # Verify metadata JSON output is valid and stable.
    def test_info_json_is_machine_readable(self) -> None:

        # Render deterministic simulator metadata.
        output = render_info_json(
            DeviceInfo(
                model="Guardian-F401-SIM",
                firmware_major=0,
                firmware_minor=2,
                firmware_patch=0,
                device_id=0xF4010001,
            )
        )

        # Decode the generated JSON using the standard library.
        decoded = json.loads(output)

        # Require the expected model identity.
        self.assertEqual(decoded["model"], "Guardian-F401-SIM")

        # Require the fixed-width hexadecimal device identifier.
        self.assertEqual(decoded["device_id"], "F4010001")

    # Verify human-readable status exposes core runtime diagnostics.
    def test_status_text_contains_runtime_diagnostics(self) -> None:

        # Render one deterministic runtime snapshot.
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

        # Require the named application state.
        self.assertIn("State: IDLE", output)

        # Require the protocol error counter.
        self.assertIn("Protocol errors: 1", output)


# Execute the tests when this file is run directly.
if __name__ == "__main__":

    # Start the standard-library unit test runner with verbose output.
    unittest.main(verbosity=2)
