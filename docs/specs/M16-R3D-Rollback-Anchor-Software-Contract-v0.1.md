# M16 R3D Rollback-Anchor Software Contract v0.1

Status: HUMAN_REQUIRED_AMENDMENTS_APPLIED_CANDIDATE
Milestone: M16
Workstream: C5-R3D-A
Contract family: ABSTRACT_ROLLBACK_ANCHOR_SOFTWARE_CONTRACT_AND_RECOVERY_REQUIREMENTS
Implementation authorization: NOT GRANTED
Physical rollback-anchor backend selected: NO
Hardware rollback resistance demonstrated: NO
Persistent anti-replay demonstrated: NO
NodeSupervisor implementation authorization: NOT GRANTED
Physical actuation authority: NOT GRANTED
AI authority: NOT GRANTED
Release readiness: NO

## 1. Purpose

This specification converts the governed R3D architecture into an implementable
software-contract and recovery-requirements boundary.

It defines the software semantics that a future rollback-anchor implementation
must satisfy before implementation may be separately authorized.

This specification does not implement a rollback anchor.

This specification does not select a physical rollback-anchor backend.

This specification does not claim that STM32F401 internal flash, ordinary
SLOT_A/SLOT_B persistence, a CRC, SHA-256, a commit marker, or a generation
number provides strong rollback resistance.

The governing invariant remains:

PERSISTENCE != PERSISTENT_ANTI_REPLAY

The ordinary R3C persistence slots remain persistence evidence.

They SHALL NOT serve as their own independent rollback anchor.

## 2. Governed source-of-truth baseline

This specification is bounded by the repository state integrated at:

REMOTE_MAIN =
ed123d25f418fa728a5eb1d0f6b67168985f9fe3

The following governed artifacts constrain this contract:

- docs/architecture/m16-r3d-persistent-anti-replay-rollback-anchor-and-recovery.md
- docs/architecture/m16-r3d-r3e-scope-specification.md
- docs/adr/ADR-M16-005-peer-freshness-epoch-replay-and-semantic-compatibility.md
- docs/specs/M16-NodeLink-Normative-Specification-v0.2.md
- docs/validation/c5-r3c-e-physical-persistence-backend-evidence-and-adjudication.md

Existing R3C persistence implementation is inherited, not reopened.

The current NodeLink persistence baseline includes:

- freshness identity;
- accepted epoch;
- accepted sequence;
- record generation;
- canonical persistence encoding;
- integrity validation;
- dual-slot persistence;
- torn/incomplete update handling;
- persistence backend abstraction;
- STM32F401 persistence adaptation;
- recovery classification.

R3D extends that model with an independent continuity proof.

## 3. Security separations

The implementation derived from this specification SHALL preserve:

PERSISTENCE != PERSISTENT_ANTI_REPLAY

VALID_SLOT != CURRENT_SLOT_PROOF

HIGHER_VISIBLE_GENERATION != STRONG_ROLLBACK_RESISTANCE

DUAL_SLOT_ATOMICITY != ANTI_ROLLBACK

FLASH_PERSISTENCE != PERSISTENT_ANTI_REPLAY

AUTHENTICATED != FRESH

FRESH != AUTHORIZED

AUTHORIZED != ACTUATION_AUTHORIZED

VALIDATION != AUTHORITY

AUTHORITY != ACTUATION

AI_ADVISORY != AUTHORITY

AI_ADVISORY != ACTUATION

No API success value defined here grants NodeSupervisor authority, physical
actuation authority, control-transfer authority, or AI authority.

## 4. Exact rollback-anchor identity contract

A rollback-anchor record SHALL be bound to the exact governed freshness
identity inherited from R3C.

The contract SHALL NOT silently reduce identity to only node ID, only producer
ID, only key ID, or any other weaker subset.

Conceptually, the rollback-anchor identity includes the security-relevant R3C
identity dimensions, including:

- sender_node_id;
- producer_id;
- key_id;
- signature_algorithm;
- producer semantic profile;
- consumer semantic profile;
- governed compatibility contract.

A future implementation MAY encode these values directly or MAY encode a
canonical cryptographic commitment to them.

Equivalent encoding is acceptable only when exact identity binding remains
mechanically demonstrable.

Conceptual software type:

ROLLBACK_ANCHOR_IDENTITY

Properties:

- CANONICAL;
- DETERMINISTIC;
- NON_AMBIGUOUS;
- EXACTLY_BOUND_TO_GOVERNED_FRESHNESS_IDENTITY;
- NOT_AUTHORITY_BEARING.

Identity mismatch SHALL fail closed.

## 5. State commitment contract

The rollback anchor SHALL commit to the security-relevant accepted freshness
state.

A commitment SHALL distinguish two different accepted states even when their
visible record generation is equal.

The commitment input SHALL bind, at minimum:

- exact governed freshness identity;
- accepted_epoch;
- accepted_sequence;
- record_generation;
- version/domain information required to prevent cross-contract ambiguity.

### 5.1 R3D-A commitment profile v1

The initial R3D software contract SHALL reuse the already governed R3C-D v1
canonical persistence representation rather than create a second representation
of the same accepted freshness state.

COMMITMENT_PROFILE_ID:

GUARDIAN_R3D_ANCHOR_COMMITMENT_V1

The commitment algorithm SHALL be SHA-256.

The commitment preimage SHALL be exactly:

DOMAIN_SEPARATOR
|| MATERIAL_LENGTH
|| R3C_D_V1_INTEGRITY_MATERIAL

DOMAIN_SEPARATOR SHALL be the exact 33 ASCII bytes:

GUARDIAN-R3D-ANCHOR-COMMITMENT-V1

No terminating NUL byte is included.

MATERIAL_LENGTH SHALL be one unsigned 32-bit integer encoded big-endian with
the exact value 332 decimal:

00 00 01 4C

R3C_D_V1_INTEGRITY_MATERIAL SHALL be exactly bytes 0 through 331 inclusive of
the governed R3C-D v1 canonical persistence serialization.

Therefore the R3D-A v1 commitment preimage length SHALL be exactly:

33 + 4 + 332 = 369 bytes

and the commitment output SHALL be exactly:

SHA-256(369-byte preimage) = 32 bytes

No native C structure bytes, padding bytes, pointer values, implementation
addresses, host byte order, implicit string lengths, locale transformations,
case folding, or character normalization may enter the commitment.

Because the governed R3C-D v1 material already fixes field offsets, semantic
reference encoding, zero fill, integer byte order, logical schema version,
accepted_epoch, accepted_sequence, and record_generation, this profile inherits
those exact canonical semantics.

The existing R3C-D integrity digest itself, bytes 332 through 363 of the
serialized record, SHALL NOT be included in R3D-A commitment material.

The R3D commitment is therefore domain-separated from the ordinary R3C-D
integrity digest even though both use SHA-256.

R3C_D_INTEGRITY_DIGEST != R3D_ROLLBACK_ANCHOR_COMMITMENT

Changing any commitment-profile field, hash algorithm, domain separator,
material-length encoding, governed R3C-D serialization semantics, or input byte
range requires a new explicitly governed commitment profile/version.

Silent reinterpretation is prohibited.

Conceptual software type:

ROLLBACK_ANCHOR_COMMITMENT

Required properties:

- deterministic;
- identity-bound;
- state-bound;
- version/domain-bound;
- comparison-safe;
- no authority semantics.

STATE_COMMITMENT_MATCH does not mean AUTHORIZED.

## 6. Monotonic generation contract

The rollback-anchor generation represents continuity of accepted history.

### 6.1 R3D-A generation profile v1

The initial R3D logical rollback-anchor generation SHALL use exactly the same
logical generation domain already governed for the R3C persisted record:

ROLLBACK_ANCHOR_GENERATION = unsigned 32-bit integer

The representable range SHALL be:

0 through UINT32_MAX inclusive

Canonical serialization SHALL be exactly four bytes, unsigned, big-endian.

The generation has numeric semantics only.

Native C object representation is not canonical serialization.

For the initial established state:

record_generation = 0
anchor_generation = 0

For normal advancement from established generation N:

expected_generation = N
next_generation = N + 1

is valid only when:

N < UINT32_MAX

The anchor generation and the accepted R3C record_generation SHALL represent
the same logical generation at the R3D durable acceptance boundary.

A provider SHALL NOT successfully establish:

anchor_generation != accepted_record_generation

for one accepted state.

When current generation equals UINT32_MAX:

- no N + 1 value exists;
- arithmetic wrap is prohibited;
- generation zero is not a successor;
- anchor reset is prohibited as continuation;
- candidate durable acceptance is prohibited;
- anchor_advance SHALL report the bounded exhaustion result;
- the established UINT32_MAX state may remain readable as historical/current
  evidence if otherwise valid;
- any further continuity requires separately governed maintenance,
  migration, replacement, or rejoin.

### 6.2 Pre-exhaustion governed transition

The implementation SHALL enter a separately governed maintenance or
reprovisioning state before ordinary continuation would consume the final
representable generation.

When:

current_generation == UINT32_MAX - 1

the numerically representable successor:

next_generation == UINT32_MAX

SHALL NOT be consumed through the ordinary automatic freshness-continuation
path.

Before any advancement that would consume UINT32_MAX, the implementation
SHALL require a separately governed maintenance, migration, replacement, or
rejoin decision.

Until that separately governed decision is completed:

- the current anchored UINT32_MAX - 1 state remains the last ordinarily
  accepted continuity state;
- ordinary automatic anchor advancement is blocked;
- arithmetic wrap is prohibited;
- generation reset is prohibited;
- anchor reset is prohibited as continuation;
- bootstrap is not a substitute for continuity recovery;
- ordinary authenticated peer traffic cannot clear the lifecycle boundary;
- no authority or actuation grant is implied.

A separately governed lifecycle procedure MAY define how a terminal
UINT32_MAX generation is used during migration, replacement, or other
explicitly authorized transition.

Absent such separate authorization:

UINT32_MAX - 1 -> UINT32_MAX

SHALL NOT occur as ordinary automatic R3D continuation.

UINT32_MAX -> 0 remains prohibited.

Comparison SHALL use unsigned mathematical ordering over the exact uint32
domain.

No signed reinterpretation is permitted.

No backend may expose a larger physical monotonic counter and silently truncate
it into the R3D logical generation.

A future wider generation requires a new governed contract version and explicit
migration semantics.

Conceptual software type:

ROLLBACK_ANCHOR_GENERATION

Required properties:

- uint32 logical domain;
- four-byte big-endian canonical encoding;
- monotonic;
- non-wrapping;
- equality-comparable;
- unsigned-order-comparable;
- maximum value UINT32_MAX;
- exhaustion-detectable.

UINT32_MAX -> 0 is prohibited.

## 7. Anchor status model

A conforming software contract SHALL distinguish at minimum:

ANCHOR_UNPROVISIONED

ANCHOR_INDETERMINATE

ANCHOR_VALID

ANCHOR_UNAVAILABLE

ANCHOR_CORRUPT

ANCHOR_EXHAUSTED

ANCHOR_IDENTITY_MISMATCH

ANCHOR_STATE_MISMATCH

ROLLBACK_SUSPECTED

These values describe anchor/recovery evidence.

They do not grant authority.

ANCHOR_UNPROVISIONED SHALL NOT be inferred merely because ordinary persistence
is absent.

A previously established anchor that cannot be read is not equivalent to a
never-established anchor.

## 8. Provider operation result model

Provider execution status SHALL be distinct from anchor security
classification.

A future provider contract SHALL distinguish at minimum:

PROVIDER_OK

PROVIDER_INVALID_ARGUMENT

PROVIDER_UNAVAILABLE

PROVIDER_IO_ERROR

PROVIDER_INTEGRITY_ERROR

PROVIDER_CONFLICT

PROVIDER_EXHAUSTED

PROVIDER_UNSUPPORTED

Provider failure SHALL NOT silently become ANCHOR_UNPROVISIONED.

Provider success SHALL NOT silently become ANCHOR_VALID.

Execution result and security interpretation remain separate.

### 8.1 Deterministic provider-result interpretation matrix

The provider result alone is not permission to invent security state.

The initial R3D contract SHALL apply the following minimum interpretation:

| Provider result | Minimum anchor interpretation | Required security handling |
| --- | --- | --- |
| PROVIDER_OK | No automatic classification | Consume and validate the operation-specific returned anchor evidence before deriving ANCHOR_VALID or another status |
| PROVIDER_INVALID_ARGUMENT | ANCHOR_INDETERMINATE | Fail closed; do not bootstrap, advance, or accept a new durable freshness state |
| PROVIDER_UNAVAILABLE | ANCHOR_UNAVAILABLE | Established continuity cannot be proven while unavailable |
| PROVIDER_IO_ERROR | ANCHOR_UNAVAILABLE | Fail closed; no assumption that previous or candidate anchor state is current |
| PROVIDER_INTEGRITY_ERROR | ANCHOR_CORRUPT | Fail closed; do not derive current-state proof |
| PROVIDER_CONFLICT | ANCHOR_INDETERMINATE pending readback | Do not retry advancement automatically; read/recovery adjudication is required before any further mutation |
| PROVIDER_EXHAUSTED | ANCHOR_EXHAUSTED for advancement | Reject the candidate advancement; do not wrap or reset generation |
| PROVIDER_UNSUPPORTED | ANCHOR_INDETERMINATE | Fail closed; unsupported behavior cannot become unprovisioned or valid state |

ANCHOR_INDETERMINATE means that the software does not possess sufficient
provider evidence to assign another governed anchor status.

ANCHOR_INDETERMINATE is not equivalent to ANCHOR_UNPROVISIONED.

For a previously established identity, if continuity cannot be established due
to:

- ANCHOR_INDETERMINATE;
- ANCHOR_UNAVAILABLE;
- ANCHOR_CORRUPT;
- ANCHOR_IDENTITY_MISMATCH;
- ANCHOR_STATE_MISMATCH; or
- ROLLBACK_SUSPECTED;

normal freshness continuation SHALL be prohibited.

The freshness consequence SHALL be:

FRESHNESS_UNKNOWN

and, where the established continuity cannot be restored by deterministic
read-only recovery:

REJOIN_REQUIRED

PROVIDER_EXHAUSTED during an attempted advance has narrower semantics:

- the candidate advancement fails;
- the candidate SHALL NOT cross the durable acceptance boundary;
- the previously verified anchored state is not invalidated solely by the
  exhaustion result;
- no wrap or reset is permitted;
- separately governed maintenance/migration/rejoin is required before further
  advancement.

PROVIDER_CONFLICT has compare-and-advance semantics.

A conflict SHALL NOT cause an automatic retry.

The caller SHALL first obtain read-only anchor evidence and execute the governed
recovery classifier.

This prevents an expected-generation mismatch from being overwritten by a
blind retry.

For an operation returning PROVIDER_OK, returned evidence remains subject to:

- exact identity validation;
- exact generation validation;
- commitment validation;
- lifecycle validation;
- operation-specific invariants.

PROVIDER_OK != ANCHOR_VALID

PROVIDER_FAILURE != ANCHOR_UNPROVISIONED

PROVIDER_CONFLICT != RETRY_PERMISSION

## 9. Abstract provider operations

A future software implementation SHALL provide semantics equivalent to:

anchor_status(identity)

anchor_read(identity)

anchor_compare(
    identity,
    candidate_generation,
    candidate_commitment
)

anchor_advance(
    identity,
    expected_generation,
    next_generation,
    next_commitment
)

This specification defines required semantics, not final C ABI signatures.

### 9.1 anchor_status

anchor_status SHALL determine the provider-visible lifecycle condition for the
exact identity.

It SHALL NOT mutate anchor state.

It SHALL NOT provision automatically.

It SHALL NOT reset established history.

### 9.2 anchor_read

anchor_read SHALL return the currently established anchor evidence for the
exact identity or an explicit failure/classification.

It SHALL NOT advance state.

It SHALL NOT infer a new anchor from ordinary persistence.

### 9.3 anchor_compare

anchor_compare SHALL compare a candidate persisted state against independently
retrieved anchor evidence.

It SHALL distinguish:

- generation behind anchor;
- generation equal to anchor with matching commitment;
- generation equal to anchor with different commitment;
- generation ahead of anchor;
- identity mismatch;
- unavailable/corrupt anchor;
- unprovisioned lifecycle where mechanically proven.

It SHALL NOT mutate anchor state.

### 9.4 anchor_advance

anchor_advance SHALL require:

- exact identity;
- expected current generation;
- next generation;
- next state commitment.

The provider SHALL reject advancement when expected current state does not
match actual current state.

This is conceptually compare-and-advance behavior.

The operation SHALL NOT:

- skip unknown history silently;
- wrap generation;
- reset generation;
- change identity implicitly;
- infer authority;
- accept an ordinary slot as proof of expected current anchor.

Successful provider execution SHALL be followed by governed verification before
the new freshness state becomes externally effective.

## 10. Durable acceptance transaction

The software implementation SHALL preserve the following ordering:

1. authenticate and structurally validate candidate peer evidence;
2. apply governed semantic interpretation;
3. evaluate freshness candidate;
4. derive canonical next freshness state;
5. derive next record generation without wrap;
6. durably write and verify the inactive ordinary R3C persistence slot;
7. keep the previously anchored state recoverable;
8. derive the rollback-anchor commitment for the candidate state;
9. invoke anchor_advance using expected-current anchor state;
10. verify the resulting anchor state;
11. only after successful anchor verification cross the R3D durable acceptance
    boundary;
12. only after that boundary may downstream logic consume the state as
    persistently fresh, subject to later authority gates.

The transaction SHALL NOT expose the new candidate as persistently accepted
between steps 6 and 10.

A power interruption before verified anchor advancement SHALL NOT make an
unanchored future slot current after reboot.

### 10.1 Recovery after verified anchor advancement

A power interruption MAY occur after step 10 has successfully verified the
advanced independent anchor but before the pre-reset execution path completes
step 11 or returns an externally observable successful acceptance result.

That crash window SHALL be recovered deterministically.

After reboot, if recovery mechanically proves all of the following:

- the governed freshness identity exactly matches the anchor identity;
- the independent anchor is valid;
- the anchor reports generation A and commitment C_A;
- an ordinary persisted candidate has generation S where S == A;
- that candidate derives commitment C_S where C_S == C_A;
- the candidate passes all inherited R3C structural, canonical, integrity,
  and identity validation;
- no same-generation conflicting candidate creates an
  ANCHOR_STATE_MISMATCH condition;

then that exact anchored persisted candidate is the only eligible state from
which R3D durable continuity may be reconstructed.

Recovery in this case SHALL NOT classify the candidate as an unanchored
future slot.

Recovery SHALL NOT invoke anchor_advance again for the same generation.

Recovery SHALL NOT increment the generation again.

Recovery SHALL NOT reset the generation or the anchor.

Recovery SHALL NOT convert the event into bootstrap.

Recovery MAY complete or reconstruct the R3D durable acceptance boundary from
the independently verified A/C_A evidence.

This does not assert that the pre-reset execution completed step 11 before
power loss.

It asserts only that reboot recovery can deterministically complete the
durable acceptance decision from the already advanced and verified independent
anchor plus the exact matching persisted state.

No caller-visible side effect from the interrupted pre-reset path SHALL be
assumed to have occurred unless independently evidenced.

Recovered durable freshness does not grant downstream authority.

Therefore:

RECOVERED_DURABLE_FRESHNESS != AUTHORITY

RECOVERED_DURABLE_FRESHNESS != ACTUATION_AUTHORITY

RECOVERED_DURABLE_FRESHNESS != AI_AUTHORITY

If exact S == A and C_S == C_A recovery cannot be proven, this subsection does
not authorize acceptance, anchor advancement, generation selection, bootstrap,
or cleanup.

The normal fail-closed recovery comparison model SHALL apply.

## 11. Recovery comparison model

Let:

A = independently established anchor generation

S = structurally valid ordinary persisted-slot generation

C_A = anchor commitment

C_S = persisted-state commitment

Recovery SHALL classify the following cases explicitly.

### 11.1 S == A and C_S == C_A

The persisted state may be a current-state candidate.

All inherited identity, integrity, semantic, and freshness validation still
applies.

This condition does not grant authority.

### 11.2 S < A

The persisted state is behind established anchor history.

Required classification:

ROLLBACK_SUSPECTED

For previously established history:

FRESHNESS_UNKNOWN
-> REJOIN_REQUIRED

The old persisted state SHALL NOT become current.

### 11.3 S > A

The persisted state SHALL NOT automatically become current.

It may represent an ordinary persistence write that completed before anchor
advancement.

Boot recovery SHALL NOT advance the anchor solely because S > A.

The initial R3D recovery algorithm SHALL evaluate the complete bounded set of
ordinary R3C persistence-slot candidates against the independently read anchor.

Let:

A = anchor generation

C_A = anchor commitment

For every structurally and integrity-valid ordinary slot candidate i:

S_i = candidate record_generation

C_i = R3D-A commitment derived from that candidate

Recovery SHALL proceed in this exact order:

1. validate the exact expected freshness identity;
2. obtain independently validated anchor evidence;
3. validate each ordinary slot using inherited R3C rules;
4. derive C_i using GUARDIAN_R3D_ANCHOR_COMMITMENT_V1;
5. identify every candidate satisfying:

   S_i == A
   and
   C_i == C_A

6. identify every generation-A candidate satisfying:

   S_i == A
   and
   C_i != C_A

7. identify future candidates where S_i > A;
8. identify older candidates where S_i < A.

If any structurally valid generation-A candidate has:

S_i == A
and
C_i != C_A

recovery SHALL fail closed with:

ANCHOR_STATE_MISMATCH

and SHALL NOT choose another copy merely because another generation-A copy
matches.

If one or more candidates exactly match both A and C_A, all exact-matching
candidates SHALL correspond to the same governed canonical R3C-D integrity
material.

If exact-matching candidates disagree in canonical bytes, recovery SHALL fail
closed as an irreconcilable state inconsistency even if a cryptographic digest
collision would otherwise make their commitments equal.

When at least one unique canonical candidate exactly matches A and C_A and no
generation-A mismatch exists:

- that anchored candidate is the only eligible current-state recovery
  candidate;
- a slot with S_i > A is classified as an unanchored future candidate;
- an unanchored future candidate SHALL NOT advance the anchor during boot;
- an unanchored future candidate SHALL NOT become current;
- an unanchored future candidate SHALL NOT replace the exact anchored
  candidate;
- an older residual slot with S_i < A SHALL NOT become current;
- the mere presence of an older residual dual-slot copy does not override an
  independently proven exact A/C_A match;
- recovery itself SHALL perform no erase, rewrite, cleanup, or anchor mutation.

Cleanup or replacement of residual/future ordinary slots is a separate mutation
boundary.

If no exact A/C_A candidate exists:

- any S_i < A candidate is insufficient to reconstruct current continuity;
- any S_i > A candidate is insufficient to reconstruct current continuity;
- a mixture of older and future candidates is insufficient;
- the numerically greatest visible generation SHALL NOT be selected;
- the anchor SHALL NOT be advanced from persisted slots;
- ordinary persistence SHALL NOT be reinitialized automatically.

For previously established history, absence of an exact A/C_A candidate SHALL
produce:

FRESHNESS_UNKNOWN
-> REJOIN_REQUIRED

This rule closes the initial R3D S > A recovery behavior.

No separately unspecified boot-time S > A recovery policy remains for the
initial contract.

RECOVERY_MATCHES_ANCHOR != AUTHORITY

RECOVERED_PERSISTED_STATE != ACTUATION_AUTHORITY

### 11.4 S == A and C_S != C_A

Generation equality with commitment disagreement SHALL be treated as evidence
of substitution, corruption, or rollback.

The implementation SHALL NOT guess which representation is authoritative.

Required fail-closed classification:

ANCHOR_STATE_MISMATCH
and/or
ROLLBACK_SUSPECTED

### 11.5 Anchor exists, no matching valid ordinary slot exists

Continuity cannot be reconstructed locally.

Required transition for previously established history:

FRESHNESS_UNKNOWN
-> REJOIN_REQUIRED

### 11.6 No anchor and no established history

This state is eligible only for a separately governed bootstrap/provisioning
path.

It SHALL NOT self-authorize peer acceptance.

It SHALL NOT be inferred solely from erased SLOT_A/SLOT_B.

## 12. Freshness-unknown contract

FRESHNESS_UNKNOWN means continuity with previously accepted freshness history
cannot be proven.

FRESHNESS_UNKNOWN SHALL fail closed for freshness-dependent,
authority-relevant consumption.

FRESHNESS_UNKNOWN SHALL NOT become FRESH merely because:

- a frame authenticates;
- a persistence record passes integrity;
- a slot contains a high generation;
- peer state reports ACTIVE;
- peer state reports SAFE_HOLD;
- the system rebooted;
- an operator requested ordinary startup.

FRESHNESS_UNKNOWN is not an authority state.

## 13. Rejoin-required contract

For previously established freshness history:

PERSISTENT_STATE_LOSS = REJOIN_REQUIRED

ROLLBACK_SUSPECTED = REJOIN_REQUIRED

IRRECONCILABLE_ANCHOR_MISMATCH = REJOIN_REQUIRED

REJOIN_REQUIRED SHALL enter a separately governed recovery/rejoin path.

Ordinary peer traffic SHALL NOT self-authorize rejoin.

BOOTSTRAP != REJOIN

No API in this contract SHALL silently convert REJOIN_REQUIRED into ordinary
bootstrap.

## 14. Provisioning lifecycle

The software contract SHALL support explicit lifecycle distinctions equivalent
to:

UNPROVISIONED

PROVISIONING_AUTHORIZED

INITIALIZED

ACTIVE

ADVANCING

ACTIVE_UPDATED

UNAVAILABLE

CORRUPT_OR_MISMATCHED

EXHAUSTED

REJOIN_REQUIRED

REPLACEMENT_AUTHORIZED

DECOMMISSIONED

Provisioning requires separate governance.

The following events SHALL NOT silently reclassify established history as
UNPROVISIONED:

- firmware update;
- reset;
- factory/service reset;
- debug attach;
- mass erase;
- ordinary persistence erase;
- board replacement;
- storage replacement;
- anchor-provider replacement.

Loss of established anchor continuity SHALL fail closed unless a separately
governed recovery procedure proves otherwise.

## 15. Backup, restore, replacement, and cloning

Restoration of an ordinary persistence backup does not restore freshness
continuity automatically.

An ordinary backup behind the established anchor SHALL produce rollback
evidence.

A backup without corresponding verifiable anchor continuity SHALL NOT become
trusted current state.

Copying ordinary persistence to another device SHALL NOT transfer freshness
authority.

Device replacement and provider replacement require explicit lifecycle
governance.

## 16. Host/test-double provider contract

Before any physical rollback-anchor backend is selected, R3D MAY use a
deterministic host/test-double provider to validate software semantics.

Such a provider is test infrastructure only.

It SHALL NOT be represented as physical rollback resistance.

The host provider SHALL be capable of deterministically modeling:

- never provisioned state;
- valid established state;
- unavailable provider;
- corrupt anchor;
- identity mismatch;
- commitment mismatch;
- monotonic advancement;
- expected-generation conflict;
- exhaustion;
- rollback observation;
- interruption before advancement;
- interruption after ordinary persistence but before anchor advancement;
- successful advancement;
- readback mismatch.

The test-double SHALL permit hostile state injection without weakening
production contracts.

HOST_TEST_DOUBLE_PASS != PHYSICAL_ROLLBACK_RESISTANCE

## 17. Required positive tests for future implementation

A future separately authorized implementation SHALL prove at minimum:

1. genuine unprovisioned lifecycle is distinguished from lost established
   history;
2. exact identity binding succeeds for the matching identity;
3. anchor read returns the exact established generation and commitment;
4. equal generation/equal commitment is accepted as a recovery candidate;
5. monotonic advance succeeds from exact expected generation;
6. verified advance permits crossing the durable acceptance boundary;
7. reboot recovery can identify the exact anchored ordinary state;
8. maximum non-exhausted generation behavior is deterministic.

Positive tests alone are insufficient for claim promotion.

## 18. Required hostile and negative tests

Future implementation validation SHALL include at minimum:

### Ordinary persistence rollback

- restore old SLOT_A;
- restore old SLOT_B;
- restore both old slots;
- restore a complete older persistence image;
- restore an older structurally valid canonical record.

### Anchor disagreement

- old slot plus newer anchor;
- future/unanchored slot plus older anchor;
- same generation plus different commitment;
- wrong identity with structurally valid anchor;
- valid slot with unavailable anchor;
- valid anchor with unavailable slots;
- corrupt anchor;
- exhausted anchor.

### Transaction interruption

Inject interruption:

- before ordinary slot write;
- during ordinary slot write;
- after ordinary slot verification;
- before anchor advance;
- during anchor advance when observable by the provider;
- after provider reports advance but before verification;
- after verified anchor advancement but before externally effective acceptance.

### Lifecycle abuse

- silent generation reset attempt;
- wrap attempt;
- silent anchor reset attempt;
- ordinary erase interpreted as factory state;
- restore/clone to wrong identity;
- rejoin attempted through ordinary peer traffic.

Each hostile case SHALL have an explicit expected fail-closed result.

## 19. Determinism requirements

For identical:

- governed identity;
- persisted slot observations;
- anchor observation;
- provider result;
- lifecycle context;

the recovery classifier SHALL produce the same result.

Recovery SHALL NOT depend on:

- iteration order;
- pointer address;
- uninitialized memory;
- wall-clock coincidence;
- unspecified integer overflow;
- undefined C behavior;
- provider-specific undocumented side effects.

Unknown or unsupported states SHALL fail closed.

## 20. Versioning and domain separation

The rollback-anchor software contract SHALL carry explicit contract/version
identity sufficient to prevent ambiguous interpretation across incompatible
future revisions.

A commitment implementation SHALL include or inherit an unambiguous domain
separator.

An implementation SHALL NOT reinterpret an existing anchor under incompatible
future semantics without an explicitly governed migration procedure.

Historical meaning is immutable.

## 21. Concurrency and reentrancy boundary

The first implementation SHALL explicitly state whether the anchor provider is:

- single-threaded;
- serialized by caller;
- internally serialized;
- interrupt-safe;
- reentrant.

No concurrency property shall be assumed implicitly.

Concurrent anchor advancement for the same exact identity SHALL NOT permit two
different next states to both be treated as successfully established.

The expected-generation precondition SHALL participate in preventing lost
updates.

## 22. Failure atomicity

The software contract SHALL distinguish:

- ordinary persistence commit;
- anchor advancement;
- anchor verification;
- external freshness acceptance.

A failure at any stage SHALL preserve enough information to recover or fail
closed deterministically.

The implementation SHALL NOT erase prior anchored history merely because a
future candidate transaction fails.

A future slot without corresponding verified anchor advancement SHALL remain
non-current.

## 23. Physical-backend boundary

This specification intentionally does not select:

- internal STM32 flash as a rollback anchor;
- option bytes;
- OTP;
- secure element;
- TPM;
- external protected NVM;
- another MCU;
- heterogeneous peer;
- remote service;
- any other physical mechanism.

Selection of a physical anchor backend requires a separate governed decision
that states:

1. attacker model;
2. independence property;
3. monotonicity property;
4. identity binding;
5. write/erase semantics;
6. exhaustion behavior;
7. failure modes;
8. provisioning/replacement lifecycle;
9. hardware evidence requirements.

Listing a mechanism is not approval of that mechanism.

## 24. Authority boundary

Rollback-anchor validation provides freshness-continuity evidence only.

It SHALL NOT directly:

- authorize an actuator;
- transfer control authority;
- elevate a peer;
- select a control source;
- bypass documentary authority;
- bypass semantic compatibility;
- bypass authentication;
- bypass deterministic policy;
- authorize NodeSupervisor;
- grant AI authority.

The ordering remains conceptually:

authenticated evidence
-> semantic compatibility
-> freshness/replay evaluation
-> persistent continuity evidence
-> documentary authority
-> deterministic policy
-> separately authorized action

No earlier stage inherits the authority of a later stage.

## 25. Evidence and claim-promotion requirements

The following claims remain prohibited at creation of this specification:

ROLLBACK_ANCHOR_IMPLEMENTED = YES

ROLLBACK_ANCHOR_DEMONSTRATED = YES

PERSISTENT_ANTI_REPLAY_IMPLEMENTED = YES

PERSISTENT_ANTI_REPLAY_DEMONSTRATED = YES

PHYSICAL_ROLLBACK_RESISTANCE = YES

R3D_VALIDATED = YES

R3D_EVIDENCE_COMPLETE = YES

NODESUPERVISOR_CONSUMPTION_AUTHORIZED = YES

ACTUATION_AUTHORITY_GRANTED = YES

AI_AUTHORITY_GRANTED = YES

RELEASE_READY = YES

Future claim promotion requires evidence appropriate to the exact claim.

Software tests can support software-contract claims.

Physical rollback-resistance claims require evidence from the selected physical
backend and applicable hardware boundary.

## 26. Requirement mapping to R3D architecture

R3D-D01:
Sections 4, 5, 10, 11.

R3D-D02:
Sections 18 and 23 plus inherited architecture attacker model.

R3D-D03:
Sections 4 through 9 and 23.

R3D-D04:
Sections 3, 5, 7, and 11.

R3D-D05:
Sections 7, 11, 12, and 13.

R3D-D06:
Section 12.

R3D-D07:
Section 13.

R3D-D08:
Sections 10, 11, and 22.

R3D-D09:
Section 6 and Section 14.

R3D-D10:
Section 15.

R3D-D11:
Section 14 and Section 23.

R3D-D12:
Sections 17 and 18.

## 27. Implementation boundary after this specification

Creation of this specification does not authorize implementation.

After creation, the next required boundary is human review of the exact
document diff.

Only after that review may staging of this specification be separately
authorized.

No firmware source, header, test, build configuration, CI configuration, or
hardware file may be created or modified by this boundary.

## 28. Explicit non-claims

This specification does not demonstrate:

- a physical rollback anchor;
- secure-element integration;
- monotonic hardware state;
- authenticated protected storage;
- physical anti-rollback;
- cross-reboot persistent anti-replay;
- real-device power-loss recovery;
- real-device rollback detection;
- NodeSupervisor authority readiness;
- actuation authority;
- AI authority;
- production readiness;
- release readiness;
- certification;
- compliance;
- approval or endorsement by any external organization.

## 29. Human boundary

AUTHORIZED_BOUNDARY =
HUMAN_R3D_A_SOFTWARE_CONTRACT_SPECIFICATION_CREATION_AUTHORIZATION

AUTHORIZED_FILE =
docs/specs/M16-R3D-Rollback-Anchor-Software-Contract-v0.1.md

AUTHORIZED_MUTATION =
CREATE_ONE_UNTRACKED_SPECIFICATION_FILE

STAGING_AUTHORIZED = NO

COMMIT_AUTHORIZED = NO

PUSH_AUTHORIZED = NO

PR_AUTHORIZED = NO

MERGE_AUTHORIZED = NO

TAG_AUTHORIZED = NO

RELEASE_AUTHORIZED = NO

NEXT_BOUNDARY =
HUMAN_R3D_A_SOFTWARE_CONTRACT_FINAL_DIFF_REVIEW

STOP_BEFORE_STAGING = YES
