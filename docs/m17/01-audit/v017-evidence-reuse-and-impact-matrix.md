# Guardian F401 v0.17.0 Evidence Reuse and Impact Matrix

Status: CONTROLLED CANDIDATE
Baseline: Closure Gate 01

## Evidence reuse policy

PREVIOUS_PASS + SAME_INPUT_HASHES = EVIDENCE_REUSED
PREVIOUS_PASS + CHANGED_DEPENDENCY = IMPACT_REVALIDATION_REQUIRED
UNCHANGED_COMPONENT = DO_NOT_REPEAT_TEST
NEW_COMPONENT = FULL_LIFECYCLE_REQUIRED

Evidence reuse is mechanical, not conversational. Chat history, operator memory, or a previous claim is not a substitute for archived evidence plus hash-locked dependencies.

## Primary inherited evidence anchors

| Evidence anchor | Demonstrated scope | Reuse condition |
| --- | --- | --- |
| V9 evidence SHA 8CA160D7E6DF9981871E6D7B8705725907DC8D7C263E14FE62611021776B4564 | M17 multinode, Signed Evidence, NodeLink selective assurance and gap adjudication lineage. | Reuse only while affected dependency hashes remain unchanged. |
| V17 evidence SHA DDD9809518200E6AB2B5E3E5B8FBC1349335623323988094E782F34F14176E19 | Monocypher 4.0.3 production Ed25519 provider, RFC 8032, hostile tests, O0/O2, real adapter wiring host evidence. | Revalidate if provider, adapter contract, key-provider ABI, compiler assumptions or build inputs change. |
| V18 evidence SHA F102DE9FC3E4314E02BBDA2552810DFBC93A03556FF903AC3175C1434A4FB20C | Production trust-anchor provenance policy and key2 LEGACY_UNVERIFIED fail-closed gate. | Revalidate if trusted-key record/provenance policy changes. |
| V19 evidence SHA BB2B914F2C7F81C900EE57B66E9DB7C1D4F562B2EA9B463FF8A879E3E4A50A76 | Trusted-signature adapter enforcement and lifecycle rejection of unverified key2. | Revalidate if provider resolution, adapter, lifecycle signature path or provenance state changes. |
| V20 evidence SHA 4475B48E412BC7F088979EA87730A93C08D62477BB095807C50E6A42F449C500 | Production-key ceremony plan, role model, secret-handling contract and evidence schema. | Reuse until ceremony plan/schema/requirements/architecture hashes change. |

## Current demonstrated components

- Multinode role/authority semantics: DEMONSTRATED at host assurance scope.
- Signed Evidence: DEMONSTRATED at host assurance scope.
- Semantic binding: DEMONSTRATED.
- Freshness: DEMONSTRATED at existing host/contract scope.
- Persistence: DEMONSTRATED at existing host/contract scope.
- Rollback-anchor host contract: DEMONSTRATED; physical anchor NOT_DEMONSTRATED.
- Persistent anti-replay host contract: DEMONSTRATED; initial provisioning NOT_DEMONSTRATED.
- Production Ed25519 provider source and host behavior: DEMONSTRATED.
- Production trust-anchor policy: DEMONSTRATED at host scope.
- Adapter provenance enforcement: DEMONSTRATED at host scope.
- Key2 production eligibility: NO; provenance LEGACY_UNVERIFIED.
- Production key ceremony: PLAN MATERIALIZED; execution NOT_DEMONSTRATED.

## Impact examples

A change to guardian_firmware_trusted_key_provider.h requires impact review of provider tests, adapter builds, lifecycle integration and any test provider using the public record type.

A change only to ceremony documentation does not justify rerunning multinode, Signed Evidence or persistence tests.

A change to the NodeLink rollback-anchor backend requires persistence/anti-replay impact tests but does not automatically require rerunning Ed25519 vectors if crypto dependencies are unchanged.

A change to authority semantics requires a wider impact review because control, multinode, failover, Signed Evidence acceptance and release claims may depend on it.