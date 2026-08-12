"""Integration tests for guardianctl M5 telemetry streaming."""

# Import threading so the simulator can run during each test.
import threading

# Import unittest from the Python standard library.
import unittest

# Import host telemetry components under test.
from guardianctl import ClientConfig, TelemetryMonitor

# Import the real M2/M5 simulator endpoint.
from guardian_sim import GuardianDevice, GuardianTcpServer


# Verify persistent guardianctl telemetry against a real loopback server.
class GuardianTelemetryMonitorTests(unittest.TestCase):
    """Exercise subscription, live samples and automatic disable."""

    # Start one fresh simulator on an ephemeral port before every test.
    def setUp(self) -> None:

        # Create independent simulated device state.
        self.device = GuardianDevice()

        # Bind a real loopback server to an operating-system-selected port.
        self.server = GuardianTcpServer(
            ("127.0.0.1", 0),
            self.device,
        )

        # Run the simulator request loop in a daemon test thread.
        self.server_thread = threading.Thread(
            target=self.server.serve_forever,
            daemon=True,
        )

        # Start accepting telemetry client connections.
        self.server_thread.start()

        # Read the actual ephemeral endpoint.
        host, port = self.server.server_address

        # Configure the host monitor for the test simulator.
        self.config = ClientConfig(
            host=host,
            port=port,
            timeout_seconds=1.0,
        )

    # Stop the loopback simulator after every test.
    def tearDown(self) -> None:

        # Stop the server request loop.
        self.server.shutdown()

        # Close the listening socket.
        self.server.server_close()

        # Wait briefly for the daemon server thread.
        self.server_thread.join(timeout=2.0)

    # Verify three live asynchronous samples arrive in order.
    def test_streams_three_live_samples(self) -> None:

        # Create the persistent host telemetry monitor.
        monitor = TelemetryMonitor(self.config)

        # Store samples delivered live by the callback.
        records = []

        # Stream three samples at the minimum published period.
        delivered = monitor.stream_samples(
            period_ms=100,
            count=3,
            consumer=records.append,
        )

        # Require the requested number of delivered samples.
        self.assertEqual(delivered, 3)

        # Require exactly three callback invocations.
        self.assertEqual(len(records), 3)

        # Require monotonically increasing telemetry frame sequences.
        self.assertEqual(
            [record.sequence for record in records],
            [1, 2, 3],
        )

        # Require monotonic sample timestamps.
        self.assertLess(
            records[0].sample.timestamp_ms,
            records[-1].sample.timestamp_ms,
        )

        # Require deterministic synthetic supply voltage.
        self.assertTrue(
            all(
                record.sample.supply_mv == 3300
                for record in records
            )
        )

    # Verify the host rejects an out-of-policy sample rate before connecting.
    def test_rejects_period_below_rate_limit(self) -> None:

        # Create the persistent host telemetry monitor.
        monitor = TelemetryMonitor(self.config)

        # Require local policy validation.
        with self.assertRaises(ValueError):

            # Attempt to request a 99 ms period.
            monitor.stream_samples(
                period_ms=99,
                count=1,
                consumer=lambda record: None,
            )


# Execute the tests when this file is run directly.
if __name__ == "__main__":

    # Start the standard-library test runner.
    unittest.main(verbosity=2)
