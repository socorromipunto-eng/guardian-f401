# Guardian F401 — TD-M15-003 Trust Store Document Schema

Status: RESOLVED
Milestone: M15
Review Type: Technical Destruction / Architecture Reconciliation
Implementation status at discovery: NOT IMPLEMENTED

---

## 1. Finding

Decision #5 defines the trusted credential record but does not freeze the exact top-level JSON document containing those records.

Implementation must not become the source of truth for that structure.

---

## 2. Decision

Guardian M15 freezes one closed top-level JSON object containing exactly:

- schema_version
- environment
- records

Exact initial schema version:

guardian-f401:m15:trust-store:v1

Allowed environment values:

- TEST
- PRODUCTION

The records member is a JSON array containing zero or more TrustRecord objects.

Unknown top-level members fail validation.
Duplicate JSON members fail validation.
Silent schema extension is prohibited.

---

## 3. Environment isolation

The configured verifier environment must exactly equal the loaded trust-store environment.

TEST trust store ≠ PRODUCTION trust store.

A TEST trust store must not be accepted by a PRODUCTION verifier.
A PRODUCTION trust store must not be silently accepted by a TEST verifier.

The signed assurance message must never select or override the trust-store environment.

---

## 4. TrustRecord

Each TrustRecord contains exactly:

- producer_id
- key_id
- algorithm
- public_key
- lifecycle_state

Private-key material is prohibited.
Unknown record members fail validation.
Duplicate record members fail validation.

---

## 5. Public key

Initial algorithm: ed25519

Ed25519 raw public key length = exactly 32 bytes.
External encoding = base64url without padding.

Invalid alphabet, padding, whitespace, malformed encoding, non-canonical encoding, or decoded length other than 32 bytes fail closed.

The signed message never supplies its own trusted public key.

---

## 6. Lifecycle

ACTIVE: may authenticate new signed assurance objects.

RETIRED: not valid for new signing, but may remain available for historical evidence evaluation.

REVOKED: must fail current authentication.

UNKNOWN is a verification result, not a stored lifecycle state.

---

## 7. Lookup

Exact deterministic lookup:

producer_id + key_id + algorithm → exactly one TrustRecord or explicit failure.

No wildcard matching is permitted.
No fallback key is permitted.
No default producer is permitted.

---

## 8. Duplicate and ambiguity handling

The following invalidate the trust store:

- duplicate JSON members
- duplicate credential records
- duplicate producer_id + key_id mappings
- conflicting lifecycle states
- same producer/key tuple with different public keys
- same producer/key tuple with different algorithms
- ambiguous mappings

First-match-wins is prohibited.
Last-match-wins is prohibited.
Validation fails closed.

---

## 9. File representation

UTF-8 JSON without BOM.
Strict JSON parsing.
Closed schema.
Deterministic validation.
RFC 8785 canonical representation.

Canonicalization supports reproducibility and hashing.
Canonicalization does not itself establish trust.

---

## 10. Cryptographic boundary

Trust resolution and Ed25519 verification remain separate operations.

VERIFIED ≠ AUTHENTICATED until trusted credential resolution has also succeeded.

ACTIVE credential ≠ physical authority.

Trust acceptance does not establish freshness, anti-replay, physical truth, actuator authority, policy authorization, or certification.

---

## 11. Integrity evidence

At minimum evidence shall include:

- raw file SHA-256
- canonical SHA-256
- schema version
- environment
- record count
- validation result
- repository commit SHA when applicable

---

## 12. Disposition

Finding: RESOLVED
Top-level trust-store schema: FROZEN
Schema version: guardian-f401:m15:trust-store:v1
Environment values: TEST, PRODUCTION
Records container: JSON array
TrustRecord schema: CLOSED
Lookup: producer_id + key_id + algorithm
Implementation acceptance: PENDING

Status: RESOLVED — trust-store runtime must conform to this document schema.
