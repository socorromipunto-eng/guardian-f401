"""Unit tests for Guardian simulator M8 baseline and health commands."""

# Import unittest from the Python standard library.
import unittest

# Import shared protocol command and payload models.
from guardian_protocol import (
    BaselineAction,
    BaselineControl,
    Command,
    Frame,
    HealthState,
    MessageType,
    decode_baseline_control,
    decode_health_status,
    encode_baseline_control,
)

# Import the transport-independent simulated device.
from guardian_sim import GuardianDevice


# Verify M8 behavior without TCP packetization.
class GuardianDeviceHealthTests(unittest.TestCase):
    """Exercise simulator baseline and health semantics."""

    # Create one fresh device before every test.
    def setUp(self) -> None:

        # Avoid health-model state leakage between tests.
        self.device = GuardianDevice()

    # Verify baseline START fast-forwards a deterministic healthy simulator baseline.
    def test_baseline_start_reaches_ready(self) -> None:

        # Build one explicit 16-sample baseline request.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.BASELINE_CONTROL,
            sequence=80,
            payload=encode_baseline_control(
                BaselineControl(
                    action=BaselineAction.START,
                    target_samples=16,
                )
            ),
        )

        # Execute the baseline command.
        response = self.device.process_frame(
            request
        )

        # Require successful normalized control response.
        self.assertEqual(
            response.message_type,
            MessageType.RESPONSE,
        )

        # Decode and verify the acknowledged baseline target.
        normalized = decode_baseline_control(
            response.payload
        )

        # Require the exact requested target.
        self.assertEqual(
            normalized.target_samples,
            16,
        )

        # Query the resulting health status.
        health_response = self.device.process_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.GET_HEALTH_STATUS,
                sequence=81,
            )
        )

        # Decode the shared M8 status.
        health = decode_health_status(
            health_response.payload
        )

        # Require simulator baseline completion.
        self.assertEqual(
            health.state,
            HealthState.READY,
        )

        # Require all requested samples to be accepted.
        self.assertEqual(
            health.baseline_samples,
            16,
        )

        # Require neutral trained health before anomalies.
        self.assertEqual(
            health.health_score,
            1000,
        )

    # Verify RESET erases the runtime baseline.
    def test_baseline_reset_returns_untrained(self) -> None:

        # Start a complete simulator baseline first.
        self.device.process_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.BASELINE_CONTROL,
                sequence=82,
                payload=encode_baseline_control(
                    BaselineControl(
                        action=BaselineAction.START,
                        target_samples=16,
                    )
                ),
            )
        )

        # Reset the runtime model.
        response = self.device.process_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.BASELINE_CONTROL,
                sequence=83,
                payload=encode_baseline_control(
                    BaselineControl(
                        action=BaselineAction.RESET,
                        target_samples=0,
                    )
                ),
            )
        )

        # Require successful reset acknowledgement.
        self.assertEqual(
            decode_baseline_control(
                response.payload
            ).action,
            BaselineAction.RESET,
        )

        # Query health after reset.
        health = decode_health_status(
            self.device.process_frame(
                Frame(
                    message_type=MessageType.REQUEST,
                    command=Command.GET_HEALTH_STATUS,
                    sequence=84,
                )
            ).payload
        )

        # Require explicit UNTRAINED state.
        self.assertEqual(
            health.state,
            HealthState.UNTRAINED,
        )

        # Require baseline samples to be erased.
        self.assertEqual(
            health.baseline_samples,
            0,
        )


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
