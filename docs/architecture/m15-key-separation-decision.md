# Guardian F401 — M15 Key Separation Decision

Status: APPROVED
Milestone: M15
Decision: #7 — Assurance Key Separation
Decision Type: Architecture / Security / Assurance
Target: Guardian F401
Depends on:
- ADR-M15-001 — Node Identity and Signed Assurance Messages
- M15-02.1 — Signed Transcript Specification
- M15 Identity and Signing Decisions #1-#4
- M15 Trust Store Decision
- M15 Provider Error API Decision
Implementation status: NOT IMPLEMENTED

---

## 1. Purpose

This document freezes the Guardian F401 M15 key-separation architecture for signed assurance objects.

The decision addresses whether Observation, Decision, and Witness objects require separate cryptographic keys or may use a common node assurance key with mandatory cryptographic domain separation.

The objective is to preserve semantic separation between assurance purposes without introducing unnecessary provisioning complexity before the threat model requires purpose-specific credentials.

---

## 2. Decision

Guardian M15 permits one assurance signing key per node for:

Observation
Decision
Witness

provided that the exact signed transcript provides mandatory and unambiguous domain separation by assurance purpose and object type.

Therefore:

one node assurance key
+
strict domain-separated transcripts
=
permitted in M15

This does not require all future Guardian roles to share one assurance key.

Purpose-specific keys remain supported and may become mandatory for higher-assurance roles.

---

## 3. Mandatory domain separation

A signature produced for one assurance purpose must not be valid for another assurance purpose.

Required invariant:

valid Observation signature
≠
valid Decision signature
≠
valid Witness signature

The exact authenticated transcript must bind the object type and applicable Guardian signing domain.

Changing:

Observation
→
Decision

or:

Observation
→
Witness

must change the authenticated transcript.

---

## 4. No semantic reinterpretation

A correctly signed object must not be reinterpretable as another assurance object type.

For example:

signed Observation
→
cannot become Decision

signed Decision
→
cannot become Witness

signed Witness
→
cannot become Observation

even when:

- producer_id is identical;
- key_id is identical;
- algorithm is identical;
- payload fields overlap.

Object semantics are authenticated.

---

## 5. Node assurance key

The M15 architecture permits a node to possess an assurance credential conceptually represented as:

producer_id
+
assurance key_id
+
Ed25519 public/private key pair

That credential may authenticate multiple assurance object types only where the signed transcript preserves explicit purpose separation.

This is an operational permission, not a requirement that every node support every object type.

---

## 6. Capability remains separate

Possession of an assurance signing key does not automatically grant permission to produce every assurance object type.

Cryptographic capability
≠
semantic capability
≠
authority

Future trust-store policy may restrict a credential to specific purposes.

For example:

key A
→
Observation only

key B
→
Witness only

The M15 architecture must not prevent that future restriction.

---

## 7. Firmware-signing separation

Firmware signing and assurance signing remain strictly separate cryptographic purposes.

Required:

firmware signing key
≠
assurance signing key

A firmware release credential shall not automatically authenticate:

- Observation;
- Decision;
- Witness.

Likewise, an assurance credential shall not automatically authorize firmware release signing.

Cross-purpose key reuse requires explicit architectural review and is not approved by this decision.

---

## 8. Future authority credentials

A future physical-authority or actuator-gate credential is outside the M15 assurance-key permission.

Guardian must not infer:

assurance signing key
=
actuator authorization key

If a future architecture introduces cryptographic credentials directly associated with physical authority, those credentials require a separate threat model and explicit key-separation decision.

Preferred direction:

assurance identity
≠
physical authority identity

---

## 9. Witness independence

M15 does not yet require every Witness to use a purpose-specific key.

However, heterogeneous/federated Guardian evolution may require stronger independence.

Future Witness roles may require:

- different processor;
- different implementation;
- different provisioning path;
- different assurance credential;
- different trust domain.

The current decision must not prevent that evolution.

---

## 10. Higher-assurance roles

Purpose-specific keys may become mandatory where:

- compromise impact differs materially by object type;
- regulatory requirements demand stronger separation;
- provisioning infrastructure supports independent credentials;
- independent Witness credibility requires separate custody;
- a Decision producer has materially greater privilege;
- heterogeneous-node threat modeling identifies unacceptable shared-key risk.

Such a requirement must be introduced through explicit architecture change.

---

## 11. Rationale

Requiring three or more keys per node immediately would increase:

- provisioning complexity;
- key inventory;
- secure storage requirements;
- rotation complexity;
- revocation complexity;
- recovery complexity;
- manufacturing complexity;
- evidence requirements.

M15 does not yet have evidence demonstrating that this cost is necessary for every node.

Mandatory transcript domain separation provides immediate cryptographic purpose separation while preserving the option to introduce distinct keys later.

---

## 12. Security limitation

Domain separation does not make one physical key equivalent to multiple independently protected keys.

If one shared node assurance private key is compromised, every assurance purpose authorized for that credential may be affected.

Therefore:

domain separation
≠
independent key custody

This limitation must remain explicit.

---

## 13. Compromise blast radius

Under the permitted M15 model:

compromise of node assurance private key
→
potential compromise of every assurance purpose assigned to that credential

unless trust policy separately limits its permitted purposes.

Future purpose-specific keys can reduce this blast radius.

This is one reason Guardian preserves support for stronger key separation.

---

## 14. Trust-store evolution

The Decision #5 trust-store architecture must remain capable of evolving from:

producer_id
+
key_id
+
algorithm

toward purpose-aware trust policy such as:

producer_id
+
key_id
+
algorithm
+
permitted_purpose

without allowing messages to establish their own privileges.

The exact purpose-authorization schema is not required for the first M15 implementation slice unless implementation review determines it is necessary to enforce the signed-object boundary.

---

## 15. Domain identifier requirements

The authenticated transcript must include an unambiguous Guardian-controlled domain identifier.

Conceptually:

guardian-f401
+
protocol/version
+
assurance purpose
+
object type

The exact byte representation remains governed by the approved M15 Signed Transcript Specification.

Domain values must not be inferred from external context.

They must be authenticated.

---

## 16. Cross-protocol protection

A Guardian assurance signature must not be valid as:

- firmware signature;
- arbitrary application signature;
- another Guardian protocol signature;
- another assurance object type.

Therefore the transcript must provide sufficient protocol and purpose binding.

Required principle:

same key
+
different cryptographic purpose
→
different authenticated domain

---

## 17. Algorithm separation

Decision #7 does not change the approved M15 algorithm:

Ed25519

Algorithm identity remains explicitly authenticated or otherwise bound according to the approved signed-transcript architecture.

No algorithm fallback is permitted.

---

## 18. Rotation

Key rotation remains credential-based.

A node may transition:

key_id A
→
key_id B

without changing:

producer_id

provided trust-store lifecycle state and evidence preserve the transition.

If a future node uses purpose-specific keys, each purpose may rotate independently.

---

## 19. Revocation

Revocation applies to the credential identified by the trust store.

If one assurance key is shared across multiple permitted purposes, revoking that key affects all purposes authenticated by that credential.

This consequence must be explicit.

Shared credential:

one revocation
→
all associated purposes revoked

Purpose-specific credentials can provide narrower revocation scope.

---

## 20. Test credentials

Host/test credentials may exercise:

- shared assurance-key mode;
- purpose-specific key mode.

Tests must prove that domain separation prevents cross-purpose signature reuse even when the same Ed25519 key pair is intentionally used.

Test credentials must never silently become production credentials.

---

## 21. Required adversarial tests

M15 implementation must include negative tests demonstrating at least:

1. Observation signature cannot verify as Decision.
2. Observation signature cannot verify as Witness.
3. Decision signature cannot verify as Observation.
4. Decision signature cannot verify as Witness.
5. Witness signature cannot verify as Observation.
6. Witness signature cannot verify as Decision.
7. Firmware signing transcript cannot verify as assurance transcript.
8. Assurance transcript cannot verify as firmware signing transcript.
9. Object-type mutation invalidates authentication.
10. Domain mutation invalidates authentication.
11. Transcript-version mutation invalidates authentication.
12. key_id substitution does not preserve authentication.

---

## 22. Authority invariant

No key-separation model changes Guardian's authority boundary.

A valid assurance signature proves authenticated provenance under the selected trust configuration.

It does not prove:

- freshness;
- physical truth;
- corroboration;
- safety;
- permission to actuate.

Therefore:

VALID ASSURANCE SIGNATURE
≠
PHYSICAL AUTHORITY

Required chain:

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

---

## 23. Security invariants

KS-01  Firmware-signing keys remain separate from assurance-signing keys.

KS-02  M15 permits one assurance signing key per node.

KS-03  Shared assurance keys require mandatory cryptographic domain separation.

KS-04  Observation signatures cannot authenticate Decision objects.

KS-05  Observation signatures cannot authenticate Witness objects.

KS-06  Decision signatures cannot authenticate Observation objects.

KS-07  Decision signatures cannot authenticate Witness objects.

KS-08  Witness signatures cannot authenticate Observation objects.

KS-09  Witness signatures cannot authenticate Decision objects.

KS-10  Object type is authenticated and cannot be inferred only from transport context.

KS-11  Protocol/signing domain is authenticated.

KS-12  Domain separation does not claim independent key custody.

KS-13  Shared-key compromise blast radius remains explicitly documented.

KS-14  Purpose-specific keys remain architecturally supported.

KS-15  Higher-assurance roles may require distinct credentials in future milestones.

KS-16  Assurance credentials do not imply physical authority.

KS-17  Future actuator-authority credentials require separate architecture review.

KS-18  Test credentials remain separate from production credentials.

---

## 24. Threats addressed

This decision addresses:

- cross-object signature reuse;
- semantic reinterpretation of signed objects;
- accidental firmware/assurance key-domain confusion;
- protocol confusion;
- object-type substitution;
- uncontrolled early proliferation of credentials;
- architecture lock-in that would prevent stronger future separation.

---

## 25. Threats not solved

This decision does not solve:

- compromise of a shared assurance private key;
- hardware-backed key custody;
- secure provisioning;
- replay;
- freshness;
- compromised trusted producer;
- quorum;
- partition handling;
- split-brain;
- failover;
- physical truth;
- physical authority.

---

## 26. Implementation target

The first implementation slice shall provide:

1. One assurance key per test node where appropriate.
2. Explicit authenticated signing domain.
3. Explicit authenticated object type.
4. Cross-purpose negative test vectors.
5. Firmware/assurance cross-domain negative tests.
6. Provider API compatibility with future purpose-specific keys.
7. Trust-store architecture that does not prevent future purpose restrictions.
8. Evidence demonstrating cross-purpose verification failure.

---

## 27. Acceptance criteria

Decision #7 implementation may be considered software-complete only when:

- Observation signs and verifies in its own domain;
- Decision signs and verifies in its own domain;
- Witness signs and verifies in its own domain;
- all six assurance cross-object substitutions fail;
- firmware/assurance cross-protocol substitutions fail;
- domain mutation fails authentication;
- object-type mutation fails authentication;
- shared-key test vectors demonstrate domain separation;
- implementation does not require firmware-signing key reuse;
- future purpose-specific credentials remain representable.

---

## 28. Decision summary

Guardian M15 permits one assurance signing key per node while requiring strict cryptographic domain separation between Observation, Decision, and Witness.

The decision intentionally distinguishes:

domain separation
≠
independent key custody

Purpose-specific keys remain supported and may become mandatory for higher-assurance roles as Guardian evolves toward heterogeneous and federated nodes.

Firmware-signing keys remain strictly separate from assurance-signing keys.

No assurance credential grants physical authority.

Status: APPROVED — architecture accepted; implementation remains pending.