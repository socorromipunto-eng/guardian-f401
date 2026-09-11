# ADR-M16-005 — Peer Freshness, Epoch, Replay, and Semantic Compatibility

## Status

ACCEPTED FOR ARCHITECTURE SOURCE-OF-TRUTH MATERIALIZATION

## Milestone

M16-C

## Decision Type

Architecture / Security / Multi-Node Assurance / Semantic Compatibility

## Depends on

- ADR-M15-001 — Node Identity and Signed Assurance Messages
- ADR-M16-001 — Heterogeneous Dual-Node Supervision
- ADR-M16-002 — Versioned Semantic Contract Architecture
- ADR-M16-004 — Evidence / Claim Promotion Governance

## Context

Guardian F401 is evolving toward heterogeneous dual-node supervision in which
an STM32F401 primary deterministic Guardian node and an NXP MCXN947 supervisory
peer may exchange authenticated NodeLink messages.

Authentication alone is not freshness.

A signed epoch alone does not establish that the epoch is current.

A monotonically increasing sequence number within one session does not establish
persistent anti-replay across reset, reboot, loss of persistent state, rollback,
or replacement of a peer.

Likewise, protocol compatibility does not establish semantic compatibility.

M16-C therefore defines the architecture contract that separates:

- session ordering;
- epoch identity and transition;
- persistent freshness and anti-replay;
- recovery after freshness-state loss;
- semantic compatibility;
- authority and actuation.

This architecture is defined before NodeSupervisor implementation.

## Normative invariants

The following implications are prohibited:

SEQUENCE_MONOTONICITY != PERSISTENT_FRESHNESS

SIGNED_EPOCH != CURRENT_EPOCH

AUTHENTICATED != FRESH

FRESH != TRUTHFUL

FRESH != AUTHORIZED

SESSION_SEQUENCE != CROSS_REBOOT_ANTI_REPLAY

REMOTE_RESET_CLAIM != REMOTE_RESET_PROVEN

EPOCH_REUSE != NEW_EPOCH

PROTOCOL_COMPATIBLE != SEMANTICALLY_COMPATIBLE

SEMANTICALLY_COMPATIBLE != AUTHORIZED

AUTHORIZED != ACTUATION_AUTHORIZED

UNKNOWN_EPOCH_STATE = FAIL_CLOSED

UNKNOWN_REPLAY_STATE = FAIL_CLOSED

UNKNOWN_SEMANTIC_COMPATIBILITY = FAIL_CLOSED

PERSISTENT_STATE_LOSS = REJOIN_REQUIRED

The existing Guardian invariant remains:

AI/advisory != authority != actuation

## 1. Session ordering

NodeLink session ordering may use a bounded 32-bit sequence number.

A receiver may use sequence state to detect:

- duplicate sequence values;
- regressing sequence values;
- forward progression;
- gaps requiring bounded policy treatment.

Session-local ordering proves only ordering relative to the active session state.

It does not establish continuity across reboot or persistent anti-replay.

A duplicate or regressing sequence must not silently be interpreted as fresh.

A large forward jump must be handled by an explicit bounded gap policy rather
than accepted merely because it is numerically greater.

### 1.1 Sequence exhaustion and wraparound

The 32-bit sequence space is finite.

The transition:

0xFFFFFFFF -> 0x00000000

SHALL NOT be interpreted as continuation of the same session.

Sequence exhaustion requires a separately governed session replacement or
equivalent freshness transition.

Therefore:

SEQUENCE_EXHAUSTION -> SESSION_REPLACEMENT_REQUIRED

SEQUENCE_WRAP_WITHIN_SAME_SESSION = FAIL_CLOSED

No implementation may infer wraparound acceptance merely from modular integer
arithmetic.

## 2. Epoch model

Each peer exchange that depends on freshness SHALL resolve an explicit epoch
context.

Conceptually, the receiver tracks at least:

- local epoch;
- remote observed epoch;
- prior accepted remote epoch;
- transition state;
- provenance of the transition decision.

An epoch value may be authenticated as part of a signed transcript.

Authentication of the epoch proves only that the authenticated peer asserted
that value.

It does not prove that:

- the epoch is new;
- the epoch is current;
- the peer actually rebooted;
- the receiver's prior state is complete;
- replay has not occurred.

Therefore:

REMOTE_EPOCH_OBSERVED != REMOTE_EPOCH_ACCEPTED

REMOTE_RESET_CLAIM != REMOTE_RESET_PROVEN

EPOCH_REUSE != NEW_EPOCH

Unverified reuse of a previously accepted epoch fails closed for
authority-relevant processing.

The future epoch mechanism must provide uniqueness, monotonicity, or an
equivalent governed property sufficient for the applicable replay threat model.
This ADR does not select the mechanism.

## 3. Epoch transition

A change in remote epoch is a security-relevant state transition.

A receiver SHALL NOT silently replace a previously accepted epoch with a new
epoch merely because the new value is authenticated.

The transition requires an explicit acceptance rule.

That rule may depend on:

- trusted rejoin or provisioning context;
- persisted receiver state;
- allowed epoch transition semantics;
- peer identity;
- key identity and lifecycle state;
- semantic compatibility;
- bounded recovery policy.

Unknown, ambiguous, rollback-like, historically impossible, reused, or
unsupported epoch transitions fail closed for trust-, authority-, supervision-,
safety-, and actuation-relevant paths.

## 4. Persistent freshness anchor

Persistent anti-replay requires state that survives the lifecycle boundary for
which replay protection is claimed.

The architecture SHALL maintain a persistent freshness anchor or equivalent
governed mechanism when cross-reboot replay resistance is required.

Conceptually, the anchor binds:

- peer logical identity;
- accepted peer epoch;
- accepted sequence state or equivalent monotonic state;
- compatibility context where required;
- provenance/version of the freshness rule;
- integrity state of the stored anchor.

The persistence provider must have explicit crash-consistency and integrity
semantics. A reader must be able to distinguish at least:

- VALID_PERSISTED_STATE;
- NO_PERSISTED_STATE;
- CORRUPTED_STATE;
- TORN_OR_INCOMPLETE_UPDATE;
- ROLLBACK_SUSPECTED;
- UNAVAILABLE_STATE.

NO_PERSISTED_STATE means that no valid persisted freshness record has yet been
established for the exact governed freshness identity.

NO_PERSISTED_STATE is a legitimate bootstrap classification. It does not imply
that previously established freshness state was lost.

Any state other than VALID_PERSISTED_STATE is not silently accepted as a valid
freshness anchor.

The exact storage technology is not selected by this ADR.

Persistent-state rollback detection is NOT_DEMONSTRATED.

Persistent anti-replay SHALL NOT be claimed until the selected persistence
mechanism demonstrates the required anti-rollback property or an equivalent
governed freshness anchor.

## 5. Persistent state loss

NO_PERSISTED_STATE and persistent-state loss are distinct conditions.

For an exact governed freshness identity with no previously established valid
persisted freshness record:

NO_PERSISTED_STATE
-> UNINITIALIZED
-> OBSERVE_ONLY

NO_PERSISTED_STATE MUST NOT automatically accept an epoch, automatically accept
a sequence, establish freshness, establish authority, or establish actuation
authority.

NO_PERSISTED_STATE MUST NOT require REJOIN_REQUIRED solely because no prior
persisted freshness record exists.

PERSISTENT_STATE_LOSS_AFTER_ESTABLISHMENT != NO_PERSISTED_STATE

Loss, corruption, rollback, deletion, torn update, incomplete update, or
unavailability of required freshness state after establishment must not silently
reset trust.

The following are freshness/security conditions, not additions to the
operational state vocabulary defined by ADR-M16-001:

- FRESHNESS_UNKNOWN;
- REJOIN_REQUIRED;
- SESSION_REPLACEMENT_REQUIRED.

The operational state machine remains bounded by the separately governed
Guardian state vocabulary.

The required security-condition transition is:

PERSISTENT_FRESHNESS_STATE_LOST
-> FRESHNESS_UNKNOWN
-> REJOIN_REQUIRED

For authority-relevant or safety-relevant paths, the operational consequence is:

REJOIN_REQUIRED
-> SAFE_HOLD

until an explicitly governed rejoin procedure establishes acceptable state.

Persistent-state loss SHALL NOT cause old messages to become fresh again.

## 6. Replay classes

M16-C distinguishes at least:

- duplicate replay within an active session;
- sequence regression within an active session;
- replay from an earlier session in the same epoch;
- replay from an earlier epoch;
- replay after receiver reboot;
- replay after peer reboot;
- replay after freshness-state loss or rollback;
- replay under a semantically incompatible context.

A mechanism that detects only one replay class SHALL NOT be described as
providing general persistent anti-replay.

If a future design permits bounded out-of-order acceptance or a replay window,
that behavior requires an explicit versioned contract. It must not be inferred
from generic sequence comparison.

## 7. Remote reboot observation

A remote reboot may be inferred from evidence such as a changed authenticated
epoch or a rejoin exchange.

However, reboot is still an observed peer claim unless independently proven by
the governed mechanism.

The system SHALL preserve the distinction:

remote reboot observation != physical reboot proof

A reboot observation may trigger policy but does not grant authority.

## 8. Semantic compatibility context

A freshness decision is insufficient when the message meaning itself is not
compatible with the receiver's governed interpretation.

The effective semantic compatibility context may include:

- node identity;
- chip family;
- hardware revision;
- platform or boot revision;
- firmware version;
- protocol version;
- semantic profile version;
- system role;
- security configuration;
- compatibility contract version.

A value without required version context is semantically incomplete.

## 9. Compatibility decision

Protocol parsing success is not semantic compatibility.

A receiver SHALL resolve a bounded compatibility decision before using remote
content for trust-, authority-, supervision-, safety-, or actuation-relevant
decisions.

At minimum, compatibility outcomes must distinguish:

- compatible;
- incompatible;
- unknown;
- unsupported;
- historically impossible.

Unknown, unsupported, ambiguous, or historically impossible combinations fail
closed for authority-relevant paths.

No validator, adapter, AI component, or runtime layer may silently normalize
incompatible semantic profiles into a common meaning.

## 10. Compatibility required before freshness interpretation

Freshness fields themselves are semantic data.

The receiver must first establish enough wire and semantic context to know how
epoch, sequence, session identity, and related freshness fields are to be
interpreted.

Therefore the architecture distinguishes:

1. PRE_FRESHNESS_COMPATIBILITY:
   enough compatible context to interpret the freshness contract safely;

2. FRESHNESS_EVALUATION:
   epoch, sequence, replay, and persistent-anchor evaluation;

3. FULL_MESSAGE_SEMANTIC_COMPATIBILITY:
   compatibility of the complete message meaning for later policy use.

A receiver SHALL NOT evaluate epoch or sequence under an unknown or incompatible
freshness grammar.

PROTOCOL_PARSE_SUCCESS != PRE_FRESHNESS_COMPATIBILITY

## 11. Freshness and compatibility ordering

Conceptually, incoming remote content follows a bounded processing order:

1. resource limits;
2. bounded parse;
3. wire-contract validation;
4. closed-contract validation;
5. producer/key resolution;
6. cryptographic authentication;
7. pre-freshness compatibility resolution;
8. epoch resolution;
9. replay/freshness evaluation;
10. full-message semantic compatibility evaluation;
11. evidence/policy evaluation;
12. authority decision;
13. physical actuation gate where separately authorized.

No earlier stage implies success of a later stage.

Unauthenticated or authentication-failed input MUST NOT advance, replace, reset,
or otherwise mutate accepted freshness state.

UNAUTHENTICATED_EPOCH
MUST NOT
ADVANCE_OR_REPLACE_ACCEPTED_FRESHNESS_STATE

Signed logical time or a timestamp, if present, does not by itself establish
trusted wall-clock time or persistent freshness.

## 12. Fail-closed behavior

The following states fail closed for authority-relevant processing:

- epoch unknown;
- epoch transition ambiguous;
- epoch reuse unverified;
- sequence wrap within same session;
- replay state unknown;
- persistent freshness state unavailable;
- persistent freshness state corrupted;
- persistent freshness update torn/incomplete;
- persistent rollback suspected;
- pre-freshness compatibility unknown;
- semantic compatibility unknown;
- semantic compatibility unsupported;
- historical version combination impossible;
- required identity/version context missing.

Fail closed does not mean all telemetry must disappear.

A bounded implementation may preserve diagnostic evidence while denying stronger
trust, authority, or actuation consequences.

## 13. Rejoin

Rejoin is a separately governed recovery boundary.

REJOIN_REQUIRED is a security/freshness condition. It is not a new operational
Guardian state.

Rejoin SHALL NOT be equivalent to "accept the next signed message."

A future implementation of rejoin must define:

- peer identity requirements;
- credential state requirements;
- freshness-anchor establishment;
- epoch acceptance rule;
- semantic compatibility requirements;
- audit evidence;
- failure behavior;
- authority restrictions during recovery.

This ADR defines the requirement but does not implement the procedure.

## 14. Authority boundary

Freshness and semantic compatibility create no actuator authority by themselves.

The following implication is forbidden:

authenticated
+ fresh
+ semantically compatible
-> actuator authorized

Instead, those properties may become inputs to a separately governed
deterministic authority decision.

The physical actuation boundary remains independent.

## 15. Historical continuity

Historical freshness and semantic interpretation must remain reconstructable.

A later software version must not retroactively reinterpret an earlier peer
exchange under a newer semantic profile.

Persistent records, when materialized, must preserve sufficient context to
reconstruct the decision that was valid at the time.

SEMANTIC DRIFT = FAIL

## 16. Nonclaims

This ADR does not demonstrate:

- persistent anti-replay implementation;
- persistent-state rollback detection;
- persistent epoch storage implementation;
- secure-element integration;
- production key custody;
- physical STM32F401 NodeLink execution;
- physical MCXN947 NodeLink execution;
- physical dual-node timing;
- hardware fault tolerance;
- distributed quorum;
- production readiness;
- certification;
- AI advisory-plane implementation.

NodeSupervisor implementation remains NOT_AUTHORIZED.

AI actuation authority remains NO.

## Consequence

M16-C establishes the architecture contract required before a NodeSupervisor
implementation may interpret authenticated peer traffic as fresh, replay-safe,
or semantically compatible.

The next implementation boundary must remain downstream of explicit schema,
policy, tooling, test, and final architecture-gate adjudication.
