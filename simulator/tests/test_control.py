"""Unit tests for Guardian simulator M9 supervisory-control behavior."""

# Import unittest from the Python standard library.
import unittest

# Import shared protocol command and state models.
from guardian_protocol import (
    BaselineAction,
    BaselineControl,
    Command,
    ControlAction,
    ControlCommand,
    ControlState,
    ErrorCode,
    Frame,
    HealthState,
    HealthStatus,
    MessageType,
    decode_control_command_result,
    decode_control_status,
    encode_baseline_control,
    encode_control_command,
)

# Import transport-independent simulator models.
from guardian_sim import (
    GuardianDevice,
    SimulatorControlModel,
)


# Verify protocol-facing M9 simulator commands.
class GuardianDeviceControlTests(unittest.TestCase):
    """Exercise M9 control semantics without TCP packetization."""

    # Create one fresh device before every test.
    def setUp(self) -> None:

        # Avoid control state leakage between tests.
        self.device = GuardianDevice()

    # Start one complete healthy M8 baseline.
    def _start_baseline(self) -> None:

        # Build one explicit 16-sample baseline request.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.BASELINE_CONTROL,
            sequence=100,
            payload=encode_baseline_control(
                BaselineControl(
                    action=BaselineAction.START,
                    target_samples=16,
                )
            ),
        )

        # Execute the baseline request.
        response = self.device.process_frame(
            request
        )

        # Require successful baseline response.
        self.assertEqual(
            response.message_type,
            MessageType.RESPONSE,
        )

    # Verify ARM is denied before M8 baseline readiness.
    def test_arm_before_baseline_returns_busy(self) -> None:

        # Build one M9 ARM command.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.CONTROL_COMMAND,
            sequence=101,
            payload=encode_control_command(
                ControlCommand(
                    action=ControlAction.ARM
                )
            ),
        )

        # Execute the unsafe ARM attempt.
        response = self.device.process_frame(
            request
        )

        # Require ERROR semantics.
        self.assertEqual(
            response.message_type,
            MessageType.ERROR,
        )

        # Require BUSY safety-policy denial.
        self.assertEqual(
            response.payload,
            bytes((int(ErrorCode.BUSY),)),
        )

    # Verify baseline followed by ARM never asserts run permit remotely.
    def test_arm_after_baseline_is_safe_armed(self) -> None:

        # Complete the software-only healthy baseline.
        self._start_baseline()

        # Execute ARM.
        response = self.device.process_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.CONTROL_COMMAND,
                sequence=102,
                payload=encode_control_command(
                    ControlCommand(
                        action=ControlAction.ARM
                    )
                ),
            )
        )

        # Decode the normalized result.
        result = decode_control_command_result(
            response.payload
        )

        # Require ARMED state.
        self.assertEqual(
            result.state,
            ControlState.ARMED,
        )

        # Require host ARM not to assert run permit.
        self.assertFalse(
            result.run_permit
        )

        # Query the current control snapshot.
        status = decode_control_status(
            self.device.process_frame(
                Frame(
                    message_type=MessageType.REQUEST,
                    command=Command.GET_CONTROL_STATUS,
                    sequence=103,
                )
            ).payload
        )

        # Require the simulator local interlock to be closed.
        self.assertTrue(
            status.interlock_closed
        )

        # Require M8 health to be ready.
        self.assertEqual(
            status.health_state,
            HealthState.READY,
        )

    # Verify the local-only simulator run request can enter ACTIVE after ARM.
    def test_local_run_request_enters_active(self) -> None:

        # Complete baseline and arm supervision.
        self._start_baseline()

        # Execute ARM.
        self.device.process_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.CONTROL_COMMAND,
                sequence=104,
                payload=encode_control_command(
                    ControlCommand(
                        action=ControlAction.ARM
                    )
                ),
            )
        )

        # Assert local-only run request outside the host protocol.
        self.device.set_local_run_request(
            True
        )

        # Read control status.
        status = decode_control_status(
            self.device.process_frame(
                Frame(
                    message_type=MessageType.REQUEST,
                    command=Command.GET_CONTROL_STATUS,
                    sequence=105,
                )
            ).payload
        )

        # Require ACTIVE state.
        self.assertEqual(
            status.state,
            ControlState.ACTIVE,
        )

        # Require logical run permit.
        self.assertTrue(
            status.run_permit
        )

    # Verify baseline reset while armed fails safe into a latched fault.
    def test_baseline_reset_while_armed_latches_fault(self) -> None:

        # Complete baseline.
        self._start_baseline()

        # Arm supervision.
        self.device.process_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.CONTROL_COMMAND,
                sequence=106,
                payload=encode_control_command(
                    ControlCommand(
                        action=ControlAction.ARM
                    )
                ),
            )
        )

        # Reset M8 baseline while M9 is armed.
        self.device.process_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.BASELINE_CONTROL,
                sequence=107,
                payload=encode_baseline_control(
                    BaselineControl(
                        action=BaselineAction.RESET,
                        target_samples=0,
                    )
                ),
            )
        )

        # Read M9 status after readiness loss.
        status = decode_control_status(
            self.device.process_frame(
                Frame(
                    message_type=MessageType.REQUEST,
                    command=Command.GET_CONTROL_STATUS,
                    sequence=108,
                )
            ).payload
        )

        # Require fault-latched state.
        self.assertEqual(
            status.state,
            ControlState.FAULT_LATCHED,
        )

        # Require health-not-ready latch bit.
        self.assertTrue(
            status.latched_faults & 0x0001
        )

        # Require safe-off.
        self.assertFalse(
            status.run_permit
        )


# Verify the standalone simulator control model warning/alarm policy.
class SimulatorControlPolicyTests(unittest.TestCase):
    """Exercise warning, alarm and recovery without protocol plumbing."""

    # Build one compact M8 health snapshot.
    @staticmethod
    def _health(
        state: HealthState,
        health_score: int,
        anomaly_score: int,
    ) -> HealthStatus:
        """Return one deterministic M8 status."""

        # Return the shared health model.
        return HealthStatus(
            state=state,
            baseline_samples=16,
            baseline_target=16,
            anomaly_score=anomaly_score,
            health_score=health_score,
            max_deviation_milli=0,
            dominant_feature=0xFF,
            consecutive_anomalous=0,
            quality_flags=0,
            block_sequence=1,
            current_rms_mg=40,
            current_crest_factor_milli=1450,
            current_dominant_frequency_centi_hz=25000,
            baseline_rms_mean_mg=40,
            baseline_rms_std_mg=5,
            exceeded_feature_mask=0,
            rejected_inputs=0,
        )

    # Verify WARNING remains permissive but ALARM latches safe-off.
    def test_warning_then_alarm(self) -> None:

        # Create one fresh simulator control model.
        control = SimulatorControlModel()

        # Publish fully READY M8 health.
        control.update_health(
            self._health(
                HealthState.READY,
                1000,
                0,
            )
        )

        # Arm supervision.
        control.action(
            ControlAction.ARM
        )

        # Assert local-only run request.
        control.set_local_run_request(
            True
        )

        # Require ACTIVE normal state.
        self.assertEqual(
            control.status().state,
            ControlState.ACTIVE,
        )

        # Publish warning-level health.
        control.update_health(
            self._health(
                HealthState.WARNING,
                600,
                400,
            )
        )

        # Require DEGRADED state while retaining permit.
        self.assertEqual(
            control.status().state,
            ControlState.DEGRADED,
        )

        # Require permit retained under warning policy.
        self.assertTrue(
            control.status().run_permit
        )

        # Publish severe M8 ALARM.
        control.update_health(
            self._health(
                HealthState.ALARM,
                0,
                1000,
            )
        )

        # Require fail-safe fault latch.
        self.assertEqual(
            control.status().state,
            ControlState.FAULT_LATCHED,
        )

        # Require safe-off.
        self.assertFalse(
            control.status().run_permit
        )


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
