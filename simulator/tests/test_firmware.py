"""Tests for Guardian simulator M12 secure firmware lifecycle policy."""

# Import hashlib for exact candidate image digests.
import hashlib

# Import unittest from the standard library.
import unittest

# Import shared firmware lifecycle models/codecs.
from guardian_protocol import (
    Command,
    ErrorCode,
    FirmwareChunk,
    FirmwareLifecycleState,
    FirmwareManifest,
    FirmwareSignatureAlgorithm,
    MessageType,
    Frame,
    demo_sign_firmware_manifest,
    encode_firmware_chunk,
    encode_firmware_manifest,
)

# Import the simulator device and lifecycle model.
from guardian_sim import (
    GuardianDevice,
    SimulatorFirmwareLifecycle,
)

# Import simulator-only demo configuration.
from guardian_sim.config import (
    DEFAULT_FIRMWARE_SIGNING_KEY_HEX,
    SimulatorConfig,
)

# Import explicit lifecycle exceptions.
from guardian_sim.firmware import (
    SimulatorFirmwareRollbackError,
    SimulatorFirmwareSignatureError,
)


# Verify M12 simulator lifecycle invariants.
class SimulatorFirmwareLifecycleTests(unittest.TestCase):
    """Exercise signed staging, rollback, activation and boot outcome policy."""

    # Preserve the public simulator-only signing key.
    KEY = bytes.fromhex(
        DEFAULT_FIRMWARE_SIGNING_KEY_HEX
    )

    # Define one deterministic candidate image.
    IMAGE = (
        b"Guardian-F401-M12-candidate\x00"
        * 64
    )

    # Build one signed candidate manifest.
    def _manifest(
        self,
        version_counter: int = 12,
    ) -> FirmwareManifest:
        """Return one simulator-HMAC signed candidate manifest."""

        # Build signed image metadata with a placeholder signature.
        unsigned = FirmwareManifest(
            signature_algorithm=FirmwareSignatureAlgorithm.DEMO_HMAC_SHA256,
            key_id=1,
            version_counter=version_counter,
            firmware_major=0,
            firmware_minor=12,
            firmware_patch=0,
            image_size=len(self.IMAGE),
            image_sha256=hashlib.sha256(
                self.IMAGE
            ).digest(),
            signature=bytes(32),
        )

        # Return the test-only signed manifest.
        return demo_sign_firmware_manifest(
            unsigned,
            self.KEY,
        )

    # Stage a complete image directly through the lifecycle model.
    def _stage(
        self,
        lifecycle: SimulatorFirmwareLifecycle,
        manifest: FirmwareManifest,
    ) -> None:
        """Begin and write every image byte sequentially."""

        # Begin the candidate transfer.
        lifecycle.begin(
            encode_firmware_manifest(
                manifest
            )
        )

        # Start at image offset zero.
        offset = 0

        # Send bounded sequential chunks until complete.
        while offset < len(self.IMAGE):

            # Select the next bounded image slice.
            data = self.IMAGE[
                offset:
                offset + 192
            ]

            # Write the shared encoded chunk.
            lifecycle.write_chunk(
                encode_firmware_chunk(
                    FirmwareChunk(
                        offset=offset,
                        data=data,
                    )
                )
            )

            # Advance by exact staged bytes.
            offset += len(data)

    # Verify a signed candidate can be verified, activated and confirmed.
    def test_valid_candidate_confirms_and_advances_floor(self) -> None:

        # Create one fresh lifecycle at version eleven.
        lifecycle = SimulatorFirmwareLifecycle(
            signing_key=self.KEY,
            active_version_counter=11,
            max_image_size=256 * 1024,
        )

        # Stage the complete version-twelve image.
        self._stage(
            lifecycle,
            self._manifest(),
        )

        # Verify digest and signature.
        lifecycle.finalize(
            b"\x01"
        )

        # Require verified state.
        self.assertEqual(
            lifecycle.status().state,
            FirmwareLifecycleState.VERIFIED,
        )

        # Mark the image pending activation.
        lifecycle.activate(
            b"\x01"
        )

        # Require pending state before confirmation.
        self.assertEqual(
            lifecycle.status().state,
            FirmwareLifecycleState.PENDING_ACTIVATION,
        )

        # Simulate successful boot confirmation.
        lifecycle.confirm_boot()

        # Read final lifecycle status.
        status = lifecycle.status()

        # Require confirmed state.
        self.assertEqual(
            status.state,
            FirmwareLifecycleState.CONFIRMED,
        )

        # Require active monotonic version advancement.
        self.assertEqual(
            status.active_version_counter,
            12,
        )

        # Require rollback floor advancement only after confirmation.
        self.assertEqual(
            status.rollback_floor,
            12,
        )

    # Verify equal/older monotonic versions are blocked before staging.
    def test_rollback_candidate_is_rejected(self) -> None:

        # Create a lifecycle already confirmed at version eleven.
        lifecycle = SimulatorFirmwareLifecycle(
            signing_key=self.KEY,
            active_version_counter=11,
            max_image_size=256 * 1024,
        )

        # Require a version-eleven reinstall to fail rollback policy.
        with self.assertRaises(
            SimulatorFirmwareRollbackError
        ):

            # Attempt to begin a non-monotonic candidate.
            lifecycle.begin(
                encode_firmware_manifest(
                    self._manifest(
                        version_counter=11
                    )
                )
            )

    # Verify a modified signature cannot reach VERIFIED state.
    def test_tampered_signature_is_rejected(self) -> None:

        # Create a fresh lifecycle.
        lifecycle = SimulatorFirmwareLifecycle(
            signing_key=self.KEY,
            active_version_counter=11,
            max_image_size=256 * 1024,
        )

        # Build one valid signed manifest.
        manifest = self._manifest()

        # Corrupt one signature byte without changing signed metadata.
        signature = bytearray(
            manifest.signature
        )

        # Flip one authenticity bit.
        signature[-1] ^= 0x01

        # Build the corrupted manifest.
        tampered = FirmwareManifest(
            signature_algorithm=manifest.signature_algorithm,
            key_id=manifest.key_id,
            version_counter=manifest.version_counter,
            firmware_major=manifest.firmware_major,
            firmware_minor=manifest.firmware_minor,
            firmware_patch=manifest.firmware_patch,
            image_size=manifest.image_size,
            image_sha256=manifest.image_sha256,
            signature=bytes(
                signature
            ),
        )

        # Stage every exact image byte.
        self._stage(
            lifecycle,
            tampered,
        )

        # Require authenticity failure at finalize.
        with self.assertRaises(
            SimulatorFirmwareSignatureError
        ):

            # Attempt to verify the forged candidate.
            lifecycle.finalize(
                b"\x01"
            )

        # Require terminal failure state.
        self.assertEqual(
            lifecycle.status().state,
            FirmwareLifecycleState.FAILED,
        )

    # Verify secure mode rejects direct unwrapped firmware mutation commands.
    def test_secure_device_rejects_direct_firmware_begin(self) -> None:

        # Create one secure-mode device.
        device = GuardianDevice(
            SimulatorConfig(
                security_enabled=True,
            )
        )

        # Send a syntactically valid manifest without M10 wrapping.
        response = device.process_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.FIRMWARE_BEGIN,
                sequence=1,
                payload=encode_firmware_manifest(
                    self._manifest()
                ),
            )
        )

        # Require explicit outer authorization failure.
        self.assertEqual(
            response.message_type,
            MessageType.ERROR,
        )

        # Require the M10 authorization code.
        self.assertEqual(
            response.payload,
            bytes(
                (
                    int(
                        ErrorCode.UNAUTHORIZED
                    ),
                )
            ),
        )


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
