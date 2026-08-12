"""Integration tests for M7 GET_DSP_FEATURES and guardianctl client decoding."""

# Import threading so the simulator can run during the test.
import threading

# Import unittest from the Python standard library.
import unittest

# Import host client components.
from guardianctl import ClientConfig, GuardianClient, GuardianTcpTransport

# Import the real Guardian simulator endpoint.
from guardian_sim import GuardianDevice, GuardianTcpServer


# Verify the complete host-to-simulator DSP feature path.
class GuardianDspClientTests(unittest.TestCase):
    """Exercise GET_DSP_FEATURES over a real loopback TCP connection."""

    # Start one ephemeral simulator before every test.
    def setUp(self) -> None:

        # Create independent simulated device state.
        self.device = GuardianDevice()

        # Bind the simulator to an operating-system-selected loopback port.
        self.server = GuardianTcpServer(
            ("127.0.0.1", 0),
            self.device,
        )

        # Run the server request loop in a daemon thread.
        self.server_thread = threading.Thread(
            target=self.server.serve_forever,
            daemon=True,
        )

        # Start accepting host connections.
        self.server_thread.start()

        # Read the actual bound endpoint.
        host, port = self.server.server_address

        # Build bounded host configuration.
        config = ClientConfig(
            host=host,
            port=port,
            timeout_seconds=1.0,
        )

        # Create the real TCP transport.
        transport = GuardianTcpTransport(config)

        # Create the high-level Guardian client.
        self.client = GuardianClient(
            transport=transport
        )

    # Stop the simulator after every test.
    def tearDown(self) -> None:

        # Stop the server request loop.
        self.server.shutdown()

        # Close the listening socket.
        self.server.server_close()

        # Wait briefly for background cleanup.
        self.server_thread.join(timeout=2.0)

    # Verify the fixed M7 feature snapshot reaches the typed client.
    def test_reads_dsp_features(self) -> None:

        # Execute GET_DSP_FEATURES.
        features = self.client.dsp_features()

        # Require the reference simulator sample rate.
        self.assertEqual(
            features.sample_rate_hz,
            4000,
        )

        # Require deterministic synthetic RMS.
        self.assertEqual(
            features.rms_mg,
            42,
        )

        # Require energy shares to remain normalized.
        self.assertEqual(
            (
                features.low_band_permille
                + features.mid_band_permille
                + features.high_band_permille
            ),
            1000,
        )


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
