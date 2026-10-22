# M17 Production Key Provenance and Trust Anchor Requirements

Status: CONTROLLED_DRAFT

## Scope

This requirement set governs promotion of an Ed25519 public verification key into the Guardian F401 production firmware-signing trust path. It does not authorize generation, storage, use, export, or recovery of a private signing key on the STM32F401 target.

## Security semantics

- A cryptographically valid signature does not by itself establish trust.
- A trusted key does not by itself establish authorization.
- Authorization does not by itself establish acceptance or actuation authority.
- Unknown provenance fails closed for production trust-anchor resolution.
- Generic key lookup and production trust-anchor resolution are different operations.

## Requirements

PKP-RQ-001: Production trust-anchor resolution shall require an ACTIVE key record.

PKP-RQ-002: Production trust-anchor resolution shall require an exact algorithm match.

PKP-RQ-003: Production trust-anchor resolution shall require provenance_state=PRODUCTION_APPROVED.

PKP-RQ-004: Production trust-anchor resolution shall require a nonzero 32-byte provenance digest bound to governed evidence.

PKP-RQ-005: A LEGACY_UNVERIFIED, UNKNOWN, RETIRED, or REVOKED key shall not resolve for production use.

PKP-RQ-006: The current key_id=2 shall remain LEGACY_UNVERIFIED until an independently governed key ceremony or equivalent provenance package demonstrates origin, generation mechanism, custody, public-key derivation, approval, and evidence hashes.

PKP-RQ-007: The production firmware target shall contain public verification material only; private signing-key custody is outside this target boundary.

PKP-RQ-008: No fallback key shall be selected after unknown key, algorithm mismatch, inactive state, or unverified provenance.

PKP-RQ-009: Provider substitution, key substitution, provenance-register substitution, and provenance-digest substitution shall require a new governed impact boundary.

PKP-RQ-010: Production-key promotion shall not claim FIPS, CMVP, airborne, CISA, NASA, IEEE, EU, or other certification merely because an algorithm, process, or reference framework is used.

PKP-RQ-011: Evidence reuse is permitted only while the bound input hashes remain unchanged.

PKP-RQ-012: Integration of production trust-anchor resolution into the trusted-signature adapter is a separate mutation boundary after this gate is demonstrated.
