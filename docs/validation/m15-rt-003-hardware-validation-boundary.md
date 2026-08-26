# RT-M15-003 — Hardware Validation Boundary

## Status

PARTIAL

## Classification

Hardware-evidence, physical-validation and claim-boundary finding.

This record distinguishes firmware implementation and structural build evidence
from physical STM32F401 validation.

## Evidence summary

Repository decision-count inspection produced:

- structural/build references: 133;
- ADC/acquisition references: 237;
- physical-board references: 9;
- WCET/jitter references: 1;
- physical-fault-injection references: 2;
- direct physical-output implementation references: 0.

Counts indicate repository surface only.

They must not be interpreted as validation results without adjudicating the
underlying evidence.

## RT-03A — Structural / build validation

Status:

`CONFIRMED`

Guardian F401 contains substantial STM32F401 structural and build evidence,
including target configuration, Keil/Arm Compiler references, structural review
records and AXF-related evidence.

This supports a software/build claim.

It does not establish physical-board qualification.

## RT-03B — ADC / acquisition implementation

Status:

`CONFIRMED_SOFTWARE / HARDWARE_VALIDATION_PENDING`

The firmware contains substantial ADC, DMA, timer and acquisition
implementation/configuration surface.

The current evidence supports implementation of acquisition logic.

It does not demonstrate physical validation of:

- ADC calibration;
- absolute measurement accuracy;
- gain/offset behavior;
- board noise characteristics;
- channel cross-coupling;
- physical saturation behavior;
- ADC/timer/DMA timing accuracy;
- sensor-chain uncertainty;
- real board sample-rate stability.

## RT-03C — Physical STM32F401 board validation

Status:

`NOT_DEMONSTRATED`

Repository documentation contains references to physical-board validation.

The reviewed evidence maintains physical-board qualification as an open or
pending gate.

No claim of completed physical STM32F401 validation is authorized.

## RT-03D — WCET / jitter / real-time timing

Status:

`NOT_DEMONSTRATED`

The repository search identified one WCET/jitter/timing-related evidence
reference in the adjudicated search surface.

The reviewed material does not demonstrate a completed on-target timing
campaign.

Current evidence does not establish:

- measured WCET;
- measured interrupt jitter;
- deterministic ADC/DMA latency;
- validated timing budget;
- cycle-count bounds;
- on-target deadline compliance.

## RT-03E — Physical fault injection

Status:

`NOT_DEMONSTRATED`

Physical-fault-related references were found in repository documentation, but
no completed physical fault-injection campaign was demonstrated.

Current evidence does not establish validation against:

- brownout;
- power interruption;
- clock glitch;
- voltage fault;
- sensor disconnect;
- electrical disturbance;
- board-level injected failure.

Software failure-injection and hostile tests must remain distinct from physical
fault injection.

## RT-03F — Physical actuator output

Status:

`NOT_DEMONSTRATED`

The adjudicated firmware search produced zero matches for direct physical output
mechanisms including:

- `HAL_GPIO_WritePin`;
- `HAL_GPIO_TogglePin`;
- `HAL_TIM_PWM_Start`;
- `HAL_TIM_PWM_Stop`;
- direct GPIO BSRR writes;
- direct GPIO ODR writes.

The current Guardian control implementation therefore demonstrates a logical
run-permit / safe-output boundary.

It does not demonstrate a physical STM32F401 actuator-output implementation.

This finding is consistent with RT-M15-001.

## Decision

RT-M15-003 remains globally:

`PARTIAL`

The repository demonstrates substantial software, structural-build and
acquisition implementation evidence.

It does not demonstrate completed physical-board, real-time timing, physical
fault-injection or physical actuator validation.

Build success must not be represented as physical hardware validation.

Host tests and simulation must not be represented as on-target validation.

## Required future work

The remaining hardware work includes:

1. physical STM32F401 board validation;
2. ADC calibration and uncertainty characterization;
3. physical noise measurement;
4. ADC/timer/DMA synchronization verification;
5. WCET measurement;
6. interrupt and acquisition jitter measurement;
7. CPU/RAM/flash/stack resource validation;
8. sensor saturation and disconnect testing;
9. power-loss and brownout testing;
10. electrical fault-injection testing;
11. physical safe-output/gate implementation and validation when applicable.

## Claim boundary

The currently supported claim is:

`Guardian F401 has demonstrated software/build and logical control behavior for
the reviewed STM32F401 architecture.`

The following claim is not authorized:

`Guardian F401 has been physically validated or hardware-qualified on STM32F401.`

## Residual restrictions

This record does not authorize:

- physical-board qualification claims;
- certified real-time claims;
- functional-safety claims;
- production actuator-control claims;
- industrial safety deployment;
- cybersecurity certification;
- production readiness.

RT-M15-003 may become CLOSED only after the outstanding physical validation
program is executed, evidenced and independently reviewed.