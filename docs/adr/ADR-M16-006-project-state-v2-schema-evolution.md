# ADR-M16-006 — Project-State v2 Schema Evolution

## Status

CANDIDATE FOR FINAL ARCHITECTURE GATE GOVERNANCE CLOSURE

## Milestone

M16 Final Architecture Gate / FAG-08

## Decision Type

Governance / Schema Evolution / Repository State Binding

## Context

Guardian F401 currently has a project-state v1 schema whose per-register
`schema_version` field is fixed to `1.0.0`.

The repository now contains governance registers whose applicable schemas do not
all share one version. In particular, the controlled document register has
evolved to schema version `2.0.0`.

Therefore the v1 project-state contract cannot faithfully represent a
heterogeneous governance schema set without either:

- silently changing v1 semantics;
- forcing a valid newer register back to an older schema; or
- misrepresenting the actual schema version used for validation.

All three are prohibited.

## Decision

Guardian SHALL evolve project-state through a new versioned contract rather than
mutating the semantics of project-state v1 in place.

The v1 schema remains immutable for historical reconstruction.

Project-state v2 SHALL permit each registered governance input to bind its own:

- repository-relative path;
- schema identifier;
- schema version;
- exact schema SHA-256;
- exact register SHA-256.

Project-state v2 SHALL bind the described register set to an immutable predecessor
payload commit and to that commit's exact Git tree.

The binding model is named:

`PREDECESSOR_PAYLOAD_ATTESTATION`

## Normative invariants

PROJECT_STATE_V1 != PROJECT_STATE_V2

SCHEMA_EVOLUTION != SILENT_SEMANTIC_MUTATION

REGISTER_SCHEMA_VERSION != PROJECT_STATE_SCHEMA_VERSION

SCHEMA_ID_MATCH != SCHEMA_CONTENT_MATCH

SCHEMA_VERSION_MATCH != SCHEMA_HASH_MATCH

REGISTER_HASH_MATCH != SEMANTIC_VALIDITY

SCHEMA_VALID != SEMANTICALLY_VALID

SEMANTIC_VALID != AUTHORIZED

VALIDATION_PASS != HUMAN_APPROVAL

VALIDATION_PASS != ACTUATION_AUTHORIZED

PROJECT_STATE != SELF_AUTHENTICATION

SELF_DECLARED_HASH != AUTHORITY

AI/advisory != authority != actuation

## Project-state v2 register binding

Each register binding SHALL include:

- `path`
- `schema_id`
- `schema_version`
- `schema_sha256`
- `register_sha256`

A consumer SHALL fail closed when any required binding is absent, malformed,
unknown, unsupported, inconsistent, or integrity-invalid.

## Heterogeneous schema versions

Different registers MAY use different schema versions when each version is
explicitly declared, supported, and integrity-bound.

No consumer may infer that every register shares the project-state schema
version.

## Historical preservation

`governance/schemas/project-state.schema.json` remains the historical v1
contract.

The v2 schema is a separate artifact and does not retroactively reinterpret a
v1 project-state record.

## Commit/tree binding semantics

Project-state v2 uses predecessor-payload attestation.

Let:

- `A` be the immutable governed payload commit being described;
- `TREE(A)` be the exact Git tree of `A`;
- `B` be a later materialization commit that stores the project-state instance.

The required relationship is:

`project_state.source_commit = A`

`project_state.source_tree = TREE(A)`

`project_state.binding_model = PREDECESSOR_PAYLOAD_ATTESTATION`

When the materialization commit is known:

`B != A`

The project-state instance describes `A`; it does not claim that its own
materialization commit is the payload commit it describes.

A self-referential binding in which the project-state file attempts to contain
the hash of the same commit whose hash depends on that file is prohibited.

Therefore:

SELF_REFERENTIAL_COMMIT_BINDING = PROHIBITED

PROJECT_STATE_SOURCE_COMMIT = GOVERNED_PAYLOAD_COMMIT

PROJECT_STATE_SOURCE_TREE = TREE_OF_GOVERNED_PAYLOAD_COMMIT

PROJECT_STATE_MATERIALIZATION_COMMIT != PROJECT_STATE_SOURCE_COMMIT

## Immutable Git object validation

Final validation SHALL resolve register and schema bytes from immutable Git
objects at `source_commit`.

A validator MUST NOT substitute mutable worktree bytes as final evidence for a
project-state record that claims to describe another commit.

Conceptually:

`register_bytes = git show source_commit:path`

`schema_bytes = git show source_commit:schema_path`

`source_tree = git rev-parse source_commit^{tree}`

The declared `source_tree` must equal the actual tree of `source_commit`.

If a materialization commit is supplied to the validator, the validator SHALL
fail closed when:

- materialization commit equals source commit;
- source commit is not an ancestor of the materialization commit; or
- the materialization commit cannot be resolved.

## Project-state instance migration boundary

This ADR adjudicates the commit/tree binding semantics but does not yet mutate
`governance/project-state.json`.

The instance migration remains a separately bounded materialization step after
the v2 contract, validator, and negative tests have been reviewed.

Until that boundary is authorized:

PROJECT_STATE_V2_INSTANCE_MIGRATION = NOT_AUTHORIZED

## Authority boundary

Project-state validation is descriptive and evidentiary.

It cannot grant:

- commit authority;
- push authority;
- PR authority;
- merge authority;
- normative architecture authority;
- NodeSupervisor authority;
- physical actuation authority.

## Nonclaims

This ADR does not claim:

- governance completeness;
- project-state v2 implementation completion;
- cross-register semantic completeness;
- physical STM32F401 validation;
- physical MCXN947 validation;
- persistent anti-replay implementation;
- production readiness;
- certification;
- NodeSupervisor architecture authorization;
- NodeSupervisor implementation authorization.

## Consequence

FAG-08 proceeds in bounded stages:

1. materialize and validate the v2 contract, immutable-object validator, and
   negative tests;
2. separately materialize the project-state v2 instance after the governed
   payload commit exists;
3. validate that instance against immutable Git objects from the payload commit;
4. require separate human authorization for any commit, push, PR, merge, or
   downstream architecture transition.
