# ADR-M16-007 â€” Governed Component Reuse and Language Boundary Architecture

Status: CANDIDATE

Milestone: M16

Decision Type: Architecture / Governance / Multi-Platform Engineering

Depends on:
- ADR-M16-001 â€” Heterogeneous Dual-Node Supervision
- ADR-M16-002 â€” Versioned Semantic Contract Architecture
- ADR-M16-004 â€” Evidence-Claim Promotion Governance
- ADR-M16-005 â€” Peer Freshness, Epoch, Replay and Semantic Compatibility
- ADR-M16-006 â€” Project-State v2 Schema Evolution

## Context

Guardian is evolving from a single-node STM32F401 system toward a heterogeneous
multi-node architecture that includes an NXP MCXN947 supervisory peer and may
later include additional nodes, platforms, languages, validators and reusable
engineering components.

Guardian requires reuse to reduce repeated work without allowing prior validation
to become automatic authority, automatic safety, automatic semantic equivalence
or automatic evidence transfer.

The governing methodology is:

ONE FAILURE = FIX THE FAILURE

TWO SIMILAR FAILURES = FIX THE METHOD

The engineering objective is to reuse demonstrated and governed semantics,
contracts, validators and patterns where applicable, while preserving explicit
platform, version, evidence and authority boundaries.

## Decision

Guardian SHALL permit governed reuse of components, contracts, validators,
state machines, evidence routines and architectural patterns when applicability
to the target context is explicitly established.

Reuse SHALL NOT be treated as proof of equivalence.

The following implications are prohibited:

REUSE == AUTOMATIC VALIDITY

REUSE == AUTOMATIC AUTHORITY

REUSE == AUTOMATIC SAFETY

REUSE == AUTOMATIC EVIDENCE TRANSFER

SAME CONTRACT == SAME IMPLEMENTATION REQUIRED

SAME IMPLEMENTATION == SAME PLATFORM VALIDITY

DIFFERENT LANGUAGE == DIFFERENT AUTHORITY MODEL

LANGUAGE == AUTHORITY

MEMORY_SAFETY == AUTHORIZATION

## Language and component selection boundary

Guardian MAY use multiple implementation languages when the language materially
reduces a demonstrated engineering risk or enables a required capability.

### C

C is the default language for:
- deterministic embedded core logic;
- hardware-facing control;
- startup and low-level platform integration;
- interrupt service routines;
- timing-critical execution;
- actuator-facing components when separately authorized.

C != AUTOMATIC AUTHORITY

### Rust

Rust is a candidate language for:
- secure boundary components;
- bounded-memory parsers;
- state machines;
- protocol handling;
- replay and epoch logic;
- semantic decoding;
- components where memory-safety materially reduces risk.

RUST != AUTOMATIC TRUST
MEMORY_SAFE != AUTHORIZED

### Python / Go

Python and Go are candidate languages for:
- analysis;
- validation;
- evidence processing;
- orchestration;
- host and supervisory tooling;
- CI and engineering automation.

PYTHON != ADVISORY BY DEFINITION
GO != SUPERVISORY AUTHORITY

## Governed reuse eligibility

An artifact MAY become a reuse candidate when it is:
- identified;
- versioned;
- governed;
- bounded;
- validated for its original context.

Before reuse in another chip, hardware revision, firmware version, semantic
profile, system role, language or trust boundary, the target context SHALL
establish, as applicable:
- semantic compatibility;
- authority compatibility;
- platform applicability;
- version compatibility;
- evidence applicability;
- trust-boundary compatibility.

If a required applicability condition is unknown, unsupported, ambiguous or
contradicted:

REUSE_NOT_AUTHORIZED
UNKNOWN = FAIL_CLOSED

## Reusable semantics and patterns

Guardian MAY reuse governed semantics and patterns including:
- freshness contracts;
- epoch semantics;
- replay rejection rules;
- semantic-version contracts;
- evidence promotion rules;
- authority-boundary rules;
- negative test patterns;
- validator architecture;
- failure-state vocabulary.

TESTED_ON_STM32F401 != VALID_ON_MCXN947
VALIDATED_IN_C != VALIDATED_IN_RUST
HOST_VALIDATION != PHYSICAL_VALIDATION

PROVEN_CONTRACT
+ NEW_IMPLEMENTATION
+ APPLICABILITY_VALIDATION
+ NEW_EVIDENCE
= POSSIBLE_REUSE

## Anti-repetition engineering rule

REPEATED_MANUAL_VALIDATION
-> CANDIDATE_FOR_AUTOMATION

REPEATED_VALIDATOR_LOGIC
-> CANDIDATE_FOR_SHARED_GOVERNED_VALIDATOR

REPEATED_SEMANTIC_RULE
-> CANDIDATE_FOR_VERSIONED_CONTRACT

REPEATED_PLATFORM_SPECIFIC_CODE
-> NOT_AUTOMATICALLY_REUSABLE

Automation may reduce repetition but SHALL NOT bypass governance, evidence,
versioning or human authority boundaries.

## NodeLink boundary

WIRE_CONTRACT
-> BOUNDED_PARSER
-> SEMANTIC_VALIDATION
-> FRESHNESS_REPLAY_VALIDATION
-> EVIDENCE
-> LOCAL_AUTHORITY_POLICY
-> OPTIONAL_LOCAL_ACTION

REMOTE_MESSAGE -> ACTUATION

The direct implication above is prohibited.

AI/advisory != authority != actuation

## Diversity boundary

DIFFERENT_CHIP != DIFFERENT_SEMANTICS_BY_DEFAULT
DIFFERENT_LANGUAGE != DIFFERENT_AUTHORITY_MODEL
SAME_CONTRACT != SAME_IMPLEMENTATION_REQUIRED
SAME_IMPLEMENTATION != SAME_PLATFORM_VALIDITY

Cross-language semantic equivalence requires explicit validation.

## Historical preservation

Reuse SHALL NOT rewrite historical evidence or reinterpret earlier observations
under later semantics.

A reusable artifact remains bound to the version, context and evidence under
which it was originally demonstrated.

Historical evidence may support a reuse decision but does not automatically
become evidence of the target implementation.

## Acceptance criteria

CR-01 Language does not imply authority.
CR-02 Reuse does not imply validity.
CR-03 Unknown applicability fails closed.
CR-04 Cross-platform evidence is not automatically transferable.
CR-05 Cross-language semantic equivalence requires validation.
CR-06 Historical evidence remains immutable.
CR-07 Reusable validators remain version-bound.
CR-08 Platform-specific physical claims require platform-specific evidence.
CR-09 NodeLink remote data cannot directly grant actuation authority.
CR-10 Automation may reduce repetition but cannot bypass governance.
CR-11 Reuse-policy approval does not approve a specific reuse instance.
CR-12 NodeSupervisor implementation remains unauthorized by this ADR.

## Non-claims

This candidate does not demonstrate:
- Rust execution on STM32F401;
- Rust execution on MCXN947;
- Go execution on any MCU target;
- NodeLink implementation completion;
- NodeSupervisor architecture authorization;
- NodeSupervisor implementation authorization;
- physical MCXN947 execution;
- physical dual-node timing;
- cross-language semantic equivalence;
- production readiness;
- certification;
- actuation authority for any supervisory component.

REUSE_POLICY_APPROVED != REUSE_INSTANCE_APPROVED

## Consequence

Guardian may reduce repeated engineering work by reusing governed semantics,
contracts, validators and evidence patterns while preserving independent
platform-specific applicability and authority decisions.

This ADR does not authorize implementation. It establishes the reusable
engineering boundary that downstream M16 NodeLink and NodeSupervisor decisions
must respect.