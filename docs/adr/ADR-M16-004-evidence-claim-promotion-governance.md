# ADR-M16-004 — Evidence / Claim Promotion Governance

## Status

ACCEPTED FOR ARCHITECTURE SOURCE-OF-TRUTH MATERIALIZATION

## Decision scope

This ADR governs how Guardian F401 distinguishes evidence existence, evidence
integrity, evidence validity, evidence applicability, evidence sufficiency, claim
assessment, claim adjudication, authority, and actuation.

It does not mutate claims or evidence registers, does not promote any claim, does
not authorize NodeSupervisor implementation, and does not establish physical
validation.

## Context

M16-B0 read-only discovery found no mechanically demonstrated cross-register
identity linkage between the current claims, evidence, requirements, document,
capability, and project-state registers sufficient to support governed automatic
claim promotion.

The project therefore requires an explicit promotion contract before any system
may interpret evidence presence as proof of claim truth.

A robust assurance model must prevent favorable test output, build success,
simulator success, host validation, generated reports, or AI analysis from being
silently converted into stronger claims than the evidence actually supports.

## Decision

The following distinctions are normative:

EVIDENCE_PRESENT != EVIDENCE_VALID

EVIDENCE_VALID != EVIDENCE_APPLICABLE

EVIDENCE_APPLICABLE != EVIDENCE_SUFFICIENT

EVIDENCE_SUFFICIENT != CLAIM_DEMONSTRATED

CLAIM_DEMONSTRATED != CLAIM_APPROVED

CLAIM_APPROVED != AUTHORITY_GRANTED

AUTHORITY_GRANTED != ACTUATION_AUTHORIZED

POSITIVE_EVIDENCE != ABSENCE_OF_CONTRADICTORY_EVIDENCE

HOST_VALIDATION != PHYSICAL_VALIDATION

SIMULATOR_PASS != HARDWARE_PASS

BUILD_SUCCESS != BEHAVIORAL_VALIDATION

AI_ANALYSIS != CLAIM_PROMOTION_AUTHORITY

MECHANICAL_EVALUATION != GOVERNANCE_DECISION

## Evidence model

Each governed evidence item must be independently identifiable and must carry
sufficient context to determine whether it is eligible to support a specific
claim.

The evidence model must support, at minimum:

- stable evidence identity;
- evidence class/type;
- source or producing mechanism;
- source commit/tree/artifact identity where applicable;
- integrity binding;
- generation method;
- execution environment;
- target identity;
- hardware/software identity where applicable;
- semantic/profile/version applicability;
- freshness boundary;
- validation state;
- evidence role;
- eligible claim scope.

Evidence role must be able to distinguish at least:

- SUPPORTING;
- CONTRADICTORY;
- NEGATIVE;
- INCONCLUSIVE.

Evidence existence alone is not evidence validity.

Evidence validity alone is not evidence applicability.

Evidence applicability alone is not evidence sufficiency.

## Claim model

A governed claim must preserve:

- stable claim identity;
- claim version;
- semantic statement;
- governed scope;
- applicability conditions;
- assessment state;
- supporting evidence references;
- contradictory evidence references;
- required evidence classes;
- evidence sufficiency rule;
- assessment provenance;
- assessment boundary/time context where required;
- human/governed adjudication status;
- supersession/revocation state.

A claim must not collapse all assurance semantics into a single PASS/FAIL field.

## Claim assessment state

Technical assessment and governance adjudication are separate dimensions.

The bounded technical assessment vocabulary is:

- NOT_ASSESSED
- NOT_DEMONSTRATED
- PARTIALLY_DEMONSTRATED
- DEMONSTRATED
- CONTRADICTED
- INVALIDATED

These values describe technical assessment state only.

They do not grant approval, normative authority, production readiness, or
actuation authorization.

## Claim adjudication state

The bounded governance adjudication vocabulary is:

- UNADJUDICATED
- ACCEPTED
- REJECTED
- SUPERSEDED
- WITHDRAWN

Adjudication is a governed decision.

A mechanically demonstrated claim remains distinct from an accepted claim.

An accepted claim remains distinct from normative authority.

Normative authority remains distinct from actuation authorization.

## Promotion contract

Claim assessment may be computed mechanically only when:

1. the claim identity and version are known;
2. the required evidence rule is explicit;
3. evidence identities are explicit;
4. evidence integrity is valid;
5. evidence applicability matches the claim scope;
6. semantic/version compatibility is valid;
7. freshness requirements are satisfied;
8. contradictory evidence is evaluated;
9. no unknown required state remains;
10. the assessment rule is deterministic and versioned.

Unknown, ambiguous, incompatible, stale, or contradictory conditions must fail
closed.

## Mechanical evaluation boundary

Validators and assurance tooling may:

- read evidence;
- read claim contracts;
- evaluate eligibility;
- evaluate applicability;
- evaluate sufficiency;
- report a claim assessment candidate;
- report contradictions;
- report unknown or stale conditions.

Validators and assurance tooling must not:

- permanently mutate claim status by themselves;
- grant governance approval;
- grant normative authority;
- authorize actuation;
- suppress contradictory evidence;
- infer physical validation from non-physical evidence.

VALIDATOR != MUTATION AUTHORITY

MECHANICAL ASSESSMENT != PERSISTED GOVERNANCE DECISION

## Persistent promotion boundary

Persistent claim-state transitions require a separately governed transaction.

A promotion transaction must preserve:

- prior claim state;
- candidate assessment;
- evidence set;
- contradictory evidence set;
- rule/version used;
- provenance;
- human/governance adjudication where required;
- resulting state;
- historical auditability.

Historical claim meaning must not be rewritten by later evidence.

## Contradictory evidence

Contradictory evidence is first-class governance input.

If relevant contradictory evidence exists, favorable evidence must not silently
win.

Example:

SIMULATOR_PASS + HARDWARE_FAIL

must not produce a physical-hardware claim of DEMONSTRATED.

Contradiction must resolve to a bounded fail-closed state until governed
adjudication or replacement evidence resolves the conflict.

## Freshness and applicability

Evidence eligibility must be bound to the claim's actual applicability domain.

Applicability may include:

- source commit/tree;
- firmware version;
- hardware identity/revision;
- configuration;
- semantic contract/profile;
- protocol version;
- environment;
- toolchain;
- execution mode;
- time/freshness boundary.

Stale or incompatible evidence is not silently reusable.

## Host / simulator / physical separation

The following promotion is prohibited:

HOST_TEST_PASS -> PHYSICAL_VALIDATION

SIMULATOR_PASS -> HARDWARE_VALIDATION

BUILD_SUCCESS -> RUNTIME_BEHAVIOR_CONFIRMED

STATIC_ANALYSIS_PASS -> PHYSICAL_SAFETY_CONFIRMED

Physical claims require evidence produced in the physical evidence class required
by the claim contract.

## AI boundary

AI may:

- search evidence;
- classify candidate relationships;
- identify contradictions;
- summarize assurance state;
- recommend adjudication;
- identify missing evidence.

AI may not:

- mutate normative claim status autonomously;
- approve a claim;
- grant authority;
- authorize actuation;
- suppress contradictory evidence.

AI/advisory != authority != actuation

## Revocation and demotion

The architecture must support demotion, contradiction, invalidation, withdrawal,
and supersession.

A claim previously assessed as DEMONSTRATED must not remain demonstrated if its
required evidence becomes invalid, stale, contradicted, or outside applicability.

Historical states remain auditable.

## Failure semantics

Unknown promotion state fails closed.

Missing required evidence fails closed.

Unknown applicability fails closed.

Integrity failure fails closed.

Contradictory unresolved evidence fails closed.

Unsupported schema or rule version fails closed.

## Nonclaims

This ADR does not claim:

- that existing claims are demonstrated;
- that existing evidence is sufficient;
- that cross-register promotion is implemented;
- that controlled-document completeness is confirmed;
- that NodeSupervisor implementation is authorized;
- that STM32F401 physical validation is complete;
- that production readiness is achieved;
- certification.

## Architecture invariant

Observation != Evidence != Assessment != Adjudication != Authority != Actuation

AI/advisory != authority != actuation

Evidence existence != claim truth

Claim demonstration != authority
