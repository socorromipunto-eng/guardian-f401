# Guardian F401 — M15 Provider Error API Decision

Status: APPROVED
Milestone: M15
Decision: #6 — Provider Error API
Decision Type: Architecture / Security / Assurance
Target: Guardian F401
Depends on:
- ADR-M15-001 — Node Identity and Signed Assurance Messages
- M15-02.1 — Signed Transcript Specification
- M15 Identity and Signing Decisions #1-#4
- M15 Trust Store Decision
Implementation status: NOT IMPLEMENTED

---

## 1. Purpose

This document freezes the Guardian F401 M15 error-result architecture for cryptographic signing and verification providers.

The objective is to prevent authentication, trust-store, malformed-input, environment, and internal failures from collapsing into a single boolean result.

Guardian requires failures to remain semantically distinguishable because:

- evidence must identify what actually failed;
- malformed input must not be confused with cryptographic failure;
- unknown identity must not be confused with invalid signature;
- revoked credentials must not be confused with backend failure;
- environmental faults must not be reclassified as attacker-controlled input;
- policy and authority logic must never infer meaning from an ambiguous authentication result.

---

## 2. Decision

Guardian signing and verification providers shall return structured result states.

A simple:

true / false

result is insufficient.

The provider API must preserve the distinction between:

input validity
authentication result
identity resolution
credential lifecycle
algorithm support
provider availability
internal failure

---

## 3. Architectural boundary

Guardian preserves the processing sequence:

INPUT
  ↓
M14 VALIDATION
  ↓
CANONICALIZATION
  ↓
M15 TRANSCRIPT CONSTRUCTION
  ↓
TRUST LOOKUP
  ↓
CRYPTOGRAPHIC VERIFICATION
  ↓
AUTHENTICATION RESULT
  ↓
FRESHNESS
  ↓
POLICY
  ↓
AUTHORITY

Each stage owns its own failure semantics.

A downstream stage must not silently reinterpret an upstream failure.

---

## 4. M14 validation errors remain separate

Existing M14 validation failures remain outside the M15 provider result namespace.

Examples include:

INPUT_INVALID
DUPLICATE_KEY
RAW_LIMIT
STRUCTURAL_LIMIT
SCHEMA_INVALID
DOMAIN_INVALID
CANONICALIZATION_FAILURE

Exact existing repository identifiers remain authoritative.

M15 shall not create duplicate error meanings where M14 already owns the condition.

---

## 5. M15 verification result states

The M15 verification path shall distinguish at least:

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

These names are architectural identifiers.

Exact implementation enum spelling may be finalized during implementation review, but semantic distinctions shall not be collapsed.

---

## 6. AUTHENTICATED

Meaning:

The assurance object passed the required upstream validation and canonicalization steps, the trusted identity/key mapping resolved successfully, the credential state permitted verification, and the cryptographic signature verified successfully.

AUTHENTICATED means:

authenticated provenance under the selected trusted credential

It does not mean:

fresh
physically true
authorized
safe to actuate
uncompromised producer

Therefore:

AUTHENTICATED
≠
FRESH
≠
TRUE
≠
AUTHORIZED

---

## 7. IDENTITY_UNKNOWN

Meaning:

The supplied `producer_id` does not resolve to a known producer in the selected trust domain.

This is not:

- malformed input;
- invalid signature;
- provider failure.

The message may be perfectly well formed but originate from an untrusted or unknown identity.

Fail closed.

---

## 8. KEY_UNKNOWN

Meaning:

The `producer_id` is known, but the supplied `key_id` does not resolve to a trusted credential for that producer and algorithm.

This is distinct from:

IDENTITY_UNKNOWN

and:

SIGNATURE_INVALID

Fail closed.

---

## 9. KEY_REVOKED

Meaning:

The trust store contains the referenced credential and marks it as REVOKED.

Current authentication fails closed.

A revoked key must not be treated as merely unknown, because evidence and incident response need to distinguish:

unknown credential

from

known credential explicitly revoked

Historical evidence interpretation remains a separate policy concern.

---

## 10. KEY_RETIRED_FOR_NEW_USE

Meaning:

The credential is known and has lifecycle state RETIRED.

It is not valid for new signing/authentication contexts where current credentials are required.

Historical verification may be handled separately by future evidence policy.

For the active M15 verification path:

RETIRED
→
not accepted for new authenticated statements

---

## 11. ALGORITHM_UNSUPPORTED

Meaning:

The requested signature algorithm is syntactically valid but not supported by the selected implementation or policy.

This is distinct from malformed algorithm encoding.

No fallback algorithm is permitted.

Fail closed.

---

## 12. SIGNATURE_ENCODING_INVALID

Meaning:

The external signature representation is malformed before cryptographic verification.

Examples:

- forbidden "=" padding;
- invalid base64url character;
- embedded whitespace;
- invalid canonical representation;
- decoded size not equal to 64 bytes for Ed25519.

This is not equivalent to a cryptographically invalid but correctly encoded signature.

---

## 13. SIGNATURE_INVALID

Meaning:

The signature encoding is valid, the trusted verification key was resolved, and the cryptographic verification operation completed successfully as an operation, but the signature does not verify for the exact M15 transcript.

Examples include:

- transcript mutation;
- wrong private key used for signing;
- corrupted signature bytes;
- wrong public key associated with otherwise valid credential metadata.

Fail closed.

---

## 14. TRANSCRIPT_VERSION_UNSUPPORTED

Meaning:

The signed-message structure identifies a transcript version that the verifier does not implement or permit.

No implicit downgrade is allowed.

A verifier must not reinterpret a future transcript as an older version.

Fail closed.

---

## 15. TRUST_STORE_INVALID

Meaning:

The selected trust-store artifact exists but fails its own schema, duplicate, key-format, lifecycle, canonicalization, or integrity validation.

This is a configuration/evidence failure.

It is not attacker-input schema failure unless the trust store itself is explicitly being processed as untrusted configuration input under a separate boundary.

Authentication must not proceed.

---

## 16. TRUST_STORE_UNAVAILABLE

Meaning:

The configured trust store cannot be loaded or accessed.

Examples:

- missing file;
- inaccessible storage;
- unavailable configuration service;
- storage I/O failure.

This is an environment/configuration availability failure.

It must not be reported as:

IDENTITY_UNKNOWN

or:

SIGNATURE_INVALID

Authentication must fail closed.

---

## 17. CRYPTO_PROVIDER_UNAVAILABLE

Meaning:

The configured signing or verification backend is unavailable.

Examples:

- library not installed;
- hardware signer unavailable;
- provider initialization failure;
- secure element unreachable;
- external signer service unavailable.

This is an environment/provider availability failure.

It must not be reclassified as invalid input.

---

## 18. CRYPTO_PROVIDER_FAILURE

Meaning:

The cryptographic provider was available but failed to complete the requested operation for reasons that are not equivalent to a normal signature mismatch.

Examples may include:

- provider API failure;
- hardware fault;
- unexpected cryptographic library error;
- backend operational failure.

This is distinct from:

SIGNATURE_INVALID

A provider failure must not be interpreted as evidence that the signature itself was invalid.

---

## 19. INTERNAL_ERROR

Meaning:

Guardian encountered an unexpected internal condition that does not fit a more precise declared state.

This result is a last-resort fail-closed condition.

Implementation should minimize use of INTERNAL_ERROR.

Known failure classes must receive explicit states rather than being collapsed into INTERNAL_ERROR.

---

## 20. Signing-provider result states

Signing operations shall also use structured results.

At minimum:

SIGNED
IDENTITY_UNKNOWN
KEY_UNKNOWN
KEY_REVOKED
KEY_RETIRED_FOR_NEW_USE
ALGORITHM_UNSUPPORTED
TRANSCRIPT_VERSION_UNSUPPORTED
CRYPTO_PROVIDER_UNAVAILABLE
CRYPTO_PROVIDER_FAILURE
INTERNAL_ERROR

A signing operation must not return only:

signature or null

without an explicit result state.

---

## 21. Provider result structure

Conceptual verification result:

VerificationResult {
    status
    producer_id
    key_id
    algorithm
    transcript_version
    diagnostic_code
}

Optional diagnostic information may exist for evidence and debugging.

However, diagnostic text must not replace the stable machine-readable status.

---

## 22. Stable machine-readable status

The machine-readable result code is authoritative.

Human-readable diagnostic text is supplemental only.

Therefore:

status code
=
control-plane meaning

diagnostic string
=
human-readable evidence

Changing diagnostic wording must not change program behavior.

---

## 23. Exception boundary

Expected security outcomes should not require uncontrolled exceptions for normal processing.

Examples of normal fail-closed outcomes:

IDENTITY_UNKNOWN
KEY_UNKNOWN
KEY_REVOKED
SIGNATURE_INVALID
ALGORITHM_UNSUPPORTED

Unexpected provider/runtime faults may internally raise exceptions, but the provider boundary must translate them into the appropriate declared structured result.

No raw third-party exception shall become a policy decision.

---

## 24. Fail-closed rule

Any verification result other than:

AUTHENTICATED

must not be treated as authenticated.

Conceptually:

status == AUTHENTICATED
→
authenticated provenance

all other states
→
NOT authenticated

However, the exact non-authenticated reason must remain available for evidence.

---

## 25. No automatic fallback

The provider must not silently recover from:

ALGORITHM_UNSUPPORTED
KEY_UNKNOWN
KEY_REVOKED
TRANSCRIPT_VERSION_UNSUPPORTED
TRUST_STORE_INVALID

by:

- choosing another key;
- choosing another producer;
- choosing another algorithm;
- ignoring lifecycle state;
- loading a default trust store;
- downgrading transcript version.

Such behavior is prohibited.

---

## 26. Error precedence

Guardian shall report the earliest semantically valid failure in the defined processing pipeline.

Example:

malformed M14 object
→
M14 validation error

not:

SIGNATURE_INVALID

Likewise:

unknown producer
→
IDENTITY_UNKNOWN

before attempting cryptographic verification.

This avoids misleading evidence.

---

## 27. Side-channel consideration

External responses may intentionally expose less detail than internal evidence.

For example, an untrusted remote peer may receive a generic authentication failure while internal evidence records:

KEY_UNKNOWN

or:

SIGNATURE_INVALID

This distinction may reduce information leakage.

However, internal Guardian evidence must preserve the actual classified result.

External error disclosure policy is deferred to the transport/protocol layer.

---

## 28. Evidence requirements

Authentication evidence should record at least:

status
producer_id
key_id
algorithm
transcript_version
object_id
trust_store_version
trust_store_hash
provider_identifier
provider_version
verification_timestamp_or_logical_context
commit_sha

Where unavailable or not yet trusted, timing fields must not be misrepresented as secure time.

No private-key material is permitted.

---

## 29. Logging rule

Logs must preserve the distinction between:

input rejection
trust failure
authentication failure
provider failure
internal failure

Logging must not expose:

- private keys;
- secret material;
- raw production secrets;
- sensitive provider internals not required for evidence.

---

## 30. Authority invariant

Provider results do not create physical authority.

Forbidden:

AUTHENTICATED
→
actuator authorization

Required:

AUTHENTICATED
  ↓
freshness evaluation
  ↓
corroboration / evidence
  ↓
deterministic policy
  ↓
authority decision
  ↓
physical gate

---

## 31. Security invariants

PE-01  M14 validation errors remain distinct from M15 authentication results.

PE-02  Unknown identity is distinct from unknown key.

PE-03  Revoked key is distinct from unknown key.

PE-04  Invalid signature encoding is distinct from cryptographic signature failure.

PE-05  Signature mismatch is distinct from provider failure.

PE-06  Provider unavailable is distinct from provider operational failure.

PE-07  Trust-store failure is distinct from message failure.

PE-08  Unsupported algorithm fails closed.

PE-09  Unsupported transcript version fails closed.

PE-10  No automatic key, producer, algorithm, or version fallback is permitted.

PE-11  Machine-readable status is authoritative over diagnostic text.

PE-12  Every non-AUTHENTICATED verification result fails authentication closed.

PE-13  Error classification must preserve evidence semantics.

PE-14  Raw third-party provider exceptions do not become policy decisions.

PE-15  AUTHENTICATED does not imply freshness.

PE-16  AUTHENTICATED does not imply physical truth.

PE-17  AUTHENTICATED does not imply authority.

---

## 32. Threats addressed

This decision reduces ambiguity associated with:

- boolean-only authentication APIs;
- misleading incident evidence;
- unknown identity being mistaken for bad signature;
- revoked credential being mistaken for unknown key;
- provider outage being mistaken for attacker input;
- silent algorithm fallback;
- silent version downgrade;
- accidental policy decisions based on provider exceptions;
- inconsistent diagnostics across implementations.

---

## 33. Threats not solved

This API does not itself solve:

- private-key compromise;
- replay;
- freshness;
- compromised trusted producer;
- quorum;
- partition handling;
- failover;
- secure production key custody;
- physical truth;
- physical authority.

---

## 34. Implementation target

The first implementation slice shall provide:

1. Stable result enum.
2. VerificationResult structured object.
3. SigningResult structured object.
4. Explicit trust-store failure mapping.
5. Explicit provider failure mapping.
6. Explicit signature-encoding failure mapping.
7. Tests for every declared state.
8. Tests proving no silent fallback.
9. Evidence output preserving status codes.
10. CI validation of result stability.

---

## 35. Acceptance criteria

Decision #6 implementation may be considered software-complete only when:

- every declared verification state has a positive test vector;
- every declared failure state has a negative test vector;
- M14 errors remain distinguishable;
- unknown identity and unknown key are distinguishable;
- revoked and retired credentials are distinguishable;
- malformed signature encoding and invalid signature are distinguishable;
- provider unavailable and provider failure are distinguishable;
- unsupported algorithms fail closed;
- unsupported transcript versions fail closed;
- no automatic fallback occurs;
- evidence preserves the exact result state;
- AUTHENTICATED cannot directly reach authority logic.

---

## 36. Decision summary

Guardian M15 shall use structured cryptographic provider results rather than boolean authentication outcomes.

The defining distinction is:

malformed input
≠
unknown identity
≠
unknown key
≠
revoked credential
≠
bad signature
≠
provider failure
≠
authority

This preserves the Guardian principle that security evidence must describe what actually happened rather than collapse distinct failures into a convenient boolean.

Status: APPROVED — architecture accepted; implementation remains pending.