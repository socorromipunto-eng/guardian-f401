# ADR-M15-004: Governance JSON Schema Dialect, Validation Engine and Offline Resolution Policy

Status: ACCEPTED

Implementation status: NOT_IMPLEMENTED

Date: 2026-08-30

Supersedes: None

Related decisions: ADR-M15-002, ADR-M15-003

Decision owners: Guardian F401 maintainers and authorized human adjudicators

## Reader summary

This ADR proposes the schema dialect, Python validation engine, meta-schema handling and reference-resolution policy for the future Guardian governance register validator.

The proposed choices are:

- JSON Schema dialect: Draft 2020-12;
- canonical dialect meta-schema URI: `https://json-schema.org/draft/2020-12/schema`;
- Python validation engine: `jsonschema` 4.26.0;
- validator family: `Draft202012Validator`;
- dependency ownership: a governance-specific locked dependency set;
- schema validation: schemas must themselves be checked before register instances are accepted;
- reference resolution: deterministic, repository-controlled and offline during validation;
- uncontrolled runtime network retrieval: prohibited;
- unresolved or unauthorized schema references: fail closed;
- implicit format checking: prohibited as an unstated dependency;
- implementation, conformance evidence and authorization remain separate.

This ADR does not install `jsonschema`, create governance schemas, create governance registers, implement a validator, modify CI, prove JSON Schema conformance, or demonstrate governance completeness.

The decision, implementation and evidence states remain separate.

## Decision-state separation

At this ADR stage:

- JSON register serialization: `DECIDED` by ADR-M15-003;
- restricted JSON parser primitive: `IMPLEMENTED` by the M15 governance foundation work preceding this ADR;
- JSON Schema Draft 2020-12 selection: `PROPOSED`;
- `jsonschema` 4.26.0 selection: `PROPOSED`;
- governance dependency lockfile: `NOT_IMPLEMENTED`;
- governance schemas: `NOT_IMPLEMENTED`;
- schema identifier registry: `NOT_IMPLEMENTED`;
- governance record identifiers: architecture `DECIDED`, concrete grammar `NOT_IMPLEMENTED`;
- offline schema registry: `NOT_IMPLEMENTED`;
- governance validator: `NOT_IMPLEMENTED`;
- cross-register validation: `NOT_IMPLEMENTED`;
- governance CI enforcement: `NOT_IMPLEMENTED`;
- JSON Schema conformance claim: `NOT_DEMONSTRATED`;
- governance completeness: `NOT_DEMONSTRATED`.

A decision recorded here must not be represented as evidence that its implementation exists.

## Context

ADR-M15-002 established the repository governance architecture and the separation among normative state, evidence, decisions, implementation and validation.

ADR-M15-003 selected JSON for future governance registers and established stable identifier architecture, schema-version requirements, cross-register consistency requirements and a fail-closed validation direction.

ADR-M15-003 explicitly left the following unresolved:

- schema dialect selection;
- validation library selection;
- meta-schema handling;
- exact schema-set identity;
- detailed schema implementation;
- validator implementation;
- CI enforcement.

ADR-M15-003 also defined the bounded implementation sequence:

1. define the restricted JSON parser contract and negative fixtures;
2. select and pin the schema dialect and validation dependency;
3. define schemas with stable identifiers and version rules;
4. create the minimal register set;
5. implement cross-register and atomicity validation;
6. emit structured validation evidence;
7. add negative and mutation tests;
8. integrate protected CI;
9. perform adversarial review;
10. adjudicate claims separately.

The restricted JSON parsing foundation now exists in `common/python/guardian_common/strict_json.py`, but that implementation does not constitute a schema engine.

## Repository evidence baseline

The repository baseline for this decision is:

`5a8db6096d05d7f45bbb41703eb7a75e9a5b1d39`

Read-only discovery against that baseline demonstrated:

- no `governance/` implementation directory;
- no governance register files;
- no governance JSON Schema files;
- no declared governance JSON Schema dialect;
- no existing governance schema validator;
- no `jsonschema` dependency in the current assurance lockfile;
- extensive handwritten schema-version validation in existing protocol and assurance code;
- handwritten schema-version validation is domain-specific and is not evidence of a reusable governance JSON Schema engine;
- the existing assurance lockfile is owned by assurance functionality and does not establish governance dependency ownership.

The existing assurance lockfile at the baseline contains:

- `rfc8785==0.1.4`;
- `cryptography==50.0.0`;
- `cffi==2.1.1`;
- `pycparser==3.0`.

No schema-engine dependency may be inferred from unrelated handwritten validation.

## External specification and implementation evidence

As of this decision date, the JSON Schema project identifies Draft 2020-12 as the current published specification version.

The canonical general-purpose meta-schema URI for that dialect is:

`https://json-schema.org/draft/2020-12/schema`

Relevant specification resources include:

- `https://json-schema.org/draft/2020-12`
- `https://json-schema.org/draft/2020-12/schema`
- `https://json-schema.org/draft/2020-12/json-schema-core`
- `https://json-schema.org/draft/2020-12/json-schema-validation`

The current stable documentation reviewed for the Python `jsonschema` implementation identifies version 4.26.0 and documents full support for Draft 2020-12.

Reference:

`https://python-jsonschema.readthedocs.io/en/stable/`

These external references support technology-selection analysis. They do not prove that Guardian has installed, configured or correctly used the technology.

## Problem

A JSON register parser answers only whether bytes can be decoded and parsed under the restricted JSON profile.

That is insufficient for governance validation.

The future governance layer additionally needs deterministic answers to questions such as:

- is this register instance structurally valid;
- is its declared schema version supported;
- is its schema identity authorized;
- does the schema itself conform to the selected dialect;
- are unexpected fields prohibited where required;
- are identifier fields correctly shaped;
- are enum values permitted;
- are arrays and objects bounded;
- does a `$ref` resolve to an authorized schema resource;
- did validation attempt uncontrolled network access;
- is a schema from the wrong commit being combined with the candidate register;
- is a newer or older schema being accepted silently;
- are unknown schema vocabularies or unsupported behaviors entering interpretation;
- are validation results deterministic enough to become evidence inputs.

Without an explicit dialect and engine decision, different implementations may interpret the same schema differently.

Without controlled reference resolution, a repository commit could depend on mutable network content.

Without explicit dependency ownership and locking, validator behavior could change independently of the reviewed governance state.

## Decision

Guardian F401 governance schema validation will target JSON Schema Draft 2020-12 using a governance-specific, pinned Python validation implementation.

The selected Python engine is:

`jsonschema==4.26.0`

The intended validator class is:

`jsonschema.Draft202012Validator`

This is a technology-selection decision.

It does not establish implementation or conformance until dependency locking, implementation, fixtures, negative tests and evidence gates succeed.

## JSON Schema dialect

Governance schemas will declare:

`"$schema": "https://json-schema.org/draft/2020-12/schema"`

The initial governance schema set must use Draft 2020-12 consistently.

A validator must not:

- infer the dialect from the newest installed library behavior;
- silently substitute another draft;
- accept a missing `$schema` declaration for an authoritative governance schema;
- treat an unknown dialect as the configured default;
- downgrade a schema to an older supported draft merely because parsing succeeds.

An absent, malformed, unsupported or unauthorized `$schema` value must fail closed.

## Meta-schema handling

Every authoritative governance schema must be validated as a schema before it can validate register instances.

The implementation must use the selected Draft 2020-12 validator's schema-checking behavior or an equivalently demonstrated mechanism.

A candidate schema that fails the applicable meta-schema must be rejected before instance validation.

Successful meta-schema validation proves only that the schema is structurally acceptable under the selected dialect.

It does not prove:

- that the schema expresses the intended Guardian policy;
- that the schema is authorized;
- that every required rule exists;
- that evidence is sufficient;
- that a register record is truthful;
- that physical behavior is demonstrated.

## Validation engine selection

The selected engine is Python `jsonschema` version 4.26.0.

Selection criteria include:

- documented Draft 2020-12 support;
- explicit versioned validator classes;
- schema-checking APIs;
- detailed validation error information;
- current integration with the separate `referencing` model;
- compatibility with repository Python validation workflows subject to implementation testing.

The exact installed wheel or source artifact and every transitive dependency must be pinned by hash before implementation can be considered reproducible.

The version string alone is insufficient dependency identity.

## Dependency ownership

The governance schema engine must not be silently added to the assurance dependency boundary merely because assurance already has a Python lockfile.

The planned governance implementation will own a governance-specific locked dependency set.

A future path may be:

`governance/requirements.lock`

That path is illustrative until created by an authorized implementation change.

The implementation gate must:

- select exact distributions;
- pin direct dependencies;
- pin transitive dependencies;
- record cryptographic hashes accepted by the installer;
- install with hash verification;
- demonstrate the locked environment from a clean environment;
- record Python runtime identity;
- prove that dependency installation does not alter repository-controlled files.

No dependency installation is authorized by this ADR alone.

## Reference-resolution policy

Governance validation must be deterministic with respect to schema references.

Runtime network retrieval during authoritative validation is prohibited.

The validator must not fetch an untrusted or mutable schema merely because a `$ref` contains an HTTP or HTTPS URI.

All schema resources required for authoritative validation must be available from an explicitly constructed, repository-controlled registry or equivalent bounded resource store.

The registry must be constructed from inputs bound to the same exact Git commit used for validation unless a separately authorized external trust root is explicitly defined.

Unknown or unavailable schema references must fail closed.

A resolver must not:

- retrieve missing schemas from the public Internet;
- follow arbitrary filesystem paths;
- traverse parent directories;
- resolve drive-letter paths;
- silently consult a user cache;
- silently substitute a similarly named schema;
- use a schema from another checkout without exact identity;
- use mutable external content as an implicit authority source.

## `$ref` policy

JSON Schema references are permitted only when their target is part of the authorized schema resource set.

The use of `$ref` does not authorize network access.

The future schema registry must associate each authorized schema resource with:

- a stable schema identifier;
- an exact repository path;
- an exact content digest;
- an applicable schema version;
- the exact Git commit or schema-set identity.

Detailed Guardian schema identifier syntax remains part of the subsequent schema-definition workstream.

This ADR deliberately does not invent that syntax prematurely.

## `$id` policy

Authoritative governance schemas will require stable schema identifiers.

The exact Guardian `$id` namespace grammar is deferred to the schema-definition workstream that follows this ADR.

Until that grammar is accepted:

- no provisional `$id` value is normative;
- implementation code must not hard-code an invented identifier namespace;
- examples must not be interpreted as allocated identifiers.

The eventual `$id` value must be an absolute identifier suitable for deterministic resource registration.

## Vocabulary policy

Draft 2020-12 supports vocabularies and extensibility.

Guardian governance validation will use a restricted profile.

The future implementation must explicitly define the vocabulary and keyword surface accepted by the governance schema set.

Unknown extensions must not silently acquire governance meaning.

A schema passing generic Draft 2020-12 meta-schema validation is not automatically authorized for Guardian use.

The approved Guardian schema inventory remains an additional trust boundary.

## `format` policy

JSON Schema `format` behavior must not be an implicit security or governance dependency.

For the initial governance schema set:

- correctness must not depend on format validation unless explicitly selected and tested;
- schema authors should prefer explicit structural constraints where practical;
- adding a format checker or format-specific dependency is a governance-sensitive change;
- an implementation must not assume that declaring `format` automatically causes assertion behavior.

If a later schema requires a format assertion, that behavior and its dependency surface must be explicitly reviewed and negatively tested.

## Unknown keyword policy

Generic JSON Schema extensibility must not be interpreted as permission to add arbitrary governance keywords.

The future implementation must maintain a controlled schema profile.

A keyword outside the accepted profile must either:

- be explicitly rejected; or
- be explicitly classified as a non-authoritative annotation.

Security, authority, lifecycle, evidence meaning and state-transition semantics must never depend on an unknown keyword that an older validator can ignore.

## Instance parsing boundary

The schema engine does not replace the restricted JSON parser.

Authoritative register bytes must first pass the Guardian restricted JSON parsing boundary.

The parser remains responsible for properties including:

- strict UTF-8 decoding;
- duplicate-key rejection;
- restricted numeric behavior;
- configured raw-byte limits;
- configured structural limits.

Only the parsed value may proceed to schema validation.

A successful schema validation cannot retroactively make unsafe parsing acceptable.

## Boolean and numeric behavior

JSON Schema's generic numeric model must not weaken Guardian's restricted parsing profile.

The implementation must preserve existing fail-closed rules for unsupported numeric values.

In particular, the governance validator must not create a second parsing path that silently accepts values prohibited by the restricted JSON primitive.

Numeric boundary tests remain mandatory.

## Schema-set identity

A schema version number alone is insufficient schema identity.

The future governance project-state binding must identify the approved schema set using independently trusted information including:

- stable schema identifiers;
- supported versions;
- exact content hashes;
- exact repository paths;
- exact Git commit and tree identity.

A digest declared only inside the candidate register set cannot authenticate the same register set.

The trusted expected schema-set identity must remain anchored outside the candidate data it validates.

## Offline validation

Authoritative governance validation must be capable of running with network access unavailable.

A passing validation that depends on live Internet retrieval is not acceptable final evidence.

Offline validation must demonstrate:

- every required schema resource is locally and explicitly registered;
- no unresolved reference triggers retrieval;
- no hidden cache is required;
- the same immutable input set produces the same validation decision;
- unavailable external network services do not change the result.

## Error handling

Schema and instance validation failures must produce deterministic machine-readable classifications suitable for later structured validation evidence.

Human-readable library exception text is diagnostic information and must not become the sole stable evidence contract.

The future Guardian wrapper should normalize engine-specific exceptions into Guardian-owned error categories.

At minimum, the future contract must distinguish:

- invalid schema;
- unsupported dialect;
- unauthorized schema identity;
- invalid instance;
- unresolved reference;
- prohibited external retrieval;
- resource-limit failure;
- internal validator/tool failure.

The exact error-code registry is deferred to implementation.

## Resource limits

JSON Schema evaluation can amplify work through nesting, references and combinational keywords.

The governance validator must therefore operate under explicit resource boundaries.

Implementation evidence must define and test bounds applicable to:

- raw JSON size;
- JSON structural depth;
- object member count;
- array item count;
- schema document size;
- schema resource count;
- reference resolution count;
- reference or evaluation depth where applicable;
- validation error count retained for evidence.

The numeric values are not invented by this ADR.

They must be fixed by implementation evidence and adversarial testing.

## Determinism

The final validator must make the aggregate decision deterministically for a fixed:

- Git commit;
- Git tree;
- schema-set identity;
- register-set identity;
- validator implementation;
- dependency set;
- runtime configuration.

Diagnostic error ordering from a third-party engine must not be assumed stable unless explicitly demonstrated.

If structured evidence includes multiple violations, Guardian-owned sorting or canonicalization must produce deterministic evidence ordering.

## Security boundary

Schema validation is a parser and policy-enforcement component, not an authority source.

A passing schema result means only that the evaluated instance satisfied the applicable machine rules under the demonstrated validator configuration.

It does not prove:

- evidence sufficiency;
- factual truth;
- physical behavior;
- authorization;
- certification;
- regulatory conformance;
- production readiness;
- human approval.

The invariant remains:

`advisory != authority != actuation`

Nothing in this ADR grants an AI system, schema engine, validator or CI process authority to actuate firmware or physical outputs.

## Change authorization

Changes to any of the following are governance-sensitive:

- JSON Schema dialect;
- validation engine;
- validation-engine version;
- dependency hashes;
- accepted vocabulary;
- accepted keyword profile;
- reference-resolution policy;
- schema identifier rules;
- format behavior;
- schema-set trust anchor;
- resource-limit behavior.

A passing validator modified in the same change is not sufficient authorization for relaxing its own policy.

Such changes require semantic-diff review and the applicable human approval.

## Required implementation evidence

Before this decision can be represented as implemented, evidence must demonstrate at least:

- `jsonschema` is installed from an exact locked dependency set;
- the observed installed version equals the selected version;
- all transitive dependencies are pinned and hash-verified;
- Draft 2020-12 validator selection is explicit;
- authoritative schemas declare the expected `$schema`;
- schemas are validated before instance use;
- unsupported dialects fail;
- missing dialect declarations fail where required;
- malformed schemas fail;
- valid instances pass;
- invalid instances fail for the intended rule;
- unknown and downgraded schema versions fail;
- unresolved `$ref` values fail;
- runtime HTTP/HTTPS retrieval is not required;
- a controlled attempted external reference cannot silently retrieve content;
- offline execution produces the expected result;
- invalid UTF-8 remains rejected before schema processing;
- duplicate JSON keys remain rejected before schema processing;
- resource-limit violations fail closed;
- exact schema, validator and dependency identities appear in validation evidence.

A generic unit test that only validates one valid JSON object is insufficient.

## Required negative tests

The schema-engine workstream must include negative tests covering at least:

1. missing `$schema`;
2. unsupported `$schema`;
3. malformed schema;
4. schema valid under a different draft but not authorized for Guardian;
5. unknown required schema resource;
6. remote HTTP reference attempt;
7. remote HTTPS reference attempt;
8. unresolved local reference;
9. duplicate schema identifier;
10. unauthorized schema identifier;
11. schema from the wrong commit;
12. instance with unexpected property where prohibited;
13. instance with missing required property;
14. enum value outside the allowed set;
15. numeric boundary violation;
16. malformed canonical identifier once that grammar exists;
17. parser-level duplicate key before schema validation;
18. parser-level invalid UTF-8 before schema validation;
19. resource exhaustion boundary;
20. dependency-version mismatch.

Mutation testing must later demonstrate that controlled relaxation of an important rule is detected.

## Compatibility policy

Compatibility is explicit, not inferred.

An older validator must not silently accept a newer governance schema simply because unknown fields are ignored.

A newer validator must not silently reinterpret an older schema under newer semantics.

Each accepted schema instance must be associated with an explicitly supported schema identity and version.

Breaking semantic changes require a new schema version and a documented migration or compatibility decision, consistent with ADR-M15-003.

## Implementation sequence after acceptance

If this ADR is accepted, implementation should proceed through bounded gates:

1. establish a governance-specific locked dependency environment;
2. demonstrate exact installation of `jsonschema==4.26.0` and transitive dependencies;
3. implement a minimal offline schema registry wrapper;
4. test Draft 2020-12 meta-schema handling;
5. define the Guardian governance schema profile;
6. define stable schema identifiers and governance record identifier grammar;
7. create minimal schemas without yet claiming register completeness;
8. implement negative tests for dialect and reference handling;
9. perform Human Readability, Devil's Advocate and Technical Destruction;
10. only then consider integration with register-set validation.

No later step is implicitly authorized by acceptance of this ADR.

## Relationship to ADR-M15-003

ADR-M15-004 refines ADR-M15-003.

It does not supersede ADR-M15-003.

ADR-M15-003 remains authoritative for:

- JSON serialization selection;
- stable record-identifier architecture;
- register-set atomicity;
- cross-register reference requirements;
- evidence/authorization separation;
- governance transition rules;
- the broader implementation sequence.

ADR-M15-004 resolves only the previously pending dialect, validation-engine and reference-resolution decision surface.

## Relationship to existing assurance validation

Existing assurance validation code contains domain-specific field and schema-version checks.

Those checks remain authoritative for their existing scope.

ADR-M15-004 does not migrate assurance objects to JSON Schema.

ADR-M15-004 does not make the future governance schema engine a dependency of assurance.

Any later attempt to share validation primitives across these domains requires separate architecture and compatibility evidence.

## Relationship to firmware

This ADR does not modify:

- STM32F401 firmware;
- device protocol;
- actuation paths;
- cryptographic authorization;
- firmware lifecycle;
- hardware validation;
- assurance-message authority boundaries.

Governance schema validation is repository-side tooling.

It must not become an actuation authority.

## Rejected alternatives

### Continue with handwritten validation only

Rejected as the schema foundation because it would require Guardian to invent and maintain a complete structural schema language while ADR-M15-003 explicitly calls for selection of a schema dialect and validation dependency.

Handwritten semantic validation will still be necessary for rules that JSON Schema cannot adequately express, such as cross-register lifecycle transitions, authorization and commit atomicity.

### JSON Schema Draft 7

Rejected for the initial governance implementation because the project has no legacy governance schemas requiring Draft 7 compatibility and the current published JSON Schema dialect is Draft 2020-12.

Supporting an older dialect would add migration and compatibility obligations without demonstrated repository need.

### JSON Schema Draft 2019-09

Rejected for the same initial reason: no existing Guardian governance schema requires it, while Draft 2020-12 is the selected current dialect.

### Multiple accepted dialects initially

Rejected because multiple dialects increase ambiguity, dependency behavior and negative-test surface before the first governance schema exists.

The initial implementation will support exactly one authoritative governance dialect.

### `fastjsonschema` as the initial engine

Not selected for the initial implementation.

Guardian prioritizes an explicitly versioned validator family, demonstrated meta-schema behavior, controlled reference handling and evidence-oriented error inspection.

This rejection is repository-specific and is not a claim that `fastjsonschema` is generally unsafe or unsuitable.

### Pydantic models as the normative schema language

Rejected as the normative governance schema mechanism.

Pydantic may be useful for application models, but using Python model definitions as the sole normative governance schema would couple the schema authority to implementation code and would not satisfy the explicit JSON Schema dialect decision required by ADR-M15-003.

### Runtime Internet schema retrieval

Rejected because it makes validation dependent on mutable external availability and content outside the exact repository state being adjudicated.

### Add governance dependencies to `assurance/requirements.lock`

Rejected for the initial implementation because assurance and governance currently have distinct dependency ownership and authority boundaries.

A future consolidation may be considered only with separate evidence.

## Consequences

### Benefits

- one explicit schema dialect;
- one explicit initial validation engine;
- deterministic failure on unsupported dialects;
- an offline validation requirement;
- explicit separation between parser and schema engine;
- explicit dependency ownership;
- a path to stable schema-set identity;
- reduced risk of mutable network schemas affecting a repository decision;
- clear negative-test obligations;
- no accidental expansion of assurance dependency scope.

### Costs

- a new locked governance dependency set will be required;
- transitive dependencies must be pinned and maintained;
- offline reference registration requires explicit implementation;
- Draft 2020-12 behavior must be tested rather than assumed;
- schema and semantic validation remain separate layers;
- future version upgrades become governance-sensitive changes.

### Limitations

JSON Schema cannot determine whether:

- a claim is factually true;
- evidence is sufficient;
- evidence provenance is authorized;
- a human approved a decision;
- firmware behaves physically as represented;
- a register set discovered every relevant architectural concept.

These remain separate validation and adjudication responsibilities.

## Current adjudication

Architecture decision: `ACCEPTED`

Draft 2020-12 selection: `PROPOSED`

`jsonschema==4.26.0` selection: `PROPOSED`

Offline-only authoritative schema resolution: `PROPOSED`

Runtime network schema retrieval: `PROPOSED_PROHIBITED`

Governance dependency lockfile: `NOT_IMPLEMENTED`

Governance schemas: `NOT_IMPLEMENTED`

Governance validator: `NOT_IMPLEMENTED`

Governance registers: `NOT_IMPLEMENTED`

Governance CI enforcement: `NOT_IMPLEMENTED`

JSON Schema conformance: `NOT_DEMONSTRATED`

Governance completeness: `NOT_DEMONSTRATED`

No certification, regulatory conformance, physical-enforcement, AI-authority or production-readiness claim is created by this proposal.

## Review requirements before ACCEPTED

This ADR must not move from `PROPOSED` to `ACCEPTED` solely because the file exists.

Before acceptance it requires:

- Human Readability review;
- Devil's Advocate review;
- Technical Destruction review;
- consistency review against ADR-M15-002 and ADR-M15-003;
- dependency-boundary review;
- security review of offline resolution;
- human adjudication of the final candidate identity.

Any material remediation after review creates a new candidate identity and requires re-review of the affected claims.

## Implementation authorization boundary

Acceptance of this ADR, if later granted, authorizes an architecture decision only.

It does not automatically authorize:

- dependency installation;
- lockfile modification;
- schema creation;
- validator implementation;
- register creation;
- CI modification;
- commit;
- push;
- pull request;
- merge;
- tag;
- release;
- Zenodo publication;
- DOI creation;
- ORCID modification.

Those actions remain separately gated.

## Draft revision history

### DRAFT.v1

Initial controlled materialization.

Baseline source of truth:

`5a8db6096d05d7f45bbb41703eb7a75e9a5b1d39`

Status:

`ACCEPTED`

Implementation:

`NOT_IMPLEMENTED`

Human Readability:

`PASS`

Devil's Advocate:

`PASS`

Technical Destruction:

`PASS`

Candidate SHA-256:

`7D85B5F026EFECA316A0E91D6289DACE566DD17A289D9632084046DA9DE15A5C`

Human adjudication:

`APPROVED`

Human-approved candidate SHA-256:

`7D85B5F026EFECA316A0E91D6289DACE566DD17A289D9632084046DA9DE15A5C`

Human-approved candidate bytes:

`30223`

Human adjudication basis:

- Human Readability: `PASS`;
- Devil's Advocate: `PASS`;
- Technical Destruction: `PASS`;
- attack coverage: `20/20`;
- attack failures: `0`;
- contradiction matches: `0`.

The human adjudication applies to the exact pre-acceptance candidate identified above. Materializing this adjudication changes the document bytes and therefore creates a new post-adjudication artifact identity.
