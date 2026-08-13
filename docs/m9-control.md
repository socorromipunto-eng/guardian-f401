# M9 — Supervisory Control and Fault Policy

## Objective

M9 converts the advisory M8 health model into a conservative supervisory-control policy.

```text
M8 health state
      |
      +-------------------+
      |                   |
local interlock      local run request
      |                   |
      +---------+---------+
                |
                v
        M9 state machine
                |
                +--> fault latching
                +--> transition diagnostics
                +--> degraded policy
                `--> safe logical run permit
                         |
                         v
              board/application output boundary
```

## Core Safety Rule

A host command cannot directly request machine motion in M9.

The protocol exposes:

```text
ARM
DISARM
CLEAR_FAULT
```

It does not expose:

```text
RUN
START
FORCE_OUTPUT
```

The actual run request is local-only firmware/application input.

This is deliberate because authentication and authorization are scheduled for M10.

## Supervisory States

```text
DISABLED
   |
   | ARM after all safe-entry checks
   v
ARMED
   |
   | local-only run request
   v
ACTIVE
   |
   | M8 WARNING
   v
DEGRADED
   |
   | M8 ALARM / interlock fault / readiness loss
   v
FAULT_LATCHED
```

`DISABLED` and `ARMED` always publish `run_permit = 0`.

`ACTIVE` publishes `run_permit = 1`.

`DEGRADED` keeps the permit active under the M9 reference policy while M8 remains at WARNING.

`FAULT_LATCHED` forces safe-off.

## ARM Policy

ARM is accepted only when all conditions hold:

```text
M8 health == READY
local run request == false
local interlock == closed
safe-output adapter installed
no latched fault
```

This prevents a remote ARM request from producing an immediate active output.

## Local Run Request

The physical/application machine request enters through:

```text
guardian_firmware_app_set_local_run_request()
```

The host protocol does not provide this operation in M9.

A local request can produce `run_permit = 1` only after successful ARM and all automatic safety conditions remain valid.

## Local Interlock

The local safety/interlock state enters through:

```text
guardian_firmware_app_set_interlock_closed()
```

Startup defaults to:

```text
interlock open
run request false
run permit false
```

Board integration must explicitly prove the interlock closed.

Opening the interlock while supervision is armed or active immediately latches a fault and requests safe-off.

## Health Policy

M8 states are interpreted as:

```text
UNTRAINED -> cannot arm
LEARNING  -> cannot arm
READY     -> normal supervisory operation
WARNING   -> DEGRADED while active
ALARM     -> immediate latched safe-off
```

If a new baseline is started or reset while M9 is armed, M8 becomes unready and M9 fails safe into `FAULT_LATCHED`.

## Fault Latching

Latched fault bits:

```text
HEALTH_NOT_READY
HEALTH_ALARM
INTERLOCK_OPEN
OUTPUT_FAILURE
```

`OUTPUT_UNAVAILABLE` is a live safe-entry fault and prevents ARM.

Faults do not clear automatically.

`CLEAR_FAULT` requires:

```text
supervision disabled
local run request false
M8 health == READY
interlock closed
safe-output adapter available
safe-off output application successful
```

The result remains `DISABLED` and `run_permit = 0`.

A separate ARM is required afterward.

## Safe Output Boundary

The portable control module does not choose a physical STM32 GPIO.

It uses:

```text
guardian_control_output_fn
```

The callback must apply the logical run permit to the board/application integration layer.

The reference `guardian_firmware_app.c` stores the requested permit in an application-visible safe-output shadow:

```text
guardian_firmware_app_run_permit()
```

A final board design must connect that logical permit to an appropriate hardware driver, relay interface, contactor-control circuit or other application-specific output implementation.

The M9 software output is not a certified safety function.

## Existing Device State Compatibility

M9 maps active supervisory state into the older M4/M5 device state:

```text
ARMED/DISABLED -> IDLE
ACTIVE         -> RUNNING
DEGRADED       -> DEGRADED
FAULT_LATCHED  -> FAULT
```

This keeps GET_STATUS and asynchronous telemetry compatible.

## Host Commands

Software simulator demo:

```text
python tools/run_simulator.py
```

Create a healthy baseline:

```text
python tools/guardianctl.py baseline start --samples 64
```

Arm supervision:

```text
python tools/guardianctl.py control arm
```

Inspect supervisory state:

```text
python tools/guardianctl.py control status
python tools/guardianctl.py --json control status
```

Force safe disabled state:

```text
python tools/guardianctl.py control disarm
```

Clear a recovered latched fault:

```text
python tools/guardianctl.py control clear-fault
```

## Security Boundary

M9 protocol commands are still unauthenticated.

The absence of a host RUN command reduces the consequence of an unauthenticated ARM request, but M9 must not be treated as a secure remote-control protocol.

M10 adds authenticated sessions, authorization and anti-replay policy.

Until then, do not expose the command channel to an untrusted network.

## Hardware Boundary

Physical completion still requires:

```text
Keil target build
board-specific interlock input
board-specific output driver
safe electrical default state
output failure behavior validation
power-cycle behavior validation
fault-injection testing
```

The STM32 application initializes the logical output safe-off and the local interlock open.

## Next Milestone

M10 adds security around the increasingly sensitive command surface:

```text
authenticated session establishment
message authenticity
role-based command authorization
anti-replay sequencing
protected baseline/control operations
```
