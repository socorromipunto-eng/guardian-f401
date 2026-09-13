# Guardian F401 M16 R3D / R3E Governed Scope Specification

Document status: GOVERNED_SCOPE_DECISION  
Milestone: M16  
Scope family: C5-R3 downstream architecture  
Human adjudication: APPROVED  
Implementation authorization: NOT GRANTED  
NodeSupervisor implementation authorization: NOT GRANTED  
Physical actuation authority: NOT GRANTED  
AI actuation authority: NOT GRANTED  

## 1. Purpose

This document records the human-adjudicated scope boundary for the
post-R3C workstreams designated C5-R3D and C5-R3E.

The purpose of this document is to freeze architecture scope and dependency
ordering before implementation.

This document does not itself authorize implementation, staging, commit,
push, pull request creation, merge, release, tag, publication,
certification, hardware programming, physical actuation, or NodeSupervisor
implementation.

Architecture approval is not implementation approval.

## 2. Governing principles

The following Guardian separations remain normative for this boundary:

REGISTERED != APPROVED

APPROVED != IMPLEMENTED

IMPLEMENTED != VALIDATED

VALIDATED != EVIDENCE_PRESENT

AUTHENTICATED != FRESH

FRESH != AUTHORIZED

AUTHORIZED != ACTUATION_AUTHORIZED

PERSISTENCE != PERSISTENT_ANTI_REPLAY

VALID_SLOT != CURRENT_SLOT_PROOF

HIGHER_VISIBLE_GENERATION != STRONG_ROLLBACK_RESISTANCE

DUAL_SLOT_ATOMICITY != ANTI_ROLLBACK

FLASH_PERSISTENCE != PERSISTENT_ANTI_REPLAY

VALIDATION != AUTHORITY

AUTHORITY != ACTUATION

SAME_TOKEN != SAME_CONCEPT

Claim strength must not exceed evidence strength.

Unknown, unsupported, contradictory, or semantically unresolved
authority-relevant conditions fail closed.

## 3. Existing R3C boundary

C5-R3C established the bounded NodeLink freshness and persistence software
architecture, including:

- runtime freshness state and evaluation;
- persistent freshness record semantics;
- persistence-state classification;
- transactional persistence semantics;
- canonical serialization and integrity encoding;
- a physical persistence backend software implementation;
- an STM32F401 persistence adapter;
- host-side and local Technical Destruction evidence;
- target/linker software-image evidence.

R3C-E-E hardware evidence remains blocked by a physical prerequisite.

Current physical boundary:

R3C_E_E_E_F_STATUS =
CLOSED_BLOCKED_BY_PHYSICAL_PREREQUISITE

Reopen condition:

FUNCTIONAL_JLINK_OR_STLINK_PHYSICALLY_ENUMERATED

Resume boundary:

R3C_E_E_E_G_PROBE_IDENTITY_AND_TARGET_ATTACH_PREFLIGHT

The absence of physical hardware does not retroactively invalidate the
software evidence already established.

It also does not permit promotion of hardware claims.

The following remain NOT DEMONSTRATED:

- real target flash programming;
- physical flash readback;
- reboot recovery on hardware;
- power-loss recovery on hardware;
- physical durability;
- strong rollback resistance;
- persistent anti-replay across the governed physical lifecycle.

## 4. C5-R3D governed scope

C5-R3D is designated:

PERSISTENT_ANTI_REPLAY_ROLLBACK_ANCHOR_AND_RECOVERY

### 4.1 Objective

R3D shall define and, only after a separate implementation authorization,
materialize the security architecture required to distinguish ordinary
persistent storage from a justified persistent anti-replay claim.

R3D exists because persistence alone is not proof that an attacker cannot
restore an older but internally valid state.

### 4.2 Required architecture decisions

R3D shall define at minimum:

1. the exact cross-reboot anti-replay property being claimed;
2. the attacker capability model relevant to rollback;
3. the trust assumptions of any rollback anchor;
4. the boundary between integrity detection and rollback detection;
5. classification of state loss, corruption, truncation, unavailability,
   torn update, and rollback suspicion;
6. the exact transition to FRESHNESS_UNKNOWN and REJOIN_REQUIRED;
7. recovery behavior that cannot silently erase anti-replay history;
8. generation exhaustion and wrap behavior;
9. state replacement and restoration semantics;
10. rollback-anchor lifecycle and provisioning assumptions;
11. negative and hostile rollback tests;
12. evidence required before any persistent anti-replay claim promotion.

### 4.3 Required fail-closed properties

R3D shall preserve:

PERSISTENT_STATE_LOSS = REJOIN_REQUIRED

for previously established governed freshness state where loss,
rollback, corruption, or unavailability prevents safe continuation.

NO_PERSISTED_STATE for a genuinely never-established exact governed identity
must not be silently conflated with previous-state loss.

An old but structurally valid persistence image must not become authoritative
merely because it passes integrity verification.

Cryptographic integrity of a record does not prove that it is the newest
authorized record.

### 4.4 R3D exclusions

R3D does not itself authorize:

- NodeSupervisor implementation;
- NodeSupervisor authority;
- physical actuation;
- automatic control transfer;
- AI authority;
- AI actuation;
- production certification;
- release;
- tag;
- publication;
- automatic bootstrap;
- automatic rejoin authorization.

### 4.5 R3D claim restrictions

Until R3D has architecture, implementation, tests, Technical Destruction,
evidence, human adjudication, and applicable hardware evidence:

ROLLBACK_ANCHOR_DEMONSTRATED = NO

PERSISTENT_ANTI_REPLAY_DEMONSTRATED = NO

## 5. C5-R3E governed scope

C5-R3E is designated:

NODESUPERVISOR_CONSUMPTION_AND_AUTHORITY_READINESS

### 5.1 Objective

R3E shall define the architecture boundary under which a future
NodeSupervisor may consume governed NodeLink evidence without converting
successful parsing, authentication, semantic compatibility, freshness,
health, or peer state into authority that was never granted.

### 5.2 Required architecture decisions

R3E shall define at minimum:

1. the exact NodeSupervisor input contract;
2. authentication prerequisites before authority-relevant consumption;
3. pre-freshness semantic compatibility requirements;
4. freshness and replay classification requirements;
5. documentary authority resolution requirements;
6. evidence provenance requirements;
7. unresolved and contradictory authority handling;
8. SAFE_HOLD interpretation across heterogeneous peers;
9. explicit prohibition of automatic authority transfer;
10. explicit separation between observation, recommendation, authorization,
    and actuation;
11. hostile tests for authority escalation;
12. negative tests for stale, replayed, incompatible, unresolved, malformed,
    partially valid, and contradictory peer evidence;
13. the gate that must close before NodeSupervisor implementation can be
    separately authorized.

### 5.3 NodeSupervisor consumption ordering

Authority-relevant NodeSupervisor consumption must not occur from raw or
merely parsed traffic.

The architecture shall preserve an ordering equivalent to:

wire input
-> structural validation
-> authentication
-> governed semantic interpretation
-> pre-freshness compatibility
-> freshness / replay evaluation
-> evidence classification
-> documentary authority resolution
-> deterministic policy boundary
-> separately authorized downstream action

No earlier stage grants the authority of a later stage.

### 5.4 SAFE_HOLD semantic boundary

Remote SAFE_HOLD is evidence about the remote node.

Remote SAFE_HOLD does not automatically force local SAFE_HOLD.

Remote SAFE_HOLD does not transfer authority.

Remote DEGRADED does not transfer authority.

Remote health does not transfer authority.

Peer presence does not transfer authority.

A local deterministic policy may consume governed peer evidence only under
a separately defined and authorized authority contract.

### 5.5 AI boundary

AI remains advisory only.

AI output must not directly:

- authorize actuation;
- modify deterministic actuation authority;
- bypass NodeSupervisor authority gates;
- bypass freshness;
- bypass semantic compatibility;
- bypass documentary authority;
- promote evidence claims;
- convert uncertainty into authority.

AI_ADVISORY != AUTHORITY

AI_ADVISORY != ACTUATION_AUTHORITY

### 5.6 R3E exclusions

R3E scope approval does not authorize:

- NodeSupervisor implementation;
- NodeSupervisor deployment;
- direct actuation authority;
- automatic takeover;
- automatic failover authority;
- remote control transfer;
- AI actuation authority;
- release;
- tag;
- publication;
- certification.

## 6. Dependency ordering

The governed architecture ordering is:

R3C-E
-> R3D architecture
-> separately authorized R3D implementation
-> R3D positive tests
-> R3D hostile and negative tests
-> R3D Technical Destruction
-> R3D evidence
-> R3D human adjudication
-> R3E architecture
-> R3E authority-readiness gate
-> separately authorized NodeSupervisor implementation

R3C-E-E physical hardware evidence remains a parallel deferred evidence
boundary and resumes only when its physical prerequisite exists.

Architecture work that does not make physical-hardware claims may continue
while R3C-E-E is blocked.

## 7. Evidence promotion requirements

No material claim may be promoted solely because code exists.

No material claim may be promoted solely because a test exists.

No material claim may be promoted solely because a test passes locally.

No material claim may be promoted solely because CI passes.

Promotion requires evidence appropriate to the exact claim.

Hardware claims require hardware evidence.

Rollback-resistance claims require evidence against the governed rollback
threat model.

Authority claims require explicit authority evidence.

Actuation claims require explicit actuation-authority evidence.

## 8. Required engineering sequence

For both R3D and R3E, the required engineering sequence is:

Architecture
-> Source of Truth
-> Requirements
-> Implementation
-> Positive Tests
-> Hostile / Negative Tests
-> Technical Destruction
-> CI
-> Human Readability Review
-> Devil's Advocate Review
-> Evidence
-> Human Adjudication
-> Commit Review
-> Pull Request
-> Protected-Main Merge

Each material mutation boundary requires explicit human authorization.

File mutation does not imply staging authorization.

Staging does not imply commit authorization.

Commit does not imply push authorization.

Push does not imply pull-request authorization.

Pull-request creation does not imply merge authorization.

Merge does not imply release, tag, publication, certification, or deployment
authorization.

## 9. Current non-claims

At adoption of this scope decision:

R3C_CLOSED = NO

R3D_IMPLEMENTED = NO

R3D_VALIDATED = NO

R3D_EVIDENCE_COMPLETE = NO

R3E_IMPLEMENTED = NO

R3E_VALIDATED = NO

R3E_EVIDENCE_COMPLETE = NO

ROLLBACK_ANCHOR_DEMONSTRATED = NO

PERSISTENT_ANTI_REPLAY_DEMONSTRATED = NO

NODESUPERVISOR_CONSUMPTION_AUTHORIZED = NO

NODESUPERVISOR_IMPLEMENTATION_AUTHORIZED = NO

ACTUATION_AUTHORITY_GRANTED = NO

AI_AUTHORITY_GRANTED = NO

RELEASE_READY = NO

## 10. Human adjudication

Human adjudication approved the following scope decisions:

C5-R3D =
PERSISTENT_ANTI_REPLAY_ROLLBACK_ANCHOR_AND_RECOVERY

C5-R3E =
NODESUPERVISOR_CONSUMPTION_AND_AUTHORITY_READINESS

The approved dependency direction is:

R3C-E -> R3D -> R3E

This approval freezes scope only.

It does not authorize implementation.

The first authorized repository mutation associated with this adjudication
is creation of this governed scope document only.

Staging remains a separate human-controlled boundary.

## 11. Next boundary

After successful creation and bounded validation of this document:

NEXT_BOUNDARY =
HUMAN_REVIEW_OF_GOVERNED_SCOPE_DOCUMENT_DIFF

Only after human review may staging be separately authorized.

STOP_BEFORE_STAGING = YES
