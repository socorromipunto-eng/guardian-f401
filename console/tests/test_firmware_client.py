"""End-to-end tests for guardianctl M12 signed firmware upload."""

# Import hashlib for signed test images.
import hashlib

# Import threading for the real simulator server.
import threading

# Import unittest from the standard library.
import unittest

# Import shared package/security models.
from guardian_protocol import (
    FirmwareLifecycleState,
    FirmwareManifest,
    FirmwarePackage,
    FirmwareSignatureAlgorithm,
    SecurityRole,
    demo_sign_firmware_manifest,
)

# Import guardianctl client and transport components.
from guardianctl import (
    ClientConfig,
    GuardianClient,
    GuardianTcpTransport,
    RemoteDeviceError,
    SecurityClientConfig,
)

# Import the real simulator endpoint.
from guardian_sim import (
    GuardianDevice,
    GuardianTcpServer,
)

# Import public simulator demo credentials.
from guardian_sim.config import (
    DEFAULT_FIRMWARE_SIGNING_KEY_HEX,
    DEFAULT_SECURITY_PSK_HEX,
    SimulatorConfig,
)


# Verify a real M10 ADMIN session can carry M12 update commands.
class GuardianFirmwareClientTests(unittest.TestCase):
    """Exercise signed firmware upload through actual TCP request/response framing."""

    # Start one secure simulator before each test.
    def setUp(self) -> None:

        # Create explicit M10 secure-mode device state.
        self.device = GuardianDevice(
            SimulatorConfig(
                security_enabled=True,
            )
        )

        # Bind a real loopback server to an ephemeral port.
        self.server = GuardianTcpServer(
            ("127.0.0.1", 0),
            self.device,
        )

        # Run the server in a background daemon thread.
        self.server_thread = threading.Thread(
            target=self.server.serve_forever,
            daemon=True,
        )

        # Start accepting connections.
        self.server_thread.start()

        # Read the actual selected endpoint.
        host, port = self.server.server_address

        # Create one real synchronous TCP transport.
        self.transport = GuardianTcpTransport(
            ClientConfig(
                host=host,
                port=port,
                timeout_seconds=2.0,
            )
        )

        # Preserve exact public demo credentials.
        self.psk = bytes.fromhex(
            DEFAULT_SECURITY_PSK_HEX
        )

        # Preserve exact firmware signing key.
        self.signing_key = bytes.fromhex(
            DEFAULT_FIRMWARE_SIGNING_KEY_HEX
        )

        # Build a deterministic candidate image.
        self.image = (
            b"Guardian-M12-TCP-candidate\x00"
            * 64
        )

    # Stop the server after each test.
    def tearDown(self) -> None:

        # Stop request processing.
        self.server.shutdown()

        # Close the listener.
        self.server.server_close()

        # Join the background thread.
        self.server_thread.join(
            timeout=2.0
        )

    # Build one valid signed package.
    def _package(self) -> FirmwarePackage:
        """Return one valid demo-HMAC signed candidate package."""

        # Build unsigned metadata with placeholder signature bytes.
        unsigned = FirmwareManifest(
            signature_algorithm=FirmwareSignatureAlgorithm.DEMO_HMAC_SHA256,
            key_id=1,
            version_counter=12,
            firmware_major=0,
            firmware_minor=12,
            firmware_patch=0,
            image_size=len(self.image),
            image_sha256=hashlib.sha256(
                self.image
            ).digest(),
            signature=bytes(32),
        )

        # Sign with the simulator-only firmware key.
        manifest = demo_sign_firmware_manifest(
            unsigned,
            self.signing_key,
        )

        # Return complete immutable package.
        return FirmwarePackage(
            manifest=manifest,
            image=self.image,
        )

    # Verify ADMIN can stage, verify and activate one signed package.
    def test_admin_upload_reaches_pending_activation(self) -> None:

        # Create an ADMIN-authenticated client.
        client = GuardianClient(
            transport=self.transport,
            security_config=SecurityClientConfig(
                psk=self.psk,
                role=SecurityRole.ADMIN,
            ),
        )

        # Upload and explicitly activate the package.
        status = client.upload_firmware_package(
            self._package(),
            activate=True,
        )

        # Require pending activation state.
        self.assertEqual(
            status.state,
            FirmwareLifecycleState.PENDING_ACTIVATION,
        )

        # Require exact candidate counter.
        self.assertEqual(
            status.candidate_version_counter,
            12,
        )

        # Require the old rollback floor before boot confirmation.
        self.assertEqual(
            status.rollback_floor,
            11,
        )

    # Verify OPERATOR authenticates but cannot begin a firmware update.
    def test_operator_cannot_begin_firmware_update(self) -> None:

        # Create an OPERATOR-authenticated client.
        client = GuardianClient(
            transport=self.transport,
            security_config=SecurityClientConfig(
                psk=self.psk,
                role=SecurityRole.OPERATOR,
            ),
        )

        # Require role authorization failure.
        with self.assertRaises(
            RemoteDeviceError
        ) as context:

            # Attempt one ADMIN-classified upload.
            client.upload_firmware_package(
                self._package(),
            )

        # Require the M10 UNAUTHORIZED wire error.
        self.assertEqual(
            context.exception.error_code,
            0x09,
        )


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
