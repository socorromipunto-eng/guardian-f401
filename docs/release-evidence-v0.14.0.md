# Guardian F401 v0.14.0 release evidence

## Classification

Software-only research release evidence. This record does not establish
physical-board qualification, real-time hardware behavior, production
readiness, functional safety, industrial safety or cybersecurity certification.

## Validated M14 baseline

- M14 Pull Request: `#3`.
- Feature head: `03656eeaf27c4efaecf768ef97a196bd8a36cdcb`.
- Merge commit on `main`: `e0cd5d11424de2acdec595e81438a935d38dc0c9`.
- Merge parents: `50ebf92f4aa2cf99e257a5a82e1717354fdacb53` and
  `03656eeaf27c4efaecf768ef97a196bd8a36cdcb`.
- Exact post-merge checks: 9 expected, 9 successful, 0 incomplete, 0 failed.
- Corrective post-merge evidence manifest:
  `321092306757B170D6E722F704AD1B23885E0437C03EA6D52D27ED111FC821BE`.

## Implemented software scope

- RFC 8785 canonicalization using locked `rfc8785==0.1.4`.
- Strict UTF-8 JSON parsing and duplicate-member rejection.
- Domain separation and closed assurance envelopes.
- Closed observation, decision and witness payload contracts.
- Bounded raw input, depth, nodes, members, arrays and strings.
- Safe-integer contract with rejection of booleans, floating point, exponent
  forms and negative zero.
- Exact 87-test Python 3.12 validation workflow.

## Open gates

- Distributed-controller, quorum, fork and recovery implementation.
- Identity, signature, freshness and attestation integration.
- Physical STM32F401 board, electrical, timing and fault-injection validation.
- Production provisioning, safety assessment and certification.

## Publication boundary

The v0.13.0 DOI remains historical and must not be represented as the DOI for
v0.14.0. The v0.14.0 version DOI may be recorded only after Zenodo archives the
immutable GitHub Release and its metadata and files are verified.
