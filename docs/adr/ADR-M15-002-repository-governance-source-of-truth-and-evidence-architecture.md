# ADR-M15-002 — Repository Governance, Source of Truth and Evidence Architecture

Status: ACCEPTED

Date: 2026-08-29

Decision owners: Guardian F401 maintainers and authorized human adjudicators

Related decisions:
- ADR-M14-001 — Canonical Envelope and Closed Payloads
- ADR-M15-001 — Node Identity and Signed Assurance Messages

Supersedes: None

Implementation status: NOT IMPLEMENTED

## Context

Guardian F401 contains firmware, protocol specifications, architecture decisions, validation documents, tests, CI workflows, release evidence and historical publications.

Relevant governance statements currently exist across multiple artifacts. Their presence does not establish a single mechanically enforceable authority model. Current implementation, documentation, evidence and historical records can describe different temporal states without an explicit repository-wide classification and precedence contract.

The repository currently has no `governance/` directory and none of the following proposed registers exists:

- `governance/project-state.yaml`
- `governance/requirements-register.yaml`
- `governance/capability-register.yaml`
- `governance/claims-register.yaml`
- `governance/evidence-register.yaml`
- `governance/document-register.yaml`

Therefore, repository-wide machine-readable governance is `NOT_DEMONSTRATED`.

Implementation code must not become the Source of Truth merely because it executes. Narrative documentation must not become implementation evidence merely because it states a requirement. Historical evidence must not be interpreted as the current project state without an explicit current-state link.

## Problem statement

Guardian requires an explicit architecture that determines:

1. which artifact has authority for each type of statement;
2. how requirements, capabilities, claims, evidence and documents are identified;
3. how current, planned, historical and unproven states are distinguished;
4. how conflicts between artifacts are detected and adjudicated;
5. how a human decision becomes an authorized repository change;
6. how CI enforces coherence without silently making architectural decisions;
7. how published releases and external records remain immutable.

Without this architecture, a true statement in one artifact can be interpreted outside its scope or temporal context, and a claim can exceed the evidence that supports it.

## Decision drivers

- Architecture before implementation.
- Evidence before claims.
- Explicit human authority.
- Fail-closed handling of ambiguity.
- Machine-verifiable traceability.
- Preservation of historical evidence.
- Separation of product, protocol and semantic-schema versions.
- Least privilege for automation.
- Independent rollback and review.
- No automatic conversion of memory, intent, implementation or simulation into demonstrated fact.

## Decision

Guardian will adopt a repository governance architecture consisting of:

1. an explicit authority hierarchy;
2. classified repository artifacts;
3. six machine-readable governance registers;
4. stable identifiers and traceability links;
5. bounded automated validation;
6. explicit human adjudication;
7. protected pull-request change control;
8. immutable treatment of published evidence.

This ADR authorizes the architecture and the subsequent design of schemas and validators. It does not declare the registers, validators or CI enforcement implemented.

## Authority model

Guardian does not use one universal precedence list for every type of dispute. Three independent authority dimensions apply.

### Normative authority

Normative authority determines intended requirements, permitted behavior and architectural constraints.

Authority belongs to an artifact's declared classification and scope. Merely existing in `repository/main` does not make an artifact normative. Implementation, historical material, tests and evidence retain their declared classifications even when stored in `main`.

The effective normative sources are:

1. controlled and approved Project Context for authorizing architectural direction;
2. applicable normative documents in protected `repository/main`;
3. accepted ADR for its explicitly declared decision scope;
4. validated machine-readable policies and registers after their implementation and activation.

Controlled Project Context may authorize a proposed change, but it cannot directly change effective repository state. Effective repository state changes only after the applicable protected pull request is validated and merged.

When an accepted ADR changes a normative rule, it authorizes reconciliation work. Until affected normative documents and registers are reconciled, the conflict must remain explicit and fail closed. The ADR must not silently rewrite the meaning of another artifact.
### Evidentiary authority

Evidentiary authority determines what has been demonstrated within a declared scope:

1. immutable evidence bound to an exact artifact, commit or release;
2. reproducible validation results;
3. implementation and test artifacts;
4. diagnostic or derived reporting.

A normative statement is not evidence that its requirement has been implemented. Implementation is not evidence beyond the validated conditions and scope.

### Change authorization

Change authorization determines whether a proposed transition may enter the protected repository or an external publication channel:

1. recorded human adjudication;
2. protected pull-request review;
3. required CI validation;
4. explicit merge, release or publication authorization.

CI may enforce declared rules, but it cannot invent policy, resolve architectural ambiguity or replace human authorization.

### Controlled Project Context

Project Context has normative authority only when it is:

- explicitly identified;
- versioned or bound to a dated controlled artifact;
- approved by an authorized human;
- traceable to the applicable repository baseline;
- preserved as part of the decision record.

Conversation history, assistant memory, informal messages and unstored instructions are not authoritative Project Context.

Memory provides continuity only. It is never repository authority or evidence.
## Artifact classifications

Every governed document must be classifiable as one of:

- `NORMATIVE`
- `CURRENT_STATE`
- `ARCHITECTURE_DECISION`
- `REQUIREMENT`
- `IMPLEMENTATION`
- `VALIDATION`
- `EVIDENCE`
- `HISTORICAL`
- `RELEASE`
- `EXTERNAL_REFERENCE`
- `ADVISORY`

An artifact may have more than one relationship, but it must have one declared primary classification.

Historical artifacts remain preserved. Deposited release content and published evidence packages are treated as immutable for their demonstrated scope. If an external service permits metadata correction, that correction must be separately authorized, recorded and must not silently alter deposited content, evidence meaning, applicable commit or historical claims. Substantive correction requires a new version or superseding record with an explicit relationship to the earlier publication.

## Adjudication states

Governed assertions will use the following bounded states:

- `CONFIRMED`
- `PARTIAL`
- `NOT_DEMONSTRATED`
- `PLANNED`
- `HISTORICAL`
- `FALSE_POSITIVE`
- `OUT_OF_SCOPE`

`CONFIRMED` requires identified evidence and an applicable validation boundary.

`NOT_DEMONSTRATED` must not be converted to `CONFIRMED` solely because implementation, documentation or a positive test exists.

Unknown or conflicting state must fail closed to `NOT_DEMONSTRATED` or require human adjudication.

## State dimensions

Guardian keeps four state dimensions separate.

### Decision state

Allowed values include:

- `PROPOSED`
- `ACCEPTED`
- `REJECTED`
- `SUPERSEDED`

### Implementation state

Allowed values include:

- `NOT_IMPLEMENTED`
- `PARTIALLY_IMPLEMENTED`
- `IMPLEMENTED`
- `RETIRED`

### Evidence adjudication state

Allowed values include:

- `CONFIRMED`
- `PARTIAL`
- `NOT_DEMONSTRATED`
- `FALSE_POSITIVE`
- `OUT_OF_SCOPE`

### Lifecycle or temporal state

Allowed values include:

- `PLANNED`
- `CURRENT`
- `HISTORICAL`
- `RETIRED`

A state from one dimension must not be used as proof of a state in another dimension. In particular:

- `IMPLEMENTED` does not imply `CONFIRMED`;
- `ACCEPTED` does not imply `IMPLEMENTED`;
- `CURRENT` does not imply sufficient evidence;
- `HISTORICAL` does not imply invalid evidence for its original scope.

## Transitional governance

Acceptance of this ADR does not make the six registers, their schemas, validators or CI gate operational.

Until a register is implemented, validated and explicitly activated:

- existing applicable normative documents in protected `repository/main` remain the effective repository authority for their declared scope;
- accepted ADR govern only their explicit decision scope;
- conflicts remain visible and require human adjudication;
- missing machine-readable state remains `NOT_IMPLEMENTED`;
- repository-wide coherence remains `NOT_DEMONSTRATED`;
- no automation may infer missing register entries.

Until `governance/claims-register.yaml` is operational, every new or materially changed public claim requires:

- exact proposed wording;
- identified evidence;
- declared evidence boundary;
- applicable commit or release;
- limitation review;
- explicit human approval.

The absence of a claims register is not permission to publish an unbounded claim.

A register becomes operational only after:

1. its schema is documented;
2. its initial content is reviewed;
3. deterministic validation passes;
4. negative tests pass;
5. its activation is explicitly adjudicated;
6. the protected change is merged.

## Required registers

### Project-state register

`governance/project-state.yaml` will identify:

- active product version;
- implemented wire-protocol version;
- semantic-schema versions;
- current baseline commit;
- active workstreams;
- workstream status;
- current-state documents;
- accepted ADR;
- release state;
- known open gates.

### Requirements register

`governance/requirements-register.yaml` will identify:

- stable requirement ID;
- normative source;
- requirement text or canonical reference;
- scope;
- status;
- verification method;
- linked capability;
- linked evidence;
- linked decision;
- change history.

### Capability register

`governance/capability-register.yaml` will identify:

- stable capability ID;
- implementation state;
- authorization state;
- supported platform;
- security boundary;
- dependencies;
- associated requirements;
- tests;
- evidence;
- limitations.

Implementation state and demonstrated state must remain separate.

### Claims register

`governance/claims-register.yaml` will identify:

- stable claim ID;
- exact allowed wording;
- prohibited overclaims;
- evidence boundary;
- applicable commit or release;
- status;
- owner;
- review and expiry conditions.

No public claim may exceed its registered evidence boundary.

### Evidence register

`governance/evidence-register.yaml` will identify:

- stable evidence ID;
- evidence category;
- source;
- producer;
- commit or release association;
- creation time;
- cryptographic digest;
- validation method;
- limitations;
- immutability or retention status.

Evidence categories must distinguish at least:

- `VENDOR_SPECIFICATION`
- `SIMULATION_EVIDENCE`
- `GUARDIAN_GENERATED_EVIDENCE`
- `PHYSICAL_VALIDATION`
- `INDEPENDENT_EXTERNAL_EVIDENCE`

One category must not be inferred automatically from another.

### Document register

`governance/document-register.yaml` will identify:

- stable document ID;
- repository path or external locator;
- classification;
- authority scope;
- lifecycle state;
- applicable product version;
- applicable wire-protocol version;
- applicable semantic-schema version;
- supersession relationship;
- owner;
- last reviewed commit;
- historical or current-state designation.

## Register-set consistency and atomicity

The six governance registers form one logically related governance state.

Validation must evaluate the complete register set from one exact Git commit. A validator must not combine register content from different commits, branches, worktrees or partially updated files.

The protected commit is the atomic repository-state unit. A change affecting cross-register relationships must update every affected register in the same protected change.

Individual schema validity is insufficient. Validation must also enforce:

- cross-register referential integrity;
- identifier uniqueness;
- compatible schema versions;
- consistent lifecycle states;
- consistent product, protocol and semantic-schema scope;
- absence of dangling references;
- absence of contradictory authority or claim relationships.

A partially updated or internally contradictory register set must fail closed.

## Schema evolution and downgrade protection

Every machine-readable register must declare a `schema_version`.

For every supported schema version, Guardian must define:

- the authoritative schema;
- compatible reader behavior;
- required and optional fields;
- migration rules;
- validation rules;
- unsupported-version behavior;
- downgrade restrictions.

Unknown schema versions must be rejected unless an accepted compatibility decision explicitly authorizes them.

A newer register must not be silently interpreted using an older schema. Migration must preserve stable identifiers, provenance, adjudication history and supersession relationships.

A schema-version change does not itself change product, wire-protocol or semantic-schema versions.

## Version separation

The governance architecture must keep these version domains independent:

- `PRODUCT_VERSION`
- `PROTOCOL_WIRE_VERSION`
- `SEMANTIC_SCHEMA_VERSION`
- `DOCUMENT_SCHEMA_VERSION`
- `EVIDENCE_PACKAGE_VERSION`

A product release may continue implementing an existing wire-protocol version.

A change to a frozen wire invariant requires a new wire-protocol version and a documented compatibility decision.

A governance-schema revision does not itself change product or wire-protocol versions.

## Traceability rules

Every governed relationship must use stable identifiers rather than filenames alone.

The minimum traceability chain is:

`Requirement -> Decision -> Capability -> Implementation -> Validation -> Evidence -> Claim`

A relationship may be absent while work is incomplete, but its absence must remain visible and must not be silently inferred.

Governed identifiers must be:

- unique within their declared namespace;
- immutable after assignment;
- never reused for another object;
- retained after retirement;
- resolvable to an authoritative record;
- protected against silent reassignment;
- validated for referential integrity.

Renaming or moving a file must not change the identity of its governed object.

Deletion of an identifier is prohibited when another governed object references it. Retirement or supersession must preserve the identifier and its history.

Circular authority is prohibited. Evidence cannot authorize itself, implementation cannot declare its own normative correctness, and a claim cannot establish the sufficiency of its supporting evidence.
## Conflict handling

When two governed artifacts conflict:

1. identify their classifications and scopes;
2. identify their applicable versions and commits;
3. determine whether either artifact is historical;
4. apply the authority hierarchy;
5. collect applicable evidence;
6. record the conflict;
7. require human adjudication when the conflict cannot be resolved mechanically;
8. fail closed until adjudication.

Automation may detect a conflict but must not invent the resolution.

## External evidence availability and interpretation

A registered cryptographic digest establishes the identity of observed bytes under the declared digest algorithm. It does not independently establish:

- availability;
- authenticity of the original producer;
- authority;
- completeness;
- sufficiency;
- correctness;
- applicability;
- interpretation.

External evidence must record its locator, producer, retrieval date, digest algorithm, digest value, applicable scope and known limitations.

If external evidence cannot be retrieved or independently preserved, its availability state must be explicit. An unavailable artifact must not be newly adjudicated as `CONFIRMED` solely because a previously recorded digest exists.

Previously frozen adjudications may remain historical for their original scope, but their evidence availability limitation must remain visible.

## Human adjudication

Human authorization is required for:

- accepting, rejecting or superseding an ADR;
- changing normative authority;
- expanding physical or logical authority;
- approving a new or materially changed public claim;
- changing a requirement's adjudicated state;
- accepting evidence as sufficient for a bounded claim;
- activating a governance register;
- merging a protected change;
- creating a tag or release;
- publishing to Zenodo;
- creating or associating a DOI;
- updating ORCID.

An authorization record must identify:

- the authorizing person or authorized role;
- the exact authorized action;
- the repository, artifact, commit, pull request or release in scope;
- the decision timestamp;
- the decision result;
- applicable conditions or limitations;
- whether the authorization was exercised;
- the resulting artifact or operation identifier.

Authorization is action-specific and scope-bound. Approval for review, commit, push, pull-request creation, merge, release, Zenodo publication, DOI action or ORCID update must not be reused as authorization for another action.

Human approval is a decision record. It is not a substitute for evidence.
## CI enforcement

A future governance validator must fail when:

- a referenced identifier does not exist;
- a current-state document is behind the governed product state;
- product and wire-protocol versions are conflated;
- an implemented capability remains described as future in a governing current-state artifact;
- a claim lacks sufficient linked evidence;
- evidence lacks required provenance or digest information;
- a historical document is treated as current;
- a normative capability lacks a Source of Truth;
- a requirement lacks required traceability;
- authority expands without an accepted decision;
- a platform claim lacks vendor or physical evidence classification;
- register schemas are invalid;
- cross-register relationships are inconsistent;
- an unknown or downgraded schema version is encountered.

Required-check semantics are fail closed.

For a required governance check:

- `success` is the only passing conclusion;
- failure, cancellation, timeout, action error or missing execution does not pass;
- skipped or neutral execution must not satisfy the protected requirement;
- the workflow must be executable for every pull request within its protected scope;
- workflow triggers and branch-protection requirements must remain coherent.

CI results must be associated with the exact candidate HEAD commit.

A pull request validated against an obsolete base must be updated and fully revalidated when protected `main` changes in a way that can affect the candidate result.

Stale approvals and stale check results must not authorize merge of a different candidate HEAD.

The validator must produce deterministic machine-readable results and actionable diagnostic text.

CI enforcement remains `NOT_IMPLEMENTED` until schemas, validators, negative tests and a required protected check exist.

## Change management## Change management

Governance changes must use:

1. discovery;
2. evidence collection;
3. architecture review;
4. requirements review;
5. human readability;
6. Devil’s Advocate review;
7. Technical Destruction review;
8. human adjudication;
9. protected pull request;
10. CI validation;
11. human merge authorization;
12. evidence freeze.

Direct publication is not part of a normal commit or merge.

A commit, push, pull request or merge must not automatically imply:

- a GitHub release;
- a Zenodo publication;
- DOI creation;
- an ORCID update.

Those actions require separate authorization and validation.

## Security properties

The governance architecture preserves:

- least privilege;
- read-only discovery by default;
- separation of advisory information from authority;
- explicit authorization boundaries;
- immutable historical evidence;
- fail-closed ambiguity handling;
- traceable claims;
- protected change paths.

This ADR does not expand firmware, cryptographic, node or physical-actuation authority.

## Consequences

### Positive

- Repository authority becomes explicit.
- Historical and current-state documents can coexist safely.
- Claims can be bounded mechanically.
- Missing evidence remains visible.
- CI can enforce coherence deterministically.
- Future ST and NXP platforms can share the same governance model.
- Release and publication boundaries become auditable.

### Negative

- Additional schemas, tooling and maintenance are required.
- Existing documents must be inventoried and classified.
- Some current claims may be reduced or marked `NOT_DEMONSTRATED`.
- Migration cannot be completed safely in a single unreviewed change.
- CI complexity will increase.

## Alternatives considered

### Continue with distributed narrative governance

Rejected because authority and temporal state remain difficult to verify mechanically.

### Treat implementation as the Source of Truth

Rejected because implementation cannot independently define requirements, allowed claims or architectural intent.

### Create one combined governance file

Rejected because project state, requirements, capabilities, claims, evidence and documents have different lifecycles and validation rules.

### Publish current evidence directly as the governance baseline

Rejected because publication provides immutability, not repository authority or semantic coherence.

## Implementation sequence

The implementation will be divided into independently reviewed changes:

1. accept this ADR;
2. define schemas and stable identifier conventions;
3. create the document register;
4. create the project-state register;
5. create the requirements register;
6. create the capability register;
7. create the evidence register;
8. create the claims register;
9. classify and migrate existing artifacts;
10. implement deterministic validators;
11. implement negative and hostile tests;
12. add required CI enforcement;
13. freeze evidence;
14. adjudicate `R2-001`.

The order after schema definition may change only through documented review.

## Validation requirements

Before this ADR may become `ACCEPTED`, it must pass:

- repository consistency review;
- Human Readability;
- Devil’s Advocate;
- Technical Destruction;
- scope and overclaim review;
- explicit human adjudication;
- protected pull-request validation.

Before implementation may become `IMPLEMENTED`, Guardian must additionally demonstrate:

- six valid registers;
- documented schemas;
- deterministic validation;
- negative tests;
- hostile tests;
- required CI enforcement;
- conflict detection;
- evidence and claim boundary checks.

## Current adjudication

At creation of this draft:

- ADR decision: `PROPOSED`
- Six governance registers: `NOT_IMPLEMENTED`
- Governance schemas: `NOT_IMPLEMENTED`
- Governance validator: `NOT_IMPLEMENTED`
- Required governance CI gate: `NOT_IMPLEMENTED`
- Repository-wide machine-readable coherence: `NOT_DEMONSTRATED`
- `R2-001`: `NOT_DEMONSTRATED`

## Out of scope

This ADR does not:

- implement the six registers;
- select final ST or NXP devices;
- implement semantic interoperability;
- modify the Guardian wire protocol;
- modify firmware behavior;
- expand authority;
- demonstrate physical actuation enforcement;
- implement cryptography or entropy;
- claim certification, conformance or production readiness;
- create a release, DOI, Zenodo publication or ORCID record.

## Draft revision history

### DRAFT.v1

SHA-256: `1DA254551D9C1217E97F868A184FC3F7B20C61A67A962ECB1817E6BECE39768F`

Human Readability: `PARTIAL`

### DRAFT.v2

SHA-256: `2321E3B8BF5A3A6DD17213D983A32FA060F6842B9A38AA5A37C7AB8F4601F2B4`

Human Readability: `PASS`

Devil's Advocate: `PARTIAL`

### DRAFT.v3

SHA-256: `BEF5B76633FE2E166466EC71B5B23F80388C197D8DD40FFA0836F664A89D5DA3`

Human Readability: `PASS`

Devil's Advocate: `PASS`

Technical Destruction: `PARTIAL`

Findings:

- cross-register atomic consistency was undefined;
- schema migration and downgrade behavior were undefined;
- required-check failure semantics were incomplete;
- stale base and stale validation behavior were incomplete;
- external evidence digests could be overinterpreted.

### DRAFT.v4

Changes are limited to the five DRAFT.v3 Technical Destruction findings.

No governance register, schema, validator or CI gate is represented as implemented.
## Candidate provenance

Source draft: `DRAFT.v4`

Source SHA-256: `0E120728177B7DD7D586A8162507104DF3B78B6AD201821095E540FC03E645C4`

Candidate transformation:

- no architectural content changed;
- review outcomes materialized;
- human adjudication remains pending;
- repository implementation remains `NOT_IMPLEMENTED`.

## Review record

Human Readability: PASS

Devil's Advocate: PASS

Technical Destruction: PASS

Human adjudication: APPROVED — Antonio José Socorro Marín — 2026-08-29