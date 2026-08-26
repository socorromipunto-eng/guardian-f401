# Guardian F401 — M15 Identity and Signing Decisions

Status: APPROVED / PARTIALLY ADJUDICATED
Milestone: M15
Decision Type: Architecture / Security / Assurance
Target: Guardian F401
Depends on:
- ADR-M15-001 — Node Identity and Signed Assurance Messages
- M15-02.1 — Signed Transcript Specification
Implementation status: NOT IMPLEMENTED

---

## 1. Purpose

This document records the M15 identity and signing design decisions adjudicated before implementation.

The objective is to ensure that implementation does not begin while key architectural choices remain implicit.

This document records decisions only.

It does not claim that the corresponding implementation already exists.

---

## 2. Decision #1 — Signed-message representation

Status: APPROVED

Decision:

Use a versioned outer signed-message wrapper.

The existing M14 assurance envelope remains closed and semantically unchanged.

Authentication metadata shall not be silently inserted into the existing M14 contract.

Conceptual structure:

{
  "signature_version": "guardian-f401:m15:signed-assurance:v1",
  "signature_algorithm": "ed25519",
  "key_id": "node-a-key-001",
  "assurance_object": {
    "domain": "...",
    "object_type": "...",
    "producer_id": "...",
    "producer_epoch": "...",
    "object_id": "...",
    "logical_time": 1,
    "payload": {}
  },
  "signature": "..."
}

The assurance object must independently satisfy the existing M14 validation boundary.

Architectural rule:

wrapper validity
≠
assurance-object validity
≠
signature validity
≠
authority

Rationale:

- Preserves the existing M14 closed contract.
- Separates assurance semantics from authentication metadata.
- Avoids silent schema drift.
- Allows future authentication evolution without redefining Observation, Decision, or Witness semantics.

---

## 3. Decision #2 — key_id representation

Status: APPROVED

Decision:

`key_id` shall be an opaque, bounded, ASCII-compatible credential identifier.

Initial grammar:

^[A-Za-z0-9._:-]{1,128}$

Properties:

- maximum length: 128 characters;
- no private key material;
- no embedded secret material;
- no requirement to use UUID;
- no implicit cryptographic meaning;
- stable for the lifecycle of the associated credential.

Semantic distinction:

producer_id = logical producer identity
key_id      = specific cryptographic credential identity

A producer may use multiple key IDs over time for:

- rotation;
- replacement;
- retirement;
- revocation;
- algorithm migration;
- forensic reconstruction.

The trust store resolves:

producer_id
+
key_id
+
algorithm
→
trusted verification credential
+
lifecycle state

Architectural rule:

key_id
≠
producer_id
≠
public key
≠
private key
≠
authority

---

## 4. Decision #3 — external signature encoding

Status: APPROVED

Decision:

Ed25519 signatures shall use:

base64url without padding

for the external signed-message representation.

Cryptographic object:

raw Ed25519 signature = exactly 64 bytes

External representation:

RFC 4648 base64url alphabet

Allowed alphabet:

A-Z
a-z
0-9
-
_

Padding:

prohibited

The verifier must reject:

- "=" padding;
- invalid alphabet;
- non-canonical encoding;
- whitespace;
- malformed encoding;
- decoded length other than exactly 64 bytes.

Architectural separation:

cryptographic signature = raw 64 bytes
wire representation     = base64url without padding

Rationale:

- More compact than hexadecimal.
- JSON-safe.
- URL-safe.
- Avoids "+" and "/" from classic Base64.
- Preserves deterministic decoding requirements.

---

## 5. Decision #4 — Host/Test Ed25519 backend

Status: APPROVED FOR HOST/TEST ONLY

Decision:

Guardian shall use the Python `cryptography` package as the reference Ed25519 backend for host/test implementation.

This dependency shall remain behind the Guardian signing and verification provider interfaces.

Conceptual architecture:

Guardian Assurance
        ↓
Signing / Verification Provider Interface
        ↓
Host/Test Ed25519 Backend
        ↓
Python cryptography package

The assurance layer shall not directly couple its semantic contracts to a specific cryptographic library.

Approved scope:

- CI;
- host tests;
- deterministic test vectors;
- interoperability tests;
- software-only validation;
- reference Ed25519 signing and verification.

Not approved by this decision:

- STM32 production signer;
- embedded production private-key storage;
- non-extractable per-device identity;
- hardware secure element;
- production provisioning backend.

Host/Test baseline:

Algorithm: Ed25519
Python baseline: 3.12
Provider abstraction: mandatory

Dependency-management requirements:

- exact dependency version must be controlled before implementation;
- dependency must be recorded in the applicable lockfile;
- dependency version and license must be included in future SBOM/evidence;
- upgrades require explicit review;
- production firmware shall not depend on Python `cryptography`.

Architectural rule:

host/test Ed25519 implementation
≠
production embedded identity

Completion of host tests shall not permit a claim of hardware-backed or non-extractable production identity.

---

## 6. Existing Guardian crypto reuse

Guardian already contains portable SHA-256 and HMAC-SHA-256 functionality in the existing firmware security layer.

Those mechanisms remain valid for their existing scoped purposes.

M15 shall not duplicate those primitives without explicit architectural justification.

However:

HMAC shall not silently become the federated production node-identity mechanism.

Existing M12 `DEMO_HMAC_SHA256` remains a demonstration mechanism only.

Architectural rule:

HMAC validity
≠
federated asymmetric node identity

---

## 7. Existing Guardian Ed25519 direction

M12 already defines Ed25519 as the intended production firmware-signature algorithm.

M15 reuses that algorithmic direction for signed assurance architecture, while keeping assurance signing and firmware signing as distinct purposes.

The following remains mandatory:

different cryptographic purpose
→
explicit domain separation
→
preferably distinct keys where operationally feasible

A firmware-signing key shall not automatically become an assurance-signing key.

A future exception requires explicit review.

---

## 8. Security invariants preserved

The following remain mandatory:

D-01  The M14 assurance envelope remains independently validatable.

D-02  Authentication metadata does not silently alter M14 schema semantics.

D-03  producer_id is a logical identity, not a credential.

D-04  key_id identifies a credential but carries no secret.

D-05  Messages cannot introduce their own trusted public key.

D-06  Signature encoding is canonical and explicitly defined.

D-07  Invalid signature encoding fails closed.

D-08  Host/test keys cannot silently become production identities.

D-09  Python cryptography is a host/test backend, not a firmware dependency.

D-10  Ed25519 signature validity does not imply freshness.

D-11  Ed25519 signature validity does not imply physical truth.

D-12  Ed25519 signature validity does not imply authority.

D-13  Firmware-signing and assurance-signing purposes remain separated.

D-14  Dependency upgrades require explicit review and evidence update.

D-15  Private-key custody remains behind a provider boundary.

---

## 9. Decisions still pending

The following decisions remain open and must be adjudicated before implementation begins.

### Decision #5 — Trust-store representation

Pending questions include:

- file/schema representation;
- producer-to-key mapping;
- lifecycle-state representation;
- revocation behavior;
- test versus production trust stores;
- duplicate-key detection;
- deterministic serialization;
- whether trust-store evidence is signed or hash-pinned.

Status: PENDING

### Decision #6 — Provider error API

Pending questions include:

- exact result enum;
- distinction between identity failure and provider failure;
- distinction between unsupported algorithm and invalid signature;
- logging/evidence behavior;
- fail-closed semantics;
- exception versus structured result model.

Status: PENDING

### Decision #7 — Key separation by assurance object type

Pending question:

Should Observation, Decision, and Witness use:

A. distinct signing keys per object purpose;

or

B. one assurance signing key with explicit object/domain separation?

This requires security, operational, provisioning, and future heterogeneous-node analysis.

Status: PENDING

---

## 10. Implementation gate

Implementation shall not begin until Decisions #5, #6, and #7 are adjudicated.

Current status:

Decision #1 — APPROVED
Decision #2 — APPROVED
Decision #3 — APPROVED
Decision #4 — APPROVED FOR HOST/TEST
Decision #5 — PENDING
Decision #6 — PENDING
Decision #7 — PENDING

Implementation status:

BLOCKED PENDING ARCHITECTURAL ADJUDICATION OF #5-#7

---

## 11. Authority invariant

Nothing in Decisions #1-#4 changes Guardian's physical-authority boundary.

The following remains forbidden:

valid signature
→
physical authority

Required conceptual chain:

VALIDATE
   ↓
CANONICALIZE
   ↓
AUTHENTICATE
   ↓
ESTABLISH FRESHNESS
   ↓
CORROBORATE
   ↓
APPLY DETERMINISTIC POLICY
   ↓
AUTHORIZE
   ↓
PHYSICAL GATE

The defining Guardian invariant remains:

Intelligence may advise.
Deterministic policy authorizes.
Evidence proves.

---

## 12. Decision summary

M15 has now frozen four pre-implementation choices:

1. Signed assurance uses an outer wrapper.
2. key_id is an opaque bounded credential identifier.
3. Ed25519 signatures use base64url without padding on the external representation.
4. Python cryptography is the reference host/test Ed25519 backend behind a provider abstraction.

Trust-store representation, provider error API, and key separation policy remain pending.

No implementation is authorized until those remaining architecture decisions are closed.

Status: APPROVED / PARTIALLY ADJUDICATED — implementation blocked pending Decisions #5-#7.