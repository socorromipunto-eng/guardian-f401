"""End-to-end tests for guardianctl M12 firmware CLI commands."""

# Import hashlib for deterministic package signing metadata.
import hashlib

# Import io for terminal capture.
import io

# Import tempfile for one disposable .gfu package.
import tempfile

# Import threading for a real simulator server.
import threading

# Import unittest from the standard library.
import unittest

# Import stdout/stderr redirection.
from contextlib import redirect_stderr, redirect_stdout

# Import Path for the disposable package.
from pathlib import Path

# Import shared package/signature codecs.
from guardian_protocol import (
    FirmwareManifest,
    FirmwarePackage,
    FirmwareSignatureAlgorithm,
    demo_sign_firmware_manifest,
    encode_firmware_package,
)

# Import the public CLI entry point.
from guardianctl.cli import main

# Import the real simulator endpoint.
from guardian_sim import GuardianDevice, GuardianTcpServer

# Import public simulator demo credentials.
from guardian_sim.config import (
    DEFAULT_FIRMWARE_SIGNING_KEY_HEX,
    DEFAULT_SECURITY_PSK_HEX,
    SimulatorConfig,
)


# Verify M12 through the public guardianctl command grammar.
class GuardianFirmwareCliTests(unittest.TestCase):
    """Exercise firmware status and ADMIN upload through real TCP."""

    # Start one secure simulator before each test.
    def setUp(self) -> None:

        # Create secure-mode device state.
        self.device = GuardianDevice(
            SimulatorConfig(
                security_enabled=True,
            )
        )

        # Bind a real loopback TCP endpoint.
        self.server = GuardianTcpServer(
            ("127.0.0.1", 0),
            self.device,
        )

        # Run server in a background daemon thread.
        self.server_thread = threading.Thread(
            target=self.server.serve_forever,
            daemon=True,
        )

        # Start accepting connections.
        self.server_thread.start()

        # Preserve the selected TCP port.
        _, self.port = self.server.server_address

        # Create one disposable package directory.
        self.temporary = tempfile.TemporaryDirectory(
            prefix="guardian-m12-cli-"
        )

        # Define the package path.
        self.package_path = (
            Path(self.temporary.name)
            / "candidate.gfu"
        )

        # Build deterministic candidate image bytes.
        image = (
            b"Guardian-M12-CLI-candidate\x00"
            * 32
        )

        # Build unsigned metadata with the required demo signature width.
        unsigned = FirmwareManifest(
            signature_algorithm=FirmwareSignatureAlgorithm.DEMO_HMAC_SHA256,
            key_id=1,
            version_counter=12,
            firmware_major=0,
            firmware_minor=12,
            firmware_patch=0,
            image_size=len(image),
            image_sha256=hashlib.sha256(
                image
            ).digest(),
            signature=bytes(32),
        )

        # Sign using the simulator-only firmware signing key.
        manifest = demo_sign_firmware_manifest(
            unsigned,
            bytes.fromhex(
                DEFAULT_FIRMWARE_SIGNING_KEY_HEX
            ),
        )

        # Write one complete package.
        self.package_path.write_bytes(
            encode_firmware_package(
                FirmwarePackage(
                    manifest=manifest,
                    image=image,
                )
            )
        )

    # Stop the server and remove the disposable package.
    def tearDown(self) -> None:

        # Stop the TCP request loop.
        self.server.shutdown()

        # Close the listener.
        self.server.server_close()

        # Join background thread.
        self.server_thread.join(
            timeout=2.0
        )

        # Delete temporary package files.
        self.temporary.cleanup()

    # Execute one CLI call with captured terminal streams.
    def _run(
        self,
        arguments: list[str],
    ) -> tuple[int, str, str]:
        """Return exit code, stdout and stderr."""

        # Create terminal buffers.
        stdout_buffer = io.StringIO()
        stderr_buffer = io.StringIO()

        # Capture both output streams.
        with redirect_stdout(stdout_buffer), redirect_stderr(stderr_buffer):

            # Execute the public CLI.
            exit_code = main(
                arguments
            )

        # Return process-like results.
        return (
            exit_code,
            stdout_buffer.getvalue(),
            stderr_buffer.getvalue(),
        )

    # Verify an ADMIN package upload can reach pending activation.
    def test_admin_firmware_upload_activate(self) -> None:

        # Execute the complete public command.
        exit_code, stdout, stderr = self._run(
            [
                "--host",
                "127.0.0.1",
                "--port",
                str(self.port),
                "--psk-hex",
                DEFAULT_SECURITY_PSK_HEX,
                "--role",
                "admin",
                "firmware",
                "upload",
                str(self.package_path),
                "--activate",
            ]
        )

        # Require successful CLI completion.
        self.assertEqual(
            exit_code,
            0,
        )

        # Require no error output.
        self.assertEqual(
            stderr,
            "",
        )

        # Require visible pending state.
        self.assertIn(
            "PENDING_ACTIVATION",
            stdout,
        )

    # Verify public firmware status requires no PSK.
    def test_firmware_status_is_public(self) -> None:

        # Execute the read-only lifecycle status query.
        exit_code, stdout, stderr = self._run(
            [
                "--host",
                "127.0.0.1",
                "--port",
                str(self.port),
                "firmware",
                "status",
            ]
        )

        # Require successful status query.
        self.assertEqual(
            exit_code,
            0,
        )

        # Require no error output.
        self.assertEqual(
            stderr,
            "",
        )

        # Require initial idle lifecycle state.
        self.assertIn(
            "Firmware lifecycle: IDLE",
            stdout,
        )


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
