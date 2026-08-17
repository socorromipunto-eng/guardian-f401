# M14 open architecture and research gates

## Current maturity boundary

The validated assurance component is not equivalent to completing the planned
M14 Distributed Guardian System Architecture. The following items remain open.

| Gate | Status |
|---|---|
| System context and bounded-assurance architecture research | approved as input for ADR drafting; not fully implemented |
| Primary, secondary and independent Guardian node model | research architecture only |
| AI as a separate advisory component | required invariant; integration pending |
| Trust-boundary and authority matrix | pending repository-grade finalization |
| Node identity and signed-message model | pending |
| Freshness, replay and evidence lifecycle | pending |
| Detect/isolate/failover/recover state machine | pending |
| Network partition, fork and split-brain behavior | pending |
| Three-controller simulation | pending |
| Complete failure-mode and attack-path campaign | pending |
| Full M14 ADR package | partially proposed; final approvals remain item-specific |
| Physical STM32F401 qualification | pending; no board evidence |

## Research hypotheses

The future three-controller and federated architecture remains a
`RESEARCH_HYPOTHESIS`. The implemented software slice may support deterministic
message handling, but it does not demonstrate distributed consensus,
attestation, independent control, failover or cross-plant assurance.

## Release gate

`VERSION` remains `0.13.0`. Selecting a later version, merging to `main`,
creating a release and updating Zenodo or ORCID are separate decisions that
require complete documentation review, exact merged-commit CI and a release
evidence package.
