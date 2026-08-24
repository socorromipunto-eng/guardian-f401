# Guardian F401 — M15 Signed Identity Implementation Validation

Status: PASS
Milestone: M15
Scope: Signed Identity and Authentication Software Slice
Validation type: Software-only implementation evidence

---

## 1. Scope

This validation records implementation evidence for the bounded M15 signed-identity and authentication slice.

Validated implementation chain:

validated signed wrapper
→ purpose-separated transcript
→ trusted credential resolution
→ Ed25519 verification
→ AUTHENTICATED

This validation does not claim persistent freshness, anti-replay across reset, attestation, witness exchange, physical truth, authority, actuation permission, production hardware identity, or certification.

---

## 2. Implementation commits

963337f — feat(m15): add purpose-separated assurance transcript builder
be7d39b — feat(m15): add closed signed assurance wrapper
6c59e2a — build(m15): lock host Ed25519 dependencies
1967413 — feat(m15): add host Ed25519 crypto provider
1a68be3 — docs(m15): freeze trust store document schema
6c29512 — docs(m15): freeze trust store resource bounds
99bdd6e — refactor(m15): expose producer identity validator
e2dd227 — feat(m15): add trust store credential resolution
1b18886 — feat(m15): add signed assurance authentication orchestrator

---

## 3. Architecture findings closed

TD-M15-001 — Purpose-Domain Representation — CLOSED
TD-M15-002 — Signed Wrapper Raw Resource Bound — CLOSED
TD-M15-003 — Trust Store Document Schema — CLOSED
TD-M15-004 — Trust Store Resource Bounds — CLOSED

---

## 4. Authentication invariants demonstrated

Trusted credential resolution precedes cryptographic verification.

ACTIVE trusted credential + valid Ed25519 signature may produce AUTHENTICATED.

REVOKED credential + mathematically valid signature produces KEY_REVOKED.

RETIRED credential used for new authentication produces KEY_RETIRED_FOR_NEW_USE.

Unknown producer produces IDENTITY_UNKNOWN.

Unknown key produces KEY_UNKNOWN.

Wrong trusted public key produces SIGNATURE_INVALID.

Malformed signature encoding is distinct from SIGNATURE_INVALID.

Unsupported algorithm fails closed.

Unsupported transcript version fails closed.

Machine-readable result state is authoritative over diagnostic text.

---

## 5. Trust-store invariants demonstrated

Trust-store schema version:
guardian-f401:m15:trust-store:v1

Environment separation:
TEST ≠ PRODUCTION

Raw trust-store maximum:
1,048,576 bytes

Maximum TrustRecord count:
4,096

Ed25519 trusted public key:
canonical base64url without padding
decoded length exactly 32 bytes

Lookup:
producer_id + key_id + algorithm → exactly one TrustRecord or explicit failure

No wildcard producer matching.
No fallback key.
No default producer.
No first-match-wins ambiguity.
No last-match-wins ambiguity.

---

## 6. Identity contract

producer_id uses the authoritative M14 identity grammar through the shared validate_producer_id validator.

key_id uses the approved M15 validate_key_id contract.

The trust-store runtime does not maintain a second independent producer identity grammar.

---

## 7. Cryptographic boundary

VERIFIED means the signature matches the supplied public key and transcript.

RESOLVED means the credential is trusted for the applicable trust environment and lifecycle context.

AUTHENTICATED requires successful trusted credential resolution plus successful cryptographic verification.

Therefore:

RESOLVED ≠ VERIFIED
VERIFIED ≠ AUTHENTICATED
AUTHENTICATED ≠ FRESH
AUTHENTICATED ≠ PHYSICALLY TRUE
AUTHENTICATED ≠ AUTHORIZED
AUTHENTICATED ≠ ACTUATION PERMISSION

---

## 8. Dependency evidence

Python validation environment:
CPython 3.12

Locked dependencies:
rfc8785==0.1.4
cryptography==50.0.0
cffi==2.1.1
pycparser==3.0

Dependency installation was validated using pip --require-hashes.

Windows and GitHub-hosted Ubuntu artifact hashes were adjudicated before the dependency lock was committed.

---

## 9. Regression evidence

Final assurance regression at HEAD 1b18886:

203 tests executed
203 tests passed
0 failures
0 errors

Result: PASS

---

## 10. Deferred properties

The following remain outside this completed slice:

M15-03 — Persistent Freshness and Anti-Replay
M15-04 — Attestation and Witness Exchange

No persistent replay resistance across reset is claimed.
No distributed witness assurance is claimed.
No attestation mechanism is claimed.

---

## 11. Disposition

M15 signed identity and authentication software slice: IMPLEMENTED AND VALIDATED

Persistent freshness / anti-replay: PENDING M15-03

Attestation / witness exchange: PENDING M15-04

Overall M15 Architecture Gate remains PASS WITH CONDITIONS until its explicitly deferred requirements are completed or separately adjudicated.
