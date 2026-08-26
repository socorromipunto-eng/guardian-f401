# Guardian F401 — TD-M15-001 Purpose-Domain Separation Finding

Status: RESOLVED
Milestone: M15
Review Type: Technical Destruction
Classification: Architecture / Security / Assurance
Implementation status at discovery: NOT COMMITTED

---

## 1. Finding

The pre-implementation review identified a mismatch between the original
M15-02.1 signed-transcript formulation and Decision #7 — Assurance Key
Separation.

The original transcript authenticated:

GUARDIAN-F401:M15:SIGNED-ASSURANCE:V1
||
key_id
||
producer_id
||
canonical_assurance_object

The canonical M14 assurance object already authenticated its validated
`domain` and `object_type`.

That protected object type against mutation, but it did not independently
freeze an explicit M15 purpose-specific cryptographic signing domain for
Observation, Decision, and Witness.

Decision #7 requires explicit cryptographic domain separation between those
assurance purposes.

---

## 2. Adversarial consequence

Without an exact byte-level purpose-domain contract, independent
implementations could make incompatible assumptions about whether purpose
separation is provided:

- only by the canonical M14 object;
- by verifier context;
- by another transcript field; or
- by a purpose-specific signing prefix.

A signed protocol transcript must not leave that choice implicit.

---

## 3. Resolution

Guardian M15 freezes these exact ASCII signing domains:

GUARDIAN-F401:M15:SIGNED-ASSURANCE:V1:OBSERVATION

GUARDIAN-F401:M15:SIGNED-ASSURANCE:V1:DECISION

GUARDIAN-F401:M15:SIGNED-ASSURANCE:V1:WITNESS

No terminating NUL byte is included.

No BOM is included.

No implicit whitespace is included.

No case folding is permitted.

---

## 4. Normative mapping

The validated M14 `object_type` determines the applicable M15 signing domain.

observation
→
GUARDIAN-F401:M15:SIGNED-ASSURANCE:V1:OBSERVATION

decision
→
GUARDIAN-F401:M15:SIGNED-ASSURANCE:V1:DECISION

witness
→
GUARDIAN-F401:M15:SIGNED-ASSURANCE:V1:WITNESS

No other mapping is valid.

The caller must not independently choose a signing domain inconsistent with the
validated M14 object type.

---

## 5. Amended transcript layout

The M15 authenticated transcript becomes:

PURPOSE_DOMAIN
||
uint16_be(key_id_length)
||
key_id
||
uint16_be(producer_id_length)
||
producer_id
||
uint32_be(canonical_assurance_object_length)
||
canonical_assurance_object

The approved semantics of `key_id`, `producer_id`, RFC 8785 canonicalization,
and big-endian length encoding remain unchanged.

---

## 6. Double binding

Assurance purpose is intentionally bound in two locations:

1. the M15 purpose-specific signing domain; and
2. the canonical M14 object's validated `domain` and `object_type`.

These representations must be semantically consistent.

---

## 7. Required validation

Positive cases:

Observation → Observation
Decision → Decision
Witness → Witness

Negative cases:

Observation → Decision
Observation → Witness
Decision → Observation
Decision → Witness
Witness → Observation
Witness → Decision

The same assurance key may be used in these tests so the rejection demonstrates
purpose separation rather than different key material.

---

## 8. Scope boundary

TD-M15-001 does not establish:

- freshness;
- anti-replay;
- physical truth;
- authorization;
- actuator authority;
- independent key custody;
- hardware identity;
- production readiness;
- certification.

---

## 9. Change-management record

The finding was discovered during pre-commit Technical Destruction of the first
M15 transcript implementation candidate.

The candidate implementation and test files were still untracked.

No M15 implementation commit had been made.

The architecture was therefore corrected before implementation became
repository history.

---

## 10. Disposition

Finding: RESOLVED

Architecture: AMENDMENT REQUIRED

Exact byte-level purpose domains: FROZEN

Implementation acceptance: PENDING

Cross-object validation: PENDING

Hardware validation: NOT CLAIMED

Status: RESOLVED — implementation must conform to the amended architecture.