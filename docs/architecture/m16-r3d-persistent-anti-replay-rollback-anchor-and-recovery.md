# M16 R3D Persistent Anti-Replay, Rollback Anchor, and Recovery Architecture

Status: HUMAN_ARCHITECTURE_ADJUDICATION_APPROVED
Repository governance status: CANDIDATE_UNTIL_COMMITTED_REVIEWED_AND_MERGED
R3D implementation authorization: NOT GRANTED
R3E implementation authorization: NOT GRANTED
NodeSupervisor implementation authorization: NOT GRANTED
Physical actuation authorization: NOT GRANTED
AI authority: NOT GRANTED
Release readiness: NO

## 1. Purpose

This document defines the architecture contract for:

R3D = PERSISTENT_ANTI_REPLAY_ROLLBACK_ANCHOR_AND_RECOVERY

R3D closes the architectural gap between durable freshness persistence and a
justified persistent anti-replay claim across reboot.

R3D does not reopen R3C persistence architecture.

R3D does not authorize firmware implementation.

R3D does not authorize NodeSupervisor implementation.

R3D does not grant authority or actuation permission.

The central invariant is:

PERSISTENCE != PERSISTENT_ANTI_REPLAY

A persistently stored record can survive reboot while still being vulnerable
to restoration of an older, structurally valid state.

## 2. Governed source-of-truth baseline

This architecture is derived from and constrained by the governed Guardian
baseline present at main commit:

4d4b021a9d271b879b1a50ca3408faf862b6e591

Main tree:

d2a6e36b11185e5522401e5ff64679cac3127196

Primary governed inputs include:

- docs/architecture/m16-r3d-r3e-scope-specification.md
- docs/adr/ADR-M16-005-peer-freshness-epoch-replay-and-semantic-compatibility.md
- docs/specs/M16-NodeLink-Normative-Specification-v0.2.md
- docs/governance/Guardian-Peer-Freshness-Replay-and-Semantic-Compatibility-Policy.md
- docs/validation/c5-r3c-e-physical-persistence-backend-evidence-and-adjudication.md

Relevant implementation and validation surfaces already exist under:

- firmware/NodeLink/
- firmware/Tests/
- assurance/src/guardian_assurance/
- assurance/tests/
- governance/
- docs/validation/

Those existing surfaces are discovery inputs only.

Their existence does not authorize R3D implementation mutation.

## 3. Inherited security separations

R3D SHALL preserve the following distinctions:

PERSISTENCE != PERSISTENT_ANTI_REPLAY

VALID_SLOT != CURRENT_SLOT_PROOF

HIGHER_VISIBLE_GENERATION != STRONG_ROLLBACK_RESISTANCE

DUAL_SLOT_ATOMICITY != ANTI_ROLLBACK

FLASH_PERSISTENCE != PERSISTENT_ANTI_REPLAY

AUTHENTICATION != FRESHNESS

AUTHENTICATION != AUTHORITY

FRESHNESS != AUTHORITY

VALIDATION != AUTHORITY

AUTHORITY != ACTUATION

AI_ADVISORY != AUTHORITY

AI_ADVISORY != ACTUATION

No implementation or evidence produced by R3D may collapse these boundaries.

## 4. Existing R3C persistence baseline

R3C already establishes a persistence architecture for freshness state,
including:

- an exact freshness identity;
- accepted_epoch;
- accepted_sequence;
- record_generation;
- canonical persistence representation;
- integrity checking;
- dual-slot storage;
- torn/incomplete update handling;
- bounded storage failure;
- STM32F401 persistence adaptation;
- recovery classification.

The exact freshness identity includes the governed identity dimensions already
defined by R3C, including:

- sender_node_id;
- producer_id;
- key_id;
- signature_algorithm;
- producer semantic profile;
- consumer semantic profile;
- compatibility contract.

R3D SHALL reuse that governed identity.

R3D SHALL NOT silently create a weaker identity key.

## 5. R3D-D01 - Exact cross-reboot anti-replay property

For an established freshness identity, after Guardian has accepted and
durably committed freshness state S, a reboot SHALL NOT permit an equal or
older freshness state to become current merely because an older persistence
image is structurally valid.

For a given governed freshness identity I, let the accepted freshness state
contain:

- epoch E;
- sequence Q;
- record generation G.

Once state (I, E, Q, G) has crossed the R3D durable acceptance boundary, a
later reboot SHALL NOT cause Guardian to accept a state that represents an
older freshness history for I.

An epoch transition is not exempt merely because the numeric epoch differs.

Epoch transition remains subject to its separately governed authorization
rules.

Persistent anti-replay therefore means continuity of accepted freshness
history across reset, not merely duplicate rejection within one boot session.

## 6. R3D-D02 - Rollback attacker capability model

R3D SHALL assume an attacker may be able to:

1. reset or power-cycle the MCU;
2. restore an older but structurally valid SLOT_A;
3. restore an older but structurally valid SLOT_B;
4. restore both ordinary persistence slots together;
5. copy a previously valid flash persistence image;
6. replay previously authenticated peer traffic;
7. cause power interruption during persistence update;
8. cause interruption between ordinary persistence update and rollback-anchor
   update;
9. present stale but internally well-formed persistence state;
10. exploit ambiguous recovery behavior.

The architecture SHALL NOT assume that SHA-256, a CRC, a commit marker, or
dual-slot flash makes an old state current.

The R3D claim does not require resistance against compromise of all trusted
hardware roots or against cryptographic primitive breakage.

The selected rollback-anchor backend SHALL explicitly declare its own attacker
boundary before any strong rollback-resistance claim is promoted.

## 7. R3D-D03 - Rollback-anchor trust requirements

Persistent anti-replay requires an independent rollback anchor.

The ordinary R3C slots SHALL NOT serve as their own rollback anchor.

The anchor SHALL provide a trust property independent from ordinary SLOT_A /
SLOT_B rollback.

At the architectural boundary, an anchor record is logically bound to:

- exact freshness identity;
- monotonic generation;
- commitment to the corresponding accepted freshness state.

Conceptually:

ANCHOR_RECORD =
    identity_binding
    + monotonic_generation
    + state_commitment

The state commitment SHALL bind the exact security-relevant state needed to
detect substitution of a different freshness record at the same visible
generation.

A conforming backend SHALL provide equivalent protection even if its physical
representation differs.

The architecture does not select the physical anchor mechanism.

Possible future implementations may use hardware monotonic state, protected
nonvolatile state, a secure element, a separate trust device, a heterogeneous
peer with an appropriate trust contract, or another mechanism proven to
satisfy this contract.

No such mechanism is approved merely by being listed here.

## 8. Abstract rollback-anchor contract

A future implementation SHALL expose semantics equivalent to:

anchor_status(identity)

anchor_read(identity)

anchor_compare(identity, candidate_generation, candidate_commitment)

anchor_advance(
    identity,
    expected_generation,
    next_generation,
    next_commitment
)

The abstract contract SHALL provide:

MONOTONIC

NON_WRAPPING

IDENTITY_BOUND

STATE_COMMITMENT_BOUND

INDEPENDENT_FROM_ORDINARY_SLOT_ROLLBACK

FAIL_CLOSED

NO_SILENT_RESET

NO_BOOT_TIME_AUTO_ADVANCE_FROM_UNTRUSTED_SLOT

NO_AUTHORITY_GRANT

The exact implementation API is deferred.

This document defines semantics, not C function signatures.

## 9. Anchor status model

The architecture SHALL distinguish at minimum:

ANCHOR_UNPROVISIONED

ANCHOR_VALID

ANCHOR_UNAVAILABLE

ANCHOR_CORRUPT

ANCHOR_EXHAUSTED

ANCHOR_IDENTITY_MISMATCH

ANCHOR_STATE_MISMATCH

ROLLBACK_SUSPECTED

An unprovisioned anchor is not equivalent to a lost established anchor.

A system SHALL retain enough governed lifecycle information to distinguish a
genuinely never-established state from loss of previously established
freshness continuity.

## 10. R3D-D04 - Integrity versus rollback detection

Ordinary persistence integrity and rollback detection are separate controls.

SHA256_INTEGRITY != AUTHENTICATED_STORAGE

SHA256_INTEGRITY != CURRENT_STATE_PROOF

COMMIT_MARKER != CURRENT_STATE_PROOF

DUAL_SLOT_RECOVERY != CURRENT_STATE_PROOF

GENERATION_NUMBER != CURRENT_STATE_PROOF

A valid slot demonstrates that the slot satisfies its structural and integrity
contract.

It does not, by itself, prove that the slot is the newest state ever accepted.

The rollback anchor exists to establish the missing continuity property.

## 11. R3D-D05 - Persistence and rollback classifications

R3D SHALL preserve the existing R3C classifications:

INVALID

VALID_PERSISTED_STATE

NO_PERSISTED_STATE

CORRUPTED_STATE

TORN_OR_INCOMPLETE_UPDATE

ROLLBACK_SUSPECTED

UNAVAILABLE_STATE

R3D SHALL interpret them in combination with anchor evidence.

No classification may be silently converted into a new trusted baseline.

### 11.1 Genuine never-established state

If both ordinary persistence and the governed anchor lifecycle prove that no
freshness history has ever been established:

NO_PERSISTED_STATE
-> UNINITIALIZED
-> OBSERVE_ONLY

This does not authorize automatic acceptance.

### 11.2 Previously established state cannot be proven

If history was previously established and continuity can no longer be proven:

-> FRESHNESS_UNKNOWN
-> REJOIN_REQUIRED

This includes loss, corruption, rollback suspicion, irreconcilable anchor
mismatch, or unavailable established state.

## 12. R3D-D06 - FRESHNESS_UNKNOWN transition

FRESHNESS_UNKNOWN means Guardian cannot prove continuity from the previously
accepted freshness history.

FRESHNESS_UNKNOWN SHALL fail closed for freshness-dependent authority-relevant
consumption.

FRESHNESS_UNKNOWN SHALL NOT be converted to FRESH merely because:

- a frame authenticates successfully;
- a slot has valid integrity;
- a slot has a high visible generation;
- the peer reports ACTIVE;
- the peer reports SAFE_HOLD;
- a reboot occurred;
- the operator requests normal startup.

FRESHNESS_UNKNOWN does not grant authority.

## 13. R3D-D07 - REJOIN_REQUIRED transition

For previously established freshness state:

PERSISTENT_STATE_LOSS = REJOIN_REQUIRED

ROLLBACK_SUSPECTED = REJOIN_REQUIRED

IRRECONCILABLE_ANCHOR_MISMATCH = REJOIN_REQUIRED

REJOIN_REQUIRED SHALL use a separately governed recovery/rejoin authority path.

Ordinary peer traffic SHALL NOT self-authorize rejoin.

BOOTSTRAP and REJOIN remain distinct lifecycle operations.

BOOTSTRAP != REJOIN

## 14. R3D-D08 - Recovery without silent history reset

Recovery SHALL operate from anchor evidence first, not from the largest visible
slot generation alone.

A recovery candidate is current only when the persisted record and anchor are
cryptographically or equivalently bound under the selected backend contract.

The recovery algorithm SHALL NOT:

- reset generation to zero;
- reset accepted_epoch silently;
- reset accepted_sequence silently;
- treat both erased slots as proof of factory state when established anchor
  history exists;
- advance the anchor solely because a future-looking slot exists;
- accept an older valid slot because the newer slot is unavailable;
- convert rollback suspicion into normal bootstrap.

If continuity cannot be established, recovery SHALL fail closed.

## 15. Durable acceptance transaction

R3D SHALL preserve a transaction boundary between:

1. candidate freshness evaluation;
2. durable ordinary persistence;
3. rollback-anchor advancement;
4. externally effective acceptance.

The required ordering is:

A. authenticate and evaluate the candidate under existing policy;

B. derive the next canonical freshness state;

C. derive next_generation without wrap;

D. durably write and verify the inactive R3C persistence slot;

E. keep the previous anchored state recoverable;

F. advance the independent rollback anchor using an expected-current
   generation check and a commitment to the new state;

G. read back or otherwise verify the anchor advancement;

H. only after successful anchor verification may the new state cross the R3D
   durable acceptance boundary;

I. only then may downstream logic treat the state as persistently fresh,
   subject to all later authority gates.

A power loss before anchor advancement SHALL NOT cause an unanchored candidate
to become current after reboot.

A boot path SHALL NOT advance the anchor merely because it discovers an
unanchored future slot.

## 16. Recovery comparison matrix

Let A be the anchor generation.

Let S represent a structurally valid persisted slot generation.

### Case 1 - slot exactly matches anchor

S == A

The slot may be a current-state candidate only if its state commitment also
matches the anchor commitment and all inherited validation succeeds.

### Case 2 - slot older than anchor

S < A

This is evidence that the visible persistence state is behind the accepted
anchor history.

The state SHALL NOT be accepted as current.

Classification:

ROLLBACK_SUSPECTED

and, for established state:

FRESHNESS_UNKNOWN
-> REJOIN_REQUIRED

### Case 3 - slot newer than anchor

S > A

The slot is not automatically current.

It may represent a prepared but not yet anchored transaction.

Boot-time recovery SHALL NOT advance the anchor from this slot.

If an exact A slot remains valid, Guardian may recover that anchored state and
ignore/discard the unanchored future candidate according to later
implementation policy.

If no state matching A can be proven, continuity is not established.

Classification SHALL fail closed.

### Case 4 - generation matches but commitment differs

S == A
but
state_commitment != anchor_commitment

Classification:

ROLLBACK_SUSPECTED
or
CORRUPTED_STATE

The implementation SHALL NOT guess which copy is authoritative.

### Case 5 - anchor exists but no matching valid slot exists

Established anchor history plus no matching persisted state means continuity
cannot be reconstructed locally.

Classification:

FRESHNESS_UNKNOWN
-> REJOIN_REQUIRED

### Case 6 - no anchor and no established history

This is eligible only for the separately governed uninitialized/bootstrap
path.

It is not proof of authorization to accept peer state.

## 17. R3D-D09 - Generation exhaustion and wrap

record_generation SHALL NOT wrap.

An anchor monotonic value SHALL NOT wrap.

UINT_MAX -> 0 or equivalent rollover SHALL NOT be used as recovery.

Before exhaustion, the implementation SHALL transition into a governed
maintenance/reprovisioning state.

At exhaustion:

- automatic freshness continuation is prohibited;
- anchor reset is prohibited;
- silent generation reset is prohibited;
- explicit governed migration/rejoin is required.

Generation exhaustion is a lifecycle event, not a numeric convenience.

## 18. R3D-D10 - Backup, restore, replacement, and cloning

Restoration of ordinary persistence is not automatically restoration of
freshness continuity.

An older backup whose generation or commitment is behind the anchor SHALL be
classified as rollback evidence.

A backup restored without a verifiable corresponding anchor SHALL NOT become a
trusted current state.

Cloning ordinary persistence to another device SHALL NOT transfer freshness
authority.

The exact freshness identity and anchor binding SHALL prevent an unrelated
device or identity from inheriting another identity's accepted history.

Device replacement, anchor replacement, and recovery from backup require
explicit governed lifecycle procedures.

## 19. R3D-D11 - Anchor lifecycle and provisioning

The rollback-anchor lifecycle SHALL explicitly govern:

1. UNPROVISIONED;
2. PROVISIONING_AUTHORIZED;
3. INITIALIZED;
4. ACTIVE;
5. ADVANCING;
6. ACTIVE_UPDATED;
7. UNAVAILABLE;
8. CORRUPT_OR_MISMATCHED;
9. EXHAUSTED;
10. REJOIN_REQUIRED;
11. REPLACEMENT_AUTHORIZED;
12. DECOMMISSIONED.

Provisioning SHALL be bound to the exact governed identity context.

Factory reset, service reset, firmware update, debug attach, mass erase, board
replacement, or storage replacement SHALL NOT silently reclassify an
established identity as never initialized.

Any operation that destroys established anchor continuity SHALL result in a
fail-closed recovery state unless a separately governed recovery procedure
proves otherwise.

## 20. R3D-D12 - Required hostile and negative tests

R3D implementation validation SHALL include at minimum:

### Ordinary persistence rollback

- restore old SLOT_A;
- restore old SLOT_B;
- restore both old slots;
- restore a complete older persistence image;
- restore an older but structurally valid canonical record.

### Anchor/persistence disagreement

- old slot plus newer anchor;
- newer unanchored slot plus older anchor;
- same generation with different state commitment;
- wrong identity bound to valid anchor;
- valid slot with unavailable anchor;
- valid anchor with unavailable slots.

### Power-loss matrix

Inject interruption:

- before slot erase;
- during erase;
- after erase;
- during header write;
- during payload write;
- before slot commit;
- after slot commit;
- before anchor advance;
- during anchor advance where the selected backend permits observable failure;
- after anchor advance;
- before downstream acceptance.

Each interruption point SHALL have deterministic recovery expectations.

### Replay tests

- duplicate authenticated message after reboot;
- lower sequence after reboot;
- older epoch replay;
- previously accepted record restored after reboot;
- replay following partial persistence update;
- replay following failed anchor advance.

### Exhaustion tests

- generation maximum minus one;
- generation maximum;
- prohibited wrap attempt;
- anchor exhaustion;
- recovery after exhaustion.

### Recovery abuse tests

- erase both ordinary slots while preserving anchor;
- lose anchor while preserving slots;
- swap persistence between identities;
- clone storage to another node;
- restore backup from earlier generation;
- attempt automatic bootstrap after established-state loss;
- attempt automatic rejoin from ordinary peer traffic.

Passing positive-path tests alone is insufficient.

## 21. R3D-D13 - Evidence gate for claim promotion

The following claims remain prohibited at architecture stage:

ROLLBACK_ANCHOR_DEMONSTRATED = NO

PERSISTENT_ANTI_REPLAY_DEMONSTRATED = NO

R3D_IMPLEMENTED = NO

R3D_VALIDATED = NO

R3D_EVIDENCE_COMPLETE = NO

These claims SHALL NOT transition to YES solely because this architecture is
approved or merged.

Claim promotion requires, at minimum:

1. separately authorized implementation;
2. exact implementation-to-architecture traceability;
3. positive tests;
4. negative tests;
5. hostile rollback tests;
6. power-loss tests applicable to the selected backend;
7. generation exhaustion tests;
8. recovery and restore tests;
9. Technical Destruction;
10. structured evidence;
11. exact commit and tool identity;
12. CI execution evidence;
13. explicit limitations and unresolved conditions;
14. human adjudication.

Physical hardware claims require physical hardware evidence.

Simulation, host tests, or architectural reasoning SHALL NOT be promoted into
physical durability evidence.

## 22. Backend selection rule

This architecture intentionally does not select a physical rollback-anchor
backend.

A candidate backend SHALL be evaluated against the abstract anchor contract.

Backend selection SHALL explicitly document:

- physical trust boundary;
- rollback resistance mechanism;
- tamper assumptions;
- write endurance;
- monotonic capacity;
- provisioning method;
- replacement method;
- failure behavior;
- recovery behavior;
- identity binding;
- state commitment binding;
- availability limitations;
- hardware evidence requirements.

If a candidate backend cannot satisfy the contract, Guardian SHALL weaken the
claim rather than weaken the architecture silently.

## 23. Heterogeneous-system rule

R3D SHALL remain usable across heterogeneous Guardian nodes.

The architecture contract SHALL NOT depend semantically on one MCU vendor.

STM32F401 remains the deterministic/reference control node at the current
project stage.

Future heterogeneous peers may use different physical anchor mechanisms.

Equivalent semantics are required.

Equivalent silicon is not required.

INTEROPERABILITY != SHARED_INTERNAL_IMPLEMENTATION

## 24. NodeSupervisor boundary

R3D does not authorize NodeSupervisor.

Persistent freshness evidence is an input to later R3E authority-readiness
architecture.

Persistent freshness SHALL NOT directly produce:

- NodeSupervisor authority;
- control transfer;
- actuator authority;
- physical actuation;
- unilateral override authority.

The downstream ordering remains governed separately.

## 25. AI boundary

AI remains advisory only.

No AI output may:

- initialize a rollback anchor;
- reset an anchor;
- advance an anchor;
- authorize rejoin;
- authorize bootstrap;
- suppress rollback suspicion;
- convert FRESHNESS_UNKNOWN to FRESH;
- grant authority;
- grant actuation permission.

AI_ADVISORY != AUTHORITY != ACTUATION

## 26. Explicit non-claims

This architecture does not claim:

- authenticated ordinary flash storage;
- tamper-proof STM32F401 flash;
- production hardware rollback resistance;
- physical durability;
- secure-element deployment;
- TPM deployment;
- OTP deployment;
- hardware monotonic-counter deployment;
- MCXN947 implementation;
- NodeSupervisor implementation;
- NodeSupervisor authority;
- physical actuation authority;
- AI authority;
- certification;
- regulatory approval;
- production readiness;
- release readiness.

## 27. Decision register

R3D-D01 = APPROVED
EXACT_CROSS_REBOOT_ANTI_REPLAY_PROPERTY

R3D-D02 = APPROVED
ROLLBACK_ATTACKER_CAPABILITY_MODEL

R3D-D03 = APPROVED
ROLLBACK_ANCHOR_TRUST_ASSUMPTIONS

R3D-D04 = APPROVED
INTEGRITY_VS_ROLLBACK_DETECTION_BOUNDARY

R3D-D05 = APPROVED
STATE_LOSS_CORRUPTION_TRUNCATION_UNAVAILABLE_TORN_ROLLBACK_CLASSIFICATION

R3D-D06 = APPROVED
FRESHNESS_UNKNOWN_TRANSITION

R3D-D07 = APPROVED
REJOIN_REQUIRED_TRANSITION

R3D-D08 = APPROVED
RECOVERY_WITHOUT_SILENT_HISTORY_RESET

R3D-D09 = APPROVED
GENERATION_EXHAUSTION_AND_WRAP

R3D-D10 = APPROVED
STATE_REPLACEMENT_AND_RESTORATION

R3D-D11 = APPROVED
ANCHOR_LIFECYCLE_AND_PROVISIONING

R3D-D12 = APPROVED
NEGATIVE_AND_HOSTILE_ROLLBACK_TESTS

R3D-D13 = APPROVED
EVIDENCE_GATE_FOR_CLAIM_PROMOTION

R3D_REQUIRED_DECISION_COUNT = 13

## 28. Human adjudication

Human architecture adjudication:

APPROVED

This approval authorizes creation of this architecture candidate.

It does not authorize R3D implementation.

It does not authorize staging, commit, push, pull request, merge, tag, release,
firmware mutation, NodeSupervisor implementation, or physical actuation.

Repository-governed status is obtained only through the separately authorized
repository governance process.

## 29. Current project claim state

R3D_SCOPE_GOVERNED = YES

R3D_ARCHITECTURE_ADJUDICATED = YES

R3D_ARCHITECTURE_DOCUMENT_CREATED = YES

R3D_ARCHITECTURE_GOVERNED = NO

R3D_IMPLEMENTATION_AUTHORIZED = NO

R3D_IMPLEMENTED = NO

R3D_VALIDATED = NO

R3D_EVIDENCE_COMPLETE = NO

ROLLBACK_ANCHOR_DEMONSTRATED = NO

PERSISTENT_ANTI_REPLAY_DEMONSTRATED = NO

R3E_IMPLEMENTATION_AUTHORIZED = NO

NODESUPERVISOR_IMPLEMENTATION_AUTHORIZED = NO

ACTUATION_AUTHORITY_GRANTED = NO

AI_AUTHORITY_GRANTED = NO

RELEASE_READY = NO

## 30. Next governance boundary

After document creation, the next boundary is:

HUMAN_R3D_ARCHITECTURE_DOCUMENT_REVIEW

No staging is permitted before that review is complete.
