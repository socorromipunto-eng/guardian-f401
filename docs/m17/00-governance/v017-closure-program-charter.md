# Guardian F401 v0.17.0 Closure Program Charter

Status: CONTROLLED CANDIDATE
Program: Guardian F401 M17 / v0.17.0 Closure Program
Closure Gate: 01 of 20 - Baseline and Evidence Freeze
Release target: v0.17.0 exact; beta labels are not release closure.

## 1. Purpose

This charter governs the final assurance path from the demonstrated M17 worktree to an evidence-backed v0.17.0 release candidate. It converts the remaining work into twenty explicit closure gates. Each gate has an entry state, exact mutation boundary, positive and hostile verification, evidence outputs, claim limits, and a separately authorized next boundary.

The program is designed to preserve the security semantics already demonstrated while preventing claim inflation. A successful host test is not hardware evidence. A valid signature is not trust. Trust is not authorization. Authorization is not acceptance. Acceptance is not actuation authority. AI or advisory output is not authority and is not actuation.

## 2. Governing execution model

Every technical runner used by the Closure Program shall use twenty numbered steps. Every repository mutation boundary requires one explicit human authorization token. A runner may reuse evidence only when the dependency hashes are unchanged. Changed dependencies trigger impact revalidation of the affected surface; unchanged components shall not be repeatedly retested merely for ceremony.

The lifecycle is:

Architecture -> Source of Truth -> Requirements -> Implementation -> Positive Verification -> Negative/Hostile Verification -> Technical Destruction -> CI -> Human Readability -> Devil's Advocate -> Evidence -> Commit -> Push -> PR -> Protected-Main.

Stage, commit, push, pull request, merge, tag, and release remain separate authorization boundaries after technical closure.

## 3. Release truth model

The following distinctions are mandatory throughout v0.17.0 closure:

- authenticated != fresh != truthful != authorized != actuation-authorized
- valid signature != trusted != authorized != accepted
- registered != approved != implemented != validated != evidence-present
- persisted state != freshness
- provider OK != anchor valid
- anchor valid != fresh
- mapping != conformance
- conformance != certification
- certification != deployment authorization
- AI/advisory != authority != actuation

Unknown, ambiguous, missing, contradictory, or stale authority state fails closed.

## 4. Release-critical domains

A v0.17.0 release decision shall not carry UNKNOWN, TBD, TODO, PLACEHOLDER, PLANNED, PENDING, or NOT_DEMONSTRATED for a release-critical domain unless that domain is formally adjudicated NOT_APPLICABLE with evidence. Release-critical domains are:

AUTHORITY, ACTUATION, IDENTITY, KEYS, SIGNING, FRESHNESS, ANTI_REPLAY, FIRMWARE_UPDATE, BOOT, RECOVERY, SEMANTICS, QUORUM, PARTITION, SPLIT_BRAIN, FAILOVER, PHYSICAL_HARDWARE, SUPPLY_CHAIN, RELEASE_INTEGRITY.

## 5. Current baseline at Closure Gate 01

The protected baseline for this program is branch feature/m17-heterogeneous-multinode-assurance at HEAD 717e53c717af1dc31af540bbcadd9b426f9c9535 and tree d16f9ffed29c4f3c1b7dd445902aeafe42a489c0. The worktree contains controlled, uncommitted M17 artifacts. Index staging is prohibited until a later separately authorized stage boundary.

Demonstrated and reusable, subject to unchanged hashes:

- heterogeneous multinode authority model
- Signed Evidence contract
- semantic binding and authority separation
- NodeLink authentication/freshness host contracts
- persistence and persistent anti-replay host contracts
- rollback-anchor host contract
- production Ed25519 provider source and host validation
- Monocypher 4.0.3 provenance lock
- RFC 8032 positive and hostile validation
- trusted-key provenance gate
- trusted-signature adapter provenance enforcement
- lifecycle rejection of unverified production keys
- production-key ceremony plan

Not yet demonstrated for release closure:

- ceremony environment qualification and executed production key ceremony
- production-approved key provenance
- initial persistent anti-replay provisioning
- physical rollback anchor backend
- physical power-loss/fault validation
- quorum/partition/split-brain closure
- bounded failover closure
- firmware update/boot/recovery cross-integration closure
- M17 CI closure
- supply-chain/release-integrity closure
- physical hardware qualification
- final adversarial release review and v0.17.0 closure

## 6. Firmware authority

STM32F401 remains deterministic reference/control authority. Heterogeneous peers may observe, supervise, measure, witness, or provide bounded compute, but no peer obtains automatic actuator authority. Future AI remains advisory and belongs outside the v0.17 actuation authority path.

## 7. Evidence rule

PREVIOUS_PASS + SAME_INPUT_HASHES = EVIDENCE_REUSED
PREVIOUS_PASS + CHANGED_DEPENDENCY = IMPACT_REVALIDATION_REQUIRED
UNCHANGED_COMPONENT = DO_NOT_REPEAT_TEST
NEW_COMPONENT = FULL_LIFECYCLE_REQUIRED

Every final claim must resolve to evidence that names the exact artifact, revision or hash, validation environment, result, and known limitation.

## 8. Closure authority

This charter authorizes no firmware change, key generation, hardware claim, Git staging, commit, push, PR, merge, tag, or release by itself. It defines the controlled path by which later human-authorized boundaries may perform those actions.