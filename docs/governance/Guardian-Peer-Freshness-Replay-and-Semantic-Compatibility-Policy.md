# Guardian Peer Freshness, Replay, and Semantic Compatibility Policy

## 1. Purpose

This policy governs how Guardian evaluates peer-message ordering, epoch state,
persistent freshness, replay risk, semantic compatibility, recovery, and the
relationship of those properties to authority.

It prevents authenticated or well-formed peer traffic from silently escalating
into trusted, current, compatible, authorized, or actuation-capable state.

## 2. Mandatory separations

The following separations are normative:

- sequence monotonicity is not persistent freshness;
- authenticated epoch is not proof of current epoch;
- authenticated is not fresh;
- fresh is not truthful;
- fresh is not authorized;
- session ordering is not cross-reboot anti-replay;
- remote reset claim is not physical reset proof;
- epoch reuse is not a new epoch;
- protocol compatibility is not semantic compatibility;
- semantic compatibility is not authority;
- authority is not actuation authorization.

AI/advisory != authority != actuation.

## 3. Session ordering policy

A session-ordering mechanism may track sequence progression within a bounded
active session.

It must reject or explicitly classify:

- duplicate values;
- regressing values;
- invalid transitions;
- gaps outside the configured bounded policy.

Session ordering must not be described as persistent anti-replay unless the
persistent lifecycle boundary is independently satisfied.

For a 32-bit sequence space:

SEQUENCE_EXHAUSTION -> SESSION_REPLACEMENT_REQUIRED

0xFFFFFFFF -> 0x00000000

must not be treated as continuation of the same session.

SEQUENCE_WRAP_WITHIN_SAME_SESSION = FAIL_CLOSED

SESSION_REPLACEMENT_REQUIRED is a security/freshness condition, not a new
operational Guardian state.

## 4. Epoch policy

Peer epoch is a security-relevant claim.

An authenticated epoch may be considered authentic as a statement by the peer,
but not automatically current.

A new observed epoch must be adjudicated against explicit transition rules.

Unknown or ambiguous epoch transitions fail closed for authority-relevant use.

EPOCH_REUSE != NEW_EPOCH

Unverified epoch reuse fails closed.

The future epoch mechanism must demonstrate uniqueness, monotonicity, or an
equivalent governed property sufficient for its replay threat model.

## 5. Persistent freshness policy

Cross-reboot anti-replay requires a persistent freshness anchor or an equivalent
governed mechanism.

The anchor must be independently meaningful and integrity-protected.

Where the architecture claims replay protection across a lifecycle boundary,
required state must survive that boundary.

The persistence provider must explicitly distinguish:

- VALID_PERSISTED_STATE;
- NO_PERSISTED_STATE;
- CORRUPTED_STATE;
- TORN_OR_INCOMPLETE_UPDATE;
- ROLLBACK_SUSPECTED;
- UNAVAILABLE_STATE.

NO_PERSISTED_STATE means that no valid persisted freshness record has yet been
established for the exact governed freshness identity.

Only VALID_PERSISTED_STATE is eligible for normal freshness continuation.

Persistent-state rollback detection is NOT_DEMONSTRATED.

Persistent anti-replay must not be claimed until the selected persistence
mechanism demonstrates the required anti-rollback property or an equivalent
governed freshness anchor.

## 6. Freshness-state loss

NO_PERSISTED_STATE and persistent-state loss are distinct conditions.

For an exact governed freshness identity with no previously established valid
persisted freshness record:

NO_PERSISTED_STATE
-> UNINITIALIZED
-> OBSERVE_ONLY

OBSERVE_ONLY != ACCEPT

NO_PERSISTED_STATE MUST NOT automatically accept an epoch, automatically accept
a sequence, establish freshness, establish authority, or establish actuation
authority.

NO_PERSISTED_STATE MUST NOT require REJOIN_REQUIRED solely because no prior
persisted freshness record exists.

PERSISTENT_STATE_LOSS_AFTER_ESTABLISHMENT != NO_PERSISTED_STATE

If required persistent freshness state is lost, corrupted, rolled back,
unavailable, torn, incomplete, or otherwise not trustworthy after establishment:

FRESHNESS_STATE = FRESHNESS_UNKNOWN

FRESHNESS_UNKNOWN -> REJOIN_REQUIRED

FRESHNESS_UNKNOWN and REJOIN_REQUIRED are security/freshness conditions, not
additions to the operational state vocabulary defined by ADR-M16-001.

For authority-relevant or safety-relevant paths:

REJOIN_REQUIRED -> SAFE_HOLD

Old peer messages must not become acceptable merely because local anti-replay
state was lost.

## 7. Replay handling

Guardian must distinguish replay mechanisms by the replay class they actually
cover.

At minimum, policy analysis must distinguish:

1. duplicate replay in the active session;
2. sequence regression;
3. prior-session replay;
4. prior-epoch replay;
5. replay across receiver reboot;
6. replay across peer reboot;
7. replay after persistent-state loss or rollback;
8. replay under incompatible semantic context.

Coverage of one class must not be generalized to all classes.

Any future bounded out-of-order acceptance or replay window must be explicitly
versioned and governed.

## 8. Remote reset policy

A peer may report or imply that it reset.

That assertion may be authenticated but remains peer-originated evidence.

A remote reset observation does not automatically authorize epoch replacement,
trust reset, or authority escalation.

## 9. Semantic compatibility policy

Guardian must resolve semantic compatibility from explicit context.

Relevant context may include:

- node identity;
- chip family;
- hardware revision;
- platform revision;
- firmware version;
- protocol version;
- semantic profile version;
- system role;
- security configuration;
- compatibility-contract version.

A missing required context item makes the interpretation semantically
incomplete.

## 10. Compatibility outcomes

The bounded compatibility vocabulary shall support at least:

- COMPATIBLE
- INCOMPATIBLE
- UNKNOWN
- UNSUPPORTED
- HISTORICALLY_IMPOSSIBLE

Only an explicitly governed COMPATIBLE result is eligible to proceed to later
policy stages for authority-relevant use.

INCOMPATIBLE, UNKNOWN, UNSUPPORTED, and HISTORICALLY_IMPOSSIBLE fail closed for
authority-relevant paths.

## 11. Pre-freshness compatibility

Epoch and sequence cannot be safely interpreted before the receiver establishes
enough compatible semantic context to understand their contract.

The receiver must distinguish:

- PRE_FRESHNESS_COMPATIBILITY;
- FRESHNESS_EVALUATION;
- FULL_MESSAGE_SEMANTIC_COMPATIBILITY.

Unknown or incompatible pre-freshness semantics prohibit freshness evaluation
for authority-relevant use.

PROTOCOL_PARSE_SUCCESS != PRE_FRESHNESS_COMPATIBILITY

## 12. No silent normalization

Adapters, validators, migration tools, AI systems, supervisors, and runtime
components must not silently reinterpret incompatible semantic versions as
equivalent.

Same field name is not proof of same meaning.

Same protocol version is not proof of same semantic profile.

Same chip family is not proof of same platform grammar.

## 13. Evaluation ordering

Where applicable, peer input shall be evaluated in this order:

1. resource bounds;
2. structural/wire validation;
3. closed-contract validation;
4. trusted identity resolution;
5. authentication;
6. pre-freshness compatibility resolution;
7. epoch resolution;
8. replay/freshness evaluation;
9. full-message semantic compatibility evaluation;
10. evidence and deterministic policy evaluation;
11. authority decision;
12. physical actuation gate.

No stage grants the properties of a later stage.

Unauthenticated or authentication-failed input must not advance, replace, reset,
or otherwise mutate accepted freshness state.

UNAUTHENTICATED_EPOCH
MUST NOT
ADVANCE_OR_REPLACE_ACCEPTED_FRESHNESS_STATE

Signed logical time or timestamps do not independently establish trusted
wall-clock time or persistent freshness.

## 14. Rejoin policy

Rejoin is not automatic.

REJOIN_REQUIRED is a security/freshness condition, not a new operational state.

A future rejoin transaction must be separately specified and governed.

It must establish, as applicable:

- acceptable peer identity;
- acceptable key lifecycle state;
- accepted epoch;
- new freshness anchor;
- semantic compatibility;
- evidence and audit trail;
- bounded failure outcome.

Until rejoin succeeds, stronger authority consequences remain denied.

## 15. Authority policy

The following is prohibited:

AUTHENTICATED
+ FRESH
+ SEMANTICALLY_COMPATIBLE
= ACTUATION_AUTHORIZED

These properties may be necessary inputs to a later decision, but they are not
sufficient authority by themselves.

No peer gains actuator authority merely by being healthy, fresh, authenticated,
or compatible.

## 16. Evidence and auditability

Freshness, epoch transition, replay classification, compatibility resolution,
and rejoin decisions should be materializable as auditable evidence when the
implementation boundary is reached.

Evidence presence does not itself prove correctness.

Assessment remains distinct from adjudication, authority, and actuation.

## 17. Semantic history

Historical peer exchanges must retain the semantic context required to interpret
their original meaning.

A later semantic profile must not retroactively rewrite earlier observations.

Unauthorized semantic drift is a failure condition.

SEMANTIC DRIFT = FAIL

## 18. Failure semantics

The following states fail closed for authority-relevant use:

- UNKNOWN_EPOCH_STATE
- UNVERIFIED_EPOCH_REUSE
- SEQUENCE_WRAP_WITHIN_SAME_SESSION
- UNKNOWN_REPLAY_STATE
- FRESHNESS_UNKNOWN
- CORRUPTED_PERSISTED_STATE
- TORN_OR_INCOMPLETE_PERSISTENCE_UPDATE
- ROLLBACK_SUSPECTED
- UNKNOWN_PRE_FRESHNESS_COMPATIBILITY
- UNKNOWN_SEMANTIC_COMPATIBILITY
- UNSUPPORTED_SEMANTIC_COMPATIBILITY
- HISTORICALLY_IMPOSSIBLE_VERSION_COMBINATION
- REQUIRED_CONTEXT_MISSING

Diagnostics may remain observable while stronger consequences are denied.

## 19. Change management

Changes to this policy require:

- architectural adjudication;
- Human Readability review;
- Devil's Advocate review;
- Technical Destruction planning;
- backward semantic impact review;
- controlled validation;
- controlled commit.

No automated tool may silently weaken fail-closed semantics.

## 20. Nonclaims

This policy does not claim:

- persistent anti-replay is implemented;
- persistent-state rollback detection is implemented;
- epoch persistence is implemented;
- secure storage is selected;
- rejoin is implemented;
- physical dual-node behavior is demonstrated;
- NodeSupervisor is implemented or authorized;
- distributed quorum exists;
- production readiness;
- certification;
- AI actuation authority.

## 21. Governing invariants

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

AI/advisory != authority != actuation
