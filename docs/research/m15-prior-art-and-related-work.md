# Guardian F401 M15 — Prior Art and Related Work

## Purpose

This document places Guardian F401 in technical and historical context.

It does not claim adoption, endorsement, certification, validation, affiliation,
compatibility, or use of Guardian F401 by any external organization or project.

The relationships described here are conceptual, architectural, historical, or
methodological unless explicitly demonstrated otherwise.

## Non-endorsement boundary

The following relationships are explicitly prohibited:

`PRIOR ART != ADOPTION`

`PRIOR ART != ENDORSEMENT`

`PRIOR ART != CERTIFICATION`

`PRIOR ART != THIRD-PARTY VALIDATION`

`PRIOR ART != IMPLEMENTATION DEPENDENCY`

`CONCEPTUAL SIMILARITY != ARCHITECTURAL EQUIVALENCE`

Guardian F401 remains an independent research implementation.

## 1. Simplex Architecture

### Established prior art

The Software Engineering Institute at Carnegie Mellon University documented the
Simplex Architecture in the 1990s as an architecture for dependable and
evolvable process-control systems.

Simplex separates a sophisticated or less-assured component from a
high-assurance control subsystem capable of maintaining or recovering safe
behavior.

The Simplex literature also addresses dynamic replacement or upgrade of complex
components while retaining a trusted control path.

### Relevance to Guardian F401

Guardian F401 shares a broad architectural principle with Simplex:

complex or advisory functionality must not obtain unrestricted authority over a
safety-relevant control boundary.

Guardian's current evidence demonstrates a deterministic logical run-permit
path below the future advisory/AI layer.

### Important differences

Guardian F401 does not currently claim to implement the Simplex Architecture.

The current Guardian implementation does not contain:

- a deployed advanced AI controller;
- a Simplex-compatible switching controller;
- a formally verified Simplex safety envelope;
- Simplex certification;
- evidence that Guardian's control architecture is equivalent to Simplex.

Simplex is therefore prior art and architectural context, not an implementation
dependency or validation authority.

## 2. Runtime Assurance

### Established prior art

Runtime Assurance (RTA) is a broader architectural approach in which a runtime
monitor evaluates relevant properties of a complex or insufficiently assured
function and invokes an assured fallback, reversionary controller, or other
mitigation when required.

NASA publications explicitly describe Simplex as an instance of Runtime
Assurance and discuss RTA as a method for bounding advanced, autonomous, or
AI/ML-based functions that cannot themselves provide the required assurance
level.

RTA guidance also emphasizes that the assurance mechanism itself must be
trusted, validated for its intended function, and prevented from introducing new
hazards.

### Relevance to Guardian F401

Guardian's planned advisory/AI architecture shares the principle that analytical
or high-complexity functions may inform decisions without automatically
receiving final physical actuation authority.

This relationship is relevant to future G15-05 authority-boundary work.

### Important differences

Guardian F401 currently has no AI/advisory execution plane.

Current RT-M15-001 evidence supports logical deterministic control behavior,
not a complete RTA implementation.

Guardian therefore does not claim:

- RTA certification;
- compliance with aerospace RTA guidance;
- equivalence to NASA RTA architectures;
- an assured physical fallback controller;
- validated AI containment.

## 3. SACEM and railway safety-software precedent

### Established prior art

SACEM was developed for automatic train protection on the Paris RER A and is a
well-documented historical example of rigorous safety-software validation in the
railway domain.

Published literature describes the use of formal specification, proof-oriented
validation, and multiple verification techniques in relation to SACEM and later
railway systems.

The railway domain subsequently became an important industrial application area
for formal methods such as the B Method.

### Relevance to Guardian F401

SACEM is relevant as historical evidence that safety-critical software
engineering requires explicit assurance arguments, rigorous validation,
traceability, and separation between implementation claims and demonstrated
safety properties.

This is methodological context for Guardian's evidence-first approach.

### Important differences

Guardian F401:

- does not use SACEM software;
- does not implement SACEM;
- is not a railway ATP system;
- is not validated by RATP, SNCF, Siemens, Alstom, or SACEM developers;
- has no demonstrated railway certification;
- does not claim equivalence to a railway SIL system.

SACEM is therefore historical prior art, not evidence for Guardian itself.

## 4. Formal methods and assurance

The wider safety-critical software literature demonstrates that formal
specification, runtime monitoring, model checking, proof, testing, redundancy
and fail-safe architecture are established engineering techniques.

Guardian F401 does not claim invention of those individual concepts.

Any future novelty or contribution claim must be limited to the specific
combination, contracts, evidence model, implementation, and experimentally
demonstrated properties that Guardian actually provides.

## 5. Guardian-specific research direction

Current Guardian work is distinguished by the particular research combination
under investigation, including:

- bounded embedded acquisition and DSP;
- deterministic supervisory-control behavior;
- authenticated command and authorization processing;
- signed assurance-object contracts;
- purpose-domain separation;
- persistent freshness and anti-replay state;
- explicit bootstrap authorization lifecycle;
- explicit epoch-transition authorization lifecycle;
- evidence-oriented development and hostile adjudication;
- future heterogeneous corroboration;
- future advisory/AI separation from final authority.

This list is not a novelty claim.

Each item must remain subject to its own implementation and validation evidence.

## 6. Current evidence boundary

The current repository demonstrates only the capabilities evidenced by its
source, tests, validation records and controlled build artifacts.

The following remain outside demonstrated prior-art comparison claims:

- physical hardware qualification;
- complete Runtime Assurance implementation;
- formal proof of the full Guardian architecture;
- heterogeneous multi-node implementation;
- quorum implementation;
- split-brain prevention implementation;
- AI/advisory implementation;
- physical actuator authority;
- third-party certification.

## References

1. Lui R. Sha, *A Software Architecture for Dependable and Evolvable
   Industrial Computing Systems*, CMU/SEI-95-TR-005, Software Engineering
   Institute, Carnegie Mellon University, 1995.
   https://www.sei.cmu.edu/library/a-software-architecture-for-dependable-and-evolvable-industrial-computing-systems/

2. Jose G. Rivera, Alejandro A. Danylyszyn, Charles Weinstock, Lui R. Sha,
   Michael J. Gagliardi, *An Architectural Description of the Simplex
   Architecture*, CMU/SEI-96-TR-006, Software Engineering Institute,
   Carnegie Mellon University, 1996.
   https://www.sei.cmu.edu/library/an-architectural-description-of-the-simplex-architecture/

3. J. Tanner Slagel, Lauren M. White, Aaron Dutle, César A. Muñoz, Nicolas
   Crespo, *A Verification Framework for Runtime Assurance of Autonomous UAS*,
   NASA Langley Research Center, 2024.
   https://ntrs.nasa.gov/citations/20240007986

4. Guillaume Brat and Ganeshmadhav Pai, *Runtime Assurance of Aeronautical
   Products: Preliminary Recommendations*, NASA Technical Memorandum, 2023.
   https://ntrs.nasa.gov/citations/20220015734

5. Guihot and Hennebert, *Results of a Safety Software Validation: SACEM*,
   IFAC/IFIP/IFORS Symposium on Control, Computers, Communications in
   Transportation, 1990.
   DOI: 10.1016/B978-0-08-037025-5.50019-0

6. Jean-Louis Boulanger, *Formal Methods: Application to the Railway Domain*,
   Techniques de l'Ingénieur, 2016.
   https://www.techniques-ingenieur.fr/en/resources/article/ti602/formal-methods-railway-applications-trp3309/v1

## Research disposition

Guardian F401 recognizes Simplex, Runtime Assurance, SACEM and formal-method
practice as relevant prior art.

No external reference in this document is evidence that the referenced
organization, author, architecture, standard, railway operator, or research
program uses, endorses, validates or certifies Guardian F401.
