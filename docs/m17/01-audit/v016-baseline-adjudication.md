> Guardian F401 M17 candidate artifact.
>
> Status: CANDIDATE / NOT YET NORMATIVE.
>
> Registration does not grant approval.
> Documentation does not grant implementation.
> Implementation does not grant validation.
> Validation does not grant authority.
> Authentication does not grant authority.
> Authority does not automatically grant actuation.
>
> AI/advisory != authority != actuation.
>
> Unknown or ambiguous authority context fails closed.
>
> No certification, regulatory conformity, production-readiness,
> hardware-qualification or external-approval claim is made here.
# v0.16 Baseline Adjudication for M17

## Purpose

This file records the initial M17 adjudication of the v0.16.0 source baseline.

It is not a final security assessment.

## Confirmed strengths

- v0.16.0 has a signed and published release evidence record.
- authority and actuation are explicitly separated in semantic governance.
- authentication does not grant authority.
- semantic compatibility does not grant authority.
- remote node state does not grant local transitions.
- AI advisory does not grant authority.
- sequence-rejection host evidence exists for an accepted claim.
- unknown semantic context is governed fail-closed.
- historical release limitations are explicitly documented.

## Governance gaps

### G-M17-001 Requirement/claim/evidence reconciliation

The requirements register records all sixteen M16 requirements as
NOT_DEMONSTRATED.

At least one corresponding behavioral property is represented by a separate
accepted demonstrated claim and evidence record.

Required action:

reconcile requirement scope, claim scope and evidence applicability before any
status promotion.

### G-M17-002 Capability register

The capability register contains zero capabilities.

Required action:

define machine-readable capabilities before multi-node authority decisions.

### G-M17-003 Semantic version reconciliation

The F401 semantic profile records firmware version 0.15.0 while the project
release baseline is 0.16.0.

Required action:

adjudicate whether the existing profile is intentionally historical or whether
a new profile/change boundary is required.

No silent edit is permitted.

### G-M17-004 Semantic compatibility

F401-MCXN947 semantic compatibility remains UNKNOWN.

Required action:

canonical cross-node semantic vectors and explicit compatibility adjudication.

### G-M17-005 Persistent freshness

Cross-reboot freshness is not represented as implemented.

Required action:

persistent replay protection, epoch lifecycle and recovery semantics.

## Threat-model gaps

The current threat model covers malformed traffic, replay, rollback and
credential exposure.

It does not currently model the M17 distributed architecture.

Required additions include:

- partition;
- split brain;
- Byzantine or malicious authenticated peer;
- stale signed peer;
- quorum manipulation;
- unauthorized failover;
- identity cloning;
- semantic downgrade;
- supply-chain compromise;
- CI/build compromise;
- physical attacker;
- debug lifecycle;
- brownout and interrupted persistence;
- key compromise;
- evidence deletion, replay, reordering and wrong-domain signing.

Threat-model expansion is a release prerequisite for multi-node claims.

## Language-assurance gaps

Repository text does not currently establish Guardian CERT C adoption.

MISRA references observed in vendor CMSIS headers do not establish Guardian
MISRA conformance.

M17 shall establish an explicit secure-C assurance strategy.

A future memory-safe language may be evaluated for a new node, but language
choice never grants authority.

## Physical and production gaps

The published v0.16.0 release keeps physical validation, geometry, runtime
stack, storage ownership, backup deployment and production security open.

These gaps must either be closed with exact evidence or remain explicitly open
at v0.17 release.

## Initial disposition

The v0.16.0 baseline is accepted as the M17 development base.

It is not accepted as proof that the planned M17 distributed architecture
already exists.
