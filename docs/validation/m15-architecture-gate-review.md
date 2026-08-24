# Guardian F401 — M15 Architecture Gate Review

Status: PASS WITH CONDITIONS
Milestone: M15
Review Type: Architecture / Security / Assurance / Implementation Readiness
Target: Guardian F401
Branch: feature/m15-node-identity-architecture
Implementation status: NOT IMPLEMENTED
Gate status: IMPLEMENTATION MAY PROCEED ONLY WITHIN THE APPROVED M15 SCOPE

---

## 1. Purpose

This document records the formal architecture gate review for the Guardian F401 M15 node-identity and signed-assurance work.

The review determines whether the architecture is sufficiently defined, internally consistent, bounded, traceable, and adversarially reviewed to permit implementation to begin.

This gate does not claim that M15 functionality is implemented.

It evaluates architecture readiness only.

---

## 2. Baseline

Reviewed branch:

feature/m15-node-identity-architecture

Architecture baseline commits:

2930d8b — docs(m15): add node identity and signed assurance ADR

ef126f9 — docs(m15): add signed transcript specification

98d664e — docs(m15): record identity signing decisions

4468520 — docs(m15): define trust store architecture

d95ee19 — docs(m15): define provider error API

e0c839a — docs(m15): define assurance key separation

09ad4c5 — docs(m15): normalize architecture document encoding

The review was performed against a clean working tree after the encoding correction commit.

---

## 3. Reviewed artifacts

The architecture gate reviewed the following controlled documents:

1. `docs/adr/ADR-M15-001-node-identity-and-signed-assurance-messages.md`

2. `docs/architecture/m15-signed-transcript-specification.md`

3. `docs/architecture/m15-identity-signing-decisions.md`

4. `docs/architecture/m15-trust-store-decision.md`

5. `docs/architecture/m15-provider-error-api-decision.md`

6. `docs/architecture/m15-key-separation-decision.md`

The six documents passed the document-integrity gate:

- strict UTF-8;
- no UTF-8 BOM;
- no U+FFFD replacement characters;
- deterministic Git line-ending policy through `.gitattributes`;
- clean working tree after corrective commit.

---

## 4. Architectural thesis preserved

The M15 architecture remains consistent with the Guardian architectural thesis.

Guardian separates:

Integrity
≠
Authenticity
≠
Authority

The architecture does not collapse those properties into one control.

### Integrity

The M14 validation boundary remains upstream of authentication.

The M15 work does not permit cryptographic authentication to bypass:

- strict parsing;
- duplicate-member rejection;
- closed schema validation;
- resource bounds;
- deterministic canonicalization.

### Authenticity

M15 introduces the architecture required to bind accepted canonical assurance objects to trusted producer credentials.

Authentication establishes provenance under a trusted credential.

It does not establish physical truth.

### Authority

M15 introduces no new physical authority.

A valid signature remains evidence input.

It does not directly authorize actuation.

---

## 5. Core processing order

The architecture preserves the following dependency order:

UNTRUSTED INPUT
  ↓
BOUNDED PARSING
  ↓
STRICT VALIDATION
  ↓
CLOSED CONTRACT
  ↓
RFC 8785 CANONICALIZATION
  ↓
SIGNED TRANSCRIPT CONSTRUCTION
  ↓
TRUST LOOKUP
  ↓
CRYPTOGRAPHIC AUTHENTICATION
  ↓
FRESHNESS EVALUATION
  ↓
CORROBORATION / EVIDENCE
  ↓
DETERMINISTIC POLICY
  ↓
AUTHORITY DECISION
  ↓
PHYSICAL GATE

The first M15 implementation slice terminates at authenticated provenance.

Freshness, corroboration, and authority remain downstream.

---

## 6. Decision #1 — Signed-message representation

Status: APPROVED

Decision:

M15 shall use a versioned outer signed-message wrapper.

The existing M14 assurance envelope remains closed and semantically unchanged.

Assessment:

PASS

Rationale:

- prevents silent M14 schema drift;
- separates assurance semantics from authentication metadata;
- preserves independent validation of the M14 object;
- allows future signature evolution without redefining Observation, Decision, and Witness semantics.

---

## 7. Decision #2 — key_id representation

Status: APPROVED

Decision:

`key_id` is an opaque bounded ASCII-compatible credential identifier.

Initial grammar:

^[A-Za-z0-9._:-]{1,128}$

Assessment:

PASS

The architecture maintains:

producer_id
≠
key_id
≠
public key
≠
private key
≠
authority

---

## 8. Decision #3 — Signature external encoding

Status: APPROVED

Decision:

Ed25519 signatures use RFC 4648 base64url without padding in the external signed-message representation.

Raw Ed25519 signature length:

64 bytes

Assessment:

PASS

The architecture explicitly rejects:

- padding;
- invalid alphabet;
- whitespace;
- non-canonical encoding;
- decoded lengths other than 64 bytes.

---

## 9. Decision #4 — Host/test Ed25519 backend

Status: APPROVED FOR HOST/TEST

Decision:

Python `cryptography` is the reference Ed25519 backend for host/test work behind a Guardian provider abstraction.

Assessment:

PASS WITH CONDITION

Condition:

The exact dependency version, dependency evidence, license metadata, and reproducible environment must be frozen during implementation.

This decision does not define:

- embedded production signer;
- STM32 private-key storage;
- non-extractable production identity;
- hardware secure element.

---

## 10. Decision #5 — Trust-store representation

Status: APPROVED

Decision:

Guardian shall use a closed, versioned external trust store based on explicit public-key pinning.

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

Assessment:

PASS

The message cannot introduce its own trust anchor.

Unknown or ambiguous mappings fail closed.

The trust store contains no private keys.

---

## 11. Decision #6 — Provider Error API

Status: APPROVED

Decision:

Signing and verification providers use structured machine-readable results rather than boolean-only outcomes.

The architecture distinguishes:

AUTHENTICATED
IDENTITY_UNKNOWN
KEY_UNKNOWN
KEY_REVOKED
KEY_RETIRED_FOR_NEW_USE
ALGORITHM_UNSUPPORTED
SIGNATURE_ENCODING_INVALID
SIGNATURE_INVALID
TRANSCRIPT_VERSION_UNSUPPORTED
TRUST_STORE_INVALID
TRUST_STORE_UNAVAILABLE
CRYPTO_PROVIDER_UNAVAILABLE
CRYPTO_PROVIDER_FAILURE
INTERNAL_ERROR

Assessment:

PASS

This preserves the distinction between:

malformed input
≠
unknown identity
≠
unknown key
≠
revoked credential
≠
invalid signature
≠
provider failure

---

## 12. Decision #7 — Assurance key separation

Status: APPROVED

Decision:

M15 permits one assurance signing key per node, provided that strict authenticated domain separation prevents cross-purpose signature reuse between:

Observation
Decision
Witness

Assessment:

PASS WITH CONDITION

Condition:

Implementation must provide adversarial test vectors proving that every cross-object substitution fails.

The architecture explicitly acknowledges:

domain separation
≠
independent key custody

Future higher-assurance roles may require purpose-specific keys.

---

## 13. Firmware-signing boundary

M15 preserves:

firmware signing key
≠
assurance signing key

The existing M12 Ed25519 direction may inform algorithm selection but does not merge trust domains.

Assessment:

PASS

No automatic cross-purpose key reuse is authorized.

---

## 14. Factory UID boundary

The STM32F401 factory UID remains a hardware attribute.

It is not:

- a secret;
- a private key;
- proof of possession;
- an authentication credential;
- authorization evidence.

Assessment:

PASS

Any future binding between physical UID and cryptographic identity requires independent provisioning evidence.

---

## 15. producer_id boundary

`producer_id` remains a logical identity claim until bound to a trusted credential.

Structural validation alone does not authenticate it.

Assessment:

PASS

M15 explicitly upgrades:

producer identity claim

toward:

authenticated producer provenance

without changing the meaning of the existing M14 field.

---

## 16. producer_epoch boundary

`producer_epoch` is authenticated when included in the signed canonical object.

However:

signed producer_epoch
≠
persistent freshness

Assessment:

PASS

Persistent interpretation across reset remains deferred to M15-03.

---

## 17. logical_time boundary

`logical_time` may be authenticated.

However:

signed logical_time
≠
trusted wall-clock time
≠
persistent freshness

Assessment:

PASS

No secure-time claim is introduced.

---

## 18. Freshness boundary

Persistent anti-replay is explicitly out of scope for the current signed-identity slice.

Assessment:

PASS

The architecture repeatedly preserves:

AUTHENTICATED
≠
FRESH

M15-03 remains required before replay resistance across reset can be claimed.

---

## 19. Physical truth boundary

Cryptographic authentication proves provenance of the signed statement under the selected trust configuration.

It does not prove correspondence with physical reality.

Assessment:

PASS

The architecture preserves:

AUTHENTICATED
≠
TRUE

A compromised legitimate signing node remains a known threat.

---

## 20. Authority boundary

The architecture explicitly prohibits:

valid signature
→
physical authority

Assessment:

PASS

Required downstream chain remains:

AUTHENTICATED
  ↓
FRESHNESS
  ↓
CORROBORATION
  ↓
DETERMINISTIC POLICY
  ↓
AUTHORIZATION
  ↓
PHYSICAL GATE

No M15 architecture artifact grants advisory or authenticated messages direct actuator authority.

---

## 21. Trust-store lifecycle consistency

Approved lifecycle states:

ACTIVE
RETIRED
REVOKED

Assessment:

PASS

Semantics are consistent across trust-store and provider-error documents.

ACTIVE:

may authenticate current statements.

RETIRED:

not approved for new authentication; historical verification remains a future evidence-policy concern.

REVOKED:

fails current authentication.

---

## 22. Fail-closed consistency

The reviewed architecture consistently requires fail-closed behavior for:

- unknown producer;
- unknown key;
- revoked key;
- unsupported algorithm;
- unsupported transcript version;
- malformed signature encoding;
- invalid signature;
- ambiguous trust mapping;
- invalid trust store;
- unavailable trust store;
- unavailable crypto provider.

Assessment:

PASS

No automatic fallback is authorized.

---

## 23. Domain separation consistency

The signed transcript and key-separation decisions both require protocol and object-purpose separation.

Assessment:

PASS WITH CONDITION

Condition:

The final implementation must freeze the exact byte-level purpose/domain representation before test vectors are considered authoritative.

Cross-protocol and cross-object negative tests are mandatory.

---

## 24. Trust injection review

Threat:

An attacker supplies a public key inside the signed message and convinces the verifier to trust it.

Architectural result:

BLOCKED

Reason:

Trusted keys are resolved externally through the approved trust store.

Message-controlled trust establishment is explicitly prohibited.

Assessment:

PASS

---

## 25. Algorithm downgrade review

Threat:

An attacker changes or omits the algorithm and triggers fallback to another verification method.

Architectural result:

BLOCKED

Reason:

Algorithm identity is explicit and unsupported algorithms fail closed.

Silent fallback is prohibited.

Assessment:

PASS

---

## 26. Transcript downgrade review

Threat:

A future signed message is interpreted using an older transcript format.

Architectural result:

BLOCKED

Reason:

Transcript version is explicit.

Unsupported transcript versions fail closed.

Assessment:

PASS

---

## 27. Cross-object signature reuse review

Threat:

A valid Observation signature is reused as a Decision or Witness signature.

Architectural result:

DESIGNED TO BE BLOCKED

Evidence status:

NOT IMPLEMENTED

Assessment:

PASS WITH CONDITION

Required evidence:

Implementation must demonstrate all six cross-object substitution failures:

Observation → Decision
Observation → Witness
Decision → Observation
Decision → Witness
Witness → Observation
Witness → Decision

---

## 28. Cross-protocol signature reuse review

Threat:

An assurance signature is reused as a firmware signature or vice versa.

Architectural result:

DESIGNED TO BE BLOCKED

Assessment:

PASS WITH CONDITION

Required evidence:

Negative interoperability tests must demonstrate firmware/assurance transcript incompatibility.

---

## 29. Shared assurance-key compromise review

Threat:

One node assurance private key is compromised.

Impact:

Every assurance purpose authorized for that credential may be forged.

Architectural response:

Known limitation.

Assessment:

ACCEPTED WITH CONDITION

Condition:

The limitation remains documented.

Purpose-specific credentials remain architecturally supported for future higher-assurance roles.

---

## 30. Compromised trusted-node review

Threat:

A legitimate node possessing its correct private key emits semantically false but correctly signed data.

Architectural response:

Not solved by cryptography.

Assessment:

KNOWN RESIDUAL RISK

Future controls include:

- independent Witness nodes;
- heterogeneous implementations;
- corroboration;
- attestation;
- quorum;
- physical-signal diversity.

No claim is made that M15 eliminates this threat.

---

## 31. Test/production identity separation

M15 distinguishes:

development/test identity

from:

production identity

Assessment:

PASS

Software-accessible test keys are permitted only in explicitly non-production contexts.

No host-test success may be used to claim non-extractable production identity.

---

## 32. Production key-custody review

Production private-key custody remains undecided.

Possible future implementations may include:

- secure element;
- external cryptographic signer;
- protected provisioning mechanism;
- future MCU security capability;
- separately reviewed architecture.

Assessment:

OPEN GATE

This does not block host/test implementation.

It does block any claim of production-grade non-extractable device identity.

---

## 33. Dependency review

The architecture selects Python `cryptography` for host/test reference Ed25519 operations.

Assessment:

PASS WITH CONDITION

Before dependency integration:

- exact version must be selected;
- dependency must be pinned;
- license must be recorded;
- runtime compatibility must be verified;
- SBOM/provenance evidence must capture the dependency;
- CI environment must be reproducible.

---

## 34. Human Readability Review

Result:

PASS

Findings:

- identity, credential, trust, authentication, freshness, truth, and authority are explicitly separated;
- implementation status remains clearly marked;
- deferred work is visible;
- claims do not imply production readiness;
- future architecture is distinguished from implemented functionality.

---

## 35. Devil's Advocate Review

Result:

PASS WITH CONDITIONS

Primary challenges:

1. One assurance key per node increases compromise blast radius.
2. Trust-store integrity is initially hash-verifiable but not yet securely distributed.
3. Production key custody remains unresolved.
4. Freshness is not yet implemented.
5. Authentication cannot detect a trusted node that lies using a legitimate key.
6. The exact host/test dependency version remains unfrozen.
7. Domain separation must be demonstrated by negative test vectors, not only documented.

None of these invalidate the current architecture slice.

They define implementation and future milestone gates.

---

## 36. Technical Destruction Review

Result:

PASS WITH CONDITIONS

Attempted architectural attacks include:

- producer_id substitution;
- key_id substitution;
- public-key injection;
- algorithm downgrade;
- transcript-version downgrade;
- signature encoding ambiguity;
- cross-object signature reuse;
- cross-protocol signature reuse;
- revoked-key acceptance;
- trust-store ambiguity;
- provider outage misclassified as attacker input;
- authenticated message interpreted as authority;
- shared-key compromise;
- legitimate compromised producer.

The architecture provides explicit controls or explicit residual-risk declarations for each category.

Where proof depends on implementation, the gate remains conditional.

---

## 37. Security Review

Result:

PASS WITH CONDITIONS

Security properties sufficiently defined for implementation:

- validated canonical object precedes authentication;
- producer credential lookup is external and deterministic;
- trust injection is prohibited;
- Ed25519 algorithm target is explicit;
- signature external encoding is explicit;
- provider failure taxonomy is explicit;
- trust lifecycle states are explicit;
- no silent fallback;
- firmware and assurance signing remain separate;
- authentication does not imply authority.

Security properties not yet implemented:

- signed assurance messages;
- operational trust store;
- Ed25519 provider integration;
- persistent freshness;
- production key custody;
- attestation;
- quorum;
- failover.

---

## 38. Regulatory-readiness review

Result:

PASS AS ARCHITECTURAL READINESS ONLY

The architecture improves:

- traceability;
- provenance;
- separation of responsibilities;
- deterministic validation;
- explicit trust boundaries;
- error classification;
- lifecycle evidence;
- demonstrable authority separation.

No claim is made of:

- IEC 61508 certification;
- IEC 62443 certification;
- SIL rating;
- EU AI Act conformity;
- Cyber Resilience Act conformity;
- Machinery Regulation conformity;
- formal conformity assessment.

Regulatory readiness remains an evidence-building objective, not a certification claim.

---

## 39. Claim/evidence review

Current architecture claims are limited to design decisions.

No M15 document shall be interpreted as evidence that node identity or signed assurance is already implemented.

Required current statement:

ARCHITECTURE APPROVED
IMPLEMENTATION NOT YET PRESENT

Assessment:

PASS

---

## 40. Conditions before implementation completion

Implementation may begin, but M15-02 cannot be declared complete until evidence demonstrates at least:

1. deterministic transcript construction;
2. Ed25519 host/test signing and verification;
3. exact signature base64url encoding;
4. closed outer wrapper validation;
5. trust-store strict parsing and deterministic lookup;
6. provider structured result states;
7. unknown identity fail-closed;
8. unknown key fail-closed;
9. revoked key fail-closed;
10. unsupported algorithm fail-closed;
11. unsupported transcript version fail-closed;
12. malformed signature encoding fail-closed;
13. invalid signature fail-closed;
14. cross-object substitution rejection;
15. firmware/assurance cross-protocol rejection;
16. reproducible test vectors;
17. evidence hashes;
18. CI gate;
19. dependency provenance;
20. no code path converting authentication directly into physical authority.

---

## 41. Explicitly deferred gates

The following remain outside the current implementation authorization:

### M15-03 — Persistent Freshness and Anti-Replay

- persistent monotonic state;
- producer epoch semantics across reset;
- rollback resistance;
- nonce lifecycle;
- replay windows;
- power-loss behavior.

### M15-04 — Attestation and Witness Exchange

- attestation transcript;
- witness relationships;
- independent observations;
- trust evaluation.

### Later architecture

- quorum;
- partition handling;
- split-brain prevention;
- distributed failover;
- heterogeneous node federation;
- advisory AI integration;
- physical authority credentials;
- production key-custody hardware.

---

## 42. Implementation authorization

Architecture gate result:

PASS WITH CONDITIONS

Implementation authorization:

GRANTED FOR THE M15 NODE-IDENTITY / SIGNED-ASSURANCE SOFTWARE SLICE ONLY

Authorized implementation scope:

- outer signed-message wrapper;
- deterministic transcript builder;
- key_id handling;
- base64url signature encoding;
- host/test Ed25519 provider;
- verification provider;
- closed trust store;
- lifecycle-state lookup;
- structured provider results;
- domain-separated Observation, Decision, and Witness authentication;
- positive and adversarial test vectors;
- CI/evidence integration.

Not authorized by this gate:

- production private-key provisioning;
- claim of non-extractable identity;
- persistent anti-replay;
- attestation;
- quorum;
- failover;
- physical authority changes;
- certification claims.

---

## 43. Final adjudication

Architecture consistency:

PASS

Threat-model consistency:

PASS WITH CONDITIONS

Claim/evidence consistency:

PASS

Document integrity:

PASS

Implementation readiness:

PASS WITH CONDITIONS

Overall gate:

PASS WITH CONDITIONS

The conditions are implementation evidence requirements and explicitly deferred security properties.

They do not require reopening the approved M15 architecture before the software implementation slice begins.

---

## 44. Final statement

Guardian F401 M15 is architecturally ready to begin implementation of node identity and signed assurance messages within the approved software-only scope.

The gate does not claim that authenticity, freshness, distributed assurance, physical truth, or production identity are already achieved.

The governing invariants remain:

VALIDATED
≠
AUTHENTICATED
≠
FRESH
≠
TRUE
≠
AUTHORIZED

and:

Intelligence may advise.
Deterministic policy authorizes.
Evidence proves.

Status: PASS WITH CONDITIONS — M15 software implementation may begin within the bounded approved scope.
---

## TD-M15-001 Closure — Purpose-Domain Representation

Status: CONDITION CLOSED

The pre-implementation Technical Destruction review identified that the original
M15-02.1 transcript authenticated the validated M14 `domain` and `object_type`
inside the canonical assurance object, but had not independently frozen the
exact byte-level purpose-specific signing domains required by Decision #7.

The architecture was corrected before the first M15 implementation commit.

M15-02.1 now freezes these exact ASCII signing domains:

GUARDIAN-F401:M15:SIGNED-ASSURANCE:V1:OBSERVATION

GUARDIAN-F401:M15:SIGNED-ASSURANCE:V1:DECISION

GUARDIAN-F401:M15:SIGNED-ASSURANCE:V1:WITNESS

The validated M14 `object_type` determines the applicable M15 purpose domain.

The caller must not independently select a purpose domain inconsistent with the
validated M14 object.

Acceptance requires successful same-purpose coverage for:

Observation → Observation

Decision → Decision

Witness → Witness

and rejection of all six cross-object substitutions:

Observation → Decision

Observation → Witness

Decision → Observation

Decision → Witness

Witness → Observation

Witness → Decision

Traceability:

Decision #7
→
TD-M15-001
→
M15-02.1 purpose-domain amendment
→
implementation
→
cross-object validation

This closure does not change any claim concerning:

- freshness;
- anti-replay;
- physical truth;
- authorization;
- actuator authority;
- hardware identity;
- production key custody;
- certification.

Gate disposition:

The condition requiring exact byte-level purpose/domain representation is CLOSED.

Implementation remains subject to software validation and all remaining M15
acceptance criteria.

Status: CLOSED — architecture corrected before first implementation commit.
---

## TD-M15-002 Closure — Signed Wrapper Raw Resource Bound

Status: CONDITION CLOSED

The pre-implementation Technical Destruction review identified that the M15
signed-message verification sequence required a raw wrapper resource limit but
the original architecture had not frozen an exact numeric bound.

The existing M14 assurance-object raw maximum is:

65,536 bytes

The approved M15 outer-wrapper overhead allowance is:

512 bytes

The frozen M15 signed-wrapper raw maximum is therefore:

66,048 bytes

Required processing order:

raw wrapper
→
enforce 66,048-byte outer limit
→
strict UTF-8 processing
→
strict JSON parsing
→
duplicate-member rejection
→
closed wrapper schema
→
signature metadata validation
→
embedded assurance_object validation

The embedded M14 assurance object remains independently subject to:

max_raw_bytes = 65,536

Therefore:

M15 wrapper allowance
≠
additional M14 payload capacity

Required boundary evidence:

66,047 bytes
→
outer raw-size gate permits processing

66,048 bytes
→
outer raw-size gate permits processing

66,049 bytes
→
outer raw-size rejection

Passing the outer size gate does not imply that the wrapper is otherwise valid.

Traceability:

M15 Architecture Gate
→
TD-M15-002
→
M15-02.1 wrapper-bound amendment
→
signed-wrapper implementation
→
resource-bound tests

This closure changes no claim concerning:

- signature validity;
- authentication;
- freshness;
- anti-replay;
- physical truth;
- authorization;
- actuator authority;
- production identity;
- certification.

Gate disposition:

The condition requiring an explicit M15 signed-wrapper raw resource bound is
CLOSED.

Implementation remains subject to software validation and all remaining M15
acceptance criteria.

Status: CLOSED — wrapper raw maximum frozen at 66,048 bytes before implementation.

---

## TD-M15-003 Closure — Trust Store Document Schema

Status: CONDITION CLOSED

Technical Destruction identified that Decision #5 defined the trusted credential record but did not freeze the exact top-level trust-store JSON document.

The architecture now freezes a closed top-level object containing exactly:

- schema_version
- environment
- records

Initial schema version:

guardian-f401:m15:trust-store:v1

Allowed environments:

- TEST
- PRODUCTION

The configured verifier environment must exactly match the loaded trust-store environment.

TEST trust store ≠ PRODUCTION trust store.

The signed assurance message cannot select or override the trust-store environment.

Each records entry remains a closed TrustRecord containing exactly:

- producer_id
- key_id
- algorithm
- public_key
- lifecycle_state

Lookup remains exact:

producer_id + key_id + algorithm → exactly one TrustRecord or explicit failure.

No wildcard matching, fallback key, default producer, first-match-wins, or last-match-wins behavior is permitted.

Ed25519 trusted public keys use canonical base64url without padding and decode to exactly 32 bytes.

Lifecycle semantics remain ACTIVE, RETIRED, and REVOKED.

Trust resolution remains separate from cryptographic verification.

VERIFIED ≠ AUTHENTICATED until trusted credential resolution succeeds.

ACTIVE credential ≠ physical authority.

File representation remains strict UTF-8 JSON without BOM with duplicate-member rejection, closed schema, deterministic validation, and RFC 8785 canonical representation.

Traceability:

Decision #5
→
TD-M15-003
→
Trust Store Document Schema Amendment
→
Trust Store Runtime implementation
→
validation evidence

Gate disposition:

The condition requiring an exact top-level trust-store representation and TEST/PRODUCTION separation is CLOSED.

Trust-store runtime implementation remains PENDING.

Status: CLOSED — trust-store document schema frozen before runtime implementation.
