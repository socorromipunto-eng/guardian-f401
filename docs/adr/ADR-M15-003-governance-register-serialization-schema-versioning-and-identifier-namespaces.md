# ADR-M15-003: Governance Register Serialization, Schema Versioning and Identifier Namespaces

Status: ACCEPTED

Implementation status: NOT IMPLEMENTED

Date: 2026-08-30

Supersedes: None

Related decision: ADR-M15-002

## Reader summary

This ADR decides how future Guardian governance registers will be represented and linked. JSON is selected as the register format; stable identifiers will connect records; schemas and validators will later enforce structure. This ADR does not create those registers or prove their completeness.

The decision, the implementation and the evidence remain separate:

- format decision: JSON;
- register implementation: `NOT_IMPLEMENTED`;
- schema dialect selection: `PENDING`;
- validation library selection: `PENDING`;
- CI enforcement: `NOT_IMPLEMENTED`;
- governance completeness: `NOT_DEMONSTRATED`.

## Context

ADR-M15-002 establishes the architectural need for repository-governed, machine-readable registers covering project state, requirements, capabilities, claims, evidence and documents. ADR-M15-003 refines that architecture by selecting JSON instead of the provisional `.yaml` filenames discussed in ADR-M15-002. It does not supersede ADR-M15-002's authority model, state dimensions, evidence rules or change-authorization requirements. It also does not, by itself, implement a register, schema or validator.

Repository discovery at baseline `7869ece662804a3baea84701a171ef4b5375372d` found:

- nine GitHub workflow files using the `.yml` extension;
- two machine-readable JSON documents;
- no existing `governance/` directory;
- no governance register implementation;
- no declared YAML parser dependency;
- existing `schema_version` usage in JSON documents;
- an existing stable policy identifier in `assurance/policies/m15-authority-boundary.json`;
- existing Python validation tooling, but no governance schema validator.

The absence of registers is not evidence of repository damage. It means the architecture accepted by ADR-M15-002 remains to be implemented.

## Problem

Governance data cannot become authoritative merely by being placed in a machine-readable file. Within the scope assigned by ADR-M15-002, a validated register may become a normative repository source, but successful validation does not prove physical truth, evidence sufficiency or human authorization. Without a controlled serialization profile, schema identity, stable identifiers, reference rules and fail-closed validation, different tools may interpret the same bytes differently or silently accept incompatible states.

The design must prevent at least these failure modes:

- duplicate object keys interpreted differently by different parsers;
- invisible encoding differences, including a leading UTF-8 byte-order mark;
- silent acceptance of unknown or downgraded schema versions;
- unstable identifiers coupled to filenames or array positions;
- dangling or ambiguous cross-register references;
- partial updates that combine records from different repository states;
- hashes misrepresented as authorization or semantic correctness;
- implementation state confused with evidence adjudication state;
- a validator or CI gate claimed before it exists and has been tested negatively.

## Decision

Guardian governance registers will use JSON under a deliberately restricted serialization and validation profile.

This decision selects an architectural direction. It does not create the registers, schemas, validator or CI gate.

Decision-state separation:

- JSON serialization format: `DECIDED`;
- stable identifier architecture: `DECIDED`;
- register files: `NOT_IMPLEMENTED`;
- schema dialect and validation library: `PENDING`;
- validator and CI gate: `NOT_IMPLEMENTED`;
- evidence sufficiency and completeness: `NOT_DEMONSTRATED`.

### Planned register set

The following paths do not currently exist. They are planned implementation targets and must not be cited as present evidence:

The initial register paths are planned as:

- `governance/project-state.json`
- `governance/requirements-register.json`
- `governance/capability-register.json`
- `governance/claims-register.json`
- `governance/evidence-register.json`
- `governance/document-register.json`

Schema documents are planned under:

- `governance/schemas/`

These paths remain `NOT_IMPLEMENTED` until created through a separately reviewed change.

## Serialization profile

Every governance register and schema document must:

- be encoded as strict UTF-8 without a byte-order mark;
- contain exactly one JSON root object;
- reject duplicate object keys;
- reject malformed Unicode and invalid UTF-8;
- reject NUL bytes;
- reject non-standard numeric values such as `NaN`, positive infinity and negative infinity;
- avoid relying on object-member ordering for meaning;
- use explicit fields rather than comments, because standard JSON has no comments;
- contain an explicit `schema_version` field;
- terminate cleanly without non-JSON trailing content.

Whitespace and member ordering may vary without changing the semantic record. A raw-file SHA-256 identifies exact bytes only; it does not by itself establish semantic equivalence.

### Byte identity and semantic identity

Guardian does not treat ordinary JSON serialization as canonical. Two documents may be semantically equivalent while having different byte hashes. Until a separately reviewed canonicalization profile exists, evidence must preserve both the exact byte hash and the validator's structured result; neither may be substituted for the other. JSON is selected for bounded parser behavior and repository fit, not because JSON is intrinsically secure.

### Resource and numeric limits

The implementation schema and parser contract must define fail-closed maximums for document bytes, nesting depth, object members, array elements, record count and string length. Limit violations are validation failures, not warnings.

Governance numbers must not depend on binary floating-point interpretation. Identifiers, digests, versions and counters use strings or bounded integers as defined by schema. The parser profile must reject negative zero, exponent notation where not explicitly allowed, fractional values for integer fields and integers outside the validator's demonstrated exact range.

## Parser behavior

The future validator must use parsing behavior that can reject duplicate object keys. A normal successful parse is insufficient if the selected library silently retains only the first or last duplicate.

The validator must fail closed when:

- the document cannot be decoded as strict UTF-8;
- the JSON cannot be parsed under the restricted profile;
- the schema version is absent, malformed, unsupported or disallowed;
- a required record identifier is missing or duplicated;
- a cross-register reference cannot be resolved;
- the register set does not represent one coherent repository state;
- a configured resource or numeric limit is exceeded;
- a filename, path or filesystem identity is ambiguous.

### Filesystem and path safety

Governance inventory paths must be repository-relative, use forward-slash separators and reject absolute paths, drive prefixes, empty segments, dot segments, parent-directory segments, NUL bytes and traversal after normalization. Validation must reject symbolic links and other indirections unless a later decision explicitly defines and tests them.

The validator must detect filename collisions under both case-sensitive and case-insensitive comparison. It must also reject unregistered files under the governance directory and missing files declared by the complete inventory.

## Schema identity and evolution

Each register will declare its schema version explicitly. A version number alone is insufficient to identify the rules applied. The future project-state binding must identify the applicable schema set by stable schema identifier, supported version and exact content digest. The precise JSON Schema dialect and validation library are not selected by this ADR and remain subject to implementation evidence.

Schema evolution must obey these rules:

1. A validator must not silently treat an unknown schema version as the latest known version.
2. A downgrade must not be accepted merely because an older schema can parse a subset of the fields.
3. A breaking semantic change requires a new schema version and a documented migration or compatibility decision.
4. Additive evolution is compatible only when the applicable schema and consumer behavior explicitly permit the new fields. A field affecting authority, security, state interpretation or evidence meaning must not be treated as safely ignorable by an older consumer.
5. Schema documents, validator code and register instances must be evaluated at one exact commit.
6. Historical records remain interpretable under the schema version applicable to their demonstrated scope.

No claim of JSON Schema conformance is made until the dialect, metaschema handling, library behavior and negative tests are fixed and demonstrated.

## Identifier namespace

Canonical governance record identifiers will use the form:

`guardian:<record-type>:<local-id>`

Examples are illustrative rather than implemented:

- `guardian:requirement:r2-001`
- `guardian:claim:r2-001-coherence-demonstrated`
- `guardian:evidence:pr-16-required-checks`
- `guardian:document:adr-m15-003`

Identifier rules:

- the canonical identifier is stable and must not depend on an array index;
- changing a filename must not silently change the record identity;
- identifiers are compared exactly after validation; consumers must not invent case folding or Unicode normalization;
- each canonical identifier must be unique within its record type and applicable register set;
- reuse of an identifier for a different meaning is prohibited;
- retirement preserves historical identity and does not permit semantic reassignment;
- aliases, if introduced, must be explicit, unambiguous and must not form cycles;
- an identifier syntax match does not prove that the referenced record exists or is authorized.

The permitted character set, maximum lengths and record-type registry will be fixed in the implementation schema and negative tests.

### Identifier allocation and collision control

The future identifier policy must define the authority allowed to allocate each record type. Creation must fail when an identifier already exists in the applicable history or when concurrent branches propose the same identifier for different meanings. Merge-time validation must detect collisions across the complete proposed register set. Branch-local uniqueness alone is insufficient.

## Cross-register references

Cross-register relationships must use canonical identifiers, not array positions or descriptive text.

The validator must eventually demonstrate at least:

- referenced records exist;
- reference types are allowed by the source field;
- required and maximum reference cardinalities are satisfied;
- prohibited reference cycles are rejected;
- lifecycle transitions follow an explicit state-transition table;
- claim-to-evidence links do not convert evidence existence into evidence sufficiency;
- document links identify the applicable version or commit where required;
- retired records cannot be used for a purpose disallowed by their lifecycle state.

Existence of a target identifier is insufficient when its record type, lifecycle state, cardinality or transition semantics are incompatible with the source field.

`IMPLEMENTED` does not imply `CONFIRMED`. A capability implementation state and a claim evidence-adjudication state remain separate dimensions.

## Register-set consistency and atomicity

The governance register set is evaluated as a unit at one Git commit. In plain language, every register, applicable schema and validator used for one decision must come from the same identified commit. A validator must not combine one register from the worktree with another from a different commit, release archive or external publication.

Final evidence must validate immutable Git blob and tree objects addressed by the exact commit, not a mutable worktree observed before or after another operation. Worktree validation may be used during development but cannot be substituted for final commit-bound evidence.

A change is accepted only if the complete proposed register set validates together. The project-state inventory must enumerate every required register, schema and validator input, and validation must reject both omissions and unexpected governance files. Updating one register while leaving a cross-dependent register inconsistent must fail closed. File-by-file success is not register-set acceptance.

The planned project-state record will bind the register set to an exact repository state and supported schema set. It must not certify itself merely by listing its own values. Trust begins with the protected Git commit, applicable branch-protection decision and independently executed validator; the project-state record describes that state but does not create its own authority.

The trusted expected schema-set identity must be anchored outside the register set it validates, for example by protected workflow configuration or another separately authorized root. A digest declared only inside the candidate register set cannot authenticate that same set. The detailed binding fields remain an implementation decision subject to adversarial review.

A content hash can detect byte changes when compared with a trusted expected value. A hash is not authorization: simultaneous modification of a document and its manifest can remain internally consistent while still being unauthorized.

Authorization therefore depends on repository protection, review, exact commit identity, semantic diff inspection and explicit human decision where required.

Changes to schemas, validator behavior, identifier-allocation rules, compatibility rules or normative allowed-value sets are governance-sensitive changes. A passing validator modified in the same pull request is not sufficient authorization. Such changes require an explicit semantic-diff classification and the applicable human approval before merge.

## External evidence

External evidence may be referenced by stable identifier, persistent URL, digest and applicable scope. External availability does not make the content authoritative, and later metadata changes must not silently change the interpretation of deposited bytes.

A reference must distinguish byte identity, provenance, retrieval status, applicable commit or release, and adjudicated evidentiary scope. A valid URL and matching digest do not demonstrate availability over time, authorized provenance or sufficiency for a claim. Unavailable evidence must produce an explicit machine-readable state rather than silent acceptance.

Zenodo publication, DOI creation and ORCID updates remain separate, explicitly authorized actions. A commit, push, pull request or merge does not automatically authorize any of those actions.

## Transitional governance

Until the register set, schemas, validator and CI gate are implemented and validated:

- existing normative repository documents retain their current authority according to ADR-M15-002;
- no nonexistent register may be cited as proof;
- manual evidence collection remains subject to explicit scope and commit identity;
- disagreements between documents must be adjudicated rather than hidden by a generated register;
- schema enforcement remains `NOT_IMPLEMENTED`;
- governance-register completeness remains `NOT_DEMONSTRATED`;
- `R2-001`: `NOT_DEMONSTRATED`.

## Implementation sequence

Implementation must occur through bounded, reviewable workstreams:

1. Define the restricted JSON parser contract and negative fixtures.
2. Select and pin the schema dialect and validation dependency.
3. Define schemas with stable identifiers and version rules.
4. Create the minimal register set from verified repository sources.
5. Implement cross-register and atomicity validation.
6. Emit structured validation evidence bound to the exact commit, tool and schema hashes.
7. Add negative and mutation tests that prove known violations are detected.
8. Integrate the validator into a protected CI workflow.
9. Perform Human Readability, Devil's Advocate and Technical Destruction review.
10. Adjudicate each claim separately; do not infer completeness from a passing gate.

## Structured validation evidence

The future validator must emit deterministic machine-readable evidence containing at least:

- evidence schema version;
- validator version and exact validator hash;
- commit and Git tree identity;
- policy and schema identifiers and hashes;
- complete input inventory and input hashes;
- result for every rule and record;
- resource-limit configuration;
- toolchain and runtime identity;
- limitations and unsupported cases;
- final aggregate result.

Human-readable PASS or FAIL text is diagnostic output, not the complete evidence record.

## CI self-verification

The protected workflow must run for every pull request subject to the required check. It must fail closed on tool failure, missing output, unsupported schema, incomplete inventory or surviving known mutation.

Negative fixtures and controlled mutations must demonstrate that the gate detects known violations and restores its temporary workspace exactly. A small, separately reviewed constitutional oracle must protect invariants such as prohibited authority expansion and required rule presence. The validator, schemas, fixtures and oracle must not all derive their expected behavior solely from the same mutable policy.

## Minimum validation requirements

Before schema enforcement can be represented as implemented, evidence must show:

- valid fixtures are accepted;
- invalid UTF-8 and BOM-bearing fixtures are rejected where required;
- duplicate keys are rejected;
- unknown and downgraded schema versions are rejected;
- missing, duplicate and malformed identifiers are rejected;
- dangling and type-invalid references are rejected;
- prohibited cycles are rejected;
- partial or mixed-commit register sets are rejected;
- the real repository tree passes;
- controlled mutations fail for the intended rule;
- the working tree is restored exactly after mutation testing;
- evidence identifies the exact commit, Git tree, validator, schemas and inputs;
- resource-exhaustion and numeric-boundary fixtures fail closed;
- path traversal, symlink, case-collision, omitted-file and unexpected-file fixtures fail closed;
- mutation tests detect a controlled relaxation of validator or policy behavior.

## Consequences

### Benefits

- one deterministic serialization family for governance registers;
- fewer parser dependencies than introducing YAML solely for registers;
- explicit separation of byte identity, semantic validity, evidence sufficiency and authorization;
- stable references across filenames and document movement;
- a defined path toward machine-verifiable R1/R2 coherence.

### Costs and limitations

- JSON is less convenient for long human commentary;
- duplicate-key detection requires deliberate parser configuration;
- schemas cannot determine whether every architectural concept was discovered;
- passing validation cannot establish physical behavior, certification or production readiness;
- register creation and maintenance add governance work;
- human adjudication remains necessary for meaning, sufficiency and authorization.

## Rejected alternatives

### YAML as the initial register format

Rejected for the first implementation because the repository has no declared YAML parser dependency for this purpose, YAML has a broader interpretation surface, and current machine-readable governance precedent is JSON. Existing GitHub workflow YAML is not evidence that application tooling has a controlled YAML parser.

This is a repository-specific engineering decision, not a claim that JSON is inherently secure. JSON still requires strict decoding, duplicate-key rejection, schemas, negative tests and controlled consumers.

### Markdown as the authoritative machine register

Rejected because prose structure is unsuitable as the sole deterministic input to strict schema and cross-reference validation. Markdown remains appropriate for human-readable architecture and explanation.

### Hash-only integrity

Rejected because a digest proves correspondence to bytes only when the expected digest and its authority are independently trusted. It does not prove authorization, correctness or semantic completeness.

### One monolithic register

Rejected because it would entangle distinct state dimensions and make ownership, review and evidence relationships harder to adjudicate. Atomic validation will provide coherence across the separated register set.

## Current adjudication

Architecture decision: PROPOSED

Guardian governance registers will use JSON: PROPOSED

Register files: NOT IMPLEMENTED

Schemas: NOT IMPLEMENTED

Validator: NOT IMPLEMENTED

CI enforcement: NOT IMPLEMENTED

Schema enforcement remains `NOT_IMPLEMENTED`.

Governance completeness: NOT_DEMONSTRATED

`R2-001`: `NOT_DEMONSTRATED`

No certification, regulatory conformance, physical-enforcement or production-readiness claim is created by this decision.

## Draft revision history

### DRAFT.v1

SHA-256: `B39C59EA748B462FFC2B4DF77FED026ED1B8B929F4482403B425D2B5ABAE531F`

Human Readability result: `PARTIAL`

DRAFT.v1 findings:

- relationship to ADR-M15-002 required explicit refinement language;
- decision, implementation and evidence states required clearer separation;
- normative authority required clearer limits;
- same-commit atomicity required a plain-language explanation;
- non-specialist readers required an initial summary;
- planned paths required stronger non-implementation notice.

### DRAFT.v2

SHA-256: `4D31AEB732BE684154DCF04C61D454CFDC12ADEC630014B0A4C52B52F23D909D`

Changes were limited to the six DRAFT.v1 Human Readability findings.

Human Readability result: `PASS`

Devil's Advocate result: `PARTIAL`

DRAFT.v2 findings:

- ordinary JSON byte identity could be confused with semantic identity;
- schema version alone did not bind the exact schema;
- identifier allocation and collision control were unspecified;
- same-commit consistency did not require all-register acceptance;
- project-state self-reference could become circular;
- governance-sensitive validator changes required explicit authorization;
- additive compatibility could hide security-relevant fields from older consumers;
- external references did not separate identity, provenance, availability and sufficiency;
- JSON selection could be misread as an intrinsic security claim;
- internally coherent register content could still be false or unauthorized.

### DRAFT.v3

SHA-256: `DD4FACC97513C423281857773BEF18C70A78449E0C889907B96A76B385E9BC78`

Changes were limited to the ten DRAFT.v2 Devil's Advocate findings.

Human Readability result: `PASS`

Devil's Advocate result: `PASS`

Technical Destruction result: `PARTIAL`

DRAFT.v3 findings:

- final evidence required immutable Git object validation rather than mutable-worktree observation;
- the register set required a complete inventory and rejection of unexpected files;
- parser resource and numeric limits were unspecified;
- Unicode, path, symlink and case-collision behavior required fail-closed rules;
- cross-register validation required type, cardinality, cycle and transition semantics;
- schema identity required a non-circular trust anchor;
- validation evidence required a deterministic machine-readable contract;
- CI required universal execution and fail-closed behavior;
- gate effectiveness required negative and mutation tests;
- a constitutional oracle had to remain independent of the mutable policy;
- simultaneous schema, validator and fixture relaxation required detection;
- platform-dependent governance filename equivalence required explicit testing.

### DRAFT.v4

SHA-256: `FCF0F13228B333305CE9A30DA43201664378EEECCB020EC66C80B5E64CB75F85`

Changes were limited to the twelve DRAFT.v3 Technical Destruction findings.

Human Readability result: `PASS`

Devil's Advocate result: `PASS`

Technical Destruction result: `PASS`

### CANDIDATE

Source DRAFT.v4 SHA-256: `FCF0F13228B333305CE9A30DA43201664378EEECCB020EC66C80B5E64CB75F85`

Candidate transformation:

- no architectural content changed;
- the three completed review outcomes were materialized;
- human adjudication was approved by Antonio Jose Socorro Marin on 2026-08-30;
- repository implementation remains `NOT_IMPLEMENTED`;
- no governance register, schema, validator or CI gate is represented as implemented.

## Human decision record

Candidate SHA-256: `4DCC0BADE1E69608A1E39BE97770E0548769370DB8F949368B8B1DE47981A71C`

Decision: `APPROVED`

Decision authority: Antonio Jose Socorro Marin

Decision date: 2026-08-30

Decision scope:

- accept ADR-M15-003 as architecture;
- keep repository implementation `NOT_IMPLEMENTED`;
- keep schema enforcement `NOT_IMPLEMENTED`;
- keep governance completeness `NOT_DEMONSTRATED`;
- keep `R2-001` as `NOT_DEMONSTRATED`;
- do not authorize release, Zenodo publication, DOI creation or ORCID update.

## Review record

Human Readability: PASS

Devil's Advocate: PASS

Technical Destruction: PASS

Human adjudication: APPROVED - Antonio Jose Socorro Marin - 2026-08-30
