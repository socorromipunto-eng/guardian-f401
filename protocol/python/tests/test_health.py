"""Unit tests for Guardian M8 machine-health payload codecs."""

# Import unittest from the Python standard library.
import unittest

# Import the public M8 protocol API under test.
from guardian_protocol import (
    BaselineAction,
    BaselineControl,
    HealthState,
    HealthStatus,
    decode_baseline_control,
    decode_health_status,
    encode_baseline_control,
    encode_health_status,
)


# Verify deterministic M8 payload serialization.
class HealthPayloadTests(unittest.TestCase):
    """Exercise baseline-control and health-status codecs."""

    # Verify baseline START survives a complete round trip.
    def test_baseline_start_round_trip(self) -> None:

        # Build one representative baseline configuration.
        original = BaselineControl(
            action=BaselineAction.START,
            target_samples=64,
        )

        # Encode and decode the fixed payload.
        decoded = decode_baseline_control(
            encode_baseline_control(original)
        )

        # Require complete field preservation.
        self.assertEqual(decoded, original)

    # Verify RESET requires a zero target.
    def test_baseline_reset_rejects_nonzero_target(self) -> None:

        # Require local validation before transport side effects.
        with self.assertRaises(ValueError):

            # Attempt contradictory reset semantics.
            encode_baseline_control(
                BaselineControl(
                    action=BaselineAction.RESET,
                    target_samples=1,
                )
            )

    # Verify complete health status survives binary serialization.
    def test_health_status_round_trip(self) -> None:

        # Build one representative trained warning snapshot.
        original = HealthStatus(
            state=HealthState.WARNING,
            baseline_samples=64,
            baseline_target=64,
            anomaly_score=620,
            health_score=380,
            max_deviation_milli=3720,
            dominant_feature=0,
            consecutive_anomalous=3,
            quality_flags=0x0019,
            block_sequence=1234,
            current_rms_mg=80,
            current_crest_factor_milli=1450,
            current_dominant_frequency_centi_hz=25000,
            baseline_rms_mean_mg=40,
            baseline_rms_std_mg=5,
            exceeded_feature_mask=0x0001,
            rejected_inputs=2,
        )

        # Encode and decode the fixed 36-byte payload.
        decoded = decode_health_status(
            encode_health_status(original)
        )

        # Require complete field preservation.
        self.assertEqual(decoded, original)

    # Verify unsupported health schemas fail closed.
    def test_health_status_rejects_unknown_schema(self) -> None:

        # Encode one valid status payload.
        payload = bytearray(
            encode_health_status(
                HealthStatus(
                    state=HealthState.UNTRAINED,
                    baseline_samples=0,
                    baseline_target=0,
                    anomaly_score=0,
                    health_score=1000,
                    max_deviation_milli=0,
                    dominant_feature=0xFF,
                    consecutive_anomalous=0,
                    quality_flags=0,
                    block_sequence=0,
                    current_rms_mg=0,
                    current_crest_factor_milli=0,
                    current_dominant_frequency_centi_hz=0,
                    baseline_rms_mean_mg=0,
                    baseline_rms_std_mg=0,
                    exceeded_feature_mask=0,
                    rejected_inputs=0,
                )
            )
        )

        # Replace schema revision one with an unsupported value.
        payload[0] = 0x7F

        # Require explicit compatibility failure.
        with self.assertRaises(ValueError):

            # Attempt to decode unknown semantics.
            decode_health_status(payload)


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
