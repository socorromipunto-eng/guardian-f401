# Guardian F401 — TD-M15-004 Trust Store Resource Bounds

Status: RESOLVED
Milestone: M15
Review Type: Technical Destruction / Resource-Bound Adjudication
Implementation status at discovery: NOT IMPLEMENTED

---

## 1. Finding

Decision #5 and TD-M15-003 define the trust-store schema, validation, lifecycle, lookup, duplicate handling, environment separation, and canonicalization requirements.

They do not freeze explicit resource limits for the complete trust-store input.

An unbounded trusted-configuration parser is not acceptable for the M15 fail-closed architecture.

---

## 2. Decision

Guardian M15 freezes the following initial trust-store resource bounds:

Raw trust-store file maximum:

1,048,576 bytes

Maximum TrustRecord count:

4,096 records

These are defensive implementation limits, not required deployment capacity.

---

## 3. Raw input boundary

Required processing rule:

raw trust-store length > 1,048,576 bytes
→
reject before UTF-8 decoding and JSON parsing

The raw-size gate occurs before canonicalization and before trust-record evaluation.

---

## 4. Record-count boundary

After strict parsing and top-level schema validation:

records count > 4,096
→
reject before credential resolution

An empty records array remains structurally valid.

Array ordering has no trust semantics.

---

## 5. Existing contracts remain authoritative

These resource limits do not modify:

- guardian-f401:m15:trust-store:v1
- TEST / PRODUCTION environment separation
- closed TrustRecord schema
- key_id contract
- Ed25519 algorithm identifier
- exactly 32-byte Ed25519 public keys
- canonical base64url without padding
- ACTIVE / RETIRED / REVOKED lifecycle semantics
- exact producer_id + key_id + algorithm lookup
- duplicate and ambiguity rejection
- RFC 8785 canonicalization

---

## 6. Required boundaries

Raw file:

1,048,575 bytes → raw-size gate permits processing
1,048,576 bytes → raw-size gate permits processing
1,048,577 bytes → raw-size rejection

Record count:

4,095 records → record-count gate permits processing
4,096 records → record-count gate permits processing
4,097 records → record-count rejection

Passing either resource gate does not imply that the trust store is otherwise valid.

---

## 7. Fail-closed ordering

raw bytes
→ raw-size gate
→ strict UTF-8
→ strict JSON
→ duplicate-member rejection
→ closed top-level schema
→ environment validation
→ records-count gate
→ TrustRecord validation
→ duplicate / ambiguity validation
→ RFC 8785 canonicalization
→ trust resolution

---

## 8. Security rationale

The limits reduce exposure to unbounded buffering, parser resource exhaustion, canonicalization amplification, excessive duplicate-search work, and configuration-based denial of service.

The limits do not establish authenticity, freshness, authority, or production capacity.

---

## 9. Change management

Increasing either resource bound requires explicit architecture review.

Implementations must not silently increase the limits.

A future deployment requiring additional capacity must produce evidence supporting the new bounds.

---

## 10. Disposition

Finding: RESOLVED
Raw trust-store maximum: 1,048,576 bytes
Maximum TrustRecord count: 4,096
Implementation acceptance: PENDING

Status: RESOLVED — trust-store runtime must enforce these resource bounds.
