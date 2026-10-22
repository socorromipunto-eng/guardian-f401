# M17 Production Ed25519 Provider Architecture

Status: ARCHITECTURE MATERIALIZED; IMPLEMENTATION NOT YET DEMONSTRATED
Release target: v0.17.0
Authority: HUMAN_M17_PRODUCTION_ED25519_PROVIDER_ARCHITECTURE_AUTHORIZATION

## 1. Problem statement

Guardian already contains:

- firmware lifecycle signature-policy enforcement;
- a trusted-key provider abstraction and current key record;
- a trusted-signature adapter;
- positive/negative/hostile host tests for the adapter and lifecycle integration;
- a declared backend function named `guardian_firmware_ed25519_backend_verify`.

The repository does not currently demonstrate a production implementation of that backend. Test harnesses provide controlled fake backends; those are evidence of integration behavior, not evidence of Ed25519 cryptographic correctness or target production readiness.

## 2. Architecture decision

Guardian shall preserve the existing lifecycle -> trusted-signature adapter -> trusted-key provider separation.

The production Ed25519 capability shall be supplied through a provider-neutral cryptographic backend that implements the existing verification contract. The project shall not create a custom Ed25519 primitive merely to avoid a dependency.

The target verifier shall require only public-key verification capability. Production firmware-signing private keys shall remain outside the target device and outside the runtime verification boundary unless a separate, explicitly authorized architecture changes that rule.

## 3. Trust and authority boundaries

A cryptographically valid signature establishes only that the selected backend validated a signature against the supplied public key and message under the configured algorithm.

It does not establish by itself:

- key provenance;
- key trust;
- firmware authorization;
- firmware acceptance;
- boot promotion;
- actuator authority;
- peer authority;
- AI authority.

The mandatory pipeline is:

PROVIDER_AVAILABLE
-> ALGORITHM_POLICY_MATCH
-> KEY_ID_RESOLVED
-> KEY_STATE_ACTIVE
-> KEY_PROVENANCE_APPROVED
-> SIGNATURE_VALID
-> LIFECYCLE_POLICY
-> ROLLBACK/FRESHNESS POLICY
-> ACCEPTANCE

No earlier state implies a later state.

## 4. Provider requirements

The selected backend shall have an explicit capability manifest covering:

- provider name and version;
- source origin and immutable source hash;
- license and redistribution obligations;
- target-platform support;
- Ed25519 verification capability;
- build configuration and compiler assumptions;
- required hash primitives and dependencies;
- memory/stack expectations;
- error/failure semantics;
- constant-time/security properties claimed by the provider;
- vulnerability-management source;
- SBOM/provenance linkage;
- validation/certification scope, if any, without extrapolation.

A provider replacement is a security-boundary change and triggers impact revalidation.

## 5. NIST cryptographic baseline

FIPS 186-5 recognizes EdDSA as a digital-signature technique. That does not mean an arbitrary Ed25519 implementation is FIPS validated. CMVP/CAVP status is provider/module/version/configuration specific.

Guardian therefore separates:

ALGORITHM_ALLOWED != IMPLEMENTATION_VALIDATED != MODULE_CERTIFIED != PRODUCT_AUTHORIZED

## 6. Existing key record adjudication

The current trusted-key provider contains a static Ed25519 public-key record for key_id 2. The bytes are observable and test-covered, but repository evidence available at this boundary does not establish production key origin, ceremony, custody, issuer, rotation authority, revocation authority, manufacturing binding, or release-signing provenance.

Therefore:

CURRENT_KEY_2_PRESENT = YES
CURRENT_KEY_2_PRODUCTION_PROVENANCE = NOT_DEMONSTRATED
CURRENT_KEY_2_PRODUCTION_PROMOTION = PROHIBITED UNTIL SEPARATE EVIDENCE

## 7. Provider selection rule

Provider selection is deferred to the implementation authorization boundary. Selection shall prefer a maintained, standards-based, license-compatible implementation with target support and auditable provenance. Any proposal to vendor cryptographic source into the repository requires a separate supply-chain/license review before mutation.

## 8. Test strategy

Implementation validation shall include at minimum:

- authoritative Ed25519 known-answer vectors;
- valid signature acceptance;
- wrong message rejection;
- wrong public key rejection;
- signature bit-flip rejection;
- truncated/oversized signature rejection at the adapter boundary;
- malformed/non-canonical input behavior as applicable to the selected provider;
- unknown, retired, revoked, and algorithm-mismatched key rejection;
- no fallback-key behavior;
- backend unavailable/failure behavior;
- O0/O2 security-behavior consistency where meaningful;
- lifecycle end-to-end integration;
- evidence that test/stub providers cannot be linked into a production build unintentionally.

## 9. Airborne/NASA/IEEE assurance analogues

The project will use traceability, independence, configuration identification, reproducibility, review, and objective evidence as assurance practices. These are audit analogues only and do not create an FAA/EASA/DO-178C/NASA/IEEE certification claim.

## 10. AI boundary

AI may assist with analysis and review but cannot select or promote production keys, approve cryptographic providers, authorize firmware, promote releases, or become an actuation authority.

## 11. Architecture result

PRODUCTION_ED25519_ARCHITECTURE = MATERIALIZED
PRODUCTION_ED25519_PROVIDER_IMPLEMENTATION = NOT_YET_DEMONSTRATED
PRODUCTION_KEY_PROVENANCE = NOT_YET_DEMONSTRATED
NEXT_BOUNDARY = HUMAN_M17_PRODUCTION_ED25519_PROVIDER_IMPLEMENTATION_AUTHORIZATION