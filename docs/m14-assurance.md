# M14 assurance software slice

## Status

Classification: `CONFIRMED_BY_EVIDENCE` within the bounded software-only scope
described here.

The M14 branch adds a deterministic validation and canonicalization component.
It is an early implementation slice of the planned Distributed Guardian System
Architecture; it is not the complete M14 system-of-systems architecture.

Validated branch tip:
`072446f8e0d633095406bb34601a6e3f3d2836e0`.

## Implemented scope

- Strict UTF-8 JSON input processing with a 65,536-byte raw-input bound.
- Duplicate-member rejection before semantic acceptance.
- A closed top-level assurance envelope with domain separation.
- Closed, versioned observation, decision and witness payload contracts.
- Bounded depth, node, object-member, array-item and decoded-string rules.
- JSON-integer-only policy with safe-integer boundaries; booleans and
  floating-point representations are rejected.
- RFC 8785 canonicalization through the locked `rfc8785==0.1.4` dependency.
- Stable classified validation failures.
- Read-only GitHub Actions validation on CPython 3.12.

## Authority boundary

Canonicalization and schema validity establish byte and structural properties.
They do not establish authenticity, freshness, authorization, safety or
permission to perform a physical action. AI output remains untrusted input.
No component in this slice can authorize actuation, failover, firmware update,
trust enrollment, shutdown or cross-plant control.

## Evidence summary

- M14-S1 commit: `e3f209d7b7aa2ca2f15ba61398fcf9d9b8797d25`.
- M14-S2 commit: `4349aa37489df1364b03a08aa610c0d0ffa9c795`.
- Python 3.12 workflow commit:
  `3e862ab7699abca30c9bc5582d0d160142901e15`.
- Mechanical EOF correction commit:
  `072446f8e0d633095406bb34601a6e3f3d2836e0`.
- Local validation on the final commit: 87 of 87 tests passed.
- GitHub Actions run `31983989907`: success on the final commit.
- Final remote-evidence manifest SHA-256:
  `0E3715DF6F80FECD9C804F0FA87FCF9AC22EFE917EFAC18F3E8C902B7C3B626A`.

See [validation evidence](validation/m14-assurance-validation.md) and the
[evidence index](evidence/m14-evidence-index.md).

## Explicit limitations

- Physical STM32F401CDU6 qualification remains pending.
- Firmware memory use, timing and compiler behavior are not established by the
  Python evidence.
- The Java oracle is bounded; it is not proof of full cross-language
  equivalence.
- Structural limits are enforced after JSON materialization, except for the raw
  input bound.
- Evidence digests identify bytes; they do not prove provenance or trust.
- No production, functional-safety or cybersecurity certification is claimed.
- The three-controller simulation, trust-boundary architecture, deterministic
  failover/recovery state machine and complete M14 ADR package remain open
  architecture gates.

## Related documents

- [Software architecture](architecture/m14-assurance-software-slice.md)
- [Threat model](threat-models/m14-assurance-threat-model.md)
- [Decision record](adr/ADR-M14-001-canonical-envelope-and-closed-payloads.md)
- [Validation](validation/m14-assurance-validation.md)
- [Open M14 gates](research/m14-open-gates.md)
