# Canonical corroboration payload contract candidate

Status: CANDIDATE_NOT_APPROVED. Architecture and portable implementation proposal only.
Baseline: main ancestry 717e53c717af1dc31af540bbcadd9b426f9c9535, working branch feature/m17-heterogeneous-multinode-assurance, 567 tracked/nonignored paths after 05ZA.

## Problem and authority boundary

The existing gc_report contains caller-supplied authentication, persistent-freshness and same-round acknowledgement indicators. Serializing those indicators would let a peer claim that local validation already occurred. This proposal transports observations only; receiving bytes never creates authentication, trust, freshness, quorum eligibility or actuation permission.

STM32F401 remains the reference deterministic authority/control node. Supervisory, measurement, witness and analysis roles gain no authority from this codec. AI remains advisory. Additional physical nodes and manufacturer independence are not demonstrated.

## Source and requirement lineage

Inputs: guardian_multinode_corroboration.h/.c and their host test; m17-corroboration-api-candidate.md; m17-partition-corroboration-contract-candidate.md; m17-partition-requirements-candidate.json; guardian_node_link.h and authentication, semantic and freshness contracts. The runner binds all current inputs through the complete 567-path baseline. The existing 12 partition requirements remain OPEN with no evidence promotion.

Proposed local controls, all OPEN pending admission and wider integration:

- CW-01: fixed canonical byte layout independent of C structure padding and host endianness.
- CW-02: exact-length, canonical version/flag/reserved-byte checks; reject zero context identifiers.
- CW-03: invalid input must not modify the decoded destination or encoded byte buffer; encoded length becomes zero when its pointer is valid.
- CW-04: never serialize authentication, freshness, acknowledgement or permission as a trusted result.
- CW-05: preserve a peer's actuation claim for subsequent policy review; never interpret it as permission.

These controls are research requirements, not EU conformity or certification findings. This proposal changes no normative source, applicability decision or existing gate status.

## Canonical 52-byte payload

| Offset | Length | Field | Encoding and constraint |
| --- | --- | --- | --- |
| 0 | 1 | version | Exactly 1 |
| 1 | 1 | claims_actuation | Exactly 0 or 1; peer claim only |
| 2 | 2 | reserved | Both zero |
| 4 | 4 | generation | Unsigned little-endian, nonzero |
| 8 | 4 | policy | Unsigned little-endian, nonzero |
| 12 | 4 | profile | Unsigned little-endian, nonzero |
| 16 | 4 | round | Unsigned little-endian, nonzero |
| 20 | 32 | commitment | Opaque bytes; meaning and construction require a separate contract |

An all-zero commitment is structurally accepted; this does not establish meaningful evidence. uint32 identifiers have no wrap/reuse policy in this codec. Context allocation and lifecycle must be governed separately.

The size fits NodeLink's current 64-byte payload capacity. This is a size observation only. No NodeLink message type, transport, profile binding or automatic HEARTBEAT reinterpretation is assigned. gc_report native structure bytes MUST NOT be sent directly.

## API and rejection behavior

The API performs no allocation, recursion, storage, signature verification, clock access, provider invocation or actuator access. Encode constructs all bytes locally before copying a complete valid payload. Decode constructs a local observation and assigns the destination only after validation. Encoding failures return INVALID or CAPACITY with zero length; decoding failures return INVALID with destination unchanged.

Callers must provide accessible buffers of the declared capacities and valid object pointers. The output_length object must not alias either input or output storage. Host tests do not qualify arbitrary invalid pointers, memory races or interrupt safety. No thread-safe or DMA-safe shared-buffer ownership is claimed.

## Future integration boundary: not implemented here

A later reviewed ingress adapter must establish sender identity from an authenticated outer frame, validate configured semantic compatibility and locally expected context, enforce the approved epoch/sequence and persistent freshness policy, and obtain acknowledgement evidence through its own contract. Then it may construct gc_report for local review. The exact verification order, storage transaction and failure behavior require a separate architecture decision before implementation.

NodeLink uses uint32 epoch/sequence while gc_report uses uint64: any later conversion must widen without information loss; truncation and implicit epoch admission are prohibited. Missing providers, unknown compatibility, failed persistence, uncertain time or missing acknowledgement must not be replaced with true indicators. Same-round acknowledgement must not be reconstructed from a wire boolean.

Atomicity between persistent cursor advancement and corroboration RAM-state updates remains unresolved. Reconciliation after reset, generation change or split brain is not supplied by this codec. No result from decode can clear a conflict latch, authorize recovery, grant replacement authority or enable actuation.

## Validation and remaining evidence

Local Linux GCC O0 and O2 executions each report 1,445 host cases. Cases exercise a fixed byte vector, endianness, exact lengths, unsupported versions, noncanonical flags, reserved bytes, zero context values, capacity failure, null arguments and uint32 boundary values. Two selected guard-removal mutants (reserved-byte validation and version validation) are tested at both optimizations; results are preserved separately. These are selected falsification checks, not exhaustive mutation coverage.

Windows compilation/execution, target compilation, physical node-to-node operation, production cryptographic provisioning, runtime stack sufficiency and power-loss qualification are NOT_DEMONSTRATED by this proposal. CI execution on this candidate is NOT_PERFORMED. Gate01, Gate13, Gate14 and Gate16 remain open.
