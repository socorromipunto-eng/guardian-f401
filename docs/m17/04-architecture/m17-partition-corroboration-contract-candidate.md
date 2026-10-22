# M17 partition and corroboration contract — candidate

Status: CANDIDATE_NOT_APPROVED. Architecture-derived requirements, not regulatory conformance. Gate13 and Gate14 remain OPEN. No firmware implementation is supplied by this document.

## Authority and sources

Basis: m17-architecture-charter.md; v017-firmware-assurance-architecture.md Gates13/14; v017-release-closure-requirements.md V017-CR-040..043. Existing EU candidate rows EU-M17-005/009/013 provide contextual hazard-related objectives only. Legal applicability, current official source chain and physical conformity are not adjudicated by this contract. NASA/CISA/aeronautical references do not certify this design.

The STM32F401 remains the deterministic reference/control role. MCXN947 is supervisory. Measurement, witness and high-compute peers remain non-actuating. Exact additional chips remain NOT_SELECTED. Portable roles do not prove five physical devices or mutually independent implementations.

## Bounded increment

Design a portable evidence-assessment component, separate from supervisor heartbeat health. It evaluates a fixed configured peer set and returns an assessment, reason flags and candidate eligibility for further local policy review. It shall never issue actuator commands, change capabilities, provision identities, implement takeover, or declare physical safety.

This increment is neither a Byzantine consensus protocol nor a leader-election protocol. It detects declared/local conflicts; it cannot prove the absence of a partition or malicious agreement outside its view.

## Inputs and assumptions

A validated configuration binds a membership generation, policy identity, semantic profile and fixed peer slots. Maximum capacity shall be selected explicitly before implementation and shall not exceed the current registry capacity without a separate change. Required role coverage and count threshold are explicit; a threshold never replaces required roles. Duplicate identities, unknown roles, zero/out-of-range threshold and inconsistent required masks reject configuration.

Each peer record binds identity, membership generation, policy/profile, boot epoch, sequence, round identity and a canonical observation commitment. Upstream authentication and persistent freshness are independently checked services. Boolean acceptance flags passed by a host test are test assumptions, not cryptographic proof. The new component must not present these flags as signature verification.

Accepted evidence belongs to one exact round and interpretation context. Distinct commitments in that context mean a reported conflict, not proof that either peer is correct. Same commitment only demonstrates agreement over the committed bytes. Equivalent quantities with different encoding require a separately approved semantic canonicalization contract.

A trusted local monotonic clock determines evidence age. Clock rollback, arithmetic overflow, unknown clock validity or unknown freshness yields denial of positive eligibility. No reliance on unsynchronized peer wall clocks. Timeout values and acceptable skew are NOT_SELECTED until the intended deployment and hazard analysis define them. Tests use explicit synthetic limits only.

## State model

| State | Meaning | Positive eligibility |
| --- | --- | --- |
| UNCONFIGURED | No validated membership/policy | Denied |
| INSUFFICIENT | Required evidence or role coverage missing, expired or asymmetric | Denied |
| CORROBORATED | All configured evidence predicates hold for one context | Eligible only for separate deterministic review |
| CONFLICT_LATCHED | Equivocation, conflicting commitments or contradictory authority claims observed | Denied |
| REJOIN_REQUIRED | Reset, membership/epoch mismatch or isolation recovery needs reconciliation | Denied |

Initial state is UNCONFIGURED. Successful configuration starts INSUFFICIENT, never CORROBORATED. A query reevaluates time-dependent predicates; no stale cached CORROBORATED result survives age expiry. A local link-loss report removes affected evidence and reevaluates eligibility. Asymmetric communication cannot count as bidirectional availability without attributable acknowledgement bound to the same round.

A late, replayed, malformed or unauthenticated record is rejected without advancing its freshness cursor or overwriting accepted evidence. Explicit loss accounting is separate: rejecting a packet does not itself prove disconnection or grant authority.

A valid conflicting record latches conflict even when a numerical majority agrees. Repeated identity reports count once; equivocation by one identity is not a second voter. A witness conflict is not silently outvoted. An authoritative-looking claim from a non-control peer cannot mint authority.

## Transitions and recovery

| Event | Transition obligation |
| --- | --- |
| Complete matching fresh evidence and required roles | INSUFFICIENT to CORROBORATED |
| Required peer missing/expired; explicit link loss | CORROBORATED to INSUFFICIENT |
| Accepted conflict/equivocation | Any configured state to CONFLICT_LATCHED |
| Reset or changed membership/context | REJOIN_REQUIRED; old-round evidence cannot be reused |
| More matching packets after conflict | Remain CONFLICT_LATCHED |
| Reconciliation approved for a new context | New empty round, INSUFFICIENT; collect fresh evidence again |

Conflict clearing/rejoin requires a separately governed reconciliation decision binding issuer authority, reason, membership generation and new context. An arbitrary boolean or automatic timeout must not be described as authorized recovery. Verification and persistent storage of that decision are integration dependencies; until available the production path remains denied. Recovery must not erase the finding/evidence record. No hot failover is implemented in this increment.

## Output and integration boundary

Assessment output identifies context, source identities, age/coverage, conflicting commitments and reason codes. Errors produce no positive eligibility. APIs shall not return an actuator permit. Even CORROBORATED is insufficient to authorize actuation; local policy, capability, plant conditions and the physical gate remain separate. No action callback, dynamic allocation, recursion, hardware access or networking belongs in this component.

Persisted identity/freshness and witness records use the existing independently reviewed interfaces; no new storage format is approved here. Scheduler bounds, synchronization, thread/interrupt ownership, stack/WCET and physical safe response require later integration evidence. A deployment-specific safe state may differ from simply removing power.

## Required validation before admission

Positive: complete matching evidence, distinct identities, required roles, threshold boundary and exact round/context.
Negative: zero/out-of-range values, duplicate/unknown identity, incompatible profile/policy/generation, replay/reordering, stale/future evidence, invalid clock and bounded arithmetic.
Hostile: majority/minority partitions, asymmetric link, authenticated liar, duplicate identity, witness conflict, stale restart, conflicting control claims and attempted recovery without authorization.
State: timeout invalidates prior eligibility; malformed input preserves accepted state; conflict remains latched; reset/rejoin needs fresh evidence; no packet creates capabilities or takeover.
Technical destruction: remove each denial guard in a separate mutant; tests must detect the change. A mutant killed by compilation alone is not runtime proof of the guard. Record surviving mutants as findings.
Integration: host O0/O2, applicable sanitizers, updated CI source/header dependencies and eventual two-ended target fault injection. All are pending for the new component.

## Reviews and limits

Human Readability: distinguish connectivity, health, corroboration and authority; publish deterministic transition and error precedence tables before C implementation.
Devil's Advocate: malicious peers can agree on false evidence; two vendors may share dependencies; authentication cannot prove truth; local visibility cannot establish global partition absence.
Change Management: new component/header/test and CI impact must be an explicit later candidate. Existing supervisor ABI and policy remain unchanged by this document.
