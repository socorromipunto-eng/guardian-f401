# M14 assurance software-slice threat model

## Assets

- Integrity and deterministic interpretation of assurance messages.
- Separation between message validity and action authority.
- Stable domain and schema meaning.
- Reproducible evidence tied to an exact commit.

## Trust boundaries

All input bytes, payload claims, evidence digests and AI-produced content are
untrusted. The Python runtime, locked dependency, CI runner and source-control
pipeline are dependencies, not unconditional roots of trust.

## Threats and implemented controls

| Threat | Control | Residual risk |
|---|---|---|
| Oversized input | pre-parse raw-byte bound | post-parse expansion still exists within the bound |
| Invalid encoding | strict UTF-8 decode | valid but adversarial Unicode remains possible in unrestricted text |
| Duplicate JSON members | duplicate rejection before semantic validation | parser equivalence outside tested implementations is not fully established |
| Schema confusion | closed schemas and exact object-type dispatch | validity does not establish authorization |
| Domain confusion | explicit domain separation | domain registry governance remains future work |
| Numeric ambiguity | integer-only safe range; float, boolean and negative-zero rejection | future schema changes require new vectors |
| Resource exhaustion | depth, node, member, array and string bounds | most structural checks occur after materialization |
| Non-deterministic serialization | RFC 8785 and repeated determinism vectors | dependency and runtime supply chain remain dependencies |
| CI mutation or credential abuse | `contents: read`, no secrets, immutable action SHAs | runner image and upstream actions retain supply-chain risk |
| Evidence substitution | commit identity, manifests and SHA-256 records | hashes do not establish provenance by themselves |

## Critical non-controls

This slice does not implement signatures, certificate validation, trusted time,
nonces, revocation, quorum, anti-rollback, secure boot, device identity,
attestation verification or actuator interlocks. Those capabilities must not be
inferred from canonical JSON or a passing schema test.

## Abuse cases

1. A correctly structured malicious decision requests an unsafe action.
   Result: structural acceptance must not bypass deterministic policy or human
   authority.
2. An attacker replays old but valid canonical bytes.
   Result: this slice alone cannot establish freshness; a higher-layer protocol
   must reject stale evidence.
3. A valid digest references forged or unavailable evidence.
   Result: the digest is only an identifier until provenance and availability
   are independently verified.
4. A compromised CI dependency returns malicious behavior.
   Result: immutable references and hashes reduce drift but do not eliminate
   supply-chain compromise.
5. Software tests are presented as hardware qualification.
   Result: documentation explicitly blocks that interpretation.

## Required future destruction tests

- Streaming or pre-materialization resource enforcement.
- Independent full-language oracle.
- Signed identity and freshness validation.
- Replay, delay, reordering and evidence-fork scenarios.
- Split-brain, verifier compromise and network-partition behavior.
- Hardware memory, timing, electrical and fault-injection validation.
