# Current-main reconciliation - 2026-11-26

## Decision

Project status remains HOLD. This correction reconciles baseline pointers and the AUD-002 operational record. The 16-row matrix remains a working assessment and does not promote requirement status. Gate 01, AUD-002 closure, physical qualification, distributed safety, and production readiness remain open.

## Frozen baseline

- Repository: socorromipunto-eng/guardian-f401
- Branch: main
- Commit: 2e8f4ea8e60a95a8c8491749c5f9acf1ecf0cce3
- Tree: d9e9b3d45be19d0fb9e553204f71a7c11bccf720
- Previous release-gap pointer: 717e53c717af1dc31af540bbcadd9b426f9c9535; tree d16f9ffed29c4f3c1b7dd445902aeafe42a489c0; freshness 2026-10-04; incident range SG-001..SG-045.

## Current evidence

- M15 run 44657215925, job 136026284459, succeeded; dependency installation succeeded.
- M17 run 44657216025: 28 returned job records, all successful.
- Hardware-contract run 44657216294, job 136026285912, succeeded. This was compile and host validation, not physical board execution.
- Requirement guardian:requirement:m16-sequence-reject-duplicate remains NOT_DEMONSTRATED. Related claim guardian:claim:m16-session-sequence-rejection is accepted and references valid host evidence guardian:evidence:m16-rr04a-session-sequence-host. Source commit 50c34dd540ed9f9c658e6cb8c6d26e58594d4846; source tree e5fe67e817b78b95b96d2c8f011de30b96173e92; artifact SHA-256 48FA31DB1D57C894FC24B5FBC71B9B2E9F8A0C784F501BCDF09B2095D61F49B8; locator evidence/m16/rr04a/node-link-host-test.txt. This evidence is host-scoped. The project-state v2 attestation binds the requirements register to an earlier immutable payload, so this correction does not change requirement status.
- Windows focused-test PASS is owner-reported; the named local package was unavailable for independent inspection.

## Independent review and CI policy

No independent reviewer identity or disposition is recorded in the inspected evidence. The owner-provided November 20 ruleset probe reported zero required approvals and omitted the M17 Portable Host context; it was not freshly queried. Verify the live ruleset before claiming enforcement. A repository document cannot configure GitHub ruleset enforcement.

## Open qualification

The closure-gate register remains ACTIVE_HOLD at Gate 1 MATERIALIZED_PENDING_VALIDATION; gates 2-20 remain NOT_STARTED. No board is available. Host CI does not demonstrate hardware behavior. Distributed partition, quorum, split-brain, failover, production key ceremony, and production release qualification remain open pending direct evidence and adjudication.

## Required follow-up

1. Admit Windows evidence with exact commands, baseline, tool identity, raw results, and hashes.
2. Reconcile claim-to-requirement state under the project-state/materialization contract without rewriting accepted claim history.
3. Complete source, design/interface, hazard, verification, evidence, baseline, and reviewer mapping for every requirement; retain HOLD where evidence is absent.
4. Obtain and record independent review.
5. Query and configure the live ruleset so intended approval and M17 checks are enforced.
6. Begin physical, distributed, and production gates only when required equipment and controlled evidence are available.

This documentation change does not alter firmware, requirement statements, claim/evidence registers, gate statuses, or GitHub rulesets.

## Current-main observation after PR #84 â€” 2026-11-26

This is a post-merge observation; it does not rewrite the frozen assessment baseline or close AUD-002.

- Current main: `04e5b0e7b7a32b54257fbb3c2975a293f87fc686`; tree `505cd2b43215b52b3a19ec3b37b24fd7dd2831a6` (PR #84 merge).
- The GitHub check-runs query returned 40 records for this commit; all 40 completed successfully. M17 Portable Host contributed 28/28 successful matrix jobs in Actions run 45869662430. The other 12 checks succeeded across runs 45869662304, 45869662364, 45869662372, 45869662385, 45869662417, 45869662455, 45869662461, 45869662503, 45869662750, and 45869662783.
- The existing M17 workflow had no stable aggregate check. This change adds `m17-portable-host-required`; it fails unless the complete matrix succeeds and must itself pass in PR CI before the ruleset requires it.
- Live repository ruleset `Protect main` (ID 21526024) was observed active, with 0 required approvals and 11 required status contexts; M17 was absent. This runner will update it only after the PR's new check succeeds, then read the policy back.
- The matrix inventories 16 registered M16 requirements and 46 V017-CR items from a document marked CONTROLLED CANDIDATE. Those 46 rows remain HOLD and are not represented as approved requirements or complete design-to-test traceability. Missing allocations/evidence are explicit.
- Windows focused-test evidence remains owner-reported and unadmitted. No independent reviewer, physical-board execution, distributed-safety qualification, or production qualification is established.

## Required closure actions

1. Obtain a successful PR run of the 40 existing checks and `m17-portable-host-required`.
2. Confirm ruleset 21526024 requires at least one approval and `m17-portable-host-required`; preserve the existing required contexts.
3. Admit Windows evidence with exact baseline, commands, tool identity, raw results, and hashes; record an independent reviewer's identity, independence basis, and disposition.
4. Approve or disposition the V017-CR candidate set, then complete architecture/interface/hazard allocation, verification method, direct evidence, and review for every approved applicable requirement.
5. Keep Gate 01, AUD-002, physical, distributed, and production states open until their own acceptance evidence is reviewed.
