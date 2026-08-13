"""Deterministic Guardian M9 supervisory-control model for the simulator."""

# Import shared protocol enums and immutable models.
from guardian_protocol import (
    ControlAction,
    ControlCommandResult,
    ControlState,
    ControlStatus,
    HealthState,
    HealthStatus,
)


# Define firmware-matching fault bits.
FAULT_HEALTH_NOT_READY = 0x0001

# Define the severe M8 health fault bit.
FAULT_HEALTH_ALARM = 0x0002

# Define the local interlock fault bit.
FAULT_INTERLOCK_OPEN = 0x0004

# Define the output-application failure bit.
FAULT_OUTPUT_FAILURE = 0x0008

# Define the missing output-boundary fault bit.
FAULT_OUTPUT_UNAVAILABLE = 0x0010


# Mirror the portable C supervisory policy for software-only tests.
class SimulatorControlModel:
    """Runtime simulator mirror of Guardian M9 control semantics."""

    # Create one disabled safe control model.
    def __init__(self) -> None:

        # Start in the explicit safe disabled state.
        self.state = ControlState.DISABLED

        # Start with supervision disabled.
        self.supervision_enabled = False

        # Start without a local machine run request.
        self.local_run_request = False

        # Start with safe logical run permit.
        self.run_permit = False

        # Keep the software simulator interlock closed by default for concise demos.
        self.interlock_closed = True

        # Start with untrained M8 health.
        self.health_state = HealthState.UNTRAINED

        # Treat the simulator logical output boundary as available.
        self.output_available = True

        # Start without latched faults.
        self.latched_faults = 0

        # Start with neutral health and anomaly scores.
        self.health_score = 1000

        # Start without anomaly severity.
        self.anomaly_score = 0

        # Start without state transitions.
        self.transition_count = 0

        # Start without newly latched fault episodes.
        self.fault_latch_count = 0

        # Start without a transition reason.
        self.last_transition_reason = 0

    # Calculate live faults from current simulator input conditions.
    def _active_faults(self) -> int:
        """Return current live M9 fault bits."""

        # Start without live faults.
        faults = 0

        # Require a trained M8 health model.
        if self.health_state in (
            HealthState.UNTRAINED,
            HealthState.LEARNING,
        ):

            # Mark health as not ready.
            faults |= FAULT_HEALTH_NOT_READY

        # Treat M8 ALARM as a severe control fault.
        if self.health_state == HealthState.ALARM:

            # Mark the severe health fault.
            faults |= FAULT_HEALTH_ALARM

        # Require the local interlock to remain closed.
        if not self.interlock_closed:

            # Mark the local interlock fault.
            faults |= FAULT_INTERLOCK_OPEN

        # Require an output boundary before arming.
        if not self.output_available:

            # Mark unavailable output.
            faults |= FAULT_OUTPUT_UNAVAILABLE

        # Keep output failure live while latched.
        if self.latched_faults & FAULT_OUTPUT_FAILURE:

            # Preserve the output fault.
            faults |= FAULT_OUTPUT_FAILURE

        # Return current live faults.
        return faults

    # Publish one state transition only when state changes.
    def _transition(
        self,
        state: ControlState,
        reason: int,
    ) -> None:
        """Update state and transition diagnostics."""

        # Count only actual state changes.
        if self.state != state:

            # Publish the new state.
            self.state = state

            # Saturate the monotonic transition counter.
            self.transition_count = min(
                0xFFFFFFFF,
                self.transition_count + 1,
            )

            # Preserve the transition reason.
            self.last_transition_reason = reason & 0xFF

    # Latch one or more faults and force safe output.
    def _latch_fault(
        self,
        faults: int,
        reason: int,
    ) -> None:
        """Latch faults and force safe supervisory state."""

        # Determine whether this episode adds a new latched bit.
        new_faults = faults & ~self.latched_faults

        # Preserve every fault until explicit reset.
        self.latched_faults |= faults

        # Count newly latched episodes only.
        if new_faults:

            # Saturate the fault-latch counter.
            self.fault_latch_count = min(
                0xFFFFFFFF,
                self.fault_latch_count + 1,
            )

        # Disable supervision.
        self.supervision_enabled = False

        # Force the logical safe output.
        self.run_permit = False

        # Enter latched fault state.
        self._transition(
            ControlState.FAULT_LATCHED,
            reason,
        )

    # Evaluate automatic state changes after local or health input changes.
    def _evaluate(self) -> None:
        """Apply M9 automatic supervisory policy."""

        # Preserve latched faults until explicit reset.
        if self.latched_faults:

            # Keep output safe.
            self.run_permit = False

            # Keep supervision disabled.
            self.supervision_enabled = False

            # Keep latched state.
            self._transition(
                ControlState.FAULT_LATCHED,
                self.last_transition_reason,
            )

            # Return without automatic recovery.
            return

        # Keep output safe while supervision is disabled.
        if not self.supervision_enabled:

            # Publish the safe output.
            self.run_permit = False

            # Preserve disabled state.
            self._transition(
                ControlState.DISABLED,
                self.last_transition_reason,
            )

            # Return because disabled supervision never energizes output.
            return

        # Latch an open interlock while supervision is active.
        if not self.interlock_closed:

            # Force safe-off and require explicit reset.
            self._latch_fault(
                FAULT_INTERLOCK_OPEN,
                0x82,
            )

            # Return after fault handling.
            return

        # Latch health readiness loss after supervision was armed.
        if self.health_state in (
            HealthState.UNTRAINED,
            HealthState.LEARNING,
        ):

            # Force safe-off and require explicit reset.
            self._latch_fault(
                FAULT_HEALTH_NOT_READY,
                0x80,
            )

            # Return after fault handling.
            return

        # Latch severe M8 alarm.
        if self.health_state == HealthState.ALARM:

            # Force safe-off.
            self._latch_fault(
                FAULT_HEALTH_ALARM,
                0x81,
            )

            # Return after fault handling.
            return

        # Keep output safe while no local run request exists.
        if not self.local_run_request:

            # Publish safe output.
            self.run_permit = False

            # Wait in ARMED.
            self._transition(
                ControlState.ARMED,
                0x85,
            )

            # Return without run permit.
            return

        # Publish active logical run permit.
        self.run_permit = True

        # Degrade active state during M8 WARNING.
        if self.health_state == HealthState.WARNING:

            # Preserve permit but expose degraded operation.
            self._transition(
                ControlState.DEGRADED,
                0x86,
            )
        else:

            # Publish normal active operation.
            self._transition(
                ControlState.ACTIVE,
                0x84,
            )

    # Update M8 health input.
    def update_health(
        self,
        health: HealthStatus,
    ) -> None:
        """Consume one current M8 health snapshot."""

        # Preserve health lifecycle state.
        self.health_state = health.state

        # Preserve bounded health score.
        self.health_score = health.health_score

        # Preserve bounded anomaly severity.
        self.anomaly_score = health.anomaly_score

        # Re-evaluate automatic control policy.
        self._evaluate()

    # Update the local-only run request.
    def set_local_run_request(
        self,
        requested: bool,
    ) -> None:
        """Update local machine run request."""

        # Publish normalized local request.
        self.local_run_request = bool(requested)

        # Re-evaluate supervisory policy.
        self._evaluate()

    # Update the local safety interlock.
    def set_interlock(
        self,
        closed: bool,
    ) -> None:
        """Update local interlock state."""

        # Publish normalized interlock state.
        self.interlock_closed = bool(closed)

        # Re-evaluate supervisory policy.
        self._evaluate()

    # Execute one host action that cannot directly request motion.
    def action(
        self,
        action: ControlAction,
    ) -> ControlCommandResult:
        """Execute one safety-gated host supervisory action."""

        # Normalize action into the published enum.
        action = ControlAction(action)

        # DISARM is always safe and permitted.
        if action == ControlAction.DISARM:

            # Disable supervision.
            self.supervision_enabled = False

            # Force safe output.
            self.run_permit = False

            # Preserve fault-latched state when faults remain.
            if self.latched_faults:

                # Stay fault-latched.
                self._transition(
                    ControlState.FAULT_LATCHED,
                    int(action),
                )
            else:

                # Return to normal disabled state.
                self._transition(
                    ControlState.DISABLED,
                    int(action),
                )

            # Return normalized acknowledgement.
            return ControlCommandResult(
                action=action,
                state=self.state,
                run_permit=self.run_permit,
            )

        # CLEAR_FAULT requires complete safe recovery.
        if action == ControlAction.CLEAR_FAULT:

            # Require disabled supervision.
            if self.supervision_enabled:

                # Deny unsafe reset.
                raise RuntimeError(
                    "control action denied by safety policy"
                )

            # Require local run request removed.
            if self.local_run_request:

                # Deny reset while motion is still requested.
                raise RuntimeError(
                    "control action denied by safety policy"
                )

            # Require fully READY health.
            if self.health_state != HealthState.READY:

                # Deny reset during warning, alarm or learning.
                raise RuntimeError(
                    "control action denied by safety policy"
                )

            # Require closed interlock.
            if not self.interlock_closed:

                # Deny reset until local interlock is restored.
                raise RuntimeError(
                    "control action denied by safety policy"
                )

            # Require logical output boundary availability.
            if not self.output_available:

                # Deny reset without safe-output control.
                raise RuntimeError(
                    "control action denied by safety policy"
                )

            # Clear every latched fault.
            self.latched_faults = 0

            # Keep output safe.
            self.run_permit = False

            # Return to disabled state.
            self._transition(
                ControlState.DISABLED,
                int(action),
            )

            # Return normalized acknowledgement.
            return ControlCommandResult(
                action=action,
                state=self.state,
                run_permit=self.run_permit,
            )

        # ARM requires complete safe-entry conditions.
        if action == ControlAction.ARM:

            # Require no latched fault.
            if self.latched_faults:

                # Deny until explicit fault reset succeeds.
                raise RuntimeError(
                    "control action denied by safety policy"
                )

            # Require local run request absent at arm boundary.
            if self.local_run_request:

                # Prevent ARM from directly causing run permit.
                raise RuntimeError(
                    "control action denied by safety policy"
                )

            # Require fully READY health.
            if self.health_state != HealthState.READY:

                # Deny initial arm during warning/alarm/learning.
                raise RuntimeError(
                    "control action denied by safety policy"
                )

            # Require local interlock closed.
            if not self.interlock_closed:

                # Deny unsafe entry.
                raise RuntimeError(
                    "control action denied by safety policy"
                )

            # Require the output boundary.
            if not self.output_available:

                # Deny without safe output enforcement.
                raise RuntimeError(
                    "control action denied by safety policy"
                )

            # Arm without asserting run permit.
            self.supervision_enabled = True

            # Keep output safe.
            self.run_permit = False

            # Enter ARMED.
            self._transition(
                ControlState.ARMED,
                int(action),
            )

            # Return normalized acknowledgement.
            return ControlCommandResult(
                action=action,
                state=self.state,
                run_permit=self.run_permit,
            )

        # Reject undefined actions defensively.
        raise ValueError(
            f"unsupported control action: {action}"
        )

    # Return one immutable shared protocol status.
    def status(self) -> ControlStatus:
        """Return the current M9 supervisory-control snapshot."""

        # Return one shared protocol model.
        return ControlStatus(
            state=self.state,
            supervision_enabled=self.supervision_enabled,
            local_run_request=self.local_run_request,
            run_permit=self.run_permit,
            interlock_closed=self.interlock_closed,
            health_state=self.health_state,
            output_available=self.output_available,
            latched_faults=self.latched_faults,
            active_faults=self._active_faults(),
            health_score=self.health_score,
            anomaly_score=self.anomaly_score,
            transition_count=self.transition_count,
            fault_latch_count=self.fault_latch_count,
            last_transition_reason=self.last_transition_reason,
        )
