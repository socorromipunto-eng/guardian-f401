# ADR-M14-001: Canonical envelope and closed payload contracts

- Status: proposed for repository documentation of an implemented and validated
  software slice
- Decision owner: Antonio José Socorro Marín
- Implemented commits: `e3f209d`, `4349aa3`
- CI workflow commit: `3e862ab`
- Final validated branch commit: `072446f`

## Context

Federated assurance messages require deterministic bytes, bounded parsing and
unambiguous schema meaning. Authentication or attestation cannot be allowed to
imply physical authority. Generic JSON objects permit silent contract drift,
duplicate-member ambiguity and cross-schema confusion.

## Decision

Use a closed, domain-separated envelope; reject duplicate members before
semantic validation; dispatch to exact versioned observation, decision or
witness schemas; enforce bounded structural and numeric rules; and produce
canonical JSON bytes using RFC 8785 through the locked `rfc8785==0.1.4`
dependency.

Schema validity is never an action-authorization decision.

## Consequences

- Generic S1 payloads that do not match a selected closed schema are rejected.
- Contract extension requires a new schema version rather than silent fields.
- Equivalent accepted objects produce deterministic canonical bytes in the
  tested scope.
- The implementation gains an external dependency that must remain locked and
  independently reviewed.
- Post-materialization checks do not provide a streaming memory guarantee.
- Evidence digests do not prove authenticity, freshness or authorization.

## Alternatives considered

- Generic extensible payloads: rejected for the current slice because they
  weaken contract closure and make drift difficult to detect.
- Custom canonicalization: rejected because it increases specification and
  interoperability risk without evidence of necessity.
- Treating canonical bytes as signed or trusted evidence: rejected because
  canonicalization supplies neither identity nor authorization.

## Validation

The final branch commit passed 87 local tests and GitHub Actions run
`31983989907` on CPython 3.12. The validation covers the stated software
vectors only.

## Revisit conditions

- New payload type or field.
- Different digest or canonicalization algorithm.
- Streaming parser or embedded implementation.
- Signed identity, freshness or attestation integration.
- Any proposal to connect accepted decisions to physical authority.
