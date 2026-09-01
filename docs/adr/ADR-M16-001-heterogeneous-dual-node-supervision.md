# ADR-M16-001 — Heterogeneous Dual-Node Supervision

Status: CANDIDATE

Milestone: M16

Decision Type: Architecture / Security / Cyber-Physical Assurance

Target: Guardian F401 + NXP MCXN947 development target

Depends on:
- ADR-M15-001 — Node Identity and Signed Assurance Messages
- M15 signed-transcript and governance decisions
- Guardian Protocol v0.1 remains a separate host/device protocol

## Context

Guardian F401 v0.15.0 establishes deterministic embedded behavior, bounded
assurance, semantic claim/evidence validation and explicit separation between
advisory information, authority and actuation.

M16 begins the transition from one Guardian node to a heterogeneous supervisory
pair.

The two processors shall not be treated as interchangeable replicas.

The initial architectural roles are:

- STM32F401: primary deterministic Guardian node.
- NXP MCXN947: independent supervisory node and future security-capable
  heterogeneous peer.

The NXP target is an engineering development target, not evidence of physical
hardware validation. No physical NXP behavior is claimed by this ADR.

## Decision

M16 shall introduce a separate node-to-node protocol named Guardian NodeLink.

NodeLink shall not silently extend or redefine Guardian Protocol v0.1.

Guardian Protocol v0.1 remains the existing host/device command and telemetry
contract.

NodeLink is a distinct trust domain for Guardian-node supervision.

## Architectural invariants

The following implications are prohibited:

authenticated == authorized

fresh == truthful

supervisor_present == authority_granted

signature_valid == actuation_authorized

node_disagreement == unilateral_authority

A supervisor may observe, challenge, corroborate, reject or recommend.

A supervisor does not gain actuator authority merely because it is
cryptographically authenticated or operationally healthy.

## Initial M16 state model

Both nodes use the bounded state vocabulary:

- BOOT
- DISCOVERING
- ACTIVE
- DEGRADED
- SAFE_HOLD
- FAULT

State transitions are local deterministic policy decisions. A remote state
claim is evidence about the remote node; it does not directly force the local
node into the same state.

## Initial NodeLink messages

M16 Block 0 defines:

- HELLO
- CHALLENGE
- RESPONSE
- HEARTBEAT
- HEALTH
- SUPERVISION_STATE
- ERROR

The Block 0 wire contract provides integrity and bounded parsing only.

Cryptographic node authentication is a separate provider boundary inherited
from M15 and is not claimed implemented by this candidate.

## Sequence and freshness boundary

NodeLink Block 0 carries a 32-bit sequence value.

The reference implementation can enforce strict session-local monotonic
ordering.

That mechanism is not persistent freshness.

Reset, epoch persistence and replay across reboot require a later explicitly
adjudicated freshness mechanism.

Therefore:

strict sequence ordering != persistent anti-replay

## Failure behavior

At minimum, later M16 policy shall explicitly address:

- lost heartbeat;
- duplicate or regressing sequence;
- malformed frame;
- CRC failure;
- unknown message type;
- incompatible protocol version;
- remote reset;
- disagreement between F401 and NXP;
- supervisor loss.

Loss or disagreement must result in a deterministic bounded state.

No silent unilateral authority escalation is permitted.

## Transport boundary

NodeLink core logic is transport-independent.

Host simulation is the first validation environment.

UART, SPI, CAN/CAN-FD or another physical transport may be selected later
through a platform adapter without changing NodeLink semantics.

## Non-claims

This candidate does not demonstrate:

- physical NXP execution;
- physical dual-node timing;
- hardware fault tolerance;
- production key custody;
- persistent anti-replay;
- secure-element integration;
- certification;
- production readiness;
- distributed quorum;
- AI advisory operation.

## Consequence

M16 can develop the shared protocol, state model and negative/fault tests in
host simulation before either MCU-specific transport is allowed to define the
architecture.
