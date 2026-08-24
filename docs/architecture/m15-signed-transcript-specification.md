# Guardian F401 — M15-02.1 Signed Transcript Specification

Status: APPROVED
Milestone: M15
Decision Type: Architecture / Security / Assurance
Target: Guardian F401
Depends on: ADR-M15-001 — Node Identity and Signed Assurance Messages
Implementation status: NOT IMPLEMENTED

---

## 1. Purpose

This document defines the exact deterministic transcript that Guardian assurance messages will authenticate.

The objective is to make signature verification reproducible across implementations and prevent ambiguity about:

- which fields are authenticated;
- how domain separation is applied;
- how identity metadata is bound;
- how object type is bound;
- how key identity is represented;
- how cross-protocol and cross-object signature reuse is prevented.

This specification does not define private-key storage, production provisioning, persistent freshness, quorum, failover, or physical authority.

---

## 2. Architectural invariant

Guardian preserves:

VALIDATE
   ↓
CANONICALIZE
   ↓
BUILD SIGNING TRANSCRIPT
   ↓
SIGN / VERIFY

No signature is evaluated as an assurance statement until the underlying object has passed the M14 validation boundary.

A signature never overrides schema rejection.

---

## 3. Source object

The source assurance envelope remains the accepted M14 object:

domain
object_type
producer_id
producer_epoch
object_id
logical_time
payload

The canonical representation of this object is produced using RFC 8785 after successful validation.

The canonical object is immutable input to transcript construction.

---

## 4. Signature metadata

M15 introduces authentication metadata conceptually consisting of:

signature_version
signature_algorithm
key_id
signature

These fields are not permitted to silently extend the existing M14 closed envelope.

They must be represented either:

1. in a separately versioned signed-message wrapper; or
2. in a new explicit envelope schema version.

The implementation decision between those two representations must be frozen before code changes.

---

## 5. Recommended signed-message wrapper

The preferred architecture is a versioned outer wrapper:

{
  "signature_version": "...",
  "signature_algorithm": "...",
  "key_id": "...",
  "assurance_object": { ... },
  "signature": "..."
}

The `assurance_object` must itself independently satisfy the existing M14 contract.

The signature field is excluded from the signed transcript.

---

## 6. Signature version

Initial signature transcript version:

guardian-f401:m15:signed-assurance:v1

This identifier is part of the transcript and shall not be inferred.

A future transcript change requires a new version.

---

## 7. Signature algorithm identifier

The target production identifier is:

ed25519

The identifier shall be encoded explicitly.

The verifier shall not infer Ed25519 from:

- signature size;
- key size;
- key format;
- producer identity;
- configuration defaults.

Unsupported algorithms fail closed.

---

## 8. key_id

`key_id` identifies the verification credential.

It must:

- be explicit;
- be bounded;
- contain no secret material;
- remain stable for the lifetime of that credential;
- permit replacement and revocation.

Recommended initial grammar:

^[A-Za-z0-9._:-]{1,128}$

The message does not carry a trusted public key.

The verifier resolves `producer_id + key_id` against an external trust store.

---

## 9. Exact transcript structure

The M15 v1 signing transcript shall be constructed from four components:

DOMAIN_SEPARATOR
LENGTH_PREFIXED_KEY_ID
LENGTH_PREFIXED_PRODUCER_ID
CANONICAL_ASSURANCE_OBJECT

The signature itself is not included.

Conceptually:

TRANSCRIPT =
    domain_separator
    || key_id
    || producer_id
    || canonical_assurance_object

Concatenation shall not rely on delimiter characters alone.

All variable-length elements must be length-delimited.

---

## 10. Domain separator

The exact ASCII domain separator for v1 is:

GUARDIAN-F401:M15:SIGNED-ASSURANCE:V1

Encoding:

ASCII / UTF-8 identical bytes

No terminating NUL byte is included.

The separator prevents reuse of the same signature transcript in other Guardian protocols.

---

## 11. Length encoding

Variable-length fields shall use an explicit unsigned big-endian length prefix.

Initial v1 rule:

uint16_be length
followed by exactly length bytes

This applies to:

key_id
producer_id

The canonical assurance object uses:

uint32_be length
followed by exactly length bytes

Current M14 resource limits remain authoritative and may impose much smaller accepted sizes.

---

## 12. Exact byte sequence

Version 1 transcript:

+------------------------------------------+
| ASCII "GUARDIAN-F401:M15:SIGNED-..."     |
+------------------------------------------+
| key_id length            uint16_be       |
+------------------------------------------+
| key_id bytes             UTF-8           |
+------------------------------------------+
| producer_id length       uint16_be       |
+------------------------------------------+
| producer_id bytes        UTF-8           |
+------------------------------------------+
| canonical length         uint32_be       |
+------------------------------------------+
| RFC8785 canonical object bytes           |
+------------------------------------------+

No whitespace is inserted between fields.

No trailing newline is included.

No NUL terminator is included.

---

## 13. producer_id binding

`producer_id` already exists inside the canonical assurance object.

It is intentionally also bound explicitly in the transcript header.

This duplication is deliberate.

The verifier must require:

transcript producer_id
==
assurance_object.producer_id

A mismatch fails closed.

---

## 14. key_id is not producer_id

Guardian preserves:

producer_id = logical producer identity
key_id      = specific credential identity

One producer may rotate through multiple keys.

A single key must not be silently accepted for multiple producers unless explicitly authorized in the trust store.

---

## 15. Canonical object construction

The canonical object is produced exactly by:

parse_and_validate_envelope(raw)
        ↓
RFC 8785 canonicalization

The signing implementation must not independently serialize the object using a different JSON library or formatting policy.

The signature transcript must consume the same canonical bytes that the assurance validation layer defines.

---

## 16. Fields authenticated

Because the complete canonical assurance object is included, the signature authenticates:

domain
object_type
producer_id
producer_epoch
object_id
logical_time
payload

Any byte-level semantic mutation to those accepted fields must invalidate the signature.

---

## 17. Fields not authenticated

The signature itself is excluded.

Transport metadata outside the signed wrapper is not authenticated unless separately defined.

Examples of data not implicitly authenticated:

TCP source address
UART channel
arrival timestamp
file name
database row id
UI labels
network routing metadata

No implementation may infer assurance properties from those external values without a separate contract.

---

## 18. Cross-object protection

Because `object_type` and the M14 domain are inside the canonical object, a signature over an `Observation` cannot be reinterpreted as a valid `Decision` without changing authenticated bytes.

Verification must never strip or rewrite object type before signature evaluation.

---

## 19. Cross-protocol protection

The domain separator:

GUARDIAN-F401:M15:SIGNED-ASSURANCE:V1

is specific to signed assurance messages.

The same private key may not automatically be reused for:

firmware signing
TLS
software-package signing
attestation
administrative login
command authorization

unless a separately reviewed key-usage policy explicitly permits it.

Preferred architecture:

different purpose
→ different key

where operationally feasible.

---

## 20. Signature size

For Ed25519:

signature length = exactly 64 bytes

Any other size is invalid.

The wire encoding of the signature must be explicit.

Recommended initial external representation:

lowercase hexadecimal

or:

base64url without padding

The final encoding must be selected before implementation and frozen in the signed-message schema.

The cryptographic signature itself remains raw 64-byte Ed25519 output.

---

## 21. Public key size

For Ed25519:

public key = exactly 32 bytes

The public key is not trusted merely because it appears in a message.

Trusted public keys reside in the verifier's configured trust store.

---

## 22. Trust-store lookup

Verification resolves:

producer_id
+
key_id
+
signature_algorithm

to a trusted verification record.

Conceptually:

TrustRecord {
    producer_id
    key_id
    algorithm
    public_key
    lifecycle_state
}

Accepted lifecycle states may include:

ACTIVE
RETIRED
REVOKED

`UNKNOWN` is an evaluation result, not necessarily stored state.

---

## 23. Verification algorithm

The receiver shall perform:

1. Apply raw wrapper resource limits.
2. Parse wrapper strictly.
3. Reject duplicate members.
4. Validate wrapper schema.
5. Extract assurance_object.
6. Validate assurance_object under M14 rules.
7. Canonicalize assurance_object.
8. Confirm wrapper producer identity semantics.
9. Resolve producer_id + key_id.
10. Reject unknown/revoked credential.
11. Build exact M15 v1 transcript.
12. Verify Ed25519 signature.
13. Return AUTHENTICATED or explicit failure.

Freshness evaluation is not performed by this specification.

---

## 24. Signing algorithm

The sender shall perform:

1. Construct assurance object.
2. Validate locally.
3. Canonicalize using the approved RFC 8785 implementation.
4. Select configured producer identity.
5. Select approved active signing key.
6. Build exact M15 v1 transcript.
7. Request signature through signing provider.
8. Construct outer signed-message wrapper.

The assurance layer never reads private-key bytes directly unless operating under an explicitly designated test provider.

---

## 25. Signing provider contract

Conceptual interface:

sign(
    signature_version,
    algorithm,
    producer_id,
    key_id,
    transcript
) -> signature

The provider must reject:

- unknown key;
- inactive key;
- unsupported algorithm;
- invalid transcript contract;
- unavailable backend.

Private-key extraction is not part of the interface.

---

## 26. Verification provider contract

Conceptual interface:

verify(
    signature_version,
    algorithm,
    producer_id,
    key_id,
    transcript,
    signature
) -> verification_result

The provider returns explicit status.

It must not return a simple boolean if doing so would collapse distinct failures that Guardian needs for evidence or diagnostics.

---

## 27. Recommended verification states

At minimum:

AUTHENTICATED
IDENTITY_UNKNOWN
KEY_UNKNOWN
KEY_REVOKED
ALGORITHM_UNSUPPORTED
SIGNATURE_INVALID
CRYPTO_PROVIDER_FAILURE
TRANSCRIPT_VERSION_UNSUPPORTED

These states are separate from M14 schema validation errors.

---

## 28. Error-boundary rule

Guardian must preserve:

malformed object
≠
unknown identity
≠
bad signature
≠
provider failure

A cryptographic-provider outage is an environment fault.

It must not be reclassified as invalid user input.

---

## 29. Replay boundary

A valid M15 signature proves only that the signed transcript was authenticated by the corresponding credential.

It does not prove that the object is new.

Therefore:

AUTHENTICATED
≠
FRESH

Persistent replay protection belongs to M15-03.

---

## 30. Truth boundary

A valid signature does not prove that a sensor reading is physically correct.

Therefore:

AUTHENTICATED
≠
TRUE

A compromised node may sign false statements with a legitimate credential.

Independent witnesses and heterogeneous corroboration are future controls.

---

## 31. Authority boundary

A valid signature never directly authorizes actuation.

Forbidden implication:

signature_valid
→
actuator_command

Required conceptual chain:

authenticated statement
        ↓
freshness
        ↓
corroboration / evidence
        ↓
deterministic policy
        ↓
explicit authority decision
        ↓
physical gate

---

## 32. Test identity profile

CI and host tests may use software-based Ed25519 keys.

Test keys must:

- be explicitly marked non-production;
- never be installed as production trust anchors;
- never be claimed as non-extractable;
- remain reproducible where test vectors require it.

---

## 33. Production identity profile

Production signing requires a separately approved backend.

This specification deliberately does not choose among:

- secure element;
- external signer;
- protected provisioning;
- future MCU security capability;
- another reviewed mechanism.

Until validated:

Production Node Identity = ARCHITECTURE DEFINED / BACKEND PENDING

---

## 34. Required positive tests

Implementation must demonstrate:

1. Same accepted assurance object → same canonical bytes.
2. Same metadata + same canonical bytes → same transcript.
3. Valid Ed25519 signature verifies.
4. Active trusted key resolves correctly.
5. Observation, Decision and Witness all support the transcript contract.

---

## 35. Required negative tests

Implementation must reject:

- changed producer_id;
- changed producer_epoch;
- changed object_id;
- changed logical_time;
- changed domain;
- changed object_type;
- changed payload;
- changed evidence digest;
- changed key_id;
- unknown key;
- revoked key;
- unsupported algorithm;
- truncated signature;
- oversized signature;
- malformed signature encoding;
- wrong public key;
- wrong producer mapping;
- cross-object reuse;
- cross-protocol reuse;
- wrapper field injection;
- duplicate wrapper fields;
- malformed canonical object.

---

## 36. Reproducibility requirement

For a frozen test vector, the repository shall preserve:

input object
canonical object bytes
transcript bytes
transcript SHA-256
public key
signature
expected verification result

The private key may be included only for explicitly marked public test vectors.

Production private keys are never evidence artifacts.

---

## 37. Evidence artifact

M15 implementation shall produce a machine-readable evidence record containing at least:

transcript_version
algorithm
producer_id
key_id
object_id
canonical_sha256
transcript_sha256
signature_validation_result
test_vector_id
tool/runtime version
commit SHA

No private-key material is permitted.

---

## 38. Change control

The following require a new transcript version or ADR review:

- different field ordering rule;
- different domain separator;
- different canonicalization algorithm;
- different length encoding;
- new authenticated wrapper field;
- removal of authenticated metadata;
- algorithm migration;
- trust-store semantic change.

No silent transcript evolution is allowed.

---

## 39. Security invariants

ST-01  Transcript construction occurs only after successful M14 validation.

ST-02  RFC 8785 canonical bytes are the authenticated object representation.

ST-03  Signature metadata is versioned.

ST-04  producer_id is cryptographically bound.

ST-05  key_id is cryptographically bound.

ST-06  object_type is cryptographically bound.

ST-07  producer_epoch is cryptographically bound.

ST-08  logical_time is cryptographically bound.

ST-09  payload is cryptographically bound.

ST-10  signature bytes are excluded from their own transcript.

ST-11  trusted public keys cannot be introduced by the message.

ST-12  unsupported algorithms fail closed.

ST-13  unknown or revoked keys fail closed.

ST-14  transcript version changes require explicit versioning.

ST-15  signature validity does not imply freshness.

ST-16  signature validity does not imply physical truth.

ST-17  signature validity does not imply authority.

---

## 40. Open decisions before implementation

The following must be adjudicated before code begins:

1. Outer wrapper vs new envelope schema version.
2. Final `key_id` encoding.
3. Signature external encoding: hex vs base64url.
4. Host Ed25519 library for test implementation.
5. Trust-store representation.
6. Exact provider error API.
7. Whether different assurance object types use distinct signing keys or only distinct domain separation.

---

## 41. Acceptance gate

M15-02.1 is architecture-complete when:

- the byte-level transcript is frozen;
- all variable-length fields are unambiguous;
- domain separation is explicit;
- key and producer binding are explicit;
- positive and negative vector requirements are documented;
- no production key-storage claim is introduced;
- freshness remains deferred to M15-03;
- authority remains downstream and unchanged.

---

## 42. Decision summary

Guardian M15 signed assurance messages will authenticate the exact accepted canonical assurance object together with explicit producer and credential identity under a versioned Guardian-specific domain-separated transcript.

The architecture preserves:

VALIDATED
   ↓
CANONICAL
   ↓
AUTHENTICATED

without collapsing:

AUTHENTICATED
≠
FRESH
≠
TRUE
≠
AUTHORIZED

Status: APPROVED — architecture accepted; implementation remains pending.
