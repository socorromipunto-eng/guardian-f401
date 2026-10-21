# Guardian M17 — NXP supervisor canonical heartbeat hardening

Date: 2026-10-08. Status: EXTERNAL FIRMWARE CANDIDATE, NOT ADMITTED.

## Governing context

Both supplied project documents were read: remenber.txt and Evolución-recomendada.txt. They require cumulative development, explicit heterogeneous roles, independently accounted heartbeat loss, bounded recovery, and no transfer of authority from silence, compute, quorum or advisory output. Their historical version roadmap is a plan, not proof that physical prerequisites were completed. Current architecture source: docs/m17/04-architecture/m17-architecture-charter.md and v017-firmware-assurance-architecture.md, particularly Gates 13/14. This change is a bounded supervisor hardening increment, not closure of those gates.

## Finding M17-SUPERVISOR-SEMANTICS-006

Observed current source accepts authenticated/fresh/compatible heartbeat with peer_healthy=1 and disagreement=2, advances epoch/sequence, and enters MONITORING. The health/disagreement fields are uint8_t; only disagreement==1 degrades. There is no check that these two fields encode canonical booleans. An external Linux GCC probe reproduced this exact behavior against the captured source. This establishes a portable firmware behavior, not network exploitability or physical execution on MCXN947. A future adapter might reject such values; no qualified target adapter is asserted here.

## Requirement and interpretation

Proposed requirement M17-SUP-BOOL-001: health and disagreement SHALL be encoded as exactly 0 or 1. A noncanonical input SHALL be rejected before any supervisor state mutation. Rejecting a message SHALL NOT count as receiving a healthy heartbeat. Independent scheduling of missed-heartbeat accounting remains required.

This is an architecture-derived requirement. It is not a claimed direct legal requirement or proof of compliance with EU law, NASA, CISA, or aeronautical assurance standards.

## Change and impact

Three existing files only: supervisor header, supervisor source, supervisor host test. Append INVALID_HEARTBEAT to the result enum, retaining existing enum numeric values and struct layout. Validate both fields before state changes. Authentication, freshness, semantic compatibility and replay checks remain mandatory. No actuation interface is added; recovery output is not authorization.

New invalid-input return precedence applies when an input is malformed and also unauthenticated. Callers must treat every non-OK result as rejected and continue their independent timeout accounting. A rejected message preserves the prior supervisor state; this module does not itself implement elapsed-time deadlines. SAFE_HOLD rejoin behavior for a later valid heartbeat is unchanged and is outside this increment.

## Validation and technical destruction

Linux GCC strict compilation at O0 and O2 passed; original positive and negative tests retained. At each optimization, 254 invalid byte values across two fields and four reachable states yield 2,032 rejection checks. Every check compares all supervisor bytes and input bytes with snapshots, and verifies zero actuation permission. Four canonical boolean combinations retain prior health semantics. Rejected future epochs do not consume freshness. Removing the validation guard while keeping the tests causes test failure: the regression detects loss of the control.

Two compiler runs are not two independent proofs of 2,032 properties. Local GCC results do not substitute for the user's Windows toolchain, GitHub CI, NXP SDK builds, target firmware or physical qualification. Toolchain dependencies are not fully qualified.

## Human readability / devil's advocate

The control rejects malformed evidence; it does not authenticate, establish truth, persist counters, authorize takeover or make a machine safe by itself. A malicious peer with valid booleans can still lie. An upstream check does not justify leaving this consumer's contract ambiguous. This change retains all prior tests and restricts the code delta; no unrelated source is modified.

## Change management

05R materializes these three exact replacements only in a new Downloads candidate folder, under the exact 558-file post-05Q baseline. It records preimage/postimage hashes, preserves all current work, and writes a verified ZIP. It executes no firmware/compiler/test on Windows. No file is replaced in the repository, no stage/commit/push occurs, and no gate is closed. The next boundary is exact candidate adjudication and impact review of the CI source-hash manifest before any worktree admission. The existing recipes manifest pins old source hashes; source admission must not leave that manifest stale.

## Integrated candidate impact review, 05S

The CI recipe manifest changes exactly two SHA-256 values: guardian_mcxn947_supervisor.c and m17_mcxn947_supervisor_host_test.c. All recipe definitions, memberships and other hash values are preserved. The current manifest hashes translation units, not headers. The changed header is bound by the complete local baseline and this change manifest; header dependency hashing in remote CI remains an assurance gap, not a completed control.

All eight existing portable M17 CI recipes passed at O0/O2 on the external Linux candidate: 16 cases. These are local executions of the helper, not GitHub Actions results. This integrated rerun verifies compatibility with the changed manifest and retained source set. No new legal or physical conformance is established.

05S admits three existing firmware replacements, one existing CI manifest replacement and this new report only to the worktree. It verifies exact source preimages, the last 05R archive, unchanged HEAD/index/refs and every unrelated file. This document's recording in the worktree is not approval of the proposed requirement or closure of Gate01, Gate13, Gate14, Gate16 or release gates. No staging, commit, push, key generation, build on Windows, target connection or flash is authorized.
