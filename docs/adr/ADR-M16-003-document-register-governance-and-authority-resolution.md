# ADR-M16-003 — Document Register Governance and Authority Resolution

## Status

ACCEPTED

## Decision scope

This ADR governs how Guardian F401 identifies, versions, registers, supersedes,
deprecates, resolves, and consumes controlled documents whose meaning may affect
requirements, trust, safety, supervision, authority, or actuation.

It does not populate the document register and does not assert completeness.

## Context

Guardian F401 now uses versioned semantic contracts across heterogeneous nodes,
firmware versions, protocol versions, semantic profiles, and system roles.
Document authority therefore cannot be inferred from repository location,
filename recency, or registration alone.

A document may exist in the repository without being controlled. A controlled
document may be registered without being current normative authority. A current
normative document may still not prove implementation, validation, or evidence.

The system must preserve historical meaning and must fail closed when normative
authority is missing, ambiguous, stale, conflicting, or integrity-invalid.

## Decision

The following distinctions are normative:

DOCUMENT LOCATION != REGISTRATION != CONTROLLED STATUS != NORMATIVE AUTHORITY

REGISTERED != APPROVED
REGISTERED != IMPLEMENTED
REGISTERED != VALIDATED
REGISTERED != EVIDENCE_PRESENT

CURRENT != NEWEST
SUPERSEDED != DELETED
HISTORICAL != INVALID
GENERATED != NORMATIVE BY DEFAULT

### Controlled-document inclusion

A document enters the controlled-document scope only through explicit governed
inclusion. Presence under `docs/**` is neither necessary nor sufficient.

Blind registration of `docs/**` is prohibited.

### Document classes

The minimum supported classes are:

- ADR
- ARCHITECTURE
- SPECIFICATION
- GOVERNANCE
- EVIDENCE
- OPERATIONAL
- INFORMATIONAL

Class does not grant normative authority by itself.

### Identity, version, and integrity

Document identity, document version, and content hash are distinct.

For repository-controlled documents, the canonical content identity is the
SHA-256 digest of the canonical Git blob bytes at `source_commit:path`.

A stable document identity may have multiple historical versions. Each governed
version must bind to exact canonical Git blob bytes through an integrity hash.

Local checkout representation is not document identity. Platform-specific
materialization such as CRLF/LF conversion must not create a different governed
document identity when the canonical Git blob is unchanged.

Current repository coherence must be checked against the canonical Git blob at
the current authoritative repository state. Local worktree dirtiness is a
separate repository-state condition and must be evaluated independently.

Therefore:

CANONICAL_DOCUMENT_IDENTITY = SHA256(Git blob at source_commit:path)

WORKTREE_REPRESENTATION != CANONICAL_DOCUMENT_IDENTITY

DIRTY_WORKTREE != HISTORICAL_CONTENT_IDENTITY

A canonical blob change requires a new integrity value and any re-adjudication
required by policy.

### Lifecycle and authority

Document lifecycle and normative authority are separate dimensions.

Lifecycle may include:

- DRAFT
- CANDIDATE
- ACCEPTED / APPROVED
- SUPERSEDED
- DEPRECATED
- WITHDRAWN

Implementation, validation, and evidence status are not document lifecycle
states and must not be inferred from them.

### Supersession and historical preservation

Supersession removes current authority only where explicitly declared. It does
not erase historical existence, historical meaning, provenance, or auditability.

A newer document must not silently reinterpret the meaning of an older governed
version.

### Conflict resolution

If two current authoritative documents conflict for the same governed scope,
authority resolution is AMBIGUOUS and implementation decisions are blocked
until explicit human adjudication resolves the conflict.

No implementation may select a document merely because it is newer.

### Generated material

Generated reports, test output, evidence bundles, README files, and informational
material have no normative authority by default.

Generated evidence may support validation or claims but cannot promote itself
into normative authority.

### NodeSupervisor authority gate

NodeSupervisor must not consume trust-, safety-, supervision-, semantic-, or
actuation-relevant normative meaning from a document unless the applicable
document version is:

- explicitly controlled,
- registered,
- current for the relevant scope,
- authoritative,
- version-bound,
- integrity-valid,
- non-conflicting.

Unknown or ambiguous document authority must resolve fail closed.

## Nonclaims

This ADR does not claim:

- controlled-document completeness,
- that all repository documentation is governed,
- that any existing document is currently normative merely because it exists,
- that this ADR grants authorization to implement NodeSupervisor,
- that physical validation is complete,
- that any generated artifact is normative,
- certification or production readiness.

## Consequences

Guardian gains a version-aware documentary authority model consistent with its
versioned semantic architecture.

The document register must later be extended or populated only through a
separate governed materialization step.

`CONTROLLED_DOCUMENT_COMPLETENESS` remains `NOT_DEMONSTRATED` until explicit
scope, registration, integrity, version, authority, and conflict invariants are
mechanically proven.

## Rejected alternatives

### Register every file under `docs/**`

Rejected. Repository path does not establish controlled or normative authority.

### Use newest document automatically

Rejected. Newest does not imply current or authoritative.

### Delete superseded documents

Rejected. This destroys historical provenance.

### Allow AI or tooling to resolve normative conflict autonomously

Rejected. Advisory analysis is not authority adjudication.

### Treat registration as approval

Rejected. Registration represents governed identity/integrity, not approval.

## Architecture invariant

Observation != Evidence != Recommendation != Authority != Actuation

AI/advisory != authority != actuation

Document existence != document authority
