# ADR-M17 - V2 scoped signature verifier candidate

Status: EXTERNAL_CANDIDATE_NOT_ADMITTED. Parent:07P ingress transaction design.
Baseline:0f2d55c722fc74dabba90a53c83a24ec7d62dbb2 on feature/m17-authenticated-ingress-state-machine,585 tracked/nonignored raw paths.

## Decision and implemented candidate scope

Add guardian_signed_envelope_auth.h/.c as a separate V2 receive verifier. Its local peer registry contains up to eight exact sender/producer/key tuples, a32-byte Ed25519 public key, a revoked flag, allowed observation/ACK types and one nonzero registry revision. Configuration copies the local registry. A duplicate tuple, invalid count/revision/discriminant, zero identity or all-zero public key invalidates the configuration; failed reconfiguration does not leave an old valid registry active. Public-key nonzero checking is input hygiene, not key provenance or complete public-key qualification.

The caller supplies received wire bytes, never a preconstructed authenticated object. The module strictly decodes using existing gse_decode, selects one configured tuple, enforces revocation and the local type allowlist, builds the existing canonical V2 transcript and checks the complete Ed25519 signature with vendored Monocypher4.0.3. It uses neither the V1 authentication domain/API nor firmware artifact key authorization. The primitive is directly linked and cannot silently be unavailable through a permissive callback. This provides bounded signature verification against a caller-trusted local registry; it does not qualify production key configuration or all cryptographic dependencies.

Successful output contains the decoded envelope plus registry revision. It is local signature evidence, not an unforgeable C capability. A later integrator must not accept arbitrary caller-supplied evidence structs. It must own the wire-to-commit path and revalidate registry revision/revocation before durable acceptance. No actuation callback or actuator authorization is exposed.

Every verify rejection preserves output bytes and context. Wire may overlap output because input is fully consumed first. Output must not overlap trusted context. Context/registry must remain immutable during verification and be owned by one serialized caller. DMA/race/hostile shared-memory handling and commit-time revocation are later integration dependencies, not established by const-qualified pointers. Dynamic allocation is absent from the candidate path; target stack/time resources remain unmeasured.

## Evidence and test construction

Two fixed signed packets represent canonical observation and ACK payloads. Their public key is RFC8032 test1. The packets were independently assembled and signed locally using Python cryptography50.0.2 with the publicly documented test seed; no production or randomly generated key was used. Only public key, packets/signatures and provenance are distributed. The Windows helper performs verification only and does not generate keys or sign messages.

The host harness checks positive observation/ACK acceptance, complete tuple/key rejection, revocation, local type restrictions, registry capacity/duplicates, invalidated reconfiguration, null/overlap behavior and wrong-domain signatures. Every bit of the two packets is flipped individually:2432 rejection cases. Lengths0..161 excluding each packet's exact valid length give322 rejection cases. The8394 CHECK evaluations include repeated output/context preservation checks; they are not8394 independent attack scenarios.

Local GCC O0/O2 evidence and three guard-removal mutants are recorded separately from the forthcoming Windows Clang/LLD execution. Removing the signature check, tuple match or revocation check must produce a harness assertion failure after successful compile/link. Sanitizer status is recorded explicitly in local-validation.json. Cross-implementation fixture generation is useful evidence but is not an independent qualification of the complete cryptographic module, runner or reviewer.

## Open dependencies and scope limits

This candidate addresses signature and locally configured identity portions of ISM-R01/R02/R03/R04/R05/R07; it does not close their production assurance obligations. ISM-R06 commit-time revision/revocation is still open. Payload semantic compatibility, physical truth, persistent freshness, pending ACK association/consumption, atomic transaction/readback/anchor, corroboration publication and recovery/rejoin are not implemented here. A signature-valid old packet remains cryptographically valid: the future freshness layer must reject it.

No firmware is admitted, no CI recipe or existing source is edited by the external runner, and no target is built/connected/flashed. The new proposed paths are firmware/NodeLink/Inc/guardian_signed_envelope_auth.h, firmware/NodeLink/Src/guardian_signed_envelope_auth.c, firmware/Tests/m17_signed_envelope_auth_host_test.c, firmware/Tests/m17_signed_envelope_auth_vectors.h and this ADR under docs/m17/04-architecture. Admission requires exact Windows evidence review and a separately scoped worktree mutation.

The MITRE mapping in07P remains a planned control interpretation; this component does not demonstrate mitigation of every mapped technique. V2 remains Ed25519 with a64-byte signature; historical ML-DSA artifact research does not make this transport post-quantum. F401 deterministic actuation authority is unchanged; NXP supervision and advisory AI gain no authority. Gates01/13/14/16 and physical/production/legal conformance remain open.
