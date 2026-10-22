# Guardian F401 v0.17.0 Closure Gate Model

Status: CONTROLLED CANDIDATE
Program: M17 / v0.17.0 Closure Program

Each closure gate is a decision boundary, not merely a checklist item. A PASS requires the stated evidence. A HOLD preserves state and does not imply failure of previously demonstrated components.

| Gate | Name | Primary purpose | Firmware / system surfaces | Exit condition |
| --- | --- | --- | --- | --- |
| 01 | Baseline and Evidence Freeze | Freeze the closure operating model, standards, evidence reuse, gaps, and claims. | Governance/docs only. No firmware mutation. | Closure artifacts mutually consistent; V20 baseline locked; next boundary explicit. |
| 02 | Production Key Ceremony Environment Selection | Select a real ceremony environment profile without generating a key. | No firmware mutation. Key management architecture and evidence only. | Selected environment has documented trust boundary, entropy source, operator controls, auditability, and no unsupported validation claim. |
| 03 | Ceremony Environment Qualification | Mechanically qualify the selected environment and capture tool/module identity. | No production key yet. | Environment identity/version/configuration/evidence locked; limitations adjudicated. |
| 04 | Ceremony Readiness / Roles / Dual Control | Bind distinct roles, approvals, recovery rules, and abort conditions. | Governance only. | Ceremony packet complete; dual-control and evidence recorder/observer roles demonstrated. |
| 05 | Production Key Generation Ceremony | Generate a new production key under the approved environment. | Trusted-key source remains unchanged until later admission. | New public key and ceremony evidence exist; private material never enters repository or target. |
| 06 | Public Key Capture and Proof of Possession | Prove possession without exposing private key. | Ed25519 production provider and test harness used for verification. | Domain-separated proof verifies; public-key digest and transcript captured. |
| 07 | Provenance Evidence Finalization | Complete custody, generation, environment, approvals, and transcript evidence. | Governance/provenance artifacts. | Evidence schema complete and internally consistent; no secret leakage. |
| 08 | Production Trust Anchor Admission | Admit only the newly proven key into production trust resolution. | guardian_firmware_trusted_key_provider.*, adapter impact tests. | New key is PRODUCTION_APPROVED with nonzero evidence digest; legacy key2 remains non-promoted; hostile substitution tests pass. |
| 09 | Production Crypto End-to-End Validation | Validate lifecycle -> trust -> real Ed25519 using the approved production public key. | firmware lifecycle, trusted-key provider, signature adapter, Ed25519 provider. | Positive, negative, hostile, O0/O2 and no-bypass behavior pass. |
| 10 | Persistent Anti-Replay Initial Provisioning | Close first-boot/provisioning semantics. | NodeLink persistent anti-replay, persistence, provisioning state. | Initial state cannot be confused with continuation; replay/regression fails closed. |
| 11 | Physical Rollback Anchor Backend | Replace host-only anchor assumption with target-backed monotonic evidence. | STM32F401 platform backend, rollback anchor interfaces. | Target-backed anchor read/advance/compare path demonstrated; rollback resistance claim bounded to evidence. |
| 12 | Hardware Power-Loss / Fault Validation | Demonstrate persistence safety under interrupted operations. | persistence backend, anti-replay, rollback anchor, target storage. | Defined cut points exercised; torn/corrupt/uncertain states fail closed and recover only by governed policy. |
| 13 | Quorum / Partition / Split-Brain Safety | Close distributed safety semantics. | MultiNode core and supervisory peers. | Minority/partition/split-brain cases cannot create authority or unsafe actuation. |
| 14 | Bounded Failover | Demonstrate failover without authority escalation. | MultiNode supervisor, reference-control handoff constraints. | Failover is bounded, observable, reversible where designed, and cannot bypass STM32F401 authority policy. |
| 15 | Firmware Update / Boot / Recovery Cross-Integration | Join signing, rollback, boot, update, and recovery into one lifecycle. | Firmware lifecycle, boot/recovery/platform integration. | Update acceptance, boot confirmation, rollback and recovery maintain signature/trust/version invariants. |
| 16 | M17 CI Integration | Make changed assurance surfaces reproducible in CI. | .github/workflows and build/test orchestration. | CI runs required positive/negative/hostile/strict builds and does not silently omit release-critical tests. |
| 17 | Supply-Chain / Release Integrity | Close third-party, compiler/tool, provenance, SBOM, manifest and release integrity. | ThirdParty provenance, notices, build metadata, release manifest. | Exact source/tool identities and hashes captured; provider substitution and stale artifact paths fail closed. |
| 18 | Physical Hardware Qualification | Collect target evidence for the actual intended hardware configuration. | STM32F401 target and declared heterogeneous hardware as applicable. | Hardware evidence exists for claimed functions; candidate hardware is not called final without evidence. |
| 19 | Full Assurance / Claims / Adversarial Release Review | Challenge every release claim against evidence. | Whole release candidate. | Devil's Advocate, claim-to-evidence, standards-language and unresolved-risk review pass. |
| 20 | v0.17.0 Release Closure | Adjudicate release readiness without Git publication side effects. | Whole release candidate and release evidence. | All critical domains DEMONSTRATED or evidenced NOT_APPLICABLE; release decision PASS; next Git boundary is stage authorization. |

## Gate runner contract

Every gate runner shall contain twenty internal steps, exact current-state locks, allowed/prohibited mutations, one human token per mutation boundary, impact-based testing, fail-closed handling, archive/evidence output, and the exact next human boundary.

A gate may finish HOLD without invalidating evidence from unrelated unchanged components.