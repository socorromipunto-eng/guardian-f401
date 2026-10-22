# M17 Production Ed25519 Provider Requirements and Verification Plan

Status: GOVERNED REQUIREMENTS BASELINE
Release target: v0.17.0

## 1. Functional requirements

EDP-RQ-001: The production backend shall implement the existing `guardian_firmware_ed25519_backend_verify` verification contract or an explicitly approved source-compatible replacement.

EDP-RQ-002: The production target shall verify Ed25519 signatures and shall not require a firmware-signing private key on the target.

EDP-RQ-003: The trusted-signature adapter shall continue to reject unknown algorithms, invalid signature lengths, unresolved keys, non-active keys, and algorithm mismatch before invoking the backend.

EDP-RQ-004: Backend failure shall fail closed and shall never be interpreted as a valid signature.

EDP-RQ-005: The lifecycle shall continue to enforce signature policy, image digest, version/rollback policy, and state-transition policy independently of cryptographic backend success.

## 2. Provenance requirements

EDP-RQ-010: Every production cryptographic provider shall have source origin, exact version, immutable hash, license, build configuration, and vulnerability-maintenance source recorded in evidence.

EDP-RQ-011: Every production public key shall have a governed provenance record identifying key_id, algorithm, origin/ceremony, owner/authority, intended purpose, activation status, rotation/revocation authority, and evidence linkage.

EDP-RQ-012: Existing key bytes without governed provenance shall not be promoted to a production trust anchor.

EDP-RQ-013: Provider/module certification or validation shall be claimed only for the exact version/configuration covered by authoritative evidence.

## 3. Semantic requirements

EDP-RQ-020: VALID_SIGNATURE != TRUSTED.
EDP-RQ-021: TRUSTED != AUTHORIZED.
EDP-RQ-022: AUTHORIZED != ACCEPTED.
EDP-RQ-023: ACCEPTED != ACTUATION_AUTHORIZED.
EDP-RQ-024: AI_ADVISORY != AUTHORITY != ACTUATION.
EDP-RQ-025: UNKNOWN provider/key/provenance state shall fail closed.

## 4. Security requirements

EDP-RQ-030: No custom Ed25519 primitive may be introduced without a separately governed research boundary and explicit approval.
EDP-RQ-031: Test/fake cryptographic backends shall be structurally separable from production builds.
EDP-RQ-032: Production provider substitution shall trigger compatibility, security-boundary, supply-chain, and certification-impact review.
EDP-RQ-033: Error handling shall not create a fallback key, fallback algorithm, or bypass path.
EDP-RQ-034: Public inputs shall not be mutated by verification APIs.
EDP-RQ-035: Provider integration shall not grant actuator authority or peer-control authority.

## 5. Verification requirements

EDP-VR-001: Positive authoritative Ed25519 known-answer vectors shall pass.
EDP-VR-002: Wrong-message vectors shall fail.
EDP-VR-003: Wrong-key vectors shall fail.
EDP-VR-004: Modified signatures shall fail.
EDP-VR-005: Adapter signature-length boundary tests shall fail closed.
EDP-VR-006: Unknown key_id shall fail before backend invocation.
EDP-VR-007: Retired/revoked key behavior shall fail closed when such records are introduced.
EDP-VR-008: Algorithm mismatch shall fail before backend invocation.
EDP-VR-009: Backend unavailable/error shall propagate to lifecycle signature failure.
EDP-VR-010: Lifecycle integration shall prove no bypass of trusted-key resolution or the signature adapter.
EDP-VR-011: O0/O2 or equivalent build-diversity checks shall preserve security outcomes where the selected provider/build model permits.
EDP-VR-012: Production-link validation shall prove test backend symbols are absent from the production artifact.

## 6. Assurance and audit requirements

EDP-AS-001: Requirements, implementation, tests, and evidence shall be traceable bidirectionally.
EDP-AS-002: Human readability review shall pass before promotion.
EDP-AS-003: Devil's Advocate review shall challenge algorithm confusion, key substitution, provider substitution, downgrade, bypass, and evidence-tampering scenarios.
EDP-AS-004: Technical Destruction shall include malformed, boundary, failure, and negative cryptographic cases supported by the chosen provider.
EDP-AS-005: Compliance/certification review shall distinguish mapping from conformance/certification.
EDP-AS-006: Evidence completeness review shall return PASS, FAIL, or HOLD.
EDP-AS-007: Reused evidence is valid only when all locked inputs remain unchanged.

## 7. Standards/assurance mapping labels

The implementation evidence package shall include non-certifying mappings to the currently verified baseline as applicable:

- NIST SP 800-218 SSDF v1.1;
- NIST SP 800-193;
- NIST SP 800-160 Vol. 1 Rev. 1;
- FIPS 186-5 / SP 800-186 for algorithm baseline;
- CISA Secure by Design / Product Security Bad Practices;
- NASA-STD-8739.8B assurance/IV&V concepts;
- IEEE 1012-2024 V&V concepts;
- FAA AC 20-115D / EASA AMC 20-115D assurance concepts;
- EU CRA applicability analysis;
- NIS2 entity applicability analysis where relevant;
- EU AI Act only for the future AI advisory plane;
- ISO/IEC 9899:2024 language baseline.

## 8. Promotion gate

The production Ed25519 gap remains HOLD until implementation, provider provenance, key provenance, positive/negative/hostile tests, integration evidence, and independent review are complete.

NEXT_BOUNDARY = HUMAN_M17_PRODUCTION_ED25519_PROVIDER_IMPLEMENTATION_AUTHORIZATION