# Guardian F401 v0.17.0 Firmware Assurance Architecture

Status: CONTROLLED CANDIDATE
Purpose: map Closure Program decisions to actual firmware/system surfaces without weakening authority or evidence semantics.

## 1. Assurance planes

### Control plane
STM32F401 is the deterministic reference/control authority. No closure gate may implicitly move actuation authority to a peer, test harness, host tool, key provider, signer, witness, AI component, or CI system.

### Trust and update plane
The firmware lifecycle consumes signature verification through the trusted-signature adapter. The adapter must use production trust-anchor resolution. Production trust-anchor resolution must enforce provenance approval and evidence binding before cryptographic verification can contribute to update acceptance.

### Freshness and persistence plane
NodeLink freshness, persistence, persistent anti-replay, and rollback-anchor state form a layered contract. Persistence alone is not freshness; host anchor behavior is not physical rollback resistance.

### Multinode supervision plane
MCXN947 supervisory, ST measurement, security witness, and high-compute peers provide bounded evidence or supervision. Their role cannot be promoted to control authority by quorum, failover, compute capacity, or communication availability.

## 2. Gate-to-firmware solution map

### Gates 02-08 - production key lifecycle
Primary files/surfaces:
- firmware/Firmware/Inc/guardian_firmware_trusted_key_provider.h
- firmware/Firmware/Src/guardian_firmware_trusted_key_provider.c
- firmware/Firmware/Src/guardian_firmware_trusted_signature_adapter.c
- firmware/Firmware/Src/guardian_firmware_ed25519_monocypher.c
- governance/production-key-provenance-register.json

Solution rule: no new key is admitted into production resolution before ceremony evidence is complete and separately authorized. Legacy key2 remains non-production unless independent provenance is ever lawfully demonstrated under a new boundary; the current closure program assumes a new production key id.

### Gate 09 - production crypto end-to-end
Primary files/surfaces:
- firmware/Firmware/Src/guardian_firmware_lifecycle.c
- trusted-key provider
- trusted-signature adapter
- Monocypher Ed25519 backend
- affected host tests

Required solution: a real production-approved public key must reach the real Ed25519 backend through the same path used by lifecycle verification, while wrong key, wrong algorithm, malformed signature, provider substitution, unknown key, revoked/non-active/non-production key and backend failure all fail closed.

### Gate 10 - initial anti-replay provisioning
Primary files/surfaces:
- firmware/NodeLink/Inc/guardian_node_link_persistent_anti_replay.h
- firmware/NodeLink/Src/guardian_node_link_persistent_anti_replay.c
- NodeLink persistence/rollback interfaces

Required solution: explicit first-provision state and evidence. First provision cannot masquerade as continuation. Continuation requires valid prior persisted record plus matching independent anchor and monotonic advancement.

### Gate 11 - physical rollback anchor
Primary files/surfaces:
- firmware/NodeLink rollback-anchor interface
- firmware/Platform/STM32F401 backend/adapters

Required solution: target-backed monotonic or otherwise justified rollback-resistant anchor. Read, advance, read-back, mismatch, unavailable, corrupt, reset and rollback-suspected cases must be evidence-backed. The chosen mechanism must not be called hardware-backed until physically demonstrated.

### Gate 12 - power-loss/fault validation
Primary files/surfaces:
- persistence backend
- anti-replay continuation
- rollback-anchor update ordering
- target storage

Required solution: fault matrix with cut points before/during/after persist, verify, commit, anchor advance and promotion. Uncertain state must not silently become accepted freshness.

### Gate 13 - quorum/partition/split brain
Primary files/surfaces:
- firmware/MultiNode/Inc/guardian_multinode.h
- firmware/MultiNode/Src/guardian_multinode.c
- peer role modules and tests

Required solution: explicit safety state machine for lost peer, conflicting witness, minority partition, delayed evidence, duplicate identity, stale evidence, asymmetric communication and split brain. Quorum may support an observation decision but shall not create actuator authority.

### Gate 14 - bounded failover
Primary files/surfaces:
- multinode supervisor and reference-control integration

Required solution: define what can fail over, what cannot, time/attempt bounds, safe fallback, rejoin policy, stale-state handling and evidence. Failover cannot bypass control authority.

### Gate 15 - firmware update/boot/recovery cross integration
Primary files/surfaces:
- firmware/Firmware lifecycle
- platform boot/recovery hooks
- rollback floor and pending-image persistence

Required solution: one cross-integration contract from signed manifest through staging, verification, activation, boot confirmation, rollback/recovery and final persistent state.

### Gate 16 - M17 CI
Primary files/surfaces:
- .github/workflows
- build/test scripts

Required solution: CI owns deterministic execution of changed release-critical tests and artifact/evidence capture. CI success is not release authorization.

### Gate 17 - supply chain/release integrity
Primary files/surfaces:
- firmware/ThirdParty/monocypher-4.0.3/PROVENANCE.json
- license/notices
- compiler/tool identity evidence
- release manifest/SBOM-style inventory

Required solution: exact source lineage, license, local patch state, tool identity, build-result hash and release-manifest binding.

### Gate 18 - physical hardware qualification
Primary surfaces:
- actual STM32F401 target
- only those heterogeneous devices actually present and evidenced

Required solution: physical evidence for claimed functions. Simulators, host tests, datasheets or candidate selections do not equal target qualification.

### Gates 19-20 - release assurance and closure
Primary surfaces: whole release candidate.

Required solution: bi-directional traceability from requirement to implementation/test/evidence, hostile review, claim firewall, unresolved-risk disposition, final release manifest and explicit human release decision.

## 3. Change-impact rule

Every firmware mutation shall identify:
1. changed files;
2. direct interfaces consumed or provided;
3. dependent tests/evidence;
4. authority/trust/freshness semantics affected;
5. positive/negative/hostile tests required;
6. O0/O2 or target verification required;
7. evidence to inherit unchanged;
8. claims that remain prohibited.

No runner may rerun unrelated assurance merely to increase apparent test volume.