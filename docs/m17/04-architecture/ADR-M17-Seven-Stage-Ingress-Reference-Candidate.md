# ADR M17 07R — Seven-stage authenticated ingress reference candidate

Status: EXTERNAL CANDIDATE; not admitted, target-qualified, released or certified.
Baseline: 0f2d55c722fc74dabba90a53c83a24ec7d62dbb2; branch feature/m17-authenticated-ingress-state-machine; 585 exact file identities.
Authority: user requested implementation and external validation of all seven ingress stages. This runner does not authorize repository admission, target access, key generation, signing, staging, commits, pushes or release.

## Engineering decision and bounded profile

Implement a distinct C11 transaction coordinator above the exact V2 codec and the external 07Q scoped Ed25519 verifier. Reuse the reviewed Monocypher 4.0.3 primitive; do not cast V2 input into a V1 authenticated structure or use firmware-signing authorization as peer identity. 07Q Windows execution has not been supplied; 07R therefore includes and executes its standalone verifier tests at both optimization levels rather than inheriting a Windows PASS.

The implemented reference profile supports ONE configured peer, ONE immutable policy/profile/generation/round, and one pending local outbound observation at a time. Both ingress observation and ACK are supported. This is a deliberate bounded candidate, not the complete heterogeneous quorum system. Moving to a new epoch, key, registry revision or round requires separately reviewed migration/rejoin. No automatic cursor reset, policy promotion, quorum formation, failover or actuator access exists.

## Interfaces, ownership and trust

`gi_init` installs an immutable local configuration and provider callbacks. It does not create storage or overwrite any record. `gi_recover` must establish the existing record and trusted anchor before any processing. `gi_prepare` consumes raw wire bytes and computes a private proposed transaction. `gi_prepare_pending` is restricted to a trusted outbound owner that already established the exact local observation was generated/sent for this context. It does not prove transmission, sign, or select authority. `gi_commit` owns durable confirmation and RAM publication. `gi_snapshot` rechecks registry, revocation, deadline and time before returning a corroboration snapshot.

Configuration contains the exact public key and identity tuple, not merely an identifier. Registry revision, revocation, peer type mask and public key are rechecked before commit and snapshot. The trusted caller owns memory/configuration and serializes all access; C structs are not unforgeable capabilities. Buffers/context/configuration/provider must not alias destructively. No interrupt/concurrent modification is permitted. PREPARED blocks additional prepares. HOLD blocks positive publication until explicit recovery succeeds. Rejected packets do not advance accepted state or touch storage.

A locally trusted nondecreasing 32-bit timebase supplies deadlines; wrap and clock-source qualification require target work. The recovery observation establishes a RAM clock floor as well as comparison against persisted commit time. Deadline equality is expired. The system does not claim to detect rollback of an untrusted clock or a malicious rollback of BOTH storage and the supposedly trusted anchor.

## Seven implemented stages

1. Strict bounded V2 decode and exact Ed25519 transcript verification against the local peer key.
2. Type authorization and fixed-profile V2 semantics: observation claims-actuation byte is zero; version/reserved bytes, generation, policy, profile, round, state and commitment must match local configuration.
3. Separate observation and ACK outer sequence cursors under a fixed provisioned peer epoch. Incoming duplicate/older sequences fail. Local outbound epoch/sequence remain separate. Exhausted inputs do not wrap.
4. One canonical transaction stores all accepted cursors, pending state, report presence, eligibility, local reference, deadline, commit time and configuration identities. Stage record, compare-and-advance trusted anchor, read anchor and complete record back, then publish RAM.
5. ACK must have its own fresh sequence and exactly match the pending local observation epoch/sequence and the fixed generation/policy/profile/round/commitment. A stored peer observation must already exist. Consumption and eligibility are one transaction.
6. Observation reception records an unacknowledged report with eligibility zero. Only committed matching ACK permits per-peer corroboration eligibility. That eligibility is neither quorum nor actuator authorization. There is no actuator callback or AI entry point.
7. Recovery accepts only the exact canonical complete record matching the trusted anchor. Unanchored staged data does not become accepted. Uncertain writes/anchor/readback latch HOLD and cannot publish partial RAM acceptance. Restart never invokes an actuator.

## Canonical record and observation commitment

Record: exactly 256 bytes, little endian. Offsets 0..2: GI and version1; byte3 zero; bytes4..6 report/pending/eligibility boolean; byte7 expected state. Eighteen LE32 words at8..79: revision, sender, producer, key, epoch, registry revision, generation, policy, profile, round, observation cursor, ACK cursor, local epoch, local sequence, deadline, last accepted time, two zero reserved words. Bytes80..111 commitment;112..143 locally trusted measurement digest;144..175 pinned public key;176..223 zero;224..255 BLAKE2b-256 of exact bytes0..223. No native struct, pointer or padding is persisted. Decoder re-encodes the complete record and compares every byte.

Commitment: BLAKE2b-256 of ASCII `GUARDIAN:CORROBORATION:COMMITMENT:V1` without NUL, then generation/policy/profile/round as four LE32 words, state byte, and 32-byte locally supplied measurement digest. This resolves a candidate canonical preimage for this profile. It does not measure the sensor, establish truth, calibrate ADC or validate measurement provenance.

## Provider obligations and host fault model

The C module delegates record staging and trusted atomic anchor compare-and-advance. The test backend uses two simulated slots, retains the previous anchored record, and atomically stores revision plus digest in its simulated anchor. A device backend must prove those properties; normal flash writes alone are not such an anchor.

Eight injected failure cuts are exercised for pending creation, observation storage and ACK consumption (24 combinations): before record write; torn write; complete record with reported failure; before anchor advance; advanced anchor with reported failure; anchor read failure; record read failure; corrupted readback. Recovery yields the old complete transaction or the new complete transaction according to the anchor, never a half-published pending/report state. Additional tests corrupt every persisted byte, restore mismatched anchor/record, restart after publication, reject replays after restart, revoke/change keys between preparation and use, and roll back local time. These tests are a bounded host model, not exhaustive instruction-level power-cut evidence or proof of real NVM atomicity.

## Assurance, deviations and remaining work

All 32 parent ISM requirements are retained with individual dispositions. None is promoted to production closure. Remaining work includes multi-peer/multi-round rejoin; integration with actual `gc_report`/quorum APIs; rejection/acceptance audit schema and audit failure policy; legacy full-suite regressions/CI admission; real flash/anchor provider; clock and boot trust; F401 resource/WCET/stack/entropy/ADC/watchdog qualification; MCXN947 build/run and real two-end link evidence. Previously reported NXP compilation is not denied or rerun by this candidate; its exact artifact evidence remains to be bound to the current release.

Historical ML-DSA-87 artifact research remains a separate workstream. V2 uses fixed 64-byte Ed25519 signatures and does not become post-quantum by incorporating those documents. Production secure update, production key custody and PQC transport remain open. CNSA software/firmware-signing profiles must be reviewed separately; ML-DSA research is not automatically a CNSA firmware-signing claim.

NASA/NIST/CISA and aviation references inform proposed assurance controls. FAA AC20-115D/DO-178C are not a certification granted to this project; no independent IV&V or coverage objective is claimed. EU Machinery and AI Act applicability depends on actual product, use, role and jurisdiction; it has not been adjudicated here. AI remains advisory and no AI implementation is introduced.
