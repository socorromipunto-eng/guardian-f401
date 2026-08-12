"""Unit tests for Guardian Protocol v0.1 command-specific payload codecs."""

# Import unittest from the Python standard library.
import unittest

# Import the public payload API under test.
from guardian_protocol import (
    DeviceInfo,
    DeviceState,
    DeviceStatus,
    decode_device_info,
    decode_device_status,
    encode_device_info,
    encode_device_status,
)


# Verify deterministic command-payload serialization and validation.
class PayloadCodecTests(unittest.TestCase):
    """Exercise DEVICE_INFO and GET_STATUS payload codecs."""

    # Verify that device metadata survives one complete binary round trip.
    def test_device_info_round_trip(self) -> None:

        # Build one representative simulator identity.
        original = DeviceInfo(
            model="Guardian-F401-SIM",
            firmware_major=0,
            firmware_minor=2,
            firmware_patch=0,
            device_id=0xF4010001,
        )

        # Encode and immediately decode the command-specific payload.
        decoded = decode_device_info(encode_device_info(original))

        # Require complete field preservation.
        self.assertEqual(decoded, original)

    # Verify that runtime status survives one complete binary round trip.
    def test_device_status_round_trip(self) -> None:

        # Build one representative runtime snapshot.
        original = DeviceStatus(
            state=DeviceState.IDLE,
            uptime_seconds=1234,
            rx_frames=9,
            tx_frames=8,
            protocol_errors=2,
            last_error=0,
        )

        # Encode and immediately decode the fixed-width status payload.
        decoded = decode_device_status(encode_device_status(original))

        # Require complete field preservation.
        self.assertEqual(decoded, original)

    # Verify that unbounded model strings cannot enter a protocol payload.
    def test_device_info_rejects_oversized_model(self) -> None:

        # Build metadata whose UTF-8 model exceeds the published 32-byte bound.
        info = DeviceInfo(
            model="X" * 33,
            firmware_major=0,
            firmware_minor=2,
            firmware_patch=0,
            device_id=1,
        )

        # Require the encoder to reject the invalid model before wire construction.
        with self.assertRaises(ValueError):

            # Attempt to encode the invalid payload.
            encode_device_info(info)

    # Verify that unsupported payload schemas fail closed.
    def test_device_status_rejects_unknown_schema(self) -> None:

        # Encode one valid fixed-width runtime status.
        encoded = bytearray(
            encode_device_status(
                DeviceStatus(
                    state=DeviceState.IDLE,
                    uptime_seconds=0,
                    rx_frames=0,
                    tx_frames=0,
                    protocol_errors=0,
                    last_error=0,
                )
            )
        )

        # Replace the first payload byte with an unsupported future schema version.
        encoded[0] = 0x7F

        # Require the decoder to reject unknown semantics instead of guessing them.
        with self.assertRaises(ValueError):

            # Attempt to decode the unsupported payload schema.
            decode_device_status(encoded)


# Execute the tests when this file is run directly.
if __name__ == "__main__":

    # Start the standard-library unit test runner with verbose output.
    unittest.main(verbosity=2)
