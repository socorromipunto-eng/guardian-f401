"""Unit tests for M5 telemetry behavior in the software Guardian device."""

# Import time for one bounded real scheduling interval.
import time

# Import unittest from the Python standard library.
import unittest

# Import protocol models and codecs used by the test host.
from guardian_protocol import (
    Command,
    Frame,
    MessageType,
    TelemetryConfig,
    decode_machine_telemetry,
    decode_telemetry_config,
    encode_telemetry_config,
)

# Import the simulated device under test.
from guardian_sim import GuardianDevice


# Verify M5 telemetry configuration and deterministic sample generation.
class GuardianDeviceTelemetryTests(unittest.TestCase):
    """Exercise telemetry semantics without TCP transport."""

    # Create one fresh simulated device before every test.
    def setUp(self) -> None:

        # Avoid scheduling or counter leakage between tests.
        self.device = GuardianDevice()

    # Verify the device acknowledges a valid telemetry configuration.
    def test_set_telemetry_returns_normalized_configuration(self) -> None:

        # Build one valid active telemetry configuration.
        config = TelemetryConfig(
            enabled=True,
            period_ms=100,
        )

        # Build the request using the canonical command payload.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.SET_TELEMETRY,
            sequence=41,
            payload=encode_telemetry_config(config),
        )

        # Execute telemetry configuration.
        response = self.device.process_frame(request)

        # Require a successful synchronous response.
        self.assertEqual(
            response.message_type,
            MessageType.RESPONSE,
        )

        # Require request/response correlation.
        self.assertEqual(response.sequence, 41)

        # Require the exact normalized active configuration.
        self.assertEqual(
            decode_telemetry_config(response.payload),
            config,
        )

    # Verify one asynchronous sample appears after the configured period.
    def test_due_telemetry_frame_is_generated(self) -> None:

        # Enable the minimum allowed telemetry period.
        self.device.process_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.SET_TELEMETRY,
                sequence=42,
                payload=encode_telemetry_config(
                    TelemetryConfig(
                        enabled=True,
                        period_ms=100,
                    )
                ),
            )
        )

        # Require no immediate sample before the configured interval.
        self.assertIsNone(
            self.device.poll_telemetry()
        )

        # Wait slightly beyond the minimum interval.
        time.sleep(0.11)

        # Request the now-due asynchronous frame.
        frame = self.device.poll_telemetry()

        # Require a telemetry frame to be produced.
        self.assertIsNotNone(frame)

        # Narrow the optional type after the explicit assertion.
        assert frame is not None

        # Require the asynchronous telemetry message class.
        self.assertEqual(
            frame.message_type,
            MessageType.TELEMETRY,
        )

        # Require the dedicated machine telemetry channel.
        self.assertEqual(
            frame.command,
            Command.MACHINE_TELEMETRY,
        )

        # Decode the deterministic synthetic sample.
        sample = decode_machine_telemetry(
            frame.payload
        )

        # Require the simulator's fixed 3.3 V synthetic supply.
        self.assertEqual(sample.supply_mv, 3300)

        # Require non-zero synthetic shaft speed.
        self.assertGreater(sample.rpm, 0)


# Execute the tests when this file is run directly.
if __name__ == "__main__":

    # Start the standard-library test runner.
    unittest.main(verbosity=2)
