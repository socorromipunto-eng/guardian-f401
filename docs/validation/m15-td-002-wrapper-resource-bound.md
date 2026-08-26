# Guardian F401 — TD-M15-002 Signed Wrapper Raw Resource Bound

Status: RESOLVED
Milestone: M15
Review Type: Technical Destruction / Resource-Bound Adjudication
Classification: Architecture / Security / Assurance
Implementation status at discovery: NOT IMPLEMENTED

---

## 1. Finding

The approved M15 signed-message verification sequence requires the verifier to
apply raw wrapper resource limits before parsing.

However, the original M15 signed-transcript specification did not freeze an
exact numeric raw-byte bound for the outer signed-message wrapper.

Leaving this value implementation-defined would permit different Guardian
implementations to accept materially different attack surfaces.

A signed protocol boundary must not leave its primary raw input bound implicit.

---

## 2. Existing M14 evidence

The existing M14 assurance boundary defines:

max_raw_bytes = 65,536

The existing regression suite demonstrates:

65,535 bytes
→
accepted

65,536 bytes
→
accepted

65,537 bytes
→
RAW_LIMIT

Therefore:

M14 assurance-object raw maximum
=
65,536 bytes

This limit remains authoritative for the inner `assurance_object`.

---

## 3. M15 wrapper overhead

The M15 outer wrapper adds bounded metadata around the M14 assurance object.

The closed wrapper contains exactly:

signature_version
signature_algorithm
key_id
assurance_object
signature

The architecture already constrains:

- `signature_version` to one fixed initial version;
- `signature_algorithm` to `ed25519`;
- `key_id` to at most 128 ASCII characters;
- Ed25519 signature wire encoding to canonical base64url without padding;
- decoded Ed25519 signature length to exactly 64 bytes;
- the wrapper schema to exactly five members.

The wrapper therefore has a bounded overhead independent of the M14 object.

---

## 4. Decision

Guardian M15 freezes the following limits:

M14 assurance-object raw maximum:

65,536 bytes

M15 outer-wrapper overhead allowance:

512 bytes

M15 signed-wrapper raw maximum:

66,048 bytes

Formula:

65,536
+
512
=
66,048

---

## 5. Interpretation

The additional 512-byte allowance exists only for the M15 outer wrapper.

It does not increase the permitted raw size of the embedded M14 assurance
object.

Required processing rule:

raw signed wrapper
→
enforce 66,048-byte M15 wrapper limit
→
strictly parse and validate wrapper
→
extract assurance_object
→
independently enforce M14 assurance limits

Therefore:

wrapper allowance
≠
additional M14 payload capacity

---

## 6. Fail-closed boundary

The M15 wrapper parser must reject:

raw wrapper length > 66,048 bytes

before:

- UTF-8 decoding;
- JSON parsing;
- duplicate-member processing;
- schema evaluation;
- base64url decoding;
- assurance-object validation;
- transcript construction;
- trust lookup;
- cryptographic verification.

The failure must be deterministic.

---

## 7. Boundary cases

Required M15 wrapper tests include:

66,047 bytes
→
raw-size gate permits processing

66,048 bytes
→
raw-size gate permits processing

66,049 bytes
→
wrapper raw-limit rejection

These tests validate only the outer raw resource gate.

A wrapper of permitted outer size may still fail later validation for any other
reason.

---

## 8. Independent inner-object enforcement

A valid-size M15 wrapper does not exempt the embedded assurance object from M14
limits.

For example:

M15 wrapper <= 66,048 bytes
+
embedded M14 object > 65,536-byte authoritative limit
→
reject

The implementation must preserve the distinction between:

M15 wrapper raw-resource failure

and

M14 assurance-object raw-resource failure

where the representation and parser boundary permit that distinction to be
observed deterministically.

---

## 9. Security rationale

An explicit outer bound reduces exposure to:

- oversized untrusted inputs;
- parser resource exhaustion;
- implementation-specific size behavior;
- accidental unbounded buffering;
- inconsistent cross-platform acceptance;
- denial-of-service amplification before validation.

The chosen 512-byte overhead allowance is deliberately bounded.

It is not a general extension area.

The wrapper remains closed.

---

## 10. Change management

Any future change that increases:

- wrapper member count;
- maximum key_id length;
- signature representation;
- algorithm metadata;
- signature size;
- authenticated wrapper metadata;

must reassess the 512-byte overhead allowance.

An implementation must not silently increase the raw wrapper maximum.

A changed limit requires explicit architecture review and version consideration.

---

## 11. Scope boundary

TD-M15-002 does not establish:

- cryptographic authenticity;
- freshness;
- anti-replay;
- physical truth;
- authorization;
- actuator authority;
- hardware identity;
- production key custody;
- certification.

It defines only the raw resource bound of the M15 outer wrapper.

---

## 12. Disposition

Finding: RESOLVED

M14 inner raw maximum: 65,536 bytes

M15 wrapper overhead allowance: 512 bytes

M15 outer raw maximum: 66,048 bytes

Implementation acceptance: PENDING

Status: RESOLVED — M15 wrapper implementations must enforce the frozen raw bound.