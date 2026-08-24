# Guardian F401 — TD-M15-005 Persistent Freshness State Model

Status: OPEN — ARCHITECTURE DECISION REQUIRED
Milestone: M15-03
Review Type: Technical Destruction / Architecture Gap
Implementation status: NOT IMPLEMENTED

---

## 1. Finding

M15 signed identity and authentication are implemented and validated.

However, the current architecture intentionally does not define persistent freshness or replay resistance across reset.

The existing authenticated assurance object contains:

- producer_id
- producer_epoch
- logical_time

These values are cryptographically authenticated but are not independently sufficient to establish freshness.

Therefore:

AUTHENTICATED ≠ FRESH

signed producer_epoch ≠ proof that the epoch is current

signed logical_time ≠ trusted wall-clock time

signed logical_time ≠ persistent freshness

---

## 2. Existing architectural ordering

The approved processing boundary remains:

VALIDATE
→ CANONICALIZE
→ AUTHENTICATE
→ ESTABLISH FRESHNESS
→ CORROBORATE
→ APPLY DETERMINISTIC POLICY
→ AUTHORIZE

Freshness must therefore consume an authenticated statement.

Freshness evaluation must not create physical authority.

---

## 3. Deferred M15-03 decisions

The following require explicit architecture decisions before implementation:

- producer_epoch semantics
- monotonic state
- reset semantics
- rollback resistance
- nonce lifecycle
- persistent storage

Additional implementation-critical questions identified by Technical Destruction:

- first-seen producer behavior
- same epoch with greater logical_time
- same epoch with equal logical_time
- same epoch with lower logical_time
- unseen new epoch behavior
- previously seen old epoch behavior
- persistent history depth
- replay-window semantics
- power-loss behavior
- atomic state update
- persistent-state corruption
- persistent-state unavailability
- state rollback detection
- recovery and reprovisioning semantics
- resource bounds

---

## 4. Minimum per-producer information under consideration

The following is a DESIGN CANDIDATE, not an approved schema:

producer_id
current_epoch
highest_logical_time
previous_epoch_evidence
state_version
integrity_metadata

This candidate must not be implemented until the architecture is adjudicated.

---

## 5. Same-epoch baseline

The following comparison rules are candidates for adjudication:

same epoch + logical_time greater than persistent high-water mark
→ candidate fresh statement

same epoch + logical_time equal to persistent high-water mark
→ duplicate/replay candidate

same epoch + logical_time below persistent high-water mark
→ rollback/replay candidate

These rules are not sufficient to resolve epoch transitions or reset.

---

## 6. Epoch transition problem

Accepting every previously unseen producer_epoch is unsafe because a producer could reset logical_time by presenting arbitrary new epochs.

Rejecting every new producer_epoch is also unsafe operationally because legitimate reset, lifecycle transition, recovery, or reprovisioning would become impossible.

Therefore:

new epoch acceptance requires a separately frozen transition rule.

The implementation must not infer that rule.

---

## 7. Previously seen epoch

A previously superseded epoch must not silently become current again.

The exact result classification and required retained epoch history remain to be adjudicated.

No first-match, latest-arrival, or message-controlled epoch preference is permitted.

---

## 8. Persistence and atomicity

Freshness state must survive the reset conditions for which persistent anti-replay is claimed.

The architecture must define:

- persistence backend contract
- update atomicity
- crash/power-loss ordering
- integrity validation
- corruption handling
- rollback handling
- unavailable-state handling

A message must not be declared FRESH if the required persistent state cannot be established reliably.

---

## 9. Nonce lifecycle

Nonce lifecycle is explicitly deferred to M15-03.

No nonce semantics are frozen by this finding.

The architecture must decide whether nonces are required in addition to producer_epoch and logical_time, what creates them, how long they remain valid, and what persistent state is required to reject reuse.

---

## 10. Failure semantics

M15-03 must define explicit machine-readable freshness outcomes.

Candidate classes requiring adjudication include:

- FRESH
- REPLAY
- LOGICAL_TIME_ROLLBACK
- EPOCH_ROLLBACK
- EPOCH_TRANSITION_REQUIRED
- FRESHNESS_STATE_INVALID
- FRESHNESS_STATE_UNAVAILABLE
- FRESHNESS_STATE_PERSIST_FAILURE

These identifiers are candidates only and are not approved by this document.

Unknown or unavailable freshness state must fail closed.

---

## 11. Security invariants

The following invariants are already authoritative:

AUTHENTICATED ≠ FRESH

FRESH ≠ TRUTHFUL

FRESH ≠ AUTHORIZED

FRESH ≠ ACTUATION PERMISSION

An authentication success cannot bypass freshness.

A freshness success cannot bypass deterministic policy or explicit authority.

---

## 12. Threats requiring tests

M15-03 validation must eventually test at least:

- exact duplicate replay
- older logical_time in current epoch
- repeated logical_time in current epoch
- arbitrary new-epoch injection
- resurrection of superseded epoch
- restart with persisted state
- restart without required state
- corrupted state
- truncated state
- state rollback
- interrupted persistent update
- duplicate nonce where applicable
- exhaustion of retained replay state
- authenticated malicious producer manipulating freshness fields

---

## 13. Scope exclusions

This finding does not define:

- trusted wall-clock time
- distributed quorum
- witness consensus
- attestation
- partition handling
- failover
- physical truth
- actuator authority
- production hardware qualification

Those properties remain separate architecture boundaries.

---

## 14. Disposition

Finding: OPEN

M15-03 architecture: NOT YET FROZEN

Persistent freshness implementation: NOT AUTHORIZED

Next action:
Adjudicate epoch transition, monotonic-state, reset, rollback, nonce, and persistence semantics before implementation.

---

## Decision A — Same-Epoch Monotonic Freshness

Status: APPROVED

Decision A defines freshness comparison only when producer identity and epoch are unchanged.

Precondition:

The assurance statement must already be AUTHENTICATED.

Authentication success alone does not establish freshness.

---

### A.1 Existing field contracts

producer_epoch is the existing M14 lowercase hexadecimal 32-character field.

logical_time is the existing M14 safe integer field.

Allowed logical_time range:

0 through 9,007,199,254,740,991 inclusive.

Boolean values are not logical_time values.

---

### A.2 Comparison key

Same-epoch monotonic evaluation is scoped by:

producer_id
+
producer_epoch

No producer may consume or advance another producer's freshness state.

---

### A.3 High-water mark

For one authenticated producer in one current epoch, the verifier maintains a persistent logical_time high-water mark.

Comparison semantics:

logical_time > high_water
→ FRESH_CANDIDATE

logical_time == high_water
→ REPLAY

logical_time < high_water
→ LOGICAL_TIME_ROLLBACK

No replay window or out-of-order acceptance is approved by Decision A.

---

### A.4 Equal logical time

A second authenticated statement carrying the same logical_time in the same producer epoch is not fresh.

Object identity, payload difference, or a valid new signature does not override the equal-time replay result.

---

### A.5 Lower logical time

An authenticated statement carrying logical_time below the persisted high-water mark in the same producer epoch fails freshness evaluation.

The result is LOGICAL_TIME_ROLLBACK.

Cryptographic authenticity does not override rollback detection.

---

### A.6 Different producer epoch

Decision A does not authorize an epoch transition.

If the authenticated producer_epoch differs from the currently established epoch:

→ EPOCH_TRANSITION_REQUIRED

The statement must not be accepted as FRESH solely because the epoch value is previously unseen.

Epoch transition, reset, recovery, and old-epoch resurrection are governed by Decision B.

---

### A.7 First-seen producer

Absence of persistent freshness state must not silently establish freshness.

A first-seen authenticated producer is classified:

→ FIRST_SEEN

The rule for establishing the initial persistent epoch/high-water state is deferred to Decisions B and C.

---

### A.8 Persistence boundary

FRESH_CANDIDATE is not yet final FRESH.

Final FRESH requires successful persistence under the atomicity and failure semantics frozen by Decision C.

Therefore:

FRESH_CANDIDATE ≠ FRESH

A crash or persistence failure must not permit a replayed message to become fresh after restart.

---

### A.9 Explicitly prohibited behavior

Decision A prohibits:

- accepting equal logical_time as fresh;
- accepting lower logical_time as fresh;
- using signature validity to bypass monotonicity;
- allowing a new object_id to bypass replay detection;
- accepting a new producer_epoch automatically;
- using arrival order instead of persisted monotonic state;
- silently resetting the high-water mark;
- treating missing persistent state as proof of freshness.

---

### A.10 Decision A disposition

Same-epoch monotonic comparison: FROZEN

logical_time > high_water → FRESH_CANDIDATE
logical_time == high_water → REPLAY
logical_time < high_water → LOGICAL_TIME_ROLLBACK
different epoch → EPOCH_TRANSITION_REQUIRED
no persistent producer state → FIRST_SEEN

Decision A does not close TD-M15-005.

Decision B remains required for epoch transition, reset, rollback, recovery, and old-epoch semantics.

Decision C remains required for persistence, atomic commit, corruption, rollback resistance, and unavailable-state behavior.

---

## Decision B — Epoch Transition, Reset, and Rollback

Status: APPROVED

Decision B defines how Guardian interprets producer_epoch changes after authentication.

It does not define persistent storage mechanics or atomic commit behavior.
Those remain Decision C.

---

### B.1 Fundamental rule

An authenticated producer must not be permitted to select an arbitrary new epoch and thereby reset freshness state.

Therefore:

new unseen producer_epoch ≠ automatically valid epoch transition

Authentication proves who authenticated the epoch claim.
Authentication does not prove that the claimed epoch is current.

---

### B.2 Current epoch

For each established producer, freshness state has exactly one current_epoch.

An authenticated statement carrying current_epoch is evaluated by Decision A.

A restart or reboot does not by itself change current_epoch.

---

### B.3 Reset semantics

Software reset, watchdog reset, power cycle, brownout, or process restart must not silently reset the logical-time high-water mark or establish a new producer epoch.

After restart, persisted freshness state remains authoritative.

Therefore:

device reset ≠ epoch transition

device reset ≠ freshness reset

---

### B.4 Epoch transition authorization

A change from current_epoch to a different producer_epoch requires explicit transition authorization external to the signed assurance message.

Conceptually, an authorized transition binds:

- producer_id
- from_epoch
- to_epoch
- transition_sequence
- authorization_reference

The exact representation and persistence mechanism are deferred to Decision C.

The assurance message must not supply authoritative transition authorization.

---

### B.5 Exact transition matching

An epoch transition authorization applies only to the exact producer_id, from_epoch, and to_epoch for which it was established.

No wildcard producer is permitted.
No wildcard source epoch is permitted.
No wildcard destination epoch is permitted.
No fallback transition is permitted.

---

### B.6 Transition sequence

Authorized epoch transitions are monotonically ordered by transition_sequence.

A transition sequence must never move backward.

Replaying an already consumed transition authorization must not reopen an old epoch.

The storage and atomicity rules required to enforce this property belong to Decision C.

---

### B.7 Previously superseded epoch

Once an epoch has been superseded by an authorized transition, that epoch must not silently become current again.

An authenticated statement from a superseded epoch is classified:

→ EPOCH_ROLLBACK

A valid signature does not override this result.

---

### B.8 Unknown different epoch

If an authenticated statement carries an epoch different from current_epoch and there is no exact authorized transition to that epoch:

→ EPOCH_TRANSITION_REQUIRED

The statement is not FRESH.

The verifier must not infer legitimacy from:

- the epoch being previously unseen;
- device reset;
- arrival order;
- a valid signature;
- logical_time beginning at zero;
- factory UID;
- RTC state;
- object_id;
- key rotation alone.

---

### B.9 Factory UID boundary

STM32 factory UID may be provisioning evidence or inventory metadata.

It is not epoch-transition authority.

Therefore:

Factory UID ≠ epoch authorization

Factory UID ≠ freshness proof

Factory UID ≠ rollback resistance

---

### B.10 RTC boundary

Clock configuration or RTC capability does not establish a trusted freshness source.

Decision B does not use RTC values to authorize epoch transitions.

Therefore:

RTC availability ≠ trusted epoch evidence

---

### B.11 First-seen producer

FIRST_SEEN applies only when the verifier has no evidence that freshness state has ever been established for that producer.

FIRST_SEEN is not automatically FRESH.

Initial establishment requires an explicit provisioning/bootstrap rule implemented under Decision C.

---

### B.12 Lost or corrupted established state

Loss, corruption, truncation, rollback, or unavailability of previously established freshness state must not be interpreted as FIRST_SEEN.

Therefore:

previously established state unavailable
≠
new producer

Such conditions fail closed under Decision C state-failure semantics.

---

### B.13 Key rotation boundary

key_id lifecycle and producer_epoch lifecycle are separate dimensions.

A key replacement does not automatically establish a new epoch.

An epoch transition does not automatically authorize a new key.

Therefore:

key rotation ≠ epoch transition

epoch transition ≠ key authorization

---

### B.14 Transition result model

Decision B freezes the following conceptual outcomes:

CURRENT_EPOCH
EPOCH_TRANSITION_REQUIRED
EPOCH_TRANSITION_AUTHORIZED
EPOCH_ROLLBACK
FIRST_SEEN

EPOCH_TRANSITION_AUTHORIZED is not final FRESH.

Final acceptance still requires Decision C durable persistence followed by Decision A monotonic evaluation in the newly established epoch.

---

### B.15 Security invariants

The following are prohibited:

- automatic acceptance of arbitrary new epochs;
- reset-driven high-water reset;
- old-epoch resurrection;
- treating missing established state as first-seen;
- using UID as epoch authority;
- using RTC configuration as epoch authority;
- using key rotation as implicit epoch transition;
- allowing assurance messages to authorize their own epoch transition.

---

### B.16 Decision B disposition

Epoch transition semantics: FROZEN

reset alone → no epoch change

current epoch → Decision A

different epoch + exact transition authorization → transition candidate

different epoch + no authorization → EPOCH_TRANSITION_REQUIRED

superseded epoch → EPOCH_ROLLBACK

never-established producer → FIRST_SEEN

lost/corrupt previously established state → NOT FIRST_SEEN and fail closed

Decision B does not close TD-M15-005.

Decision C remains required to freeze the persistent state schema, transition-authorization representation, atomic update protocol, integrity protection, corruption behavior, rollback detection, bootstrap, and persistence failure semantics.

---

## Decision C — Persistent State, Atomic Commit, and Recovery

Status: APPROVED

Decision C defines the durable-state contract required before Guardian may convert FRESH_CANDIDATE into final FRESH.

It defines persistence semantics, not a hardware-specific storage backend.

---

### C.1 Fundamental persistence rule

A freshness decision must not become final before the corresponding state transition is durably committed.

Required ordering:

AUTHENTICATED
→ freshness evaluation
→ FRESH_CANDIDATE
→ durable state commit
→ commit verification
→ FRESH

Therefore:

FRESH_CANDIDATE ≠ FRESH

persist failure → NOT FRESH

---

### C.2 Per-producer persistent state

The persistent freshness model maintains one logical state record per established producer.

The logical record contains at minimum:

- schema_version
- producer_id
- current_epoch
- highest_logical_time
- transition_sequence
- superseded_epoch evidence
- generation
- integrity metadata

Private-key material is not part of freshness state.

---

### C.3 State identity

Persistent state is keyed by exact producer_id.

No wildcard producer state is permitted.
No default producer state is permitted.
No fallback producer state is permitted.

One producer must never advance another producer's monotonic state.

---

### C.4 Atomic replacement model

The persistence contract must support recovery from interrupted writes without treating a partially written replacement as authoritative.

Conceptual model:

committed generation N
+
candidate generation N+1

Generation N remains authoritative until generation N+1 is completely written, integrity-validated, and committed.

An implementation may use two slots, journaling, transactional storage, or an equivalent mechanism only if it demonstrates the same semantics.

The architecture does not require one specific physical backend.

---

### C.5 Generation monotonicity

Each successful persistent state replacement advances generation monotonically.

Generation rollback must not be silently accepted.

A lower generation than the established committed generation is a persistent-state rollback condition.

Generation wraparound is not silently permitted.

---

### C.6 Integrity validation

Persistent freshness state must have integrity metadata sufficient to detect accidental corruption, truncation, incomplete writes, and inconsistent record contents.

Integrity metadata is not equivalent to authenticity against an attacker with arbitrary storage-write capability.

The exact integrity primitive and hardware protection mechanism may vary by backend but must be explicit and validated.

Invalid integrity metadata fails closed.

---

### C.7 Load behavior

On load, the persistence layer must classify state before freshness evaluation.

Possible conceptual load outcomes:

- STATE_VALID
- STATE_NOT_ESTABLISHED
- STATE_INVALID
- STATE_UNAVAILABLE
- STATE_ROLLBACK_DETECTED

STATE_NOT_ESTABLISHED is valid only when there is no evidence that state was previously provisioned for that producer.

STATE_INVALID, STATE_UNAVAILABLE, and STATE_ROLLBACK_DETECTED must not be reinterpreted as FIRST_SEEN.

---

### C.8 First establishment

A never-established producer requires an explicit bootstrap/provisioning operation before normal freshness acceptance.

Bootstrap establishes:

- producer_id
- initial current_epoch
- initial transition_sequence
- persistent state generation
- initial high-water semantics

FIRST_SEEN alone does not produce FRESH.

The signed assurance message must not bootstrap its own trust state.

---

### C.9 Initial high-water mark

Bootstrap must define the initial logical-time state explicitly.

The implementation must not use an implicit numeric sentinel that can collide with a valid logical_time.

An explicit state such as HIGH_WATER_UNSET is required until the first accepted logical_time is durably committed.

Therefore logical_time zero remains a valid authenticated value and is not reserved as an implementation sentinel.

---

### C.10 Same-epoch commit

For Decision A result FRESH_CANDIDATE:

candidate highest_logical_time = authenticated logical_time

The candidate state must be persisted and verified before returning FRESH.

If commit or verification fails:

→ FRESHNESS_STATE_PERSIST_FAILURE

The previous committed state remains authoritative.

---

### C.11 Epoch-transition commit

For an exact authorized epoch transition:

the replacement state must atomically bind:

- producer_id
- previous current_epoch
- new current_epoch
- transition_sequence
- superseded old epoch evidence
- initial high-water state for the new epoch
- new generation

The old epoch must become superseded only as part of the same durable transition.

The verifier must not expose a state in which the new epoch is current but the old epoch has not yet been recorded as superseded.

---

### C.12 Power-loss behavior

Power loss or process termination at any point before durable commit completion must recover to the previous valid committed generation.

Power loss after commit completion must recover to the new committed generation.

No intermediate state may be treated as FRESH.

---

### C.13 Corruption behavior

Corrupted persistent state fails closed.

Required conceptual result:

→ FRESHNESS_STATE_INVALID

Corruption must not:

- reset high-water state;
- clear superseded epochs;
- reset transition_sequence;
- create FIRST_SEEN;
- authorize a new epoch.

---

### C.14 Unavailable-state behavior

If required persistent state cannot be read or the persistence backend is unavailable:

→ FRESHNESS_STATE_UNAVAILABLE

The message is not FRESH.

Authentication success does not override state unavailability.

---

### C.15 Rollback behavior

If a previously valid but older persistent generation is presented after a newer generation has been established and rollback can be detected:

→ FRESHNESS_STATE_ROLLBACK

The message is not FRESH.

Rollback detection capability must be explicit for each persistence backend.

Software integrity checks alone must not be described as hardware-backed rollback resistance.

---

### C.16 Recovery and reprovisioning

Recovery from invalid, unavailable, or rollback-detected state is an explicit administrative or provisioning operation.

Normal assurance messages cannot perform recovery.

Recovery must not silently erase anti-replay history.

Any intentional freshness-state reset requires separately auditable change-management evidence.

---

### C.17 Backend separation

The persistence backend is separate from freshness policy.

Host/test implementations may use a filesystem-backed transactional representation.

STM32 implementations may use flash or another explicitly approved persistent mechanism.

Both must satisfy the same logical atomicity, integrity, recovery, and fail-closed contract.

No host-only behavior may silently become the production firmware contract.

---

### C.18 M12 reuse boundary

Existing M12 persistence mechanisms may provide an engineering pattern or reusable backend abstraction.

They do not automatically become M15 freshness state.

Reuse requires explicit interface compatibility and validation.

Therefore:

M12 persistent lifecycle state ≠ M15 freshness state

---

### C.19 Result classes frozen by Decision C

Decision C freezes these conceptual persistence outcomes:

- STATE_VALID
- STATE_NOT_ESTABLISHED
- FRESHNESS_STATE_INVALID
- FRESHNESS_STATE_UNAVAILABLE
- FRESHNESS_STATE_ROLLBACK
- FRESHNESS_STATE_PERSIST_FAILURE

Machine-readable state is authoritative over diagnostic text.

---

### C.20 Security boundaries

Durable freshness state does not establish:

- trusted wall-clock time
- physical truth
- policy authorization
- actuator authority
- witness consensus
- attestation
- hardware-backed rollback resistance unless separately demonstrated

Therefore:

FRESH ≠ AUTHORIZED

FRESH ≠ PHYSICALLY TRUE

---

### C.21 Required failure tests

Implementation validation must eventually demonstrate at least:

- interrupted write before candidate completion
- interrupted write after candidate write but before commit
- restart after successful commit
- truncated state
- corrupted integrity metadata
- conflicting valid-looking generations
- older generation replay
- persistence backend unavailable
- persistence write failure
- persistence verification failure
- first establishment
- logical_time zero as a valid first value
- epoch transition interrupted before commit
- epoch transition completed and old epoch rejected
- state loss not interpreted as FIRST_SEEN

---

### C.22 Decision C disposition

Persistent-state semantics: FROZEN

FRESH_CANDIDATE → durable commit → verified commit → FRESH

commit failure → FRESHNESS_STATE_PERSIST_FAILURE

invalid state → FRESHNESS_STATE_INVALID

unavailable state → FRESHNESS_STATE_UNAVAILABLE

detectable persistent rollback → FRESHNESS_STATE_ROLLBACK

missing previously established state → fail closed and NOT FIRST_SEEN

Decision C does not yet close TD-M15-005.

Before implementation authorization, M15-03 still requires explicit resource bounds and adjudication of whether nonce state is necessary in addition to producer_epoch plus persistent logical_time.

---

## Decision D2/D3 — Replay Window and Nonce Lifecycle

Status: APPROVED

This decision adjudicates the M15-03 replay-window and nonce-lifecycle requirements.

It does not freeze freshness resource bounds.

---

### D2.1 Replay-window decision

M15-03 v1 uses strict monotonic acceptance.

Replay acceptance window:

0

No authenticated statement with logical_time less than or equal to the established high-water mark is accepted as fresh.

Therefore:

logical_time > high_water
→ FRESH_CANDIDATE

logical_time == high_water
→ REPLAY

logical_time < high_water
→ LOGICAL_TIME_ROLLBACK

No out-of-order acceptance window is implemented in M15-03 v1.

---

### D2.2 Rationale

Decision A already freezes strict high-water semantics.

No current Guardian requirement demonstrates a need to accept authenticated assurance statements out of order.

Introducing a positive replay window would expand state complexity and would require tracking already-consumed sequence positions below the high-water mark.

That complexity is not justified by the current bounded M15-03 requirements.

If a future transport requires out-of-order acceptance, replay-window semantics require a new architecture review.

---

### D2.3 Delivery-order boundary

Transport delivery order does not override freshness policy.

A cryptographically valid late statement may still be classified as replay or rollback.

Therefore:

AUTHENTICATED ≠ FRESH

late authenticated message ≠ fresh message

---

### D3.1 Nonce decision

Nonce state is NOT REQUIRED for M15-03 v1 freshness evaluation.

The M15-03 v1 anti-replay property is established by the combination of:

- exact producer identity;
- explicitly established producer epoch;
- strictly increasing logical_time;
- persistent monotonic high-water state;
- explicit epoch-transition authorization;
- durable atomic state commit;
- fail-closed state recovery.

---

### D3.2 Replay analysis

An exact replay carries a logical_time that is not greater than the durable high-water mark and therefore fails freshness.

An older authenticated statement fails freshness.

An arbitrary new epoch does not reset freshness because epoch transition requires explicit authorization.

A reboot does not reset freshness state.

Therefore a nonce is not required to provide the replay property defined by M15-03 v1.

---

### D3.3 Challenge-response boundary

M15-03 v1 does not define an interactive challenge-response protocol.

Nonce absence must not be interpreted as proof of live interactive possession at an externally chosen instant.

If Guardian later requires verifier-issued challenges, liveness proofs, or challenge-response attestation, nonce semantics must be defined by that protocol.

---

### D3.4 Nonce prohibition by implication

Implementations must not add undocumented nonce semantics to M15-03 v1.

Transport identifiers, object_id, signatures, timestamps, or random fields must not be silently treated as freshness nonces.

---

### D2/D3 disposition

Replay-window semantics: FROZEN

M15-03 v1 replay acceptance window = 0

Out-of-order freshness acceptance = NOT SUPPORTED

Nonce lifecycle: ADJUDICATED

M15-03 v1 nonce requirement = NOT REQUIRED

Future positive replay windows or challenge-response nonce protocols require explicit architecture review.

TD-M15-005 remains OPEN pending Decision D1 freshness resource bounds.

---

## Decision D1 — Freshness Resource Bounds

Status: APPROVED

Decision D1 freezes bounded resource semantics for the portable M15-03 freshness model.

It does not allocate a physical STM32 flash partition.

---

### D1.1 Producer-state upper bound

The portable M15-03 software model supports at most:

4,096 producer freshness states

Rationale:

An authenticated producer must resolve through the bounded M15 trust store.
The trust store permits at most 4,096 TrustRecord entries.
Therefore the number of simultaneously authenticated producer identities cannot exceed the existing trust-record upper bound.

This is a software-model upper bound, not a claim that the STM32F401 production backend can persist 4,096 producer records.

---

### D1.2 Backend capacity

Each persistence backend declares a fixed capacity less than or equal to the portable 4,096-producer upper bound.

Backend capacity must be known before accepting producer-state establishment.

Capacity exhaustion fails closed:

→ FRESHNESS_CAPACITY_EXCEEDED

Existing producer state must not be evicted automatically to make room for another producer.

No least-recently-used eviction is permitted.
No oldest-producer eviction is permitted.
No message-controlled eviction is permitted.

---

### D1.3 Bounded epoch retention

M15-03 v1 does not retain an unbounded list of superseded epochs.

Each producer freshness state retains:

- current_epoch
- previous_epoch when established
- transition_sequence

previous_epoch is retained to classify immediate epoch reversal explicitly.

---

### D1.4 Older epoch behavior

A producer_epoch that differs from current_epoch and is not the retained previous_epoch does not receive automatic trust merely because its full historical membership is no longer retained.

Without an exact authorized transition from current_epoch:

→ EPOCH_TRANSITION_REQUIRED

The statement is not FRESH.

Therefore bounded epoch history does not permit old epochs to become current automatically.

---

### D1.5 Immediate rollback classification

If authenticated producer_epoch equals retained previous_epoch:

→ EPOCH_ROLLBACK

The statement is not FRESH.

If an older superseded epoch is outside retained history:

→ EPOCH_TRANSITION_REQUIRED

The distinction affects evidence classification only.
Both outcomes fail freshness closed.

---

### D1.6 Transition-sequence bound

transition_sequence is an unsigned 64-bit monotonic integer.

Allowed range:

0 through 18,446,744,073,709,551,615 inclusive.

Transition-sequence wraparound is prohibited.

If the maximum value is reached:

→ FRESHNESS_SEQUENCE_EXHAUSTED

No implicit wrap, reset, or epoch transition is permitted.

---

### D1.7 Generation bound

Persistent generation is an unsigned 64-bit monotonic integer.

Generation wraparound is prohibited.

If the maximum generation is reached:

→ FRESHNESS_GENERATION_EXHAUSTED

Normal freshness mutation stops fail closed until explicit recovery or reprovisioning.

---

### D1.8 Logical state remains fixed-size

The portable per-producer freshness state is conceptually bounded to:

- producer_id
- current_epoch
- previous_epoch or explicit unset state
- highest_logical_time or HIGH_WATER_UNSET
- transition_sequence
- generation
- integrity metadata

No unbounded replay list is required.
No unbounded epoch history is required.
No nonce history is required for M15-03 v1.

---

### D1.9 Physical-storage boundary

The current architecture does not define a fixed STM32 flash partition for M15-03.

Therefore:

portable maximum = 4,096 producers

does not imply

STM32 persistent capacity = 4,096 producers

Before a production STM32 backend is claimed, its flash layout, erase granularity, endurance, atomic replacement strategy, record size, and demonstrated capacity must be explicitly documented and validated.

---

### D1.10 Resource-exhaustion invariants

Resource exhaustion must never:

- evict established producer state automatically;
- reset high-water state;
- forget current_epoch;
- establish FIRST_SEEN;
- permit an epoch transition;
- convert a replay into FRESH.

---

### D1.11 Decision D1 disposition

Portable producer freshness-state maximum: 4,096

Per-backend capacity: fixed, explicit, and <= 4,096

Retained epochs per producer:
current_epoch + previous_epoch

Replay window: 0

Nonce history: none for M15-03 v1

transition_sequence: unsigned 64-bit monotonic

generation: unsigned 64-bit monotonic

Capacity exhaustion → FRESHNESS_CAPACITY_EXCEEDED

Transition-sequence exhaustion → FRESHNESS_SEQUENCE_EXHAUSTED

Generation exhaustion → FRESHNESS_GENERATION_EXHAUSTED

Decision D1 completes the remaining resource-bound adjudication for TD-M15-005.

---

## TD-M15-005 Closure — Persistent Freshness and Anti-Replay

Status: RESOLVED

Finding: CLOSED

Architecture disposition: APPROVED FOR SOFTWARE IMPLEMENTATION

The persistent freshness and anti-replay architecture finding opened by TD-M15-005 has completed architecture adjudication and final Technical Destruction.

The closure is based on Decisions A, B, C, D1, D2, and D3 contained in this document.

### Closure basis

Decision A freezes same-epoch monotonic freshness.

Decision B freezes epoch transition, reset, rollback, and FIRST_SEEN boundaries.

Decision C freezes persistent state, atomic commit, verified commit, recovery, corruption, and persistence-failure semantics.

Decision D1 freezes bounded freshness resource semantics.

Decision D2 freezes the M15-03 v1 replay acceptance window at zero.

Decision D3 establishes that nonce history is not required for the defined M15-03 v1 anti-replay property.

### Authoritative freshness progression

AUTHENTICATED
→ persistent-state evaluation
→ same-epoch or explicitly authorized epoch-transition evaluation
→ FRESH_CANDIDATE
→ durable commit
→ verified commit
→ FRESH

FRESH_CANDIDATE ≠ FRESH

AUTHENTICATED ≠ FRESH

### Fail-closed invariants

The following do not establish FRESH:

- replay;
- logical-time rollback;
- unauthorized epoch transition;
- epoch rollback;
- missing previously established state;
- invalid persistent state;
- unavailable persistent state;
- detectable persistent-state rollback;
- persistence commit failure;
- capacity exhaustion;
- transition-sequence exhaustion;
- generation exhaustion.

### Resource disposition

Portable M15-03 producer freshness-state upper bound:

4,096 producer freshness states

Physical backend capacity remains backend-specific, fixed, explicit, and less than or equal to the portable upper bound.

This closure does not allocate an STM32F401 flash partition and does not claim that a production STM32F401 backend can persist 4,096 producer states.

### Replay and nonce disposition

M15-03 v1 replay acceptance window = 0

Out-of-order freshness acceptance = NOT SUPPORTED

M15-03 v1 nonce requirement = NOT REQUIRED

These semantics apply to M15-03 persistent freshness and do not redefine challenge-response or session semantics in other Guardian protocols.

### Security boundary

FRESH establishes only that an authenticated statement satisfies the approved M15-03 freshness and anti-replay contract.

FRESH does not establish:

- authorization;
- physical truth;
- actuator permission;
- quorum;
- witness corroboration;
- attestation;
- production hardware root of trust.

Therefore:

AUTHENTICATED ≠ FRESH

FRESH ≠ AUTHORIZED

FRESH ≠ PHYSICALLY TRUE

FRESH ≠ ACTUATION AUTHORITY

### Implementation authorization

Software implementation of the bounded M15-03 persistent freshness and anti-replay contract is now authorized.

Implementation must conform to the decisions frozen in TD-M15-005 and must not silently introduce:

- replay windows;
- nonce requirements;
- automatic epoch acceptance;
- automatic producer eviction;
- state-reset-as-FIRST_SEEN behavior;
- implicit sequence wraparound;
- implicit generation wraparound;
- physical backend capacity claims.

Production STM32 persistence remains subject to separate backend evidence including flash layout, erase granularity, endurance, atomic replacement, power-loss behavior, integrity behavior, and demonstrated capacity.

### Remaining M15 boundary

TD-M15-005 closure resolves M15-03 architecture adjudication.

It does not resolve M15-04 — Attestation and Witness Exchange.

M15-04 remains separately deferred.

### Traceability

TD-M15-005
→ Decisions A/B/C/D1/D2/D3
→ final Technical Destruction
→ formal closure
→ ADR reconciliation
→ Architecture Gate reconciliation
→ bounded software implementation
→ validation evidence

Status: RESOLVED — M15-03 architecture frozen and bounded software implementation authorized.

---

## Post-Implementation Evidence Reconciliation — M15-03 Bounded Host Software

Status: IMPLEMENTED AND VALIDATED — BOUNDED HOST SOFTWARE PATH

Evidence baseline: `d37714d`

The bounded M15-03 host software path authorized by this Technical Destruction has now been implemented and validated.

Implementation sequence:

- `c5e1bab` — expose freshness field validators;
- `803967f` — add bounded freshness state model;
- `818fa38` — add pure freshness evaluator;
- `6c25097` — add freshness persistence contract;
- `8cff65c` — add host freshness persistence backend;
- `7c59741` — add freshness orchestration;
- `4680a4a` — expose authenticated freshness claims;
- `d37714d` — integrate authentication and freshness.

Validation evidence:

- complete assurance regression: 307 tests passed;
- unauthenticated statements cannot enter freshness evaluation;
- authenticated producer_id, producer_epoch, and logical_time are handed directly to freshness without wrapper reparsing;
- same-epoch strict monotonic freshness is implemented;
- replay acceptance window remains 0;
- FRESH_CANDIDATE becomes FRESH only after persistence commit and verification;
- interrupted host writes preserve the previous committed state;
- stale candidate files are non-authoritative;
- corruption and truncation fail closed;
- AUTHENTICATED != FRESH;
- FRESH_CANDIDATE != FRESH;
- COMMITTED != VERIFIED;
- FRESH != AUTHORIZED.

Implementation boundaries preserved:

- FIRST_SEEN does not bootstrap persistent freshness state;
- explicit bootstrap/provisioning authorization remains outside normal freshness orchestration;
- EPOCH_TRANSITION_REQUIRED does not authorize an epoch transition;
- explicit epoch-transition authorization remains outside normal freshness orchestration;
- SHA-256 integrity evidence is not authenticated storage;
- arbitrary-storage rollback resistance is not demonstrated;
- no hardware-backed monotonic anchor is claimed;
- no production STM32F401 persistence backend is claimed;
- no STM32F401 flash allocation, endurance, or physical power-loss qualification is claimed;
- freshness does not establish authorization or actuation permission;
- M15-04 remains separately deferred.

Disposition:

M15-03 architecture: CLOSED

M15-03 bounded host software implementation: IMPLEMENTED AND VALIDATED

Production STM32F401 persistence evidence: PENDING

Explicit bootstrap authorization implementation: PENDING

Explicit epoch-transition authorization implementation: PENDING

Hardware-backed rollback resistance: NOT DEMONSTRATED

M15-04: PENDING

Historical NOT IMPLEMENTED, NOT AUTHORIZED, and implementation-PENDING statements earlier in this document remain preserved as temporal architecture evidence. They do not represent the current status of the bounded host software path after commit d37714d.
