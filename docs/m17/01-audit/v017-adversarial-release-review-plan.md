# Guardian F401 v0.17.0 Adversarial Release Review Plan

Status: CONTROLLED CANDIDATE
Purpose: define the hostile questions Gate 19 must answer before Gate 20 can close v0.17.0.

## 1. Authority attacks

- Can any supervisory, witness, measurement or high-compute peer become actuator authority through quorum, failover, communication loss, restart ordering or a malformed role value?
- Can a stale but validly signed artifact be treated as current authority?
- Can unknown authority, duplicate identity or split-brain state degrade to permissive behavior?
- Can AI/advisory output reach actuation directly or indirectly?

## 2. Cryptographic and trust attacks

- Can a cryptographically valid signature from an unapproved key reach update acceptance?
- Can key_id substitution, algorithm confusion, signature truncation, zero signatures, wrong message, wrong public key or provider substitution bypass policy?
- Can a test backend, synthetic approved test key or stale provider be linked into production?
- Can historical key2 be promoted without evidence?
- Can production private material enter repository, logs, evidence, target firmware or crash artifacts?

## 3. Freshness, replay and persistence attacks

- Can reboot erase replay memory and allow a previously accepted record?
- Can first-provision state be confused with continuation?
- Can a torn persistence write, stale anchor, rollback, corrupt record or anchor failure promote runtime state?
- Can persisted data be mistaken for freshness without independent monotonic evidence?

## 4. Firmware update/boot/recovery attacks

- Can an image be activated before digest/signature/trust/rollback checks complete?
- Can rollback floor advance before confirmed boot evidence is safely persisted?
- Can failed boot or recovery leave a candidate implicitly trusted?
- Can interrupted update create a state that later bypasses validation?

## 5. Distributed-system attacks

- Minority partition and majority partition disagreement.
- Conflicting witnesses.
- Duplicate peer identity.
- Peer restart with stale state.
- Delayed evidence and reordering.
- Asymmetric communication.
- Quorum availability used as authority escalation.
- Failover oscillation or repeated handoff.

## 6. Supply-chain attacks

- Upstream cryptographic source substitution.
- Stale tag or mismatched commit/blob/hash.
- License/provenance omission.
- Local unrecorded patch.
- Test-only code entering production link.
- Compiler/tool drift not captured in evidence.
- Release artifact replaced after tests.

## 7. Evidence attacks

- Claim references an evidence file that does not contain the claimed result.
- Evidence is generated from stale inputs.
- Evidence hash is self-referential or computed before final mutation.
- Cross-artifact terminology agrees lexically but not semantically.
- Human-readable claim overstates host evidence as physical evidence.
- Mapping is presented as conformance or certification.

## 8. Physical attacks and limits

- Power interruption at each persistence/anchor/update cut point.
- Reset during state promotion.
- Storage corruption/torn write.
- Target-specific undefined or timing-sensitive behavior.
- Hardware evidence missing for a hardware-backed claim.

## 9. Gate 19 decision rule

Every hostile scenario shall be CLOSED by evidence, formally NOT_APPLICABLE with rationale/evidence, or remain an explicit HOLD blocker. Silence, absence of a failing test, or successful unrelated tests do not close an adversarial scenario.