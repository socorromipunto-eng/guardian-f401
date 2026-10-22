# Guardian F401 M17 Production Key Ceremony Architecture

Status: CONTROLLED_DRAFT
Boundary: PRODUCTION_KEY_CEREMONY_PLAN
Private-key generation authorized by this document: NO

## Security objective

Create a future production Ed25519 trust anchor whose provenance, custody, public-key identity, and approval can be mechanically demonstrated without exposing the signing private key to the Guardian repository or STM32F401 target.

## Authority model

Ceremony Authority -> authorizes ceremony stage transitions.
Key Custodians -> jointly control private-key operations under dual control.
Evidence Recorder -> records non-secret evidence and hashes.
Independent Observer -> confirms procedure and evidence without gaining actuation authority.
Machine Gates -> validate schema, hashes, proof of possession, and fail-closed conditions.

No role above grants Guardian actuation authority. The STM32F401 remains the deterministic reference/control authority.

## Ceremony state machine

PLAN_AUTHORIZED
  -> ENVIRONMENT_SELECTED
  -> CEREMONY_READY
  -> KEY_GENERATED
  -> PUBLIC_ARTIFACT_CAPTURED
  -> POSSESSION_PROVEN
  -> EVIDENCE_COMPLETE
  -> HUMAN_APPROVED
  -> PROMOTION_ELIGIBLE

Any missing, ambiguous, substituted, contradictory, or secret-leaking artifact transitions to HOLD_OR_ABORT, never to PROMOTION_ELIGIBLE.

## Environment boundary

V20 does not select an HSM, secure element, offline workstation, cloud KMS, or other generator. Environment selection is a separate human boundary.

Preferred profile:
- private key non-exportable;
- dual-control capable;
- auditable generator/module identity;
- evidence for entropy and key generation;
- explicit backup/recovery policy;
- no repository or target access to private-key bytes.

A hardware-backed or FIPS-validated claim requires direct hardware/module evidence and, for FIPS validation, exact CMVP certificate evidence. MAPPING != CONFORMANCE != CERTIFICATION.

## Data flows

Private path:
Selected ceremony environment -> private Ed25519 key -> remains inside approved custody boundary.

Public path:
Selected ceremony environment -> public key -> SHA-256 -> proof-of-possession verification -> ceremony evidence package -> future provenance admission boundary -> Guardian public trust-anchor material.

The private path shall never flow into the repository, runner logs, evidence JSON, source code, target firmware, or chat transcript.

## Proof of possession

Domain: GUARDIAN-F401:M17:PRODUCTION-KEY-CEREMONY:V1

A public non-secret fresh ceremony nonce is incorporated into a ceremony challenge. The new private key signs the challenge inside its approved custody boundary. The signature, public key, and challenge are non-secret ceremony evidence. Guardian verifies the signature with the real Monocypher Ed25519 provider.

Proof of possession proves key-pair control at ceremony time. It does not alone prove trusted, authorized, accepted, or actuation-authorized state.

## Evidence binding

The ceremony evidence package binds at minimum:
- ceremony plan hash;
- selected-environment record hash;
- generator/module identities;
- entropy evidence hash;
- new key_id;
- public key bytes and SHA-256;
- custody evidence hash;
- proof-of-possession challenge and signature;
- backup/no-backup policy evidence;
- participant-role records;
- two distinct approvals;
- transcript hash;
- canonical provenance digest.

## Fail-closed threats

- operator or custodian substitution;
- generator/module substitution;
- unverified entropy source;
- reuse of key_id 1 or key_id 2;
- public-key substitution;
- private-key export or evidence leakage;
- plaintext backup;
- single-person control;
- stale/replayed ceremony package;
- approval replay or evidence tampering;
- promotion before proof of possession;
- hardware-backed claim without hardware evidence;
- FIPS/CMVP claim without certificate evidence.

## Recovery and rotation

Recovery, wrapped backup, revocation, and rotation are distinct lifecycle operations. No recovery mechanism is considered demonstrated by this plan. Compromise forces revocation and a newly governed key lifecycle; historical keys remain historically identifiable and are not silently repurposed.

## Current adjudication

CEREMONY_PLAN=MATERIALIZED only after V20 passes.
CEREMONY_EXECUTION=NOT_DEMONSTRATED.
PRODUCTION_APPROVED_KEY_PRESENT=NO.
PRODUCTION_KEY_PROVENANCE=NOT_DEMONSTRATED.
key_id 2 remains LEGACY_UNVERIFIED and production-ineligible.
