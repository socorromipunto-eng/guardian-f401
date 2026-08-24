# Guardian F401 — M15 Trust Store Decision

Status: APPROVED
Milestone: M15
Decision: #5 — Trust Store Representation
Decision Type: Architecture / Security / Assurance
Target: Guardian F401
Depends on:
- ADR-M15-001 — Node Identity and Signed Assurance Messages
- M15-02.1 — Signed Transcript Specification
- M15 Identity and Signing Decisions #1-#4
Implementation status: NOT IMPLEMENTED

---

## 1. Decision

Guardian shall use a closed, versioned, external trust store based on explicit public-key pinning.

The trust store resolves:

producer_id
+
key_id
+
algorithm
→
trusted public key
+
lifecycle state

The trust store does not grant physical authority.

---

## 2. Trust record

Each record shall contain exactly:

producer_id
key_id
algorithm
public_key
lifecycle_state

Conceptual structure:

{
  "producer_id": "guardian-primary",
  "key_id": "node-a-key-001",
  "algorithm": "ed25519",
  "public_key": "...",
  "lifecycle_state": "ACTIVE"
}

No private-key material is permitted.

---

## 3. Schema

Initial schema version:

guardian-f401:m15:trust-store:v1

The schema shall be closed and versioned.

Unknown members fail validation.

Silent schema extension is prohibited.

---

## 4. key_id

`key_id` remains an opaque bounded credential identifier.

Initial grammar:

^[A-Za-z0-9._:-]{1,128}$

The tuple:

producer_id + key_id

must be unique.

Ambiguous mappings fail closed.

---

## 5. Algorithm

Initial supported algorithm:

ed25519

Algorithm selection is explicit.

It shall not be inferred from:

- key length;
- key_id;
- producer_id;
- signature size;
- configuration defaults.

Unsupported algorithms fail closed.

---

## 6. Public key

For Ed25519:

raw public key length = exactly 32 bytes

External encoding:

base64url without padding

Reject:

- invalid alphabet;
- "=" padding;
- whitespace;
- malformed encoding;
- non-canonical encoding;
- decoded length other than 32 bytes.

The message shall never define its own trusted public key.

---

## 7. Lifecycle states

Initial states:

ACTIVE
RETIRED
REVOKED

ACTIVE:

May authenticate new signed assurance objects.

RETIRED:

Not valid for new signing, but may remain available for historical evidence evaluation.

REVOKED:

Must fail current authentication.

`UNKNOWN` is a verification result, not a required stored state.

---

## 8. Lookup

Verification lookup is exact and deterministic:

producer_id
+
key_id
+
algorithm
→
exactly one trust record

or explicit failure.

No wildcard matching is permitted.

No fallback key is permitted.

No default producer is permitted.

---

## 9. Fail-closed behavior

Verification fails closed when:

- producer_id is unknown;
- key_id is unknown;
- algorithm is unsupported;
- mapping is ambiguous;
- public key is malformed;
- key length is invalid;
- lifecycle state is invalid;
- key is REVOKED;
- trust-store schema is invalid;
- trust-store loading fails.

---

## 10. Message-controlled trust prohibition

Forbidden:

message
contains public_key
→
verifier trusts that key

Required:

message identity metadata
→
external trust-store lookup
→
preconfigured trusted public key
→
signature verification

---

## 11. Test and production separation

TEST TRUST STORE
≠
PRODUCTION TRUST STORE

Test credentials must not silently become production trust anchors.

Production configuration must be distinguishable from test configuration.

---

## 12. File representation

Initial implementation target:

UTF-8 JSON without BOM

Requirements:

- strict parsing;
- duplicate-member rejection;
- closed schema;
- deterministic validation;
- canonical representation.

Preferred canonicalization:

RFC 8785

Canonicalization supports reproducibility and hashing.

It does not itself establish trust.

---

## 13. Integrity evidence

At minimum, evidence shall record:

- raw file SHA-256;
- canonical SHA-256;
- schema version;
- record count;
- validation result;
- commit SHA.

A signed trust-store distribution mechanism is deferred.

---

## 14. Duplicate handling

The following are invalid:

- duplicate producer_id + key_id mapping;
- duplicate JSON members;
- duplicate credential records;
- conflicting lifecycle states;
- same producer/key tuple with different public keys;
- same producer/key tuple with different algorithms.

Validation fails closed.

---

## 15. Cross-purpose separation

Firmware-signing trust and assurance-signing trust are separate domains.

firmware signing identity
≠
assurance signing identity

Preferred rule:

different purpose
→
different key

Cross-purpose reuse requires explicit review.

---

## 16. Authority invariant

An ACTIVE trust record means only:

the credential is trusted to authenticate statements for that producer and purpose.

It does not mean:

- producer may actuate;
- producer may override policy;
- producer is physically correct;
- producer is fresh;
- producer is uncompromised.

Therefore:

ACTIVE credential
≠
physical authority

---

## 17. Security invariants

TS-01  Trust is resolved externally from attacker-controlled messages.

TS-02  Messages cannot introduce trusted public keys.

TS-03  producer_id + key_id mapping is deterministic.

TS-04  Ambiguous mappings fail closed.

TS-05  Unknown producers fail closed.

TS-06  Unknown keys fail closed.

TS-07  Unsupported algorithms fail closed.

TS-08  Revoked credentials fail current authentication.

TS-09  Test and production trust stores remain distinct.

TS-10  No private-key material exists in the trust store.

TS-11  Ed25519 public keys decode to exactly 32 bytes.

TS-12  Public-key encoding is canonical base64url without padding.

TS-13  Duplicate JSON members are rejected.

TS-14  Trust-store schema is closed and versioned.

TS-15  ACTIVE credential status does not imply authority.

TS-16  Firmware and assurance trust domains remain separated.

TS-17  Trust-store changes are hash-verifiable and traceable.

---

## 18. Threats addressed

This architecture addresses:

- public-key injection;
- attacker-controlled key substitution;
- unknown producer spoofing;
- ambiguous identity mapping;
- key-id substitution;
- algorithm substitution;
- revoked-key acceptance;
- silent schema drift;
- test-key leakage into production configuration;
- accidental cross-purpose key reuse;
- duplicate trust records;
- malformed public-key encoding.

---

## 19. Not solved by this decision

This decision does not solve:

- compromise of a legitimate private key;
- compromised trusted node producing false statements;
- persistent replay;
- freshness across reset;
- quorum;
- partition handling;
- split-brain;
- failover;
- secure production provisioning;
- hardware-backed key custody;
- physical truth.

---

## 20. Decision summary

Guardian M15 shall use a closed, versioned external trust store based on explicit public-key pinning.

Core resolution:

producer_id
+
key_id
+
algorithm
→
trusted public key
+
lifecycle state

The message does not define its own trust.

The trust store authenticates producer provenance only.

It does not establish freshness, physical truth, or authority.

Status: APPROVED — architecture accepted; implementation remains pending.