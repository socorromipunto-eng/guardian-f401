# TD-M15-006 — Bootstrap and Epoch-Transition Authorization Model

Status: OPEN

Finding: OPEN

Implementation authorization: NOT AUTHORIZED

## 1. Purpose

This Technical Destruction defines the authorization boundaries required to resolve the two deliberately non-final M15-03 outcomes:

- FIRST_SEEN
- EPOCH_TRANSITION_REQUIRED

These outcomes are not themselves authorization.

```text
FIRST_SEEN != BOOTSTRAP_AUTHORIZED
EPOCH_TRANSITION_REQUIRED != EPOCH_TRANSITION_AUTHORIZED
```

The purpose of this document is to freeze the authorization contract before any bootstrap or epoch-transition implementation is created.

## 2. Security boundary

The normal signed assurance message must not authorize its own bootstrap or epoch transition.

Authentication proves provenance of the assurance statement.
Freshness proves the approved M15-03 anti-replay property.
Neither fact grants bootstrap or epoch-transition authority.

```text
AUTHENTICATED != BOOTSTRAP_AUTHORIZED
AUTHENTICATED != EPOCH_TRANSITION_AUTHORIZED
FRESH != AUTHORIZED
```

## 3. Separate privilege domains

Bootstrap authorization and epoch-transition authorization are distinct privileges.

```text
BOOTSTRAP_AUTHORIZATION != EPOCH_TRANSITION_AUTHORIZATION
```

An implementation must not collapse them into a generic administrative override.

## 4. Bootstrap authorization

Bootstrap applies only when the verifier can establish that the producer has never previously established M15-03 freshness state.

Bootstrap must not be used to recover from:

- missing previously established state;
- corrupted state;
- truncated state;
- unavailable state;
- detectable persistent-state rollback;
- superseded epoch state.

Those conditions remain fail-closed.

### 4.1 Bootstrap authorization candidate contract

A bootstrap authorization candidate must bind at least:

- producer_id;
- initial producer_epoch;
- explicit initial high-water state;
- explicit initial logical-time interpretation;
- authorization issuer / authority identity;
- authorization credential or trust reference;
- authorization purpose/domain;
- unique authorization identifier;
- anti-replay value or monotonic authorization sequence;
- authorization version.

The authorization must not reserve logical_time zero as an implicit unset sentinel.

### 4.2 Bootstrap result boundary

Successful authorization may establish a bootstrap candidate only.

```text
FIRST_SEEN
-> valid external bootstrap authorization
-> bootstrap candidate
-> persistent prepare
-> commit
-> verify
-> established freshness state
```

No bootstrap result becomes authoritative before verified persistence.

## 5. Epoch-transition authorization

An epoch transition applies only to one exact transition.

The authorization candidate must bind at least:

- producer_id;
- from_epoch;
- to_epoch;
- transition_sequence;
- authorization issuer / authority identity;
- authorization credential or trust reference;
- authorization purpose/domain;
- unique authorization identifier;
- anti-replay value;
- authorization version.

The authorization must not be reusable for another producer, from_epoch, to_epoch, or transition_sequence.

### 5.1 Transition candidate boundary

```text
EPOCH_TRANSITION_REQUIRED
-> exact external epoch-transition authorization
-> transition candidate
-> persistent prepare
-> commit
-> verify
-> new current_epoch
```

The previous current_epoch becomes retained previous_epoch according to the frozen M15-03 state model.

### 5.2 Replay and rollback

A consumed transition authorization must not be reusable.

An authorization for a superseded or already-consumed transition must fail closed.

An authorization must not permit silent transition-sequence rollback or wraparound.

## 6. Authority source

The authority model is NOT YET FROZEN.

This TD does not yet decide whether bootstrap and epoch-transition authorization are represented by:

- a dedicated signed authorization object;
- an offline provisioning artifact;
- an operator-signed manifest;
- a separate trust-store role;
- another explicitly reviewed mechanism.

No implementation may choose one silently.

## 7. Trust separation

The trust credential used to authenticate ordinary assurance statements must not automatically imply bootstrap or epoch-transition authority.

Least privilege requires explicit authorization capability.

The final model must distinguish at least:

- assurance-message authentication authority;
- bootstrap authorization authority;
- epoch-transition authorization authority.

## 8. Anti-replay requirement

Authorization replay protection is separate from ordinary assurance logical_time freshness.

The final authorization model must define how authorization consumption is made persistent and replay-safe.

No authorization object may rely solely on the assurance statement logical_time for its own replay protection.

## 9. Persistence requirement

Bootstrap and epoch-transition authorization consumption must be transactionally coupled to the resulting persistent freshness-state change or otherwise provide equivalent fail-closed semantics.

Power loss or commit failure must not produce an ambiguous state in which authorization appears consumed while state was not updated, or state was updated while authorization replay remains possible.

## 10. Current implementation disposition

Current M15-03 software intentionally stops at:

```text
FIRST_SEEN
EPOCH_TRANSITION_REQUIRED
```

No bootstrap implementation exists.
No epoch-transition authorization implementation exists.
No authorization object schema exists.
No authorization trust role exists.

## 11. Open decisions

Before implementation authorization, this TD must adjudicate:

1. exact authorization object representation;
2. exact purpose/domain separation;
3. authority credential model;
4. trust-store role separation;
5. bootstrap authorization lifecycle;
6. epoch-transition authorization lifecycle;
7. authorization anti-replay state;
8. authorization resource bounds;
9. atomic authorization-consumption semantics;
10. failure and recovery semantics;
11. authorization expiration or validity rules, if any;
12. operator/provisioning workflow;
13. host validation evidence;
14. STM32 applicability boundaries.

## 12. Initial disposition

Finding: OPEN

Bootstrap implementation: NOT AUTHORIZED

Epoch-transition implementation: NOT AUTHORIZED

No code implementation may begin until the remaining authorization decisions are frozen and this TD is formally resolved.


## Decision A - Authorization Object and Authority Separation

Status: FROZEN

### A.1 Separate authorization object

Bootstrap and epoch-transition authorization shall use dedicated signed authorization objects.

An ordinary signed assurance message shall not authorize its own bootstrap or epoch transition.

SIGNED_ASSURANCE_MESSAGE != SIGNED_AUTHORIZATION_OBJECT

ASSURANCE_AUTHENTICATED != BOOTSTRAP_AUTHORIZED

ASSURANCE_AUTHENTICATED != EPOCH_TRANSITION_AUTHORIZED

### A.2 Separate authorization classes

BootstrapAuthorizationV1

EpochTransitionAuthorizationV1

BOOTSTRAP_AUTHORIZATION != EPOCH_TRANSITION_AUTHORIZATION

The two authorization classes must not be collapsed into a generic administrative override.

### A.3 Purpose-domain separation

Bootstrap authorization domain:

guardian-f401:m15:authorization:bootstrap:v1

Epoch-transition authorization domain:

guardian-f401:m15:authorization:epoch-transition:v1

An authorization authenticated under one domain must not be accepted under another domain.

Domain mismatch fails closed.

### A.4 Authority capabilities

The architecture defines three logically distinct capabilities:

ASSURANCE_SIGNER

BOOTSTRAP_AUTHORITY

EPOCH_TRANSITION_AUTHORITY

Possession of one capability does not imply possession of another.

No capability inheritance is permitted.

A credential may hold multiple capabilities only through explicit configuration and validation.

### A.5 Existing trust-store boundary

The existing assurance trust store resolves ordinary producer assurance credentials.

It shall not be silently reinterpreted as bootstrap or epoch-transition authority.

Authorization trust requires an explicit authorization-capability model.

Trust-store extension versus a separate authorization trust store remains a subsequent design decision.

Until that decision is frozen, the current assurance trust store grants no bootstrap or epoch-transition authority.

### A.6 Bootstrap authorization minimum binding

BootstrapAuthorizationV1 shall bind at minimum:

- authorization_version;
- authorization_id;
- authorization purpose domain;
- authority identity;
- authority key_id;
- producer_id;
- initial producer_epoch;
- initial high-water-state interpretation;
- initial logical-time interpretation;
- authorization anti-replay value.

Bootstrap authorization applies only to a producer that has never established freshness state.

It must not recover lost, corrupt, unavailable, or rolled-back previously established state.

### A.7 Epoch-transition authorization minimum binding

EpochTransitionAuthorizationV1 shall bind at minimum:

- authorization_version;
- authorization_id;
- authorization purpose domain;
- authority identity;
- authority key_id;
- producer_id;
- from_epoch;
- to_epoch;
- transition_sequence;
- authorization anti-replay value.

The authorization applies only to the exact producer_id + from_epoch + to_epoch + transition_sequence tuple.

Changing any member requires a different valid authorization.

### A.8 Authorization identifier

authorization_id belongs to the authorization security domain.

authorization_id != ordinary assurance object_id

authorization_id != producer logical_time

authorization_id != producer_epoch

authorization_id != transition_sequence

### A.9 Independent authorization anti-replay

Authorization replay protection is independent of ordinary producer freshness.

producer logical_time != authorization anti-replay state

A consumed authorization must not become reusable because a later assurance statement is fresh.

The exact bounded authorization anti-replay state model remains deferred to Decision B.

### A.10 Producer and authority separation

producer identity != authority identity

The producer is the subject whose freshness state changes.

The authority is the security principal permitted to authorize that change.

No self-authorization is implied by matching identifiers or credentials.

### A.11 Fail-closed requirements

The following fail closed:

- unknown authorization authority;
- unknown authority credential;
- revoked authority credential;
- unsupported authorization algorithm;
- wrong authorization purpose domain;
- malformed authorization object;
- authorization signature failure;
- authorization replay;
- authorization subject mismatch;
- from_epoch mismatch;
- to_epoch mismatch;
- transition_sequence mismatch;
- authorization capability mismatch.

No fallback to ordinary assurance trust is permitted.

### A.12 Decision A disposition

Decision A is FROZEN.

Dedicated signed authorization objects are required.

Bootstrap and epoch-transition authorization remain separate privileges.

Authorization capabilities remain separate from ordinary assurance signing.

Authorization replay protection remains independent from producer logical_time.

Decision A does not authorize implementation.

Remaining decisions include:

- exact wire/schema representation;
- exact authorization transcript encoding;
- trust-store extension versus separate authorization trust store;
- authorization anti-replay state model;
- authorization consumption transaction;
- bootstrap lifecycle;
- epoch-transition lifecycle;
- resource bounds;
- recovery behavior;
- STM32 implementation mapping.

Implementation authorization remains: NOT AUTHORIZED.

## Decision B - Authorization Anti-Replay and Consumption Semantics

Status: FROZEN

### B.1 Independent authorization sequence

Authorization replay protection shall use an explicit unsigned 64-bit authorization_sequence.

authorization_sequence belongs to the authorization security domain.

authorization_sequence != producer logical_time

authorization_sequence != transition_sequence

authorization_sequence != authorization_id

The sequence is authenticated as part of the signed authorization object.

### B.2 Replay-state scope

Authorization replay state is maintained for the exact scope:

authority identity + authorization purpose domain + producer_id

Bootstrap authorization and epoch-transition authorization therefore maintain logically separate replay state because their purpose domains are distinct.

One producer authorization sequence does not advance another producer authorization sequence.

One authorization purpose does not advance another authorization purpose.

### B.3 Strict monotonic acceptance

For an established authorization replay state with durable high-water mark H:

authorization_sequence > H -> AUTHORIZATION_CANDIDATE

authorization_sequence == H -> AUTHORIZATION_REPLAY

authorization_sequence < H -> AUTHORIZATION_SEQUENCE_ROLLBACK

Authorization replay acceptance window = 0.

Out-of-order authorization acceptance is NOT SUPPORTED in v1.

A future positive replay window requires a new architecture decision.

### B.4 Initial authorization replay state

Absence of authorization replay state does not by itself authorize an operation.

For bootstrap, the producer must independently satisfy the frozen FIRST_SEEN and explicit bootstrap-authorization requirements.

For epoch transition, the producer must independently satisfy the frozen EPOCH_TRANSITION_REQUIRED and exact-transition authorization requirements.

Authorization replay-state initialization is subordinate to a valid authorization operation; it is not an authorization source.

### B.5 Authorization identifier

authorization_id remains a mandatory signed identifier for audit, correlation, and exact-object identity.

authorization_id is not the persistent anti-replay high-water mark.

The v1 architecture does not require an unbounded persistent set of all historical authorization_id values.

Replay resistance is established by strict monotonic authorization_sequence within the frozen replay-state scope.

### B.6 Sequence exhaustion

authorization_sequence is unsigned 64-bit.

The value must not wrap.

If the maximum authorization_sequence has already been durably consumed:

-> AUTHORIZATION_SEQUENCE_EXHAUSTED

AUTHORIZATION_SEQUENCE_EXHAUSTED fails closed.

No implicit reset, wraparound, or new sequence domain is permitted.

### B.7 Logical transaction

Authorization consumption and the authorized M15-03 freshness-state mutation form one logical security transaction.

For bootstrap this transaction couples:

- authorization replay-state advancement;
- initial freshness-state establishment.

For epoch transition this transaction couples:

- authorization replay-state advancement;
- current_epoch transition;
- previous_epoch retention;
- transition_sequence advancement;
- freshness-state generation advancement.

### B.8 Atomic authority rule

The implementation must not expose either half of the logical transaction as authoritative unless the complete transaction is durably committed and verified.

The required ordering is:

validated authorization
-> authorization anti-replay evaluation
-> authorized state-change candidate
-> transaction PREPARED
-> transaction COMMITTED
-> transaction VERIFIED
-> authorization consumed and state change authoritative

PREPARED != COMMITTED

COMMITTED != VERIFIED

AUTHORIZATION_CANDIDATE != AUTHORIZATION_CONSUMED

### B.9 Failure before verified commit

If preparation, durable commit, or verification fails, the requested bootstrap or epoch transition must not be reported as successfully authorized and applied.

The system must recover to one unambiguous authoritative generation.

A failed or interrupted transaction must not create a state where:

- the authorization is reusable while the protected state mutation became authoritative; or
- the authorization is permanently consumed while the protected state mutation did not become authoritative.

### B.10 Recovery generations

The authorization-consumption transaction requires a monotonically advancing persistent generation.

The generation identifies complete transaction-state generations, not producer logical time and not authorization_sequence.

transaction generation != authorization_sequence

transaction generation != producer logical_time

A newer generation becomes authoritative only after complete durable commit and verification.

An incomplete candidate generation is non-authoritative.

### B.11 Corruption and unavailable state

Authorization replay state corruption, truncation, structural invalidity, or unavailability fails closed.

Such conditions must not be interpreted as authorization state never established.

Authorization anti-replay history must not be silently erased.

Representative terminal results include:

AUTHORIZATION_STATE_INVALID

AUTHORIZATION_STATE_UNAVAILABLE

AUTHORIZATION_STATE_PERSIST_FAILURE

### B.12 Persistent rollback

If persistent authorization-state rollback is detectable, the result is:

AUTHORIZATION_STATE_ROLLBACK

and the operation fails closed.

Software integrity checks alone do not prove arbitrary-storage rollback resistance.

Decision B does not claim hardware-backed or externally anchored rollback resistance.

### B.13 Existing persistence precedent

The already validated M15-03 freshness persistence model provides an architectural precedent for PREPARED, COMMITTED, VERIFIED, stale-candidate rejection, previous-state preservation, and fail-closed recovery.

Decision B reuses those security properties as requirements.

Decision B does not require authorization consumption to use the existing physical host freshness backend.

The final backend may use a composite record, journal, transactional store, two-slot representation, or another explicitly validated mechanism only if it preserves the complete logical transaction.

### B.14 Concurrency and stale preparation

A prepared authorization transaction must bind the exact previously authoritative authorization replay state and freshness state.

If either authoritative state changes after prepare and before commit, the prepared transaction is stale and must fail closed.

A stale prepared transaction must not overwrite a newer authoritative transaction generation.

### B.15 Bootstrap-specific consumption

A bootstrap authorization may be consumed only as part of successful initial freshness-state establishment for a producer that satisfies the frozen never-established condition.

Bootstrap authorization must not repair, recreate, or replace lost previously established state.

### B.16 Epoch-transition-specific consumption

An epoch-transition authorization may be consumed only for its exact producer_id, from_epoch, to_epoch, and transition_sequence.

The authorization transaction must not change an unrelated producer or epoch.

Replaying a consumed transition authorization produces AUTHORIZATION_REPLAY or another stronger fail-closed rollback result.

### B.17 Resource disposition

The authorization replay-state resource bound is NOT YET FROZEN.

No unbounded authorization history is required by Decision B.

The persistent model requires bounded high-water replay state plus transaction metadata.

Exact maximum authority scopes, producers, records, and backend capacity remain for a subsequent resource decision.

### B.18 Decision B disposition

Decision B is FROZEN.

Authorization anti-replay uses strict monotonic authorization_sequence with replay window zero.

Replay state is scoped by authority identity + authorization purpose domain + producer_id.

authorization_id remains an authenticated audit identity but is not the persistent replay high-water mark.

Authorization consumption and protected freshness-state mutation form one logical verified transaction.

Interrupted or failed transactions must recover fail closed without ambiguous partial authority.

Authorization replay-state loss or corruption must not be interpreted as never established.

Hardware-backed rollback resistance is not claimed.

Decision B does not authorize implementation.

Remaining decisions include:

- exact wire/schema representation;
- exact signed authorization transcript;
- authorization trust-store representation;
- exact persistent composite-state schema;
- resource bounds;
- bootstrap lifecycle details;
- epoch-transition lifecycle details;
- host implementation design;
- STM32 implementation mapping.

Implementation authorization remains: NOT AUTHORIZED.

## Decision C - Authorization Schema Transcript and Trust Representation

Status: FROZEN

### C.1 Separate authorization trust domain

M15 authorization authorities shall use a dedicated AuthorizationTrustStoreV1.

The existing guardian-f401:m15:trust-store:v1 remains the ordinary producer assurance trust store.

The existing assurance trust store must not be extended or reinterpreted at runtime to grant bootstrap or epoch-transition authority.

ASSURANCE_TRUST_STORE != AUTHORIZATION_TRUST_STORE

producer_id != authority_id

A credential trusted for ordinary assurance signing grants no authorization capability unless that credential is independently and explicitly present in the authorization trust domain.

### C.2 Authorization trust-store schema

The authorization trust-store schema identifier is frozen as:

guardian-f401:m15:authorization-trust-store:v1

The top-level authorization trust store contains exactly:

- schema_version;
- environment;
- records.

Unknown top-level members fail closed.

The authorization trust environment must exactly match the configured verifier environment.

### C.3 Authorization trust record

Each AuthorizationTrustRecordV1 contains exactly:

- authority_id;
- key_id;
- algorithm;
- public_key;
- lifecycle_state;
- capabilities.

Unknown record members fail closed.

authority_id identifies the authorization principal and must not be interpreted as producer_id.

key_id identifies one authority credential.

algorithm for v1 is Ed25519 only.

public_key carries the authority verification key using the already approved Ed25519 public-key representation.

lifecycle_state is explicit and fail closed.

### C.4 Authorization capabilities

The only v1 authorization capabilities are:

BOOTSTRAP_AUTHORITY

EPOCH_TRANSITION_AUTHORITY

Capability matching is exact.

No wildcard capability exists.

No capability inheritance exists.

An authorization trust record may contain both capabilities only through explicit configuration.

An empty capability set is not authorization.

ASSURANCE_SIGNER is not an authorization capability in AuthorizationTrustStoreV1.

### C.5 Authorization credential lookup

Authorization credential resolution binds at minimum:

authority_id + key_id + algorithm + required capability + environment

Resolution must return exactly one eligible authorization credential.

Unknown authority, unknown key, unsupported algorithm, inactive credential, capability mismatch, duplicate credential ambiguity, or environment mismatch fails closed.

No fallback to the ordinary assurance trust store is permitted.

### C.6 Authorization object envelope

Authorization objects use a dedicated signed envelope.

SignedAuthorizationEnvelopeV1 contains exactly:

- signature_version;
- signature_algorithm;
- authority_id;
- key_id;
- authorization_object;
- signature.

Unknown envelope members fail closed.

signature_algorithm for v1 is Ed25519 only.

The signed authorization envelope is not the ordinary signed assurance wrapper.

SIGNED_AUTHORIZATION_ENVELOPE != SIGNED_ASSURANCE_WRAPPER

### C.7 Common authorization object fields

Every authorization_object contains exactly the common fields plus the fields required by its authorization_type.

Common fields are:

- schema_version;
- authorization_type;
- authorization_id;
- authority_id;
- producer_id;
- authorization_sequence.

authority_id inside the signed authorization_object must exactly match authority_id in the signed envelope.

A mismatch fails closed.

authorization_sequence is unsigned 64-bit and follows Decision B.

authorization_id is a signed authorization-domain identifier and remains distinct from authorization_sequence.

### C.8 BootstrapAuthorizationV1 schema

BootstrapAuthorizationV1 uses schema_version:

guardian-f401:m15:bootstrap-authorization:v1

authorization_type is exactly:

bootstrap

The complete authorization object contains exactly:

- schema_version;
- authorization_type;
- authorization_id;
- authority_id;
- producer_id;
- authorization_sequence;
- producer_epoch;
- initial_high_water_state;
- initial_logical_time.

initial_high_water_state is explicit.

No implicit numeric sentinel is permitted.

If initial_high_water_state represents unset high-water state, initial_logical_time must follow the explicit unset representation frozen by the M15-03 state model.

If initial_high_water_state represents established high-water state, initial_logical_time must satisfy the authoritative logical_time contract.

### C.9 EpochTransitionAuthorizationV1 schema

EpochTransitionAuthorizationV1 uses schema_version:

guardian-f401:m15:epoch-transition-authorization:v1

authorization_type is exactly:

epoch-transition

The complete authorization object contains exactly:

- schema_version;
- authorization_type;
- authorization_id;
- authority_id;
- producer_id;
- authorization_sequence;
- from_epoch;
- to_epoch;
- transition_sequence.

from_epoch and to_epoch use the authoritative producer_epoch grammar.

from_epoch must not equal to_epoch.

transition_sequence follows the frozen M15-03 transition-sequence contract.

### C.10 Authorization purpose domains

The frozen authorization purpose domains remain:

guardian-f401:m15:authorization:bootstrap:v1

guardian-f401:m15:authorization:epoch-transition:v1

The authorization_type determines exactly one purpose domain.

Unknown authorization_type fails closed.

No caller-supplied purpose domain may override the type-to-domain mapping.

### C.11 Authorization transcript

Authorization signatures use a dedicated deterministic transcript.

The transcript version identifier is frozen as:

guardian-f401:m15:authorization-transcript:v1

The transcript binds:

- authorization purpose domain;
- transcript version;
- signature algorithm;
- authority key_id;
- authority_id;
- exact canonical authorization_object bytes.

Conceptually:

purpose_domain
+ transcript_version
+ signature_algorithm
+ length-prefixed key_id
+ length-prefixed authority_id
+ length-prefixed canonical authorization_object

The exact binary fields must use deterministic length-prefixing and unambiguous encoding.

No delimiter-only concatenation is permitted.

### C.12 Canonical authorization bytes

The authorization signature authenticates the exact deterministic canonical representation produced after strict authorization-object validation.

The implementation must not sign one representation and evaluate another.

AUTHENTICATED_AUTHORIZATION_BYTES == AUTHORIZATION_BYTES_USED_FOR_DECISION

Duplicate keys, unknown keys, unsupported JSON constants, malformed UTF-8, invalid identifiers, and invalid numeric values fail before authorization authentication.

### C.13 Identifier grammars

producer_id reuses the authoritative existing producer_id grammar.

producer_epoch, from_epoch, and to_epoch reuse the authoritative existing producer_epoch grammar.

authority key_id reuses the existing validated key_id grammar unless a later Technical Destruction demonstrates incompatibility.

authority_id and authorization_id require explicit bounded identifier grammars before implementation.

Decision C does not silently equate authority_id with producer_id grammar.

Decision C does not silently equate authorization_id with object_id.

### C.14 Numeric contracts

authorization_sequence is unsigned 64-bit.

transition_sequence remains governed by the frozen M15-03 transition contract.

initial_logical_time reuses the authoritative M15 logical_time contract when present as an established value.

No numeric field may use wraparound semantics.

### C.15 Signature verification order

The required authorization verification sequence is:

strict envelope validation
-> strict authorization-object validation
-> authorization_type to purpose-domain resolution
-> authorization trust credential resolution
-> deterministic authorization transcript construction
-> Ed25519 signature verification
-> AUTHORIZATION_AUTHENTICATED

Only after AUTHORIZATION_AUTHENTICATED may Decision B anti-replay evaluation occur.

AUTHORIZATION_AUTHENTICATED != AUTHORIZATION_CANDIDATE

AUTHORIZATION_AUTHENTICATED != AUTHORIZATION_CONSUMED

### C.16 Capability requirement by type

BootstrapAuthorizationV1 requires capability:

BOOTSTRAP_AUTHORITY

EpochTransitionAuthorizationV1 requires capability:

EPOCH_TRANSITION_AUTHORITY

A cryptographically valid signature under a credential lacking the required capability fails authorization.

### C.17 Ordinary assurance trust isolation

Ordinary producer authentication continues to use the existing assurance trust store and existing signed-assurance transcript.

Authorization validation must not alter the meaning of the existing assurance trust store, transcript, signed wrapper, or AuthenticationResult.

The two trust domains may reuse common low-level parsing, canonicalization, key validation, and Ed25519 primitives only where their contracts are explicitly compatible.

### C.18 Resource limits

Authorization parsing must be bounded.

Exact maximum authorization object bytes, trust-store bytes, trust records, capability entries, identifier lengths, and parser structural limits remain for a subsequent resource decision.

No implementation may inherit the ordinary trust-store maximum of 4,096 records without explicit authorization-resource adjudication.

### C.19 Decision C disposition

Decision C is FROZEN.

Authorization uses a dedicated AuthorizationTrustStoreV1.

The existing ordinary assurance trust store is not authorization authority.

BootstrapAuthorizationV1 and EpochTransitionAuthorizationV1 have closed distinct schemas.

Authorization signatures use a dedicated purpose-separated deterministic transcript.

Authorization verification binds authority_id, key_id, algorithm, purpose domain, transcript version, and exact canonical authorization-object bytes.

Capability checks are explicit and fail closed.

Ordinary assurance authentication semantics remain unchanged.

Decision C does not authorize implementation.

Remaining decisions include:

- exact authority_id grammar;
- exact authorization_id grammar;
- exact authorization parser resource bounds;
- exact AuthorizationTrustStoreV1 resource bounds;
- exact persistent composite transaction schema;
- bootstrap lifecycle completion;
- epoch-transition lifecycle completion;
- host implementation design;
- STM32 mapping.

Implementation authorization remains: NOT AUTHORIZED.

## Decision D - Identifier Grammars Resource Bounds and Lifecycle Completion

Status: FROZEN

### D.1 authority_id grammar

authority_id is a bounded authorization-principal identifier.

The v1 grammar is:

^[A-Za-z0-9._:-]{1,128}$

authority_id is ASCII-compatible and case-sensitive.

authority_id must not contain whitespace.

authority_id != producer_id by semantic role even if both currently use compatible character classes.

No implementation may infer producer authority from identifier equality.

### D.2 authorization_id grammar

authorization_id is a 128-bit lowercase hexadecimal identifier.

The v1 grammar is:

^[0-9a-f]{32}$

authorization_id is not a sequence counter.

authorization_id != authorization_sequence

authorization_id != object_id

authorization_id != producer_epoch

### D.3 key_id grammar

Authorization authority key_id reuses the existing validated key_id grammar.

This reuse is limited to syntax.

It does not merge ordinary assurance trust with authorization trust.

### D.4 authorization object raw-size bound

Maximum raw signed authorization envelope size for v1:

16,384 bytes

Inputs larger than 16,384 bytes fail closed before authorization processing.

This limit is independent of the ordinary signed assurance-wrapper limit.

### D.5 canonical authorization object bound

Maximum canonical authorization_object size for v1:

8,192 bytes

The canonical authorization object must fit entirely within this bound.

No streaming or partial authorization-object interpretation is permitted in v1.

### D.6 authorization parser structural limits

Authorization parsing uses bounded structural limits.

The v1 maximums are:

- maximum nesting depth: 8;
- maximum total structural nodes: 256;
- maximum object members: 32;
- maximum array items: 16;
- maximum UTF-8 string bytes: 512.

These are authorization-specific limits and are not inherited implicitly from the M14 assurance parser.

### D.7 AuthorizationTrustStoreV1 raw-size bound

Maximum raw AuthorizationTrustStoreV1 size:

262,144 bytes

Inputs larger than this bound fail closed.

### D.8 AuthorizationTrustStoreV1 record bound

Maximum AuthorizationTrustRecordV1 records:

1,024 records

This is an authorization-specific software upper bound.

It does not inherit the ordinary assurance trust-store limit of 4,096 records.

It is not a claim that an STM32F401 production backend stores 1,024 authorization records.

### D.9 Capability bound

Maximum capabilities per authorization trust record:

2

The only v1 capability values are:

BOOTSTRAP_AUTHORITY

EPOCH_TRANSITION_AUTHORITY

Duplicate capability entries fail closed.

Unknown capability entries fail closed.

### D.10 Authorization replay-state bound

The portable software authorization replay-state upper bound is:

4,096 authorization replay scopes

One replay scope is identified by:

authority_id + authorization purpose domain + producer_id

Capacity exhaustion produces:

AUTHORIZATION_CAPACITY_EXCEEDED

AUTHORIZATION_CAPACITY_EXCEEDED fails closed.

No eviction of authoritative replay history is permitted.

### D.11 Authorization sequence bound

authorization_sequence is unsigned 64-bit monotonic.

Maximum value:

18446744073709551615

No wraparound is permitted.

At exhaustion:

AUTHORIZATION_SEQUENCE_EXHAUSTED

### D.12 Transaction generation bound

Authorization transaction generation is unsigned 64-bit monotonic.

No wraparound is permitted.

At exhaustion:

AUTHORIZATION_GENERATION_EXHAUSTED

Transaction generation remains separate from authorization_sequence and producer logical_time.

### D.13 Bootstrap lifecycle entry conditions

Bootstrap processing may begin only when all of the following are true:

- ordinary assurance authentication has succeeded for the producer statement;
- freshness evaluation produced FIRST_SEEN;
- there is no evidence that producer freshness state was previously established;
- BootstrapAuthorizationV1 is strictly valid;
- the authorization authority has BOOTSTRAP_AUTHORITY capability;
- authorization anti-replay evaluation produced AUTHORIZATION_CANDIDATE;
- the authorization subject exactly matches the producer and initial epoch.

Any failure terminates bootstrap fail closed.

### D.14 Bootstrap authoritative transition

The bootstrap lifecycle is:

FIRST_SEEN
-> AUTHORIZATION_AUTHENTICATED
-> AUTHORIZATION_CANDIDATE
-> bootstrap state-change candidate
-> transaction PREPARED
-> transaction COMMITTED
-> transaction VERIFIED
-> bootstrap authorization consumed
-> initial freshness state established

FIRST_SEEN alone never establishes persistent state.

AUTHORIZATION_AUTHENTICATED alone never establishes persistent state.

AUTHORIZATION_CANDIDATE alone never establishes persistent state.

### D.15 Bootstrap initial high-water semantics

BootstrapAuthorizationV1 explicitly selects one of two initial high-water modes:

HIGH_WATER_UNSET

or

HIGH_WATER_ESTABLISHED

For HIGH_WATER_UNSET:

- no initial logical-time high-water value is authoritative;
- the next authenticated statement in the established epoch is evaluated under the frozen M15-03 unset-state rule.

For HIGH_WATER_ESTABLISHED:

- initial_logical_time is mandatory;
- initial_logical_time becomes the authoritative durable high-water mark only after the verified bootstrap transaction.

No implicit sentinel is permitted.

### D.16 Bootstrap replay behavior

Once successfully consumed, the same bootstrap authorization_sequence within its replay scope cannot be reused.

A later authorization_sequence may not bootstrap a producer whose freshness state is already established.

Established-state presence takes precedence over possession of a newer bootstrap authorization.

### D.17 Epoch-transition lifecycle entry conditions

Epoch-transition processing may begin only when all of the following are true:

- ordinary assurance authentication has succeeded;
- freshness evaluation produced EPOCH_TRANSITION_REQUIRED;
- EpochTransitionAuthorizationV1 is strictly valid;
- the authority has EPOCH_TRANSITION_AUTHORITY capability;
- authorization anti-replay evaluation produced AUTHORIZATION_CANDIDATE;
- producer_id matches exactly;
- from_epoch equals the currently authoritative current_epoch;
- to_epoch equals the authenticated statement producer_epoch;
- transition_sequence is exactly the next required transition sequence.

Any mismatch fails closed.

### D.18 Epoch-transition authoritative transition

The epoch-transition lifecycle is:

EPOCH_TRANSITION_REQUIRED
-> AUTHORIZATION_AUTHENTICATED
-> AUTHORIZATION_CANDIDATE
-> exact transition candidate
-> transaction PREPARED
-> transaction COMMITTED
-> transaction VERIFIED
-> authorization consumed
-> previous current_epoch retained as previous_epoch
-> to_epoch becomes current_epoch
-> transition_sequence advances
-> freshness-state generation advances

No new epoch becomes authoritative before transaction VERIFIED.

### D.19 First statement in new epoch

Epoch transition authorization does not by itself declare the triggering assurance statement FRESH.

After the verified epoch transition, the triggering statement must still satisfy the frozen M15-03 logical-time rule for the new epoch state.

EPOCH_TRANSITION_AUTHORIZED != FRESH

The transition transaction and freshness acceptance may be composed only through an explicitly validated orchestration that preserves this distinction.

### D.20 Superseded epoch behavior

After a verified transition, the prior epoch is superseded.

An assurance statement using the retained previous_epoch produces EPOCH_ROLLBACK according to the frozen M15-03 state model.

A new authorization must not resurrect a superseded epoch as current without a separately valid forward transition from the currently authoritative epoch.

### D.21 Transition-sequence exactness

Epoch-transition authorization requires the exact next transition_sequence.

Equal, older, skipped, wrapped, or otherwise unexpected transition_sequence fails closed.

No transition-sequence window is supported.

### D.22 Composite persistent transaction state

The persistent authorization transaction must represent, as one logically atomic generation:

- authorization replay high-water state;
- authorization replay scope identity;
- authorization transaction generation;
- resulting producer freshness state;
- bootstrap or epoch-transition operation type;
- authorization_id for audit correlation;
- authorization_sequence consumed by the generation.

The exact physical encoding is backend-specific.

The logical fields are mandatory.

### D.23 Recovery rule

After restart or interruption, recovery must choose exactly one complete verified authoritative transaction generation.

Incomplete candidate generations are non-authoritative.

If no unambiguous previously established authoritative generation can be recovered where one is expected, processing fails closed.

Recovery must not reinterpret missing established transaction state as first authorization use.

### D.24 Host and STM32 boundary

Decision D freezes portable software limits and lifecycle semantics.

It does not allocate STM32F401 flash.

It does not define erase sectors, wear-leveling, endurance, or physical power-loss behavior.

It does not claim hardware-backed rollback resistance.

A production STM32 authorization backend requires separate backend evidence.

### D.25 Resource non-eviction

When any bounded authorization capacity is exhausted, the implementation fails closed.

It must not evict authoritative authorization replay history to admit a new authorization scope.

It must not silently reuse transaction generations or authorization sequences.

### D.26 Decision D disposition

Decision D is FROZEN.

authority_id grammar is bounded and explicit.

authorization_id grammar is fixed to lowercase 128-bit hexadecimal.

Authorization object and trust-store parser limits are bounded.

AuthorizationTrustStoreV1 is bounded to 1,024 records in the portable software model.

Authorization replay state is bounded to 4,096 scopes in the portable software model.

Bootstrap lifecycle is fully defined.

Epoch-transition lifecycle is fully defined.

Composite persistent transaction semantics are fully defined at the portable architecture level.

Production STM32 capacity and physical persistence remain unclaimed.

Decision D does not by itself authorize implementation.

TD-M15-006 now requires final cross-decision Technical Destruction and closure adjudication before bounded software implementation may be authorized.

Implementation authorization remains: NOT AUTHORIZED.

## TD-M15-006 Closure

Status: RESOLVED

Finding: CLOSED

Architecture disposition: APPROVED FOR BOUNDED SOFTWARE IMPLEMENTATION

### Closure basis

TD-M15-006 completed Decisions A, B, C, and D and survived final Technical Destruction without an approved contradiction.

The architecture now freezes:

- separate bootstrap and epoch-transition authorization privileges;
- dedicated signed authorization objects;
- dedicated authorization purpose domains;
- dedicated AuthorizationTrustStoreV1;
- explicit BOOTSTRAP_AUTHORITY and EPOCH_TRANSITION_AUTHORITY capabilities;
- strict authorization anti-replay with replay window zero;
- unsigned 64-bit authorization_sequence;
- authorization replay state scoped by authority identity + purpose domain + producer_id;
- deterministic purpose-separated authorization transcript;
- exact canonical signed authorization bytes;
- closed BootstrapAuthorizationV1 schema;
- closed EpochTransitionAuthorizationV1 schema;
- bounded authorization parsing;
- bounded authorization trust-store resources;
- bounded authorization replay-state resources;
- bootstrap lifecycle;
- epoch-transition lifecycle;
- composite authorization-consumption and freshness-state transaction semantics;
- fail-closed corruption, loss, rollback, exhaustion, stale preparation, and recovery behavior.

### Authorized software scope

Bounded host software implementation is authorized for:

- authorization identifier validation;
- authorization object parsing and validation;
- AuthorizationTrustStoreV1 parsing and credential resolution;
- explicit authorization capability validation;
- deterministic authorization transcript construction;
- Ed25519 authorization authentication;
- bounded authorization anti-replay state model;
- bootstrap authorization evaluation;
- epoch-transition authorization evaluation;
- backend-neutral composite persistence contract;
- host transactional authorization persistence backend;
- bootstrap orchestration;
- epoch-transition orchestration;
- integration with the existing authenticated freshness path;
- host tests and adversarial validation.

### Required semantic separations

The implementation must preserve:

ASSURANCE_AUTHENTICATED != BOOTSTRAP_AUTHORIZED

ASSURANCE_AUTHENTICATED != EPOCH_TRANSITION_AUTHORIZED

AUTHORIZATION_AUTHENTICATED != AUTHORIZATION_CANDIDATE

AUTHORIZATION_CANDIDATE != AUTHORIZATION_CONSUMED

FIRST_SEEN != BOOTSTRAP_AUTHORIZED

EPOCH_TRANSITION_REQUIRED != EPOCH_TRANSITION_AUTHORIZED

EPOCH_TRANSITION_AUTHORIZED != FRESH

FRESH != AUTHORIZED

PREPARED != COMMITTED

COMMITTED != VERIFIED

### Explicitly unauthorized or unclaimed

This closure does not authorize or establish:

- production STM32F401 authorization persistence;
- STM32F401 flash allocation;
- flash-sector or page layout;
- wear-leveling or endurance claims;
- physical target power-loss qualification;
- hardware-backed monotonic state;
- hardware-backed rollback resistance;
- arbitrary-storage rollback resistance;
- distributed witness or attestation semantics;
- M15-04 implementation;
- authorization as physical truth;
- authorization as general actuation permission.

### Resource disposition

Portable software authorization limits remain:

- signed authorization envelope: 16,384 bytes maximum;
- canonical authorization object: 8,192 bytes maximum;
- AuthorizationTrustStoreV1: 262,144 bytes maximum raw size;
- AuthorizationTrustStoreV1: 1,024 records maximum;
- authorization replay state: 4,096 scopes maximum;
- capabilities per authorization trust record: 2 maximum;
- authorization_sequence: unsigned 64-bit monotonic;
- transaction generation: unsigned 64-bit monotonic.

These are portable software bounds and are not STM32F401 production-capacity claims.

### Implementation gate

TD-M15-006 architecture condition: CLOSED

Bounded host authorization software implementation: AUTHORIZED

Bounded host authorization validation evidence: PENDING

Production STM32 authorization persistence: NOT AUTHORIZED

Hardware-backed rollback resistance: NOT DEMONSTRATED

M15-04: PENDING

### Final disposition

TD-M15-006 is RESOLVED.

The architecture is approved for bounded host software implementation only.

Implementation evidence must be produced and independently reconciled before this architecture may be represented as implemented.

Production-target claims remain separately gated.
