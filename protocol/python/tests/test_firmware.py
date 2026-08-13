"""Unit tests for Guardian M12 secure firmware lifecycle codecs."""

# Import hashlib for exact image-digest fixtures.
import hashlib

# Import unittest from the Python standard library.
import unittest

# Import the complete public M12 codec API.
from guardian_protocol import (
    FirmwareChunk,
    FirmwareLifecycleState,
    FirmwareManifest,
    FirmwarePackage,
    FirmwareSignatureAlgorithm,
    FirmwareStatus,
    FirmwareFailureCode,
    canonical_firmware_manifest,
    decode_firmware_chunk,
    decode_firmware_manifest,
    decode_firmware_package,
    decode_firmware_status,
    demo_sign_firmware_manifest,
    demo_verify_firmware_manifest,
    encode_firmware_chunk,
    encode_firmware_manifest,
    encode_firmware_package,
    encode_firmware_status,
)


# Verify signed manifest, chunk, status and package interoperability.
class FirmwareCodecTests(unittest.TestCase):
    """Exercise the complete M12 wire/package codec surface."""

    # Define one deterministic simulator-only signing key.
    KEY = bytes(range(32))

    # Define one deterministic fake firmware image.
    IMAGE = bytes(
        index & 0xFF
        for index in range(513)
    )

    # Build one signed demo manifest.
    def _manifest(self) -> FirmwareManifest:
        """Return one deterministic signed M12 manifest."""

        # Build the unsigned metadata with a placeholder demo signature.
        unsigned = FirmwareManifest(
            signature_algorithm=FirmwareSignatureAlgorithm.DEMO_HMAC_SHA256,
            key_id=7,
            version_counter=12,
            firmware_major=0,
            firmware_minor=12,
            firmware_patch=0,
            image_size=len(self.IMAGE),
            image_sha256=hashlib.sha256(
                self.IMAGE
            ).digest(),
            signature=bytes(32),
        )

        # Return the simulator/test-only signed form.
        return demo_sign_firmware_manifest(
            unsigned,
            self.KEY,
        )

    # Verify the canonical signed transcript remains exactly sixty-four bytes.
    def test_canonical_manifest_size_and_demo_signature(self) -> None:

        # Build one signed manifest.
        manifest = self._manifest()

        # Require the frozen canonical transcript width.
        self.assertEqual(
            len(
                canonical_firmware_manifest(
                    manifest
                )
            ),
            64,
        )

        # Require demo signature verification.
        self.assertTrue(
            demo_verify_firmware_manifest(
                manifest,
                self.KEY,
            )
        )

    # Verify manifest wire round-trip.
    def test_manifest_round_trip(self) -> None:

        # Build one signed manifest.
        manifest = self._manifest()

        # Require exact immutable round-trip equality.
        self.assertEqual(
            decode_firmware_manifest(
                encode_firmware_manifest(
                    manifest
                )
            ),
            manifest,
        )

    # Verify bounded chunk wire round-trip.
    def test_chunk_round_trip(self) -> None:

        # Build one representative chunk.
        chunk = FirmwareChunk(
            offset=192,
            data=self.IMAGE[
                192:
                384
            ],
        )

        # Require exact chunk preservation.
        self.assertEqual(
            decode_firmware_chunk(
                encode_firmware_chunk(
                    chunk
                )
            ),
            chunk,
        )

    # Verify public lifecycle status round-trip.
    def test_status_round_trip(self) -> None:

        # Build one pending-activation snapshot.
        status = FirmwareStatus(
            state=FirmwareLifecycleState.PENDING_ACTIVATION,
            failure=FirmwareFailureCode.NONE,
            signature_algorithm=int(
                FirmwareSignatureAlgorithm.DEMO_HMAC_SHA256
            ),
            active_version_counter=11,
            rollback_floor=11,
            candidate_version_counter=12,
            bytes_received=len(self.IMAGE),
            image_size=len(self.IMAGE),
            key_id=7,
            firmware_major=0,
            firmware_minor=12,
            firmware_patch=0,
        )

        # Require exact public diagnostics preservation.
        self.assertEqual(
            decode_firmware_status(
                encode_firmware_status(
                    status
                )
            ),
            status,
        )

    # Verify one complete .gfu package round-trip.
    def test_package_round_trip(self) -> None:

        # Build one complete signed package.
        package = FirmwarePackage(
            manifest=self._manifest(),
            image=self.IMAGE,
        )

        # Require exact package preservation.
        self.assertEqual(
            decode_firmware_package(
                encode_firmware_package(
                    package
                )
            ),
            package,
        )

    # Verify package image tampering fails before transport.
    def test_package_rejects_tampered_image(self) -> None:

        # Encode one valid package.
        encoded = bytearray(
            encode_firmware_package(
                FirmwarePackage(
                    manifest=self._manifest(),
                    image=self.IMAGE,
                )
            )
        )

        # Corrupt one image byte near EOF.
        encoded[-1] ^= 0x01

        # Require SHA-256 mismatch rejection.
        with self.assertRaises(ValueError):

            # Attempt to decode corrupted package.
            decode_firmware_package(
                encoded
            )


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
