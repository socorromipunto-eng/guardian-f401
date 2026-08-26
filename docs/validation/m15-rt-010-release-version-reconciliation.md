# RT-M15-010 — Release / Version Traceability Reconciliation

## Status

CLOSED

## Classification

Release-governance and branch-reconciliation finding.

This record does not establish physical-board qualification, production
readiness, functional-safety certification, industrial-safety certification,
or cybersecurity certification.

## Finding

Guardian F401 M15 development and the public release line evolved
independently after the v0.14.0 baseline.

Initial local inspection showed stale remote-tracking references and therefore
did not expose the already-published v0.14.0 -> v0.14.1 -> v0.14.2 release
chain.

A controlled `git fetch --prune origin` refreshed remote-tracking references
without changing the M15 working tree, index, or branch.

## Frozen M15 baseline

- Branch: `feature/m15-node-identity-architecture`
- Commit: `62b3e8d`
- VERSION before reconciliation: `0.14.0`

This baseline remains preserved and was not rewritten.

## Public release baseline

- Ref: `origin/main`
- Commit: `156cd84`
- VERSION: `0.14.2`

Observed public release tags after remote-reference refresh:

- `v0.14.0`
- `v0.14.1`
- `v0.14.2`

## Common ancestor

- Commit: `efd5d7c`
- Description: `chore(release): prepare Guardian F401 v0.14.0`

This commit was confirmed as the merge base between the M15 development line
and the refreshed public main line.

## Divergence

At adjudication time:

- M15-only commits: 47
- public-main-only commits: 15

No critical M15 assurance-file overlap was identified by the collision audit.

`git merge-tree` produced no textual conflict indicators.

## Isolated integration probe

Before modifying any persistent integration branch, a disposable Git worktree
was created from the frozen M15 baseline.

Inside that disposable worktree, `origin/main` at `156cd84` was merged without
commit for validation.

The isolated combined tree demonstrated:

- VERSION `0.14.2`;
- assurance production syntax PASS;
- complete assurance regression PASS;
- no `actuation_allowed` semantic in the assurance plane;
- no `policy_authorized` semantic in the assurance plane;
- M15 bootstrap lifecycle preserved;
- M15 epoch-transition lifecycle preserved;
- signed uint64 / RFC8785 repair preserved;
- public v0.14.2 release evidence preserved;
- public structural-review evidence preserved.

The disposable merge was aborted and the worktree was removed.

The source M15 branch remained byte- and history-preserved.

## Controlled reconciliation

A dedicated branch was created:

`integration/m15-v0.14.2-reconciliation`

Controlled reconciliation merge:

`a5d969e`

Commit description:

`merge: reconcile M15 with public v0.14.2 baseline`

The merge commit was adjudicated as an exact two-parent merge.

Parent 1:

`62b3e8d`

Frozen M15 development baseline.

Parent 2:

`156cd84`

Public v0.14.2 main baseline.

## Post-reconciliation validation

The reconciled tree demonstrated:

- integrated VERSION = `0.14.2`;
- complete assurance regression = PASS;
- M15 lifecycle invariants = PASS;
- FIRST_SEEN remains distinct from BOOTSTRAP_AUTHORIZED;
- EPOCH_TRANSITION_REQUIRED remains distinct from
  EPOCH_TRANSITION_AUTHORIZED;
- EPOCH_TRANSITION_AUTHORIZED remains distinct from FRESH;
- PREPARED remains distinct from COMMITTED;
- COMMITTED remains distinct from VERIFIED;
- signed uint64 decimal-string wire representation preserved;
- RFC8785 authorization canonicalization compatibility preserved;
- no policy-authorization semantics introduced into assurance;
- no actuation semantics introduced into assurance;
- original M15 baseline preserved;
- public v0.14.2 baseline preserved.

## Validation-tool incident

During integration validation, one PowerShell check incorrectly passed the
`-split` expression as part of a Git command invocation.

This produced:

`error: switch 'l' expects an integer value with an optional k/m/g suffix`

The resulting STOP was adjudicated as a validation-script defect.

The merge itself was not defective.

A corrected parent parser subsequently confirmed exactly two expected merge
parents.

## Decision

RT-M15-010 is CLOSED.

The release/version discrepancy was not caused by corruption or rollback of
Guardian F401 VERSION metadata.

The root cause was independent branch evolution combined with stale local
remote-tracking references.

The controlled reconciliation preserves both development and public-release
provenance.

## Residual restriction

This reconciliation does not authorize:

- push;
- tag creation;
- GitHub release creation;
- Zenodo publication;
- merge into `feature/m15-node-identity-architecture`;
- merge into `main`;
- production deployment;
- hardware qualification.

Those actions require separate change-control decisions and validation.
