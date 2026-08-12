"""Unit tests for Guardian Protocol M5 telemetry payload codecs."""

# Import unittest from the Python standard library.
import unittest

# Import the public telemetry API under test.
from guardian_protocol import (
    DeviceState,
    MachineTelemetry,
    TelemetryConfig,
    decode_machine_telemetry,
    decode_telemetry_config,
    encode_machine_telemetry,
    encode_telemetry_config,
)


# Verify deterministic M5 telemetry payload serialization.
class TelemetryPayloadTests(unittest.TestCase):
    """Exercise telemetry configuration and sample codecs."""

    # Verify telemetry configuration survives a complete binary round trip.
    def test_telemetry_config_round_trip(self) -> None:

        # Build one representative active telemetry configuration.
        original = TelemetryConfig(
            enabled=True,
            period_ms=250,
        )

        # Encode and decode the fixed configuration payload.
        decoded = decode_telemetry_config(
            encode_telemetry_config(original)
        )

        # Require complete field preservation.
        self.assertEqual(decoded, original)

    # Verify periods below the M5 rate limit are rejected.
    def test_telemetry_config_rejects_fast_rate(self) -> None:

        # Build an out-of-policy telemetry configuration.
        config = TelemetryConfig(
            enabled=True,
            period_ms=99,
        )

        # Require validation before wire bytes are produced.
        with self.assertRaises(ValueError):

            # Attempt to encode the invalid rate.
            encode_telemetry_config(config)

    # Verify one complete machine sample survives binary serialization.
    def test_machine_telemetry_round_trip(self) -> None:

        # Build one representative machine telemetry sample.
        original = MachineTelemetry(
            state=DeviceState.RUNNING,
            timestamp_ms=0x12345678,
            temperature_centi_c=-525,
            vibration_mg_rms=87,
            current_ma=1234,
            rpm=2875,
            supply_mv=3298,
            status_flags=0x0003,
        )

        # Encode and decode the fixed-width telemetry payload.
        decoded = decode_machine_telemetry(
            encode_machine_telemetry(original)
        )

        # Require complete field preservation including signed temperature.
        self.assertEqual(decoded, original)

    # Verify unsupported telemetry payload schema revisions fail closed.
    def test_machine_telemetry_rejects_unknown_schema(self) -> None:

        # Encode one valid machine telemetry payload.
        encoded = bytearray(
            encode_machine_telemetry(
                MachineTelemetry(
                    state=DeviceState.IDLE,
                    timestamp_ms=1,
                    temperature_centi_c=2500,
                    vibration_mg_rms=10,
                    current_ma=20,
                    rpm=30,
                    supply_mv=3300,
                    status_flags=0,
                )
            )
        )

        # Replace the schema byte with an unsupported future value.
        encoded[0] = 0x7F

        # Require the decoder to reject unknown semantics.
        with self.assertRaises(ValueError):

            # Attempt to decode the unsupported payload.
            decode_machine_telemetry(encoded)


# Execute the tests when this file is run directly.
if __name__ == "__main__":

    # Start the standard-library test runner.
    unittest.main(verbosity=2)
