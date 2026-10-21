# Guardian F401 M17 Production Key Ceremony Requirements

Status: CONTROLLED_DRAFT
Boundary: PRODUCTION_KEY_CEREMONY_PLAN
Execution authorized by this document: NO

## Purpose

Define the requirements for a future production Ed25519 firmware-signing key ceremony. This plan does not generate, import, export, activate, promote, revoke, or rotate any production key.

## Normative and assurance references

- NIST SP 800-57 Part 1 Rev. 5: key-management lifecycle and protection guidance.
- NIST SP 800-133 Rev. 2: cryptographic key-generation guidance.
- FIPS 186-5: EdDSA/Ed25519 algorithm baseline only.
- FIPS 140-3 / CMVP: cryptographic-module assurance reference only; no validation claim is made unless certificate evidence is captured.
- NIST SP 800-218 v1.1 and NIST SP 800-160 Vol. 1 Rev. 1: secure-development and systems-security-engineering references.
- Existing Guardian CISA, NASA, IEEE, FAA/EASA, and EU assurance mappings remain reference benchmarks, not certifications.

## Requirements

KCP-RQ-001: Ceremony planning and ceremony execution shall remain separate human-authorized mutation boundaries.
KCP-RQ-002: The planned signing algorithm shall be Ed25519; algorithm approval shall not imply provider or module validation.
KCP-RQ-003: A new production key shall receive a new key_id and shall not reuse legacy key_id 1 or key_id 2.
KCP-RQ-004: The execution environment shall be selected and authorized in a separate boundary before key generation.
KCP-RQ-005: Hardware-backed non-exportable private-key custody is the preferred execution profile, but shall not be claimed without mechanical evidence.
KCP-RQ-006: Any software-generation fallback shall require an explicit exception boundary and shall not be represented as hardware-backed.
KCP-RQ-007: At least two distinct key custodians shall participate in the ceremony.
KCP-RQ-008: Single-person end-to-end control over generation, custody, evidence, and approval is prohibited.
KCP-RQ-009: The ceremony shall identify a ceremony authority, evidence recorder, and independent observer in addition to the key-custodian function.
KCP-RQ-010: The private key, seed, plaintext backup, PIN, password, or equivalent secret shall never be written to the repository or ceremony evidence.
KCP-RQ-011: The STM32F401 target shall receive only public trust-anchor material; it shall never receive the firmware-signing private key.
KCP-RQ-012: Generator identity, module identity, software/firmware version, and selected environment identity shall be captured as evidence.
KCP-RQ-013: Entropy-source evidence shall be captured before production provenance may be approved.
KCP-RQ-014: A FIPS or CMVP validation claim shall require exact certificate evidence; algorithm use alone is insufficient.
KCP-RQ-015: Exact public-key bytes and SHA-256 shall be captured immediately after generation.
KCP-RQ-016: Proof of possession shall sign a domain-separated public challenge without exporting the private key.
KCP-RQ-017: Proof of possession shall be verified using the real Guardian Monocypher Ed25519 verification provider.
KCP-RQ-018: The ceremony transcript and every admission artifact shall be integrity-bound by SHA-256.
KCP-RQ-019: A canonical provenance digest shall bind the approved ceremony evidence package.
KCP-RQ-020: At least two distinct human approvals shall be required before the key may become promotion-eligible.
KCP-RQ-021: Completion of the ceremony shall not itself promote a key into production; promotion requires a separate authorization boundary.
KCP-RQ-022: Plaintext private-key backup is prohibited.
KCP-RQ-023: Any wrapped backup or recovery mechanism shall require separate policy, custody, and recovery validation evidence.
KCP-RQ-024: Compromise shall force revocation and rotation; compromised key identifiers shall never be silently reused.
KCP-RQ-025: Unknown, incomplete, ambiguous, stale, or contradictory ceremony evidence shall fail closed.
KCP-RQ-026: Historical possession of a public key shall not be promoted into production provenance.
KCP-RQ-027: A valid signature shall not imply trusted; trusted shall not imply authorized; authorized shall not imply accepted; accepted shall not imply actuation-authorized.
KCP-RQ-028: AI/advisory output shall not authorize ceremony execution, key promotion, or actuation.
KCP-RQ-029: Timestamps are transcript metadata and shall not by themselves establish freshness or non-replay.
KCP-RQ-030: The ceremony evidence schema shall prohibit secret-key, seed, PIN, password, and plaintext-backup fields.
KCP-RQ-031: Release readiness shall remain HOLD until a real production key is generated, evidence-complete, approved, admitted, and target integration is independently validated.

## Current state

- Ed25519 provider: HOST_DEMONSTRATED.
- Trusted-signature adapter provenance enforcement: HOST_DEMONSTRATED.
- key_id 2: LEGACY_UNVERIFIED and production-ineligible.
- Production-approved key present: NO.
- Production key provenance: NOT_DEMONSTRATED.
- Production key ceremony execution: NOT_DEMONSTRATED.
