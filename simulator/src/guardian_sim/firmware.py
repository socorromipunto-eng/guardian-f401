"""Guardian M12 in-memory secure firmware lifecycle simulator."""

# Import hashlib for exact staged image SHA-256 verification.
import hashlib

# Import shared M12 lifecycle models and codecs.
from guardian_protocol import (
    FirmwareFailureCode,
    FirmwareLifecycleState,
    FirmwareManifest,
    FirmwareSignatureAlgorithm,
    FirmwareStatus,
    decode_firmware_action,
    decode_firmware_chunk,
    decode_firmware_manifest,
    demo_verify_firmware_manifest,
)


# Represent anti-rollback rejection distinctly for wire error mapping.
class SimulatorFirmwareRollbackError(Exception):
    """Raised when a candidate version violates monotonic rollback policy."""


# Represent image-signature failure distinctly for wire error mapping.
class SimulatorFirmwareSignatureError(Exception):
    """Raised when a staged firmware signature cannot be verified."""


# Represent staging/hash/activation backend failure distinctly.
class SimulatorFirmwareUpdateError(Exception):
    """Raised when a firmware lifecycle operation fails internally."""


# Implement the M12 lifecycle independently from TCP packetization.
class SimulatorFirmwareLifecycle:
    """Mirror M12 staging, verification, activation and rollback policy."""

    # Create one in-memory firmware lifecycle backend.
    def __init__(
        self,
        signing_key: bytes,
        active_version_counter: int,
        max_image_size: int,
    ) -> None:

        # Preserve the exact simulator-only signing key.
        self._signing_key = bytes(
            signing_key
        )

        # Require the frozen demonstration key width.
        if len(self._signing_key) != 32:

            # Reject invalid simulator configuration.
            raise ValueError(
                "firmware signing key must contain exactly 32 bytes"
            )

        # Preserve confirmed active monotonic version.
        self._active_version_counter = active_version_counter

        # Start rollback floor at the confirmed active version.
        self._rollback_floor = active_version_counter

        # Preserve bounded in-memory staging capacity.
        self._max_image_size = max_image_size

        # Start without candidate metadata.
        self._manifest: FirmwareManifest | None = None

        # Start without candidate image storage.
        self._image = bytearray()

        # Start without accepted bytes.
        self._bytes_received = 0

        # Start idle.
        self._state = FirmwareLifecycleState.IDLE

        # Start without a failure.
        self._failure = FirmwareFailureCode.NONE

    # Record one terminal candidate failure.
    def _fail(
        self,
        failure: FirmwareFailureCode,
    ) -> None:
        """Move the current candidate into FAILED state."""

        # Publish stable failure diagnostics.
        self._failure = failure

        # Publish terminal candidate failure state.
        self._state = FirmwareLifecycleState.FAILED

    # Begin one new signed candidate transfer.
    def begin(
        self,
        payload: bytes,
    ) -> None:
        """Decode manifest, enforce rollback policy and erase staging storage."""

        # Decode and validate shared manifest semantics.
        manifest = decode_firmware_manifest(
            payload
        )

        # The simulator supports only its explicitly test-only signing backend.
        if manifest.signature_algorithm != FirmwareSignatureAlgorithm.DEMO_HMAC_SHA256:

            # Record unsupported image-authentication backend.
            self._fail(
                FirmwareFailureCode.SIGNATURE_INVALID
            )

            # Reject unsupported simulator algorithm.
            raise SimulatorFirmwareSignatureError(
                "simulator accepts only the demo HMAC firmware signature algorithm"
            )

        # Require candidate version above both active version and rollback floor.
        if manifest.version_counter <= max(
            self._active_version_counter,
            self._rollback_floor,
        ):

            # Record anti-rollback rejection.
            self._fail(
                FirmwareFailureCode.ROLLBACK_BLOCKED
            )

            # Reject downgrade/reinstall.
            raise SimulatorFirmwareRollbackError(
                "candidate firmware version counter is not monotonic"
            )

        # Require candidate to fit bounded simulator staging storage.
        if manifest.image_size > self._max_image_size:

            # Record invalid candidate size.
            self._fail(
                FirmwareFailureCode.INVALID_PAYLOAD
            )

            # Reject oversized candidate before allocation.
            raise ValueError(
                "candidate firmware image exceeds simulator staging capacity"
            )

        # Publish candidate signed metadata.
        self._manifest = manifest

        # Allocate exact zeroed candidate staging storage.
        self._image = bytearray(
            manifest.image_size
        )

        # Reset sequential transfer ownership.
        self._bytes_received = 0

        # Clear prior failure.
        self._failure = FirmwareFailureCode.NONE

        # Enter receiving state.
        self._state = FirmwareLifecycleState.RECEIVING

    # Write one exact sequential chunk.
    def write_chunk(
        self,
        payload: bytes,
    ) -> None:
        """Write one shared FIRMWARE_CHUNK payload."""

        # Require an active transfer.
        if (
            self._state != FirmwareLifecycleState.RECEIVING
            or self._manifest is None
        ):

            # Reject invalid lifecycle state.
            raise SimulatorFirmwareUpdateError(
                "firmware chunk received outside RECEIVING state"
            )

        # Decode shared chunk semantics.
        chunk = decode_firmware_chunk(
            payload
        )

        # Require exact sequential offset.
        if chunk.offset != self._bytes_received:

            # Record out-of-order failure.
            self._fail(
                FirmwareFailureCode.OUT_OF_ORDER
            )

            # Reject gaps and overlap.
            raise ValueError(
                "firmware chunk offset is not the next sequential offset"
            )

        # Calculate the exclusive destination end.
        end = (
            chunk.offset
            + len(
                chunk.data
            )
        )

        # Require chunk within signed image size.
        if end > self._manifest.image_size:

            # Record invalid candidate bytes.
            self._fail(
                FirmwareFailureCode.INVALID_PAYLOAD
            )

            # Reject write beyond signed image size.
            raise ValueError(
                "firmware chunk exceeds signed image size"
            )

        # Copy exact candidate bytes into staging storage.
        self._image[
            chunk.offset:
            end
        ] = chunk.data

        # Advance sequential transfer ownership.
        self._bytes_received = end

    # Verify candidate digest and signature.
    def finalize(
        self,
        payload: bytes,
    ) -> None:
        """Verify one completely staged candidate image."""

        # Validate schema-only action payload.
        decode_firmware_action(
            payload
        )

        # Require a complete receiving candidate.
        if (
            self._state != FirmwareLifecycleState.RECEIVING
            or self._manifest is None
        ):

            # Reject invalid lifecycle state.
            raise SimulatorFirmwareUpdateError(
                "firmware finalize received outside RECEIVING state"
            )

        # Require every signed image byte.
        if self._bytes_received != self._manifest.image_size:

            # Record incomplete image failure.
            self._fail(
                FirmwareFailureCode.INVALID_PAYLOAD
            )

            # Reject incomplete candidate.
            raise ValueError(
                "firmware image is incomplete"
            )

        # Calculate exact staged image digest.
        digest = hashlib.sha256(
            self._image
        ).digest()

        # Require digest equality.
        if digest != self._manifest.image_sha256:

            # Record image-integrity failure.
            self._fail(
                FirmwareFailureCode.HASH_MISMATCH
            )

            # Reject corrupted staged image.
            raise SimulatorFirmwareUpdateError(
                "firmware image SHA-256 mismatch"
            )

        # Verify the signed manifest with the simulator-only demonstration key.
        if not demo_verify_firmware_manifest(
            self._manifest,
            self._signing_key,
        ):

            # Record authenticity failure.
            self._fail(
                FirmwareFailureCode.SIGNATURE_INVALID
            )

            # Reject forged/incorrect signature.
            raise SimulatorFirmwareSignatureError(
                "firmware signature verification failed"
            )

        # Re-check rollback policy at verification time.
        if self._manifest.version_counter <= max(
            self._active_version_counter,
            self._rollback_floor,
        ):

            # Record anti-rollback rejection.
            self._fail(
                FirmwareFailureCode.ROLLBACK_BLOCKED
            )

            # Reject stale candidate.
            raise SimulatorFirmwareRollbackError(
                "candidate firmware version no longer satisfies rollback policy"
            )

        # Publish verified candidate state.
        self._state = FirmwareLifecycleState.VERIFIED

        # Clear failure diagnostics.
        self._failure = FirmwareFailureCode.NONE

    # Mark one verified candidate pending activation.
    def activate(
        self,
        payload: bytes,
    ) -> None:
        """Mark one verified candidate for simulated next boot."""

        # Validate schema-only action payload.
        decode_firmware_action(
            payload
        )

        # Require a verified candidate.
        if (
            self._state != FirmwareLifecycleState.VERIFIED
            or self._manifest is None
        ):

            # Reject activation before verification.
            raise SimulatorFirmwareUpdateError(
                "firmware activation requires VERIFIED state"
            )

        # Publish pending activation state.
        self._state = FirmwareLifecycleState.PENDING_ACTIVATION

        # Clear failure diagnostics.
        self._failure = FirmwareFailureCode.NONE

    # Simulate successful boot confirmation.
    def confirm_boot(self) -> None:
        """Confirm the pending image and advance rollback floor."""

        # Require one pending candidate.
        if (
            self._state != FirmwareLifecycleState.PENDING_ACTIVATION
            or self._manifest is None
        ):

            # Reject invalid simulation state.
            raise SimulatorFirmwareUpdateError(
                "no pending firmware image to confirm"
            )

        # Publish new active monotonic version.
        self._active_version_counter = (
            self._manifest.version_counter
        )

        # Advance anti-rollback floor only after confirmation.
        self._rollback_floor = (
            self._manifest.version_counter
        )

        # Publish confirmed state.
        self._state = FirmwareLifecycleState.CONFIRMED

        # Clear failure diagnostics.
        self._failure = FirmwareFailureCode.NONE

    # Simulate a failed pending boot and safe rollback.
    def report_boot_failure(self) -> None:
        """Leave the previous confirmed version authoritative."""

        # Require one pending candidate.
        if self._state != FirmwareLifecycleState.PENDING_ACTIVATION:

            # Reject invalid simulation state.
            raise SimulatorFirmwareUpdateError(
                "no pending firmware image to roll back"
            )

        # Preserve active version and rollback floor unchanged.
        self._state = FirmwareLifecycleState.ROLLED_BACK

        # Rollback itself is a safe expected lifecycle outcome.
        self._failure = FirmwareFailureCode.NONE

    # Return public lifecycle diagnostics.
    def status(self) -> FirmwareStatus:
        """Return one immutable M12 firmware lifecycle snapshot."""

        # Preserve candidate metadata when present.
        manifest = self._manifest

        # Return public diagnostics without signing-key material.
        return FirmwareStatus(
            state=self._state,
            failure=self._failure,
            signature_algorithm=(
                int(
                    manifest.signature_algorithm
                )
                if manifest is not None
                else 0
            ),
            active_version_counter=self._active_version_counter,
            rollback_floor=self._rollback_floor,
            candidate_version_counter=(
                manifest.version_counter
                if manifest is not None
                else 0
            ),
            bytes_received=self._bytes_received,
            image_size=(
                manifest.image_size
                if manifest is not None
                else 0
            ),
            key_id=(
                manifest.key_id
                if manifest is not None
                else 0
            ),
            firmware_major=(
                manifest.firmware_major
                if manifest is not None
                else 0
            ),
            firmware_minor=(
                manifest.firmware_minor
                if manifest is not None
                else 0
            ),
            firmware_patch=(
                manifest.firmware_patch
                if manifest is not None
                else 0
            ),
        )
