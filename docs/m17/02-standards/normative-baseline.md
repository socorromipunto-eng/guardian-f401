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
# M17 Normative and Assurance Baseline

## Rule

Guardian distinguishes:

REFERENCE
MAPPING
ALIGNMENT
CONFORMANCE
CERTIFICATION

These words are not interchangeable.

A document listed here is a benchmark or applicability candidate unless a later
evidence-backed decision establishes a stronger status.

## NASA

NASA-STD-8739.8B
Software Assurance and Software Safety Standard
Status at M17 planning: ACTIVE

Use:

software assurance;
software safety process;
independent verification and validation concepts;
objective evidence;
lifecycle assurance.

NASA-HDBK-2203
Software Engineering Handbook

Use:

implementation guidance.

No NASA approval or certification claim is authorized.

## NIST

NIST SP 800-218
Secure Software Development Framework 1.1

Use:

secure development practices.

NIST SP 800-160 Volume 1 Revision 1
Engineering Trustworthy Secure Systems

Use:

systems security engineering.

NIST SP 800-193
Platform Firmware Resiliency Guidelines

Use:

Protect;
Detect;
Recover;
firmware integrity and recovery.

NIST SP 800-161 Revision 1 Update 1
Cybersecurity Supply Chain Risk Management Practices

Use:

supplier;
dependency;
toolchain;
provenance;
C-SCRM.

NIST cryptographic publications shall be mapped separately for algorithms,
entropy, DRBG, signatures and key management.

No NIST validation claim is authorized.

## CISA

CISA/FBI Product Security Bad Practices
2025 update

Use:

adversarial product-security benchmark;
memory-safety decision discipline;
cryptographic bad-practice review;
hardcoded credential review;
vulnerability and support-lifecycle practices.

CISA guidance is not a Guardian certification.

## ISO and IEC

ISO/IEC 9899:2024
Programming Languages - C

Use:

current C language reference.

No immediate C dialect migration is implied.

IEC 62443-4-1:2018
Secure Product Development Lifecycle Requirements

Use:

security requirements;
secure design;
secure implementation;
verification and validation;
defect management;
patch management;
end of life.

IEC 62443-4-2:2019
Technical Security Requirements for IACS Components

Use:

identification/authentication;
use control;
system integrity;
data confidentiality;
restricted data flow;
timely response;
resource availability.

ISO/IEC 27001:2022 may be used for management-system mapping.

No IEC or ISO certification claim is authorized.

## Aerospace assurance references

RTCA DO-178C
Software Considerations in Airborne Systems and Equipment Certification

Use:

software development assurance benchmark.

RTCA DO-254
Design Assurance Guidance for Airborne Electronic Hardware

Use:

electronic hardware assurance benchmark.

SAE ARP4754B
Guidelines for Development of Civil Aircraft and Systems

Use:

system development assurance;
requirements validation;
design verification.

SAE ARP4761A / EUROCAE ED-135
Safety Assessment Process

Use:

hazard analysis;
failure analysis;
safety assessment methods.

RTCA DO-326A / EUROCAE ED-202A
Airworthiness Security Process Specification

RTCA DO-356A / EUROCAE ED-203A
Airworthiness Security Methods and Considerations

Use:

cybersecurity process;
security risk assessment;
security verification;
lifecycle security.

These are assurance benchmarks.

Guardian is not represented as airborne-certified.

## European Union

Regulation EU 2024/2847
Cyber Resilience Act

M17 action:

perform product-scope applicability assessment;
map secure development;
vulnerability handling;
technical documentation;
support lifecycle;
reporting obligations where applicable.

ENISA Cyber Resilience Maturity Assessment Model
2026

Use:

European cyber-resilience assessment benchmark.

Regulation EU 2024/1689
Artificial Intelligence Act

M17 status:

APPLICABILITY_WATCH

Reason:

integrated AI advisory architecture belongs primarily to a later milestone.

Existing repository references do not prove legal applicability.

## Secure C

SEI CERT C shall be evaluated as a secure-coding benchmark.

MISRA C may be evaluated only with an authorized source and license.

References inside vendor CMSIS headers do not establish Guardian MISRA
conformance.

## Mandatory mapping record

Each formal standards mapping shall record:

SOURCE_ID
SOURCE_VERSION
SOURCE_STATUS
APPLICABILITY
GUARDIAN_REQUIREMENT
IMPLEMENTATION_REFERENCE
TEST_REFERENCE
EVIDENCE_REFERENCE
ASSESSMENT
LIMITATION
HUMAN_ADJUDICATION


## EU machinery source review candidate - 2026-10-08

Status: CONTROLLED CANDIDATE; no gate or compliance closure.
The original document bytes above are preserved.
See eu-machinery-applicability-and-assurance-candidate.md and ../03-requirements/eu-machinery-requirements-candidate.json for the sixteen-row focused mapping and exact five-PDF identities.
The supplied chain includes 2023/1230, its 2023 corrigendum, 2024/2748 and 2026/1744; consolidation 27 July 2026 is documentary only.
Applicability and exhaustive currentness remain review items. Five-year upload/intervention tracing and conditional one-year safety decision retention remain distinct.
NASA/CISA/IEEE/aerospace references above are unchanged; mapping does not establish conformity or certification.
