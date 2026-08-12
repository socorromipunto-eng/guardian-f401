"""Unit tests for the Guardian M7 DSP payload codec."""

# Import unittest from the Python standard library.
import unittest

# Import the public M7 protocol API under test.
from guardian_protocol import (
    DspFeatures,
    decode_dsp_features,
    encode_dsp_features,
)


# Verify deterministic M7 DSP binary serialization.
class DspPayloadTests(unittest.TestCase):
    """Exercise DSP feature encoding and decoding."""

    # Verify a complete feature snapshot survives a round trip.
    def test_round_trip(self) -> None:

        # Build one representative feature snapshot.
        original = DspFeatures(
            block_sequence=0x12345678,
            sample_rate_hz=4000,
            rms_mg=42,
            peak_mg=61,
            crest_factor_milli=1452,
            dominant_frequency_centi_hz=25000,
            dominant_peak_mg=58,
            spectral_centroid_centi_hz=41250,
            low_band_permille=760,
            mid_band_permille=190,
            high_band_permille=50,
            acquisition_status_flags=0x0011,
        )

        # Encode and decode the fixed payload.
        decoded = decode_dsp_features(
            encode_dsp_features(original)
        )

        # Require complete field preservation.
        self.assertEqual(decoded, original)

    # Verify unknown payload schema revisions fail closed.
    def test_rejects_unknown_schema(self) -> None:

        # Build one valid encoded payload.
        payload = bytearray(
            encode_dsp_features(
                DspFeatures(
                    block_sequence=1,
                    sample_rate_hz=4000,
                    rms_mg=1,
                    peak_mg=2,
                    crest_factor_milli=2000,
                    dominant_frequency_centi_hz=6250,
                    dominant_peak_mg=2,
                    spectral_centroid_centi_hz=6250,
                    low_band_permille=1000,
                    mid_band_permille=0,
                    high_band_permille=0,
                    acquisition_status_flags=1,
                )
            )
        )

        # Replace schema v1 with an unsupported revision.
        payload[0] = 0x7F

        # Require explicit compatibility failure.
        with self.assertRaises(ValueError):

            # Attempt to decode unsupported semantics.
            decode_dsp_features(payload)


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
