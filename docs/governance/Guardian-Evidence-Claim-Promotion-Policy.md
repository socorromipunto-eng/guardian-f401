# Guardian Evidence / Claim Promotion Policy

## 1. Purpose

This policy defines the governance boundary for converting evidence into claim
assessment within Guardian F401.

Its purpose is to prevent evidence presence, test success, generated output,
human interpretation, or AI analysis from silently escalating into stronger
assurance claims, authority, or actuation rights.

This policy is architecture-before-implementation governance.

It does not itself mutate claims or evidence registers.

## 2. Normative separation

The following separations are mandatory:

- evidence presence is not evidence validity;
- evidence validity is not applicability;
- applicability is not sufficiency;
- sufficiency is not claim demonstration;
- claim demonstration is not approval;
- approval is not authority;
- authority is not actuation authorization;
- positive evidence is not absence of contradiction;
- host execution is not physical execution;
- simulator success is not hardware success;
- build success is not behavioral validation;
- AI analysis is not claim-promotion authority.

## 3. Evidence lifecycle

Evidence must be governed independently of the claim it may support.

Evidence must have explicit identity, integrity, provenance, applicability, and
validation semantics.

Evidence must not become valid merely because a file exists.

Evidence must not become applicable merely because it is valid.

Evidence must not become sufficient merely because it is applicable.

## 4. Evidence roles

The minimum evidence roles are:

1. SUPPORTING
2. CONTRADICTORY
3. NEGATIVE
4. INCONCLUSIVE

Contradictory evidence must not be hidden by favorable evidence.

Negative evidence must remain independently discoverable.

Inconclusive evidence must not be interpreted as supporting evidence.

## 5. Claim assessment vocabulary

The bounded technical assessment states are:

- NOT_ASSESSED
- NOT_DEMONSTRATED
- PARTIALLY_DEMONSTRATED
- DEMONSTRATED
- CONTRADICTED
- INVALIDATED

Assessment state describes technical assurance only.

Assessment state does not grant approval or authority.

## 6. Governance adjudication vocabulary

The bounded adjudication states are:

- UNADJUDICATED
- ACCEPTED
- REJECTED
- SUPERSEDED
- WITHDRAWN

Adjudication is separate from assessment.

A claim may be technically demonstrated and remain unadjudicated.

A claim may be adjudicated as accepted and still not grant normative authority.

## 7. Promotion eligibility

A claim is eligible for mechanical assessment only when all required inputs are
known and valid.

Eligibility requires:

- explicit claim identity and version;
- explicit evidence identities;
- explicit evidence requirement rule;
- integrity-valid evidence;
- scope-applicable evidence;
- semantic/version-compatible evidence;
- freshness-valid evidence where applicable;
- evaluated contradictory evidence;
- supported rule/schema version.

Any unknown required input fails closed.

## 8. Sufficiency

Evidence sufficiency must be explicit and versioned.

Sufficiency must not be inferred from evidence count alone.

A large set of weak evidence does not automatically equal one required strong
evidence class.

Evidence classes required by a claim contract must not be substituted silently.

## 9. Physical validation boundary

Claims requiring physical validation must require evidence produced from the
specified physical target class.

The following substitutions are prohibited:

- host test for physical hardware execution;
- simulator result for physical target execution;
- build result for runtime validation;
- static analysis for physical behavioral validation.

Tooling must enforce this boundary mechanically where possible.

## 10. Contradiction handling

Contradictory evidence is a first-class assessment input.

A claim with unresolved contradictory evidence must not remain silently
DEMONSTRATED.

The resulting assessment must fail closed to the bounded state required by the
claim rule, including CONTRADICTED where applicable.

## 11. Freshness

Evidence applicability may expire.

Freshness requirements must be explicit where the claim depends on:

- changing firmware;
- changing hardware;
- changing configuration;
- changing semantic contract;
- changing toolchain;
- changing environment;
- changing threat or trust assumptions.

Stale evidence must not be silently reused.

## 12. Version and semantic applicability

Evidence must be evaluated against the exact semantic and version context of the
claim.

Evidence from one semantic contract, protocol version, hardware revision, or
firmware generation must not silently promote a claim for another.

Mixed-generation evidence requires explicit compatibility rules.

## 13. Mechanical evaluation

Mechanical tooling may compute candidate assessment state.

Mechanical tooling may not persist governance approval autonomously.

Mechanical tooling may not grant authority.

Mechanical tooling may not authorize actuation.

Mechanical evaluation output is evidence for governance decision-making; it is
not the governance decision itself.

## 14. Persistent state transition

Persistent claim transitions require a governed transaction.

The transaction must bind:

- prior state;
- candidate assessment;
- evidence inputs;
- contradictory evidence;
- assessment rule/version;
- provenance;
- adjudication;
- resulting state.

No silent in-place historical rewrite is permitted.

## 15. Demotion, invalidation, and revocation

Promotion is not irreversible.

A demonstrated claim must be reassessed when required evidence becomes:

- invalid;
- stale;
- contradicted;
- superseded;
- out of scope;
- integrity-invalid.

Historical state must remain auditable.

## 16. AI boundary

AI may assist analysis.

AI may not:

- autonomously mutate claim assessment state;
- autonomously mutate adjudication state;
- grant authority;
- authorize actuation;
- suppress contradiction;
- convert advisory output into normative evidence without governed intake.

AI/advisory != authority != actuation.

## 17. Cross-register traceability

Claim, evidence, requirement, document, capability, and project-state identities
must remain independently meaningful.

Cross-register references must be explicit.

Reference presence does not itself prove semantic applicability.

Traceability is necessary but not sufficient for claim promotion.

## 18. Change management

Changes to this policy require:

- architectural adjudication;
- backward semantic impact review;
- Human Readability review;
- Devil's Advocate review;
- Technical Destruction planning;
- controlled validation;
- controlled commit.

## 19. Explicit nonclaims

This policy does not claim:

- current claim sufficiency;
- current evidence sufficiency;
- automatic claim promotion implementation;
- complete evidence inventory;
- controlled-document completeness;
- NodeSupervisor implementation readiness;
- physical F401 validation;
- production readiness;
- certification.

## 20. Current governance state

At creation of this policy:

- M16-B0 discovery: PASS;
- cross-register promotion mechanism: NOT_DEMONSTRATED;
- automatic claim promotion: PROHIBITED;
- evidence presence as claim proof: PROHIBITED;
- evidence/claim promotion architecture: DEFINED;
- claim-promotion implementation authorization: NO;
- NodeSupervisor documentary authority gate: BLOCKED;
- NodeSupervisor implementation authorization: NO.
