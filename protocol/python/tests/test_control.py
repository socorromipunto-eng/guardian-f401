"""Unit tests for Guardian M9 supervisory-control payload codecs."""

# Import unittest from the Python standard library.
import unittest

# Import the public M9 protocol API under test.
from guardian_protocol import (
    ControlAction,
    ControlCommand,
    ControlCommandResult,
    ControlState,
    ControlStatus,
    HealthState,
    decode_control_command,
    decode_control_command_result,
    decode_control_status,
    encode_control_command,
    encode_control_command_result,
    encode_control_status,
)


# Verify deterministic M9 binary serialization.
class ControlPayloadTests(unittest.TestCase):
    """Exercise M9 control request, response and status codecs."""

    # Verify one host action survives request encoding and decoding.
    def test_control_command_round_trip(self) -> None:

        # Build one ARM command.
        original = ControlCommand(
            action=ControlAction.ARM
        )

        # Encode and decode the fixed request.
        decoded = decode_control_command(
            encode_control_command(original)
        )

        # Require complete field preservation.
        self.assertEqual(decoded, original)

    # Verify one successful action result survives a round trip.
    def test_control_result_round_trip(self) -> None:

        # Build one safe ARM acknowledgement.
        original = ControlCommandResult(
            action=ControlAction.ARM,
            state=ControlState.ARMED,
            run_permit=False,
        )

        # Encode and decode the fixed response.
        decoded = decode_control_command_result(
            encode_control_command_result(
                original
            )
        )

        # Require complete field preservation.
        self.assertEqual(decoded, original)

    # Verify one complete status snapshot survives a round trip.
    def test_control_status_round_trip(self) -> None:

        # Build one representative degraded snapshot.
        original = ControlStatus(
            state=ControlState.DEGRADED,
            supervision_enabled=True,
            local_run_request=True,
            run_permit=True,
            interlock_closed=True,
            health_state=HealthState.WARNING,
            output_available=True,
            latched_faults=0,
            active_faults=0,
            health_score=620,
            anomaly_score=380,
            transition_count=12,
            fault_latch_count=2,
            last_transition_reason=0x86,
        )

        # Encode and decode the fixed 28-byte payload.
        decoded = decode_control_status(
            encode_control_status(original)
        )

        # Require complete field preservation.
        self.assertEqual(decoded, original)

    # Verify noncanonical wire booleans fail closed.
    def test_control_status_rejects_noncanonical_boolean(self) -> None:

        # Encode one valid safe status.
        payload = bytearray(
            encode_control_status(
                ControlStatus(
                    state=ControlState.DISABLED,
                    supervision_enabled=False,
                    local_run_request=False,
                    run_permit=False,
                    interlock_closed=True,
                    health_state=HealthState.READY,
                    output_available=True,
                    latched_faults=0,
                    active_faults=0,
                    health_score=1000,
                    anomaly_score=0,
                    transition_count=0,
                    fault_latch_count=0,
                    last_transition_reason=0,
                )
            )
        )

        # Replace the supervision boolean with an invalid value.
        payload[2] = 2

        # Require explicit semantic failure.
        with self.assertRaises(ValueError):

            # Attempt to decode ambiguous wire semantics.
            decode_control_status(payload)


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
