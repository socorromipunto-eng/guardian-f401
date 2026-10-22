# M17 Production Key Provenance and Trust Anchor Architecture

Status: CONTROLLED_DRAFT

## Objective

Add a deterministic production trust-anchor gate between generic key inventory and any future production signature-verification path. The gate prevents historical presence of a public key from being promoted into production trust without provenance evidence.

## Current adjudication

key_id=2 is present in the trusted-key inventory and remains ACTIVE for generic inventory compatibility. Its production provenance is NOT_DEMONSTRATED. It is therefore classified LEGACY_UNVERIFIED and is not production eligible.

## Resolution model

GENERIC_LOOKUP
  -> key exists
  -> active state
  -> algorithm match
  -> record may be returned for inventory/test analysis

PRODUCTION_RESOLVE
  -> GENERIC_LOOKUP PASS
  -> provenance_state == PRODUCTION_APPROVED
  -> provenance_digest is nonzero
  -> only then return production trust-anchor record

Any failure returns no key record.

## Provenance evidence expected before promotion

A future key ceremony or equivalent governed package shall bind at minimum:

- key identifier;
- Ed25519 public key bytes and SHA-256;
- generation mechanism and tool/provider identity;
- generation environment and custody boundary;
- private-key non-exportability or export policy, as applicable;
- operator/approver identities appropriate to the ceremony;
- timestamps and evidence hashes;
- public-key derivation verification;
- rotation, retirement, revocation, destruction, and recovery rules;
- explicit production approval decision.

No private key is required or permitted in this repository to satisfy this architecture.

## Authority semantics

VALID_SIGNATURE != TRUSTED
TRUSTED != AUTHORIZED
AUTHORIZED != ACCEPTED
ACCEPTED != ACTUATION_AUTHORIZED

The STM32F401 remains the reference/control authority. A production trust anchor can authorize signature trust evaluation only; it does not grant actuator authority.

## Boundary status

The production trust-anchor policy API and fail-closed behavior may be host-demonstrated in this boundary. Actual production key provenance remains NOT_DEMONSTRATED until a separate ceremony/evidence boundary occurs. Adapter integration is separate and must not be inferred from provider-gate implementation.
