# RT-M15-001 — AI / Advisory to Actuation Authority Boundary

## Status

PARTIAL_RECONCILED

## Classification

Architecture-authority and actuation-boundary finding.

This record distinguishes current logical-output enforcement from physical
actuator enforcement and from future AI/advisory isolation.

It does not establish physical STM32F401 actuator qualification.

## RT-01A — Existing AI/advisory direct actuation

Status:

`CLOSED_FOR_CURRENT_IMPLEMENTATION`

Repository inspection found:

- zero AI/advisory implementation code hits;
- zero direct advisory-to-control textual paths;
- zero `actuation_allowed` semantics in the M15 assurance plane;
- zero `policy_authorized` semantics in the M15 assurance plane.

This finding means no current AI/advisory execution path was demonstrated.

It must not be interpreted as proof that a future AI implementation would be
unable to bypass control.

Future AI/advisory integration requires separate enforcement under G15-05.

## RT-01B — Deterministic logical output authority

Status:

`CLOSED_FOR_CURRENT_IMPLEMENTATION`

The current logical run-permit boundary is owned by `guardian_control`.

Observed properties include:

- startup supervision is disabled;
- logical run permit starts safe-off;
- local interlock starts open;
- a board/application output adapter is required before supervision can arm;
- output-adapter configuration requires immediate successful safe-off;
- missing output adapter denies control operation;
- open interlock denies or forces safe state;
- health ALARM forces safe-off;
- output-application failure is latched;
- host ARM does not assert run permit directly;
- local run request remains separately required for active permit;
- fail-safe reset requires safe-entry conditions.

## Logical output ownership

The application-visible logical output is:

`guardian_control_output_shadow`

Observed production ownership:

- defined only in `guardian_firmware_app.c`;
- initialized safe-off;
- written by `guardian_firmware_control_output()`;
- exposed through a read accessor;
- no address/pointer escape was identified.

The production callback installation path is:

`guardian_firmware_app`
-> `guardian_embedded_link_configure_control_output()`
-> `guardian_control_configure_output()`
-> `control->output = *output`

The reviewed production callback is:

`guardian_firmware_control_output`

Direct output-adapter assignments outside this path were observed only in test
fixtures.

## Fail-closed test evidence

Existing control tests demonstrate at least the following properties:

- ARM before safe-entry conditions is denied;
- ARM while interlock is open is denied;
- successful ARM remains run-permit safe-off;
- host ARM cannot assert run permit directly;
- local run request is required before run permit becomes active;
- health ALARM forces safe-off;
- interlock opening while active forces safe-off;
- loss of baseline readiness fails safe;
- output-application failure leaves logical output safe-off;
- embedded-link integration preserves the same ARM/local-run/interlock
  distinction.

## RT-01C — Physical actuator authority

Status:

`NOT_DEMONSTRATED`

The reviewed firmware path produced no direct matches for:

- `HAL_GPIO_WritePin`;
- `HAL_GPIO_TogglePin`;
- `HAL_TIM_PWM_Start`;
- `HAL_TIM_PWM_Stop`;
- GPIO BSRR writes;
- GPIO ODR writes.

The current application adapter terminates at
`guardian_control_output_shadow`.

Therefore current evidence establishes a logical safe-output boundary.

It does not establish a physical STM32F401 actuator gate.

No physical actuator-enforcement claim is authorized by this record.

## RT-01D — Future AI/advisory bypass resistance

Status:

`CLOSED_FOR_CURRENT_LOGICAL_MECHANICAL_BOUNDARY`

G15-05 has converted the current architectural authority restriction into a
mechanically enforced repository property.

This disposition applies to the reviewed implementation in which no
AI/advisory execution plane is present.

Introduction of an AI/advisory execution plane requires reopening this
validation surface for implementation-specific isolation and bypass-resistance
review.

Required future control:

`G15-05 — Demonstration of the Authority Boundary`

Required evidence includes:

- generated call graph;
- explicit allowlist of modules permitted to reach actuator authority;
- negative architectural test that fails if advisory/AI reaches control or
  acquisition through a forbidden path;
- CI enforcement of the authority boundary;
- formal authority diagram;
- explicit physical gate boundary when physical actuation is implemented;
- future advisory/AI isolation validation.

## Indirect-call surface

Repository inspection identified an indirect-call/callback/dispatch surface.

The current review found no demonstrated advisory/AI bypass through that
surface.

However, absence of a current AI implementation is not a permanent mechanical
guarantee.

G15-05 must convert the architectural restriction into a continuously enforced
repository property.

## Decision

RT-M15-001 is reconciled globally as:

`PARTIAL_RECONCILED`

The current implementation demonstrates deterministic logical-output ownership,
fail-closed control behavior and mechanically enforced logical authority
boundaries.

G15-05 logical/mechanical authority enforcement is closed for the reviewed
implementation.

The current implementation still does not demonstrate physical actuator
enforcement.

Future introduction of an AI/advisory execution plane requires a new
implementation-specific isolation and bypass-resistance validation.

No claim equivalent to:

`AI is physically incapable of actuating`

is authorized by current evidence.

The currently supported statement is:

`No AI/advisory execution plane is present, and the implemented logical
run-permit path is mediated by deterministic Guardian control policy.`

## Residual restrictions

This adjudication does not authorize:

- physical actuator qualification;
- AI/advisory implementation;
- physical gate claims;
- production deployment;
- functional-safety claims;
- certification claims.

The former G15-05 logical/mechanical authority-boundary closure condition has
been satisfied.

RT-M15-001 is not globally CLOSED because RT-01C physical actuator authority
remains NOT_DEMONSTRATED.

Global closure requires separate evidence for any remaining applicable
boundaries; physical actuator qualification must not be inferred from G15-05
logical/mechanical closure.
