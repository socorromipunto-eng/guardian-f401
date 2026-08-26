# Guardian F401 M15 - G15-05 Authority Reachability Model

## Status

PRE-IMPLEMENTATION ARCHITECTURE BASELINE

## Source baseline

Branch: integration/m15-v0.14.2-reconciliation

Baseline commit: f7ba4fe

## Observed authority facts

- Guardian Control owns the observed production run_permit writes.
- Guardian Control is the sole observed production invoker of output.apply(...).
- Observed output.apply calls: 6.
- Observed output.apply calls outside Guardian Control: 0.
- Direct physical GPIO/PWM output hits: 0.
- App/Platform may provide the output adapter through explicit configuration APIs.
- Providing the adapter does not confer operational authority.

## Portable authority boundary

Guardian Core Authority
        |
        v
Platform Authority Adapter
        |
        v
Target-Specific Physical I/O

The platform adapter may change between hardware targets.
Guardian authority semantics must remain portable.

## G15-05 closure requirement

G15-05 remains OPEN until authority ownership and reachability are mechanically enforced by tests and CI.

## Authority ownership model

### Guardian Control

Guardian Control is the observed logical operational authority owner.

The reviewed production baseline demonstrates that:

- production writes to control->status.run_permit occur in guardian_control.c;
- all six observed calls to control->output.apply(...) occur in guardian_control.c;
- zero output.apply(...) invocations were observed outside Guardian Control;
- zero direct physical GPIO/PWM output mechanisms were observed in the inspected firmware surface.

Guardian Control therefore owns the current logical decision of whether the configured authority adapter may be invoked.

### Platform / Embedded Link

Platform and Embedded Link may provide state and requests through defined Guardian Control APIs.

Observed relationships include:

- guardian_control_update_health(...);
- guardian_control_handle_request(...);
- guardian_control_run_permit(...);
- guardian_embedded_link_configure_control_output(...).

These interfaces do not establish ownership of the canonical logical run-permit state.

### Application initialization

Application initialization may select and provide a platform output callback.

The reviewed path is:

guardian_firmware_app
    -> guardian_embedded_link_configure_control_output(...)
    -> guardian_control_configure_output(...)
    -> control->output = *output

Providing the adapter is configuration authority only.

It does not grant operational authority to decide when the adapter is invoked.

### Analytics and future advisory functions

Analytics, DSP, anomaly detection and future AI/advisory functions may provide observations or recommendations.

They must not:

- write the canonical logical run-permit directly;
- invoke the platform authority adapter directly;
- create a parallel authority path;
- bypass deterministic Guardian Control decisions.

## Required authority invariant

The intended invariant is:

Only Guardian Control may own the canonical logical authority state and invoke the configured authority-output adapter during normal operation.

Platform-specific code may implement or provide the adapter but must remain subordinate to Guardian Control invocation.

## Mechanical enforcement requirements

This architecture model does not close G15-05.

G15-05 requires repository-enforced controls that make unauthorized authority reachability mechanically detectable.

### G15-05-A - Machine-readable authority allowlist

A controlled allowlist must identify:

- the canonical logical authority owner;
- the only modules permitted to invoke the authority-output adapter;
- the allowed adapter-configuration path;
- modules permitted to provide observations, requests or state;
- modules prohibited from owning or invoking operational authority.

The allowlist must describe architectural roles rather than STM32F401-specific hardware behavior.

### G15-05-B - Authority reachability analysis

A generated or otherwise reproducible analysis must identify paths capable of reaching:

- canonical run-permit mutation;
- authority-output callback invocation;
- platform authority adapter configuration;
- future direct physical-output APIs.

The analysis must account for indirect calls and callbacks.

A direct-call-only grep is insufficient.

### G15-05-C - Negative architecture tests

The validation suite must prove that prohibited authority paths are rejected.

Required hostile cases include at least:

- analytics -> authority adapter;
- future advisory/AI -> authority adapter;
- arbitrary Platform module -> authority adapter;
- direct run-permit mutation outside Guardian Control;
- direct physical output outside the Platform Authority Adapter;
- alternate callback invocation outside Guardian Control.

### G15-05-D - CI enforcement

Authority-boundary validation must become a mandatory repository gate.

A change that introduces prohibited authority reachability must fail validation before acceptance.

### G15-05-E - Platform-portability enforcement

The enforcement model must survive replacement of the target platform.

The rule must remain:

Guardian Core Authority
    -> Platform Authority Adapter
    -> Target-Specific Physical I/O

The enforcement mechanism must not define Guardian authority solely by STM32 HAL names or STM32F401-specific file names.

### G15-05-F - Fail-closed configuration

If required authority configuration is absent, invalid or ambiguous:

- logical authority must not become permissive by default;
- missing adapter configuration must not create implicit authorization;
- invalid state must not permit physical action;
- authority configuration errors must remain distinguishable from analytics or advisory results.

## Mechanical closure criteria

G15-05 may become CLOSED only when all of the following are demonstrated:

1. authority ownership is machine-readable;
2. reachability analysis is reproducible;
3. indirect callback paths are covered;
4. negative architecture tests demonstrate rejection of prohibited paths;
5. CI enforces the authority boundary;
6. the enforcement mechanism remains portable beyond STM32F401;
7. the current assurance regression remains green.

## Current disposition

G15-05 remains OPEN.

The current repository demonstrates a favorable authority structure but not yet a mechanically enforced invariant.

## Technical Destruction adjudication

### Current operational authority discipline

Status: CONFIRMED

The reviewed production baseline demonstrated:

- no direct external production access to Guardian Control status/output authority fields;
- all observed output.apply(...) invocations remain inside guardian_control.c;
- no direct physical GPIO/PWM authority path was demonstrated.

No active authority bypass was demonstrated.

### Structural encapsulation finding

Status: REQUIRES_ENFORCEMENT

guardian_embedded_link contains guardian_control_t by value.

In C, an object embedded by value requires guardian_control_t to be a complete type at the embedding compilation boundary.

Therefore Guardian Control authority ownership is currently respected by implementation discipline, but is not yet protected by an opaque structural boundary.

Future code with access to the complete Guardian Control object could potentially violate ownership discipline unless mechanical enforcement or stronger encapsulation is introduced.

This is not classified as an active authority bypass because direct external production field access was observed at count zero.

### Required hardening direction

G15-05 should reduce direct structural authority exposure while preserving deterministic static-memory operation.

The preferred design shall:

- preserve static allocation where required;
- avoid requiring dynamic allocation;
- prevent arbitrary Platform code from directly mutating canonical authority state;
- keep authority transitions behind Guardian Control APIs;
- keep output invocation behind Guardian Control;
- allow the platform adapter implementation to change without changing Guardian Core authority semantics.

### Technical Destruction disposition

ACTIVE_AUTHORITY_BYPASS = NOT_DEMONSTRATED

CURRENT_AUTHORITY_DISCIPLINE = CONFIRMED

STRUCTURAL_ENCAPSULATION = NOT_ENFORCED

G15_05_ENCAPSULATION_FINDING = REQUIRES_ENFORCEMENT

G15-05 remains OPEN.

## G15-05 enforcement design decision

### Decision

The current G15-05 implementation increment will preserve static Guardian Control storage and enforce authority ownership mechanically.

An invasive opaque-object ABI refactor is not required to establish the current G15-05 authority invariant.

### Evidence supporting this decision

Observed design-impact evidence:

- guardian_control_t by-value occurrences: 8;
- productive by-value owner occurrences: 1;
- test by-value owner occurrences: 7;
- guardian_control_t pointer surface occurrences: 24;
- sizeof(guardian_control_t) dependencies: 0;
- heap allocation dependencies: 0;
- direct productive Guardian Control authority-field access outside guardian_control.c: 0;
- output.apply(...) calls outside Guardian Control: 0;
- demonstrated direct physical-output paths: 0.

### Detector adjudication

A broad field-access probe reported 65 productive .status/.output matches.

Inspection demonstrated that these matches primarily belonged to unrelated acquisition, health and telemetry structures.

They do not demonstrate direct access to Guardian Control authority fields.

The authority-specific probe remains authoritative for this finding and reported zero direct external production accesses.

The broad 65-hit result is therefore classified as a validation-instrument false-positive set for Guardian Control authority ownership.

### Selected enforcement strategy

G15-05 will implement:

1. a machine-readable authority ownership policy;
2. exact authority-sensitive symbol and field rules;
3. callback invocation ownership rules;
4. platform-adapter configuration rules;
5. prohibited physical-output path rules;
6. negative hostile tests;
7. mandatory validation suitable for CI.

The enforcement must analyze Guardian-specific authority surfaces rather than generic field names such as status or output.

### Static-memory requirement

Guardian shall preserve deterministic static-memory operation.

G15-05 must not introduce malloc, calloc, realloc or runtime heap dependence.

### Future encapsulation

A later hardening increment may convert Guardian Control to a stronger opaque or private representation if that provides additional assurance without harming portability or deterministic storage.

Such a representation change is not required to claim that G15-05 mechanically enforces the authority boundary defined by this model.

### Portability requirement

The enforcement policy shall target architectural roles and Guardian authority symbols, not STM32F401-specific HAL behavior.

Future platforms may replace the Platform Authority Adapter while preserving the same Guardian Core authority ownership policy.

### Implementation gate

Architecture is now sufficiently defined to begin the G15-05 mechanical-enforcement implementation.

G15-05 remains OPEN until the enforcement implementation and hostile validation pass.
