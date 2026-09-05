# Guardian Engineering Operating Principles

Status: CONTROLLED PROJECT GUIDANCE

Purpose: durable project-context rules for architecture, implementation,
validation and future AI-assisted work. This document does not replace ADRs,
schemas, requirements, evidence or human adjudication.

## Source-of-Truth hierarchy

Use project evidence in this order:

Project Context
→ Repository
→ Controlled Documentation
→ Evidence
→ Decisions / ADRs
→ Implementation
→ Validation
→ Documentation updates

Memory and conversational context may assist continuity but SHALL NOT override
controlled repository evidence.

## Architecture before implementation

For a new chip, platform, authority path, security mechanism or semantic change,
define the architecture and boundaries before implementation.

## Evidence first

Unknown information remains unknown.

NOT_DEMONSTRATED is preferable to an unsupported PASS.

Hash integrity does not imply truth, approval, completeness or semantic
correctness.

## Versioned semantic contract

Guardian is a heterogeneous multi-chip system.

Never assume:

protocol compatibility == semantic compatibility

same field name == same meaning across versions

same chip family == same semantic profile

registered == approved

software version change == automatic semantic version change

Interpret trust-, authority-, supervision-, safety- and actuation-relevant
information using the applicable versioned context:

Node Identity
+ Chip Family
+ Hardware Revision
+ Firmware/Platform Version
+ Protocol Version
+ Semantic Profile
+ System Role
+ Compatibility Contract

Separate:

Wire Grammar
→ Platform Grammar
→ System Semantics

Material semantic changes require an explicit Semantic Change Boundary.

Historical semantics remain immutable. New meaning begins at the declared
boundary and does not retroactively reinterpret prior evidence.

Runtime processing may resolve to the current applicable profile without
replaying complete history. Historical profiles remain retained for audit,
forensic reconstruction, compatibility and rollback adjudication.

Unknown or unsupported semantic combinations SHALL NOT be guessed where trust,
authority, supervision, safety or actuation may be affected.

SEMANTIC DRIFT = FAIL

## Authority

AI/advisory != authority != actuation

Authentication, freshness, health, supervisor presence or successful parsing
do not independently grant physical authority.

## Change management

Before staging or publishing material changes:
- validate locally where possible;
- run negative/adversarial tests;
- perform Internal Audit;
- perform Human Readability review;
- perform Devil's Advocate review;
- perform Technical Destruction;
- review evidence and non-claims;
- obtain human adjudication.

Only then proceed to staging/commit/push/PR according to the authorized
boundary.

## Multi-version fleets

Multiple instances of the same chip family may operate with different firmware
or semantic profiles simultaneously.

Interpret semantics per node and per applicable profile.

Do not silently normalize a mixed-version fleet.

## Auditability

Preserve:

Evidence
→ Context
→ Previous Semantics
→ Decision
→ Semantic Change Boundary
→ New Semantics
→ Implementation
→ Validation
→ Documentation

The goal is rigorous, reproducible, conservative, adversarially reviewable
engineering aligned with selected high-assurance practices where applicable.
No certification, compliance or external approval is implied without formal
evidence.
