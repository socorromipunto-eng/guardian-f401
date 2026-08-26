# RT-M15-005 — Prior Art and Related Work

## Status

CLOSED_FOR_PRIOR_ART_CONTEXT

## Classification

Research-context, novelty-boundary and related-work finding.

This record does not establish novelty, patentability, third-party validation,
certification, adoption, endorsement, interoperability, or architectural
equivalence.

## Finding

The initial red-team inspection found no explicit repository treatment of
relevant prior art such as Simplex Architecture, Runtime Assurance, SACEM or
related formal/safety-assurance practice.

This created a research-context gap.

The repository did not, however, demonstrate explicit absolute novelty claims
such as:

- first;
- unique;
- state of the art;
- novel architecture;
- invention of fail-safe control;
- invention of Runtime Assurance;
- invention of formal safety methods.

Therefore the defect was lack of contextualization, not a demonstrated false
novelty claim.

## Repair

A dedicated research record was created:

`docs/research/m15-prior-art-and-related-work.md`

Reviewed SHA-256:

`15DA88DDA7BB8EB292C43FC9707852F781C8B4552B2DC7D18208488ABD30A913`

The research record covers:

- Simplex Architecture;
- Runtime Assurance;
- SACEM and railway safety-software precedent;
- formal-method and assurance context;
- Guardian-specific research direction;
- current evidence boundaries.

## Prior-art boundary

The following rules are normative for Guardian F401 documentation:

`PRIOR ART != ADOPTION`

`PRIOR ART != ENDORSEMENT`

`PRIOR ART != CERTIFICATION`

`PRIOR ART != THIRD-PARTY VALIDATION`

`PRIOR ART != IMPLEMENTATION DEPENDENCY`

`CONCEPTUAL SIMILARITY != ARCHITECTURAL EQUIVALENCE`

External work may be cited to establish historical, technical or methodological
context.

External work must not be represented as evidence that Guardian F401 itself is
validated.

## Simplex / Runtime Assurance adjudication

Simplex and Runtime Assurance are relevant prior art for the general principle
that a complex or insufficiently assured component must not automatically own
final authority over a safety-relevant control boundary.

Guardian F401 shares conceptual concerns with that principle.

Guardian F401 does not currently claim:

- implementation of the Simplex Architecture;
- compliance with an RTA standard;
- Simplex certification;
- NASA validation;
- an implemented AI/advisory RTA controller;
- architectural equivalence to a published Simplex system.

RT-M15-001 remains the authoritative evidence record for Guardian's current
advisory/actuation boundary.

## SACEM / railway precedent adjudication

SACEM is treated solely as historical and methodological prior art for rigorous
safety-software engineering and validation practice.

Guardian F401 does not:

- use SACEM software;
- implement SACEM;
- provide railway ATP functionality;
- claim railway SIL equivalence;
- claim validation by SACEM developers, operators or vendors.

No railway certification claim is authorized.

## Novelty boundary

Guardian F401 does not claim invention of the individual concepts of:

- fail-safe control;
- runtime monitoring;
- safety fallback;
- formal verification;
- redundancy;
- authenticated messaging;
- anti-replay;
- distributed corroboration;
- deterministic policy enforcement.

Any future novelty claim must identify the exact Guardian contribution and must
be bounded by demonstrated implementation and evidence.

## Guardian research contribution boundary

Guardian may describe the particular combination under research, including:

- embedded acquisition and DSP;
- deterministic supervisory control;
- authenticated and authorized command processing;
- signed assurance objects;
- purpose-domain separation;
- persistent freshness and anti-replay state;
- bootstrap authorization;
- explicit epoch-transition authorization;
- evidence-oriented hostile adjudication;
- planned heterogeneous corroboration;
- planned advisory/AI separation.

This list is a research-scope description.

It is not, by itself, a novelty claim.

## Decision

RT-M15-005 is:

`CLOSED_FOR_PRIOR_ART_CONTEXT`

The original gap — absence of an explicit related-work/prior-art boundary — has
been repaired.

No third-party-use, endorsement, certification or validation claim was
introduced.

## Residual research obligation

Prior-art review is not permanently complete.

Future architecture changes must extend the related-work analysis when they
introduce materially new areas such as:

- heterogeneous fault-tolerant control;
- quorum protocols;
- split-brain prevention;
- physical Runtime Assurance;
- AI/ML advisory components;
- certified safety architectures;
- industrial fieldbus/brownfield integration.

A future publication claiming technical novelty may require a broader,
publication-specific literature review and independent academic review.

## Residual restrictions

This record does not establish:

- novelty;
- patentability;
- academic peer review;
- external validation;
- external adoption;
- certification;
- standards compliance;
- production readiness.
