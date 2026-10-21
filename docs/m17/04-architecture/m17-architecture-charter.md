> Guardian F401 M17 candidate artifact.
>
> Status: CANDIDATE / NOT YET NORMATIVE.
>
> Registration does not grant approval.
> Documentation does not grant implementation.
> Implementation does not grant validation.
> Validation does not grant authority.
> Authentication does not grant authority.
> Authority does not automatically grant actuation.
>
> AI/advisory != authority != actuation.
>
> Unknown or ambiguous authority context fails closed.
>
> No certification, regulatory conformity, production-readiness,
> hardware-qualification or external-approval claim is made here.
# M17 Architecture Charter

## Architectural objective

M17 shall demonstrate heterogeneous corroboration and distributed evidence
without decentralizing physical authority by accident.

## Node model

### Node 1 - STM32F401

Role:

DETERMINISTIC_REFERENCE_CONTROL_NODE

Primary properties:

deterministic local policy;
bounded control;
reference semantics;
physical authority only through explicitly governed capability.

### Node 2 - MCXN947

Role:

INDEPENDENT_SUPERVISORY_PEER

Primary properties:

independent supervision;
challenge;
corroboration;
disagreement reporting;
evidence preservation;
bounded recovery support.

No automatic direct actuation authority.

### Node 3 - second ST device

Provisional role:

ST_HETEROGENEOUS_REFERENCE_OR_MEASUREMENT_NODE

Purpose:

test interoperability across a different ST platform;
support richer physical acquisition or independent reference behavior;
avoid treating same-vendor portability as universal portability.

Exact device:

NOT_SELECTED

### Node 4 - security witness

Provisional role:

SECURITY_WITNESS_AND_EVIDENCE_NODE

Purpose:

independent evidence;
identity;
signature verification;
witnessing;
security-state corroboration;
possible protected key functions if supported and later approved.

Exact device:

NOT_SELECTED

Witness status does not grant actuation authority.

### Node 5 - high-compute processing

Provisional role:

HIGH_COMPUTE_SIGNAL_PROCESSING_NODE

Purpose:

spectral analysis;
multi-sensor correlation;
signal processing;
diagnostic computation;
future advisory acceleration.

Exact device:

NOT_SELECTED

Compute power does not grant authority.

## Physical actuation

Physical actuation remains behind a deterministic, separately governed gate.

No single remote node may acquire actuator authority from:

authentication;
freshness;
message validity;
semantic compatibility;
quorum membership;
witness status;
compute capability;
AI recommendation;
peer failure;
peer silence.

## Multi-node semantic objects

M17 shall govern versioned semantics for applicable objects including:

Observation
Evidence
Intent
Recommendation
PolicyDecision
Authorization
Witness
Capability
ExecutionResult
Fault
RecoveryState

## Distributed failure model

M17 shall explicitly define behavior for:

link loss;
asymmetric communication;
partition;
split brain;
duplicate messages;
reordering;
stale node restart;
conflicting observations;
malicious authenticated peer;
Byzantine behavior;
quorum disagreement;
witness disagreement;
persistent-state corruption;
power interruption;
recovery after isolation.

## Quorum

Quorum is a policy input.

Quorum is not actuation authority.

Different decisions may require different evidence sets.

Not every node votes.

Not every vote has equal meaning.

## Failover

Failover is explicit and capability-scoped.

Silence does not authorize takeover.

Fault does not authorize takeover.

Majority does not authorize takeover.

A failover transition must prove its policy, preconditions, authority scope,
expiry and recovery behavior.

## Chip selection

Exact devices shall be selected through a governed technical matrix.

Required candidate dimensions include:

manufacturer;
part number;
lifecycle;
silicon revision;
datasheet;
reference manual;
errata;
CPU;
memory;
MPU/MMU;
TrustZone or equivalent;
secure boot;
root material;
key storage;
cryptographic accelerator;
TRNG;
debug lifecycle;
watchdog;
brownout behavior;
ADC/DMA/timers;
communications;
SDK;
compiler;
debugger;
CI feasibility;
simulator/emulator support;
availability;
licensing;
supply chain;
SBOM feasibility;
semantic-contract feasibility;
physical-validation feasibility.

Performance alone is insufficient.

Security marketing alone is insufficient.

## M17 architecture release gate

M17 shall not claim heterogeneous multi-node assurance until:

node identities are explicit;
roles are explicit;
capabilities are machine-readable;
semantic profiles are versioned;
compatibility is explicit;
authority boundaries are mechanically tested;
persistent freshness is adjudicated;
partition behavior is defined;
split-brain behavior is defined;
failover is bounded;
evidence is attributable;
hostile tests exist;
physical limitations are declared;
claims are linked to evidence.
