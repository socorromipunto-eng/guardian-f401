# Guardian Controlled Document Governance Policy

## 1. Purpose

This policy defines the controlled-document authority model for Guardian F401.
It governs inclusion, identity, versioning, integrity, lifecycle, authority,
supersession, conflict resolution, completeness, and consumption of controlled
documentation.

This policy is architecture-before-implementation governance. It does not itself
register documents or authorize NodeSupervisor implementation.

## 2. Normative principles

The following distinctions are mandatory:

- document location is not registration;
- registration is not approval;
- approval is not implementation;
- implementation is not validation;
- validation is not evidence presence;
- newest is not necessarily current;
- current is not necessarily authoritative outside its governed scope;
- superseded does not mean deleted;
- historical does not mean invalid;
- generated does not mean normative.

## 3. Controlled-document inclusion

A document becomes controlled only through explicit governed inclusion.

Repository path, file extension, filename, directory name, commit recency, or
tool generation must never independently create controlled status.

Blanket registration of `docs/**` is prohibited.

## 4. Document classes

The minimum classes are:

1. ADR
2. ARCHITECTURE
3. SPECIFICATION
4. GOVERNANCE
5. EVIDENCE
6. OPERATIONAL
7. INFORMATIONAL

A class is descriptive. Authority requires separate adjudication.

## 5. Document identity model

A governed document version must have, at minimum:

- stable document identity;
- document class;
- explicit version or revision identity;
- repository path or governed content location;
- exact content integrity hash;
- lifecycle status;
- authority status;
- governed scope;
- effective boundary or applicability context where required;
- supersession relationship where applicable.

Identity, version, path, and hash are distinct concepts.

For repository-controlled documents, the content integrity hash is computed from
the canonical Git blob bytes at `source_commit:path`.

The following distinctions are normative:

CANONICAL_DOCUMENT_IDENTITY = SHA256(Git blob at source_commit:path)

WORKTREE_REPRESENTATION != CANONICAL_DOCUMENT_IDENTITY

CRLF_LF_CHECKOUT_TRANSFORMATION != DOCUMENT_IDENTITY_CHANGE

DIRTY_WORKTREE != HISTORICAL_CONTENT_IDENTITY

Current repository coherence must be evaluated against the canonical Git blob at
the current authoritative repository state. Local worktree dirtiness must be
evaluated separately.

A platform-specific checkout transformation must not alter governed document
identity when the underlying canonical Git blob is unchanged.

## 6. Lifecycle model

Allowed lifecycle concepts include:

- DRAFT
- CANDIDATE
- ACCEPTED / APPROVED
- SUPERSEDED
- DEPRECATED
- WITHDRAWN

Lifecycle transitions must be explicit and governed.

Silent escalation is prohibited.

A registered document must not become APPROVED, IMPLEMENTED, VALIDATED, or
EVIDENCE_PRESENT merely through registration.

## 7. Normative authority model

Normative authority requires explicit authority adjudication.

For a document to govern implementation behavior, the applicable version must be:

- controlled;
- registered;
- current for the governed scope;
- authoritative;
- integrity-valid;
- version-bound;
- non-conflicting.

If any required property is unknown, authority resolution fails closed.

## 8. Versioning and historical meaning

Historical governed versions remain reconstructible.

A newer version must not reinterpret prior meaning silently.

Where semantics change materially, the new version must establish an explicit
change boundary and preserve the meaning and applicability of prior versions.

`CURRENT != NEWEST` is a normative rule.

## 9. Supersession, deprecation, and withdrawal

A superseded, deprecated, or withdrawn document remains historically
discoverable and integrity-verifiable.

Supersession must identify the relationship between prior and successor versions
or explicitly declare that no successor exists.

Deleting history to simplify the current state is prohibited.

## 10. Conflict resolution

If multiple current authoritative documents govern the same scope and conflict:

- authority resolution becomes AMBIGUOUS;
- implementation decisions are blocked;
- automated "latest wins" behavior is prohibited;
- AI or advisory tooling may analyze the conflict but may not resolve authority;
- human adjudication must create a controlled resolution.

## 11. Generated and informational material

Generated reports, test output, logs, evidence bundles, README files, and
informational documents have no normative authority by default.

Generated evidence may support validation but may not self-promote into
normative authority.

## 12. Cross-register traceability

Controlled documentation may relate to:

- requirements;
- capabilities;
- claims;
- evidence;
- implementation artifacts.

Relationships must remain independently meaningful.

Requirement existence is not implementation evidence.
Implementation existence is not validation evidence.
Evidence existence is not automatic claim confirmation.

## 13. Completeness boundary

The document register must not claim coverage of every repository document.

Completeness applies only to the explicitly declared controlled-document scope.

The controlled-document completeness status may transition from
`NOT_DEMONSTRATED` to `CONFIRMED` only after all of the following are
mechanically demonstrated:

1. controlled scope is explicitly defined;
2. every in-scope controlled document is registered;
3. every registered identity is unique;
4. every registered content path/location resolves;
5. every registered integrity hash verifies;
6. document versions and lifecycle states are valid;
7. current-authority resolution is unambiguous;
8. supersession/deprecation relationships are valid;
9. no conflicting current normative authority remains unresolved.

Until then:

`CONTROLLED_DOCUMENT_COMPLETENESS=NOT_DEMONSTRATED`

## 14. NodeSupervisor documentary authority gate

NodeSupervisor must not consume normative semantic meaning from any document
that is:

- unregistered;
- non-current;
- non-authoritative;
- integrity-invalid;
- ambiguous;
- conflicting;
- outside its governed applicability scope.

Unknown document authority must not be guessed.

The documentary authority gate must be closed before NodeSupervisor
implementation is authorized.

## 15. AI and advisory boundary

AI may:

- search;
- compare;
- summarize;
- identify conflicts;
- recommend adjudication.

AI may not:

- promote document status autonomously;
- declare normative authority autonomously;
- resolve conflicting normative authority autonomously;
- convert evidence into authority;
- authorize actuation.

AI/advisory != authority != actuation.

## 16. Change management

Changes to this policy or to the controlled-document authority model require:

- explicit architectural adjudication;
- preservation of historical meaning;
- review of backward semantic impact;
- Human Readability review;
- Devil's Advocate review;
- Technical Destruction planning;
- controlled commit and validation.

## 17. Explicit nonclaims

This policy does not claim:

- complete controlled-document inventory;
- complete document register population;
- NodeSupervisor implementation readiness;
- physical F401 or MCXN947 validation;
- production readiness;
- certification.

## 18. Current governance state

At creation of this policy:

- controlled-document policy architecture: defined;
- controlled-document register completeness: NOT_DEMONSTRATED;
- document register population: deferred;
- NodeSupervisor documentary authority gate: BLOCKED;
- NodeSupervisor implementation authorization: NO.
