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
# M17 Program Charter

## Objective

M17 shall establish the architecture, requirements, implementation,
validation and evidence needed to demonstrate a bounded heterogeneous
multi-node Guardian research system.

## Existing nodes

Node 1:

STM32F401

Role:

DETERMINISTIC_REFERENCE_CONTROL_NODE

The F401 remains the deterministic reference/control authority unless a later
separately governed decision explicitly changes a bounded capability.

Node 2:

MCXN947

Role:

INDEPENDENT_SUPERVISORY_PEER

The MCXN947 may supervise, challenge, corroborate and preserve evidence within
its governed role.

It receives no automatic actuator authority.

## Planned additional roles

M17 plans three additional heterogeneous roles.

The exact devices are NOT SELECTED by this charter.

Candidate role A:

ST_HETEROGENEOUS_REFERENCE_OR_MEASUREMENT_NODE

Candidate role B:

SECURITY_WITNESS_AND_EVIDENCE_NODE

Candidate role C:

HIGH_COMPUTE_SIGNAL_PROCESSING_NODE

The exact part numbers require a technical candidate matrix and ADR.

## AI boundary

Integrated AI authority is not an M17 objective.

Any AI-related component remains advisory only.

Broader integrated AI belongs to a later milestone.

## Mandatory workstreams

- v0.16 source-of-truth reconciliation;
- requirement/claim/evidence reconciliation;
- capability model;
- distributed threat model;
- node identity;
- key lifecycle;
- signed evidence;
- persistent freshness;
- semantic interoperability;
- disagreement semantics;
- quorum semantics;
- partition behavior;
- split-brain prevention;
- bounded failover;
- recovery;
- firmware update resilience;
- supply-chain assurance;
- SBOM/provenance;
- static and dynamic analysis;
- composed fuzzing;
- physical hardware qualification;
- independent technical destruction;
- standards mapping;
- release evidence.

## Development assurance lifecycle

Architecture
-> Source of Truth
-> Requirements
-> Implementation
-> Positive Tests
-> Negative Tests
-> Hostile Tests
-> Technical Destruction
-> CI
-> Human Readability
-> Devil's Advocate
-> Evidence Freeze
-> Human Adjudication
-> Commit
-> Push
-> PR
-> Protected Main
-> Release Decision

No adjacent mutation boundary is implicitly authorized.
