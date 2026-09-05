# ADR-M16-002 — Versioned Semantic Contract Architecture

Status: CANDIDATE

Milestone: M16

Decision Type: Architecture / Governance / Multi-Platform Assurance

Depends on:
- ADR-M16-001 — Heterogeneous Dual-Node Supervision
- ADR-M15-002 — Repository Governance Source of Truth and Evidence Architecture
- ADR-M15-003 — Governance Register Serialization, Schema Versioning and Identifier Namespaces
- Guardian NodeLink M16 protocol and F401 adapter candidates

## Context

Guardian is evolving from a single-node STM32F401 system into a heterogeneous
multi-node architecture. Future deployments may contain multiple chip families,
multiple revisions of the same chip, different firmware versions, different
protocol versions and different semantic interpretations active at the same time.

A common wire protocol is necessary for interoperability but is not sufficient
for correct interpretation.

A node may correctly parse or emit a field while assigning a meaning that is
different from another node because of chip family, silicon revision, firmware,
platform implementation, protocol version, enabled capability set, security
configuration or system role.

Therefore:

protocol compatibility != semantic compatibility

same field name != same meaning across versions

same chip family != same semantic profile

software version change != automatic semantic version change

value without required version context = semantically incomplete

## Decision

Guardian SHALL introduce a Versioned Semantic Contract Architecture.

Interpretation is separated into three layers.

### 1. Wire Grammar

Wire Grammar defines representation and transport: framing, field encoding,
message type, serialization, lengths, integrity fields and protocol version.

Wire Grammar compatibility SHALL NOT be treated as proof of semantic
compatibility.

### 2. Platform Grammar

Platform Grammar defines what states, fields, commands, errors, timing
conditions, capabilities and security properties mean for a particular
platform/version context.

A Platform Grammar may depend on:
- chip family;
- silicon/hardware revision;
- boot or platform revision;
- firmware version;
- protocol version;
- semantic profile version;
- security configuration;
- system role.

Two instances of the same chip family MAY legitimately use different Platform
Grammars.

### 3. System Semantics

System Semantics defines what Guardian as a system may infer or do from the
platform-local meaning.

System Semantics includes trust decisions, supervision decisions,
compatibility, authority, advisory interpretation, actuation eligibility,
bounded failure behavior and recovery.

The existing invariant remains:

AI/advisory != authority != actuation

## Semantic profiles

Each supported platform SHALL identify an applicable semantic profile or
equivalent controlled contract when version-dependent meaning exists.

The effective interpretation context is:

Node Identity
+ Chip Family
+ Hardware Revision
+ Platform/Firmware Version
+ Protocol Version
+ Semantic Profile
+ System Role
+ Compatibility Contract

A profile SHALL NOT retroactively reinterpret historical observations that were
governed by an earlier profile.

## Semantic change boundaries

A material change in meaning is an architectural change even if the binary
encoding or field name is unchanged.

A semantic change boundary SHALL identify, as applicable:
- prior semantic profile;
- new semantic profile;
- affected chip/platform;
- affected firmware/protocol ranges;
- effective transition condition;
- compatibility impact;
- migration rule;
- rollback rule;
- ADR or controlled decision;
- supporting evidence;
- validation evidence.

Historical meaning remains immutable.

Runtime processing may resolve directly to the applicable current semantic
profile. It does not need to replay every historical profile on every message.
The historical ledger remains available for audit, forensic reconstruction,
migration, rollback adjudication and compatibility analysis.

## Multi-version fleet

Guardian SHALL permit the possibility that two nodes using the same chip family
operate with different firmware revisions or semantic profiles.

Interpretation is therefore node-specific and version-bound.

The system SHALL NOT silently normalize distinct profiles into a common meaning.

## Compatibility

Compatibility SHALL be explicit and version-bound.

Protocol-compatible nodes may still be semantically incompatible.

Unknown, ambiguous, unsupported or historically impossible version/profile
combinations SHALL NOT be guessed for trust-, authority-, supervision-,
safety- or actuation-relevant decisions. They SHALL resolve through a
specifically defined bounded fail-safe/fail-closed path.

## Upgrade and downgrade

An upgrade SHALL NOT silently redefine semantics.

A software downgrade SHALL NOT automatically authorize a semantic rollback.

Semantic transition and rollback require explicit compatibility rules.

## Historical semantic ledger

Guardian SHALL preserve sufficient evidence to reconstruct:

Evidence
→ Context
→ Previous Semantics
→ Decision
→ Semantic Change Boundary
→ New Semantics
→ Implementation
→ Validation
→ Documentation

Semantic history SHALL NOT be rewritten merely to make current CI or
validation pass.

## Integrity and semantic continuity

Guardian distinguishes:

Integrity:
Is this the exact artifact that was recorded?

Semantic continuity:
Does the artifact still mean what it meant at the governed decision boundary?

Hash integrity alone is not proof of semantic correctness, truth, approval,
completeness or compatibility.

## Governance status semantics

registered != approved

registered != implemented

registered != validated

registered != evidence-present

Registration governs identity/integrity. Status escalation requires its own
evidence and decision boundary.

## Semantic drift

Unauthorized semantic drift is a failure condition.

SEMANTIC DRIFT = FAIL

A semantic change requires explicit governance. Silent reinterpretation by a
validator, migration tool, AI component, adapter or runtime layer is prohibited.

## Consequences

Before implementing a new chip/platform integration, Architecture Before
Implementation requires identifying its Platform Grammar and the version
boundaries that may alter meaning.

M16 NodeSupervisor implementation remains downstream of this architecture.

This ADR is a CANDIDATE. It does not claim that semantic profile runtime
resolution, compatibility matrices, hardware-version discovery or cross-chip
semantic enforcement are implemented.
