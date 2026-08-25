# Guardian F401 M15 — Red-Team Closure Matrix

## Status

CONTROLLED CLOSURE BASELINE

## Purpose

This document is the consolidated Source of Truth for the M15 adversarial
review and claim-boundary pass.

It records:

- what was challenged;
- what was demonstrated;
- what was corrected;
- what remains partial;
- which future gate owns the remaining work;
- how each finding affects portability beyond STM32F401.

This matrix does not convert open technical work into completed capability.

## Baseline

Integration branch:

`integration/m15-v0.14.2-reconciliation`

Closure-matrix source baseline:

`455c2fb`

Public release baseline reconciled into this branch:

`v0.14.2`

The STM32F401 is the first demonstrated hardware target.

It is not the architectural limit of Guardian F401.

## Architectural portability principle

Guardian shall preserve a separation between:

`Guardian Core`

and:

`Platform / Provider Adaptation`

The portable Guardian Core includes, where applicable:

- protocol contracts;
- assurance contracts;
- authorization semantics;
- canonicalization;
- signed message semantics;
- freshness / replay state;
- lifecycle state machines;
- deterministic policy semantics;
- evidence and validation contracts.

Platform-specific implementations may include:

- ADC;
- DMA;
- timers;
- GPIO;
- physical output adapters;
- secure storage;
- cryptographic providers;
- bootloader integration;
- TrustZone;
- secure elements;
- TPM/HSM-style providers;
- RTOS integration;
- device-specific peripherals.

A future port must not require redesign of Guardian's core assurance semantics
merely because the target MCU, SoC or execution environment changes.

## Closure matrix

| RT | Finding | Current status | Demonstrated boundary | Remaining gap | Future owner | Portability impact |
| --- | --- | --- | --- | --- | --- | --- |
| RT-M15-001 | AI/advisory -> actuation authority | PARTIAL | No current AI/advisory execution plane; logical deterministic run-permit authority demonstrated | Physical gate and continuously enforced future advisory boundary | G15-05 | Must be platform-neutral above the physical actuator adapter |
| RT-M15-003 | Hardware validation | PARTIAL | Structural/build and software acquisition evidence demonstrated | Physical board, ADC characterization, WCET/jitter, fault injection, physical output | Hardware validation program | Each target requires its own physical qualification package |
| RT-M15-004 | Fuzzing claims and coverage | PARTIAL | M11 parser/security fuzz infrastructure and documented campaigns confirmed | `guardian_control`, `guardian_firmware_lifecycle`, `guardian_embedded_link` fuzzing | Critical hardening | Fuzz contracts should remain reusable across target ports |
| RT-M15-005 | Prior art / related work | CLOSED_FOR_PRIOR_ART_CONTEXT | Simplex, Runtime Assurance, SACEM and formal-method context bounded correctly | Extend only when architecture or novelty claims materially change | Research governance | Prior-art context applies to architecture independent of MCU |
| RT-M15-006 | SBOM / dependency accounting | PARTIAL | Third-party inventory and some dependency constraints demonstrated | Formal SBOM, provenance, CI generation, immutable dependency/action pinning | G15-07 | SBOM must distinguish portable core dependencies from target/toolchain dependencies |
| RT-M15-007 | License / adoption boundary | CLOSED_FOR_LICENSE_BOUNDARY | Public repository clearly distinguished from open source; third-party terms preserved | Ongoing governance only | Release governance | Licensing boundary remains independent of target architecture |
| RT-M15-008 | Multi-node / heterogeneous architecture | PARTIAL | Future architecture documented and correctly qualified | Quorum, partition handling, split-brain, witness, failover and heterogeneous implementation | Future distributed stage | A second distinct platform should eventually demonstrate real portability and heterogeneity |
| RT-M15-009 | Independent review | PARTIAL | Internal adversarial review confirmed; false external-review claim not found | Genuine independent external technical review | External review gate | External review should assess both portable core and target-specific evidence |
| RT-M15-010 | Release/version reconciliation | CLOSED | Local M15 and public v0.14.2 histories reconciled and validated | None for this finding | Release governance | Portable release lineage must remain traceable across future target variants |
| RT-M15-011 | Technical Destruction terminology | CLOSED_FOR_TERMINOLOGY_BOUNDARY | Project-specific term bounded as internal adversarial technical review | None immediate | Documentation governance | Terminology rule remains common to all platforms |
| RT-M15-012 | Material claim / evidence boundary | CLOSED_FOR_CLAIM_BOUNDARY | No known material reviewed public/release claim remains classified OVERBROAD | Reopen when capability or public wording changes | Claim governance | Every future target must maintain claim-to-evidence separation |

## Closed boundary findings

The following findings are closed for the specific boundary adjudicated:

- RT-M15-005;
- RT-M15-007;
- RT-M15-010;
- RT-M15-011;
- RT-M15-012.

Closure of these findings does not imply complete project maturity.

## Open technical findings

The following findings remain PARTIAL:

- RT-M15-001;
- RT-M15-003;
- RT-M15-004;
- RT-M15-006;
- RT-M15-008;
- RT-M15-009.

Each PARTIAL finding has an explicit technical, physical or organizational
closure condition.

## Immediate engineering priority

### Priority 1 — G15-05 Authority Boundary

Required outcome:

Convert the advisory/authority restriction from an architectural rule into a
continuously enforced repository property.

Expected controls include:

- generated call graph;
- authority-reachable module allowlist;
- negative architectural tests;
- CI enforcement;
- explicit platform-actuator adapter boundary;
- prohibition of advisory/AI direct access to acquisition or actuation paths.

Portability requirement:

The authority contract must be platform-neutral.

Only the final physical adapter should depend on target-specific output
mechanisms.

### Priority 2 — G15-07 Build and Supply-Chain Security

Required outcome:

Create reproducible software-supply-chain evidence.

Expected controls include:

- authoritative dependency inventory;
- CycloneDX or SPDX SBOM;
- direct dependency versions;
- applicable dependency hashes;
- third-party license mapping;
- immutable CI-action pinning;
- build provenance;
- release artifact integrity;
- controlled SBOM generation and validation.

Portability requirement:

The SBOM must distinguish:

- Guardian Core;
- target adaptation code;
- compiler/toolchain;
- device packs;
- vendor-generated dependencies;
- host/test dependencies.

### Priority 3 — Critical-module fuzzing

Required targets:

- `guardian_control`;
- `guardian_firmware_lifecycle`;
- `guardian_embedded_link`.

Required property:

Fuzzing must understand state-machine and fail-closed semantics rather than
relying only on arbitrary-byte mutation.

Portability requirement:

Portable-core fuzz harnesses should execute without dependence on STM32-specific
physical hardware.

## Subsequent engineering stages

### Physical STM32F401 validation

Required evidence includes:

- ADC calibration;
- gain/offset characterization;
- noise characterization;
- ADC/timer/DMA timing;
- WCET;
- jitter;
- stack/RAM/flash usage;
- brownout and power-failure behavior;
- sensor disconnect;
- physical safe-output behavior when implemented.

This evidence is target-specific.

It must not be generalized to another MCU without corresponding validation.

### Second-platform portability demonstration

A future milestone should port Guardian Core to a materially different
hardware/software platform.

The objective is not merely compilation.

The objective is to demonstrate:

`same Guardian assurance contracts + different platform adaptation layer`

with equivalent contract-level behavior.

The second platform should preferably differ meaningfully in one or more of:

- MCU family;
- cryptographic provider;
- secure-storage mechanism;
- peripheral architecture;
- execution environment.

A successful second target becomes evidence that Guardian is an architecture
rather than an STM32F401-specific code base.

### Heterogeneous multi-node stage

Only after portable single-node contracts are demonstrated should Guardian move
to:

- heterogeneous nodes;
- authenticated corroboration;
- quorum;
- witness behavior;
- partition handling;
- split-brain prevention;
- failover;
- recovery coordination.

Multi-node architecture must not be implemented by weakening the deterministic
single-node authority boundary.

### Advisory / AI stage

AI/advisory functionality remains downstream of:

- deterministic policy;
- authorization;
- assurance state;
- authority-boundary enforcement.

The intended relationship remains:

`AI / advisory`
-> `recommendation`
-> `deterministic policy`
-> `authorization`
-> `platform authority adapter`

AI must not obtain direct physical authority merely because a target platform
supports an actuator.

## Manufacturer-facing evolution

The long-term engineering objective is to make Guardian assessable as a
portable embedded assurance component rather than as an STM32F401-only
demonstration.

Future manufacturer-facing evidence should be capable of packaging:

- exact source baseline;
- firmware architecture;
- integration contract;
- threat model;
- authority model;
- reproducible build;
- tests;
- fuzzing;
- SBOM;
- dependency provenance;
- hardware assumptions;
- target-specific validation;
- open limitations;
- release hashes;
- claim/evidence matrix.

A manufacturer must be able to determine what Guardian guarantees without
having to infer guarantees from implementation size or documentation volume.

## Evidence discipline

The following distinctions remain normative:

`MEMORY != SOURCE OF TRUTH`

`ARCHITECTURE != IMPLEMENTATION`

`IMPLEMENTATION != VALIDATION`

`SOFTWARE VALIDATION != HARDWARE QUALIFICATION`

`BUILD SUCCESS != PRODUCTION READINESS`

`FUZZ TARGET EXISTS != COMPLETE FUZZ COVERAGE`

`PUBLIC REPOSITORY != OPEN SOURCE`

`INTERNAL REVIEW != INDEPENDENT EXTERNAL REVIEW`

`DOCUMENTED MULTI-NODE ARCHITECTURE != MULTI-NODE IMPLEMENTATION`

`AI RECOMMENDATION != ACTUATION AUTHORITY`

`FIRST TARGET != ARCHITECTURAL LIMIT`

## Re-open rules

This matrix must be reviewed or reopened when:

- a PARTIAL RT is materially advanced;
- G15-05 is implemented;
- G15-07 is implemented;
- critical fuzz coverage changes;
- physical STM32F401 validation is performed;
- a second hardware platform is introduced;
- multi-node code is introduced;
- AI/advisory code is introduced;
- an independent external review occurs;
- a new public release is prepared;
- material public claims change.

## Closure disposition

The current M15 Red-Team pass is complete for the reviewed claim,
governance and architecture-boundary scope.

It does not close the remaining technical gates.

The remaining gaps are explicit engineering work, not undocumented
uncertainty.

Guardian F401 may now proceed from:

`red-team discovery / claim reconciliation`

to:

`controlled implementation of the remaining hardening gates`

while preserving portability beyond the STM32F401 target.
