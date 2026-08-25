# RT-M15-012 — Material Claim / Evidence Boundary

## Status

CLOSED_FOR_CLAIM_BOUNDARY

## Classification

Public-claim, release-governance, evidence-traceability and human-readability
finding.

## Purpose

This record adjudicates whether material claims in Guardian F401 public and
release-facing documentation exceed the implementation or validation evidence
currently demonstrated by the repository.

This record does not establish completeness of Guardian F401 itself.

A claim may be accurate while the capability remains bounded, experimental or
not physically qualified.

## Reviewed public/release surfaces

The claim review included:

- `README.md`;
- `CHANGELOG.md`;
- `CITATION.cff`;
- `docs/release-evidence-v0.14.2.md`.

Material claim terms were inspected together with limitation and negation
surfaces.

Claims were adjudicated in context rather than by keyword count alone.

## Adjudication classes

Material claims were classified as:

`SUPPORTED`

The claim is directly supported by reviewed evidence within its stated scope.

`SUPPORTED_WITH_BOUNDARY`

The claim is supported only within an explicit software, build, host,
structural, release or other bounded scope.

`NEGATIVE_OR_FALSE_POSITIVE`

The automated detector matched a limitation, negation or unrelated phrase
rather than a positive capability claim.

`OVERBROAD`

The claim exceeds demonstrated evidence and requires correction.

## README claim boundary

The README explicitly states that the reviewed assurance slice does not
establish:

- hardware qualification;
- real-time behavior;
- signatures where outside the slice;
- freshness where outside the slice;
- attestation where outside the slice;
- distributed failover;
- production readiness;
- certification.

The README also qualifies M11 fuzzing as completed only for the documented
bounded parser/security scope.

RT-M15-004 remains authoritative for fuzz-coverage limitations.

Disposition:

`SUPPORTED_WITH_BOUNDARY`

## CHANGELOG — post-merge validation claim

Reviewed claim:

`Exact post-merge validation: 9 of 9 GitHub checks completed successfully.`

This is a bounded CI/post-merge validation claim.

It does not assert hardware qualification, production readiness, certification
or physical correctness.

Disposition:

`SUPPORTED`

## CHANGELOG — software capability summary

The reviewed capability summary includes:

- STM32F401 USART2 transport and ADC/timer/DMA acquisition architecture;
- RMS, peak, Hann-window, FFT, dominant-frequency and spectral-energy
  processing;
- deterministic machine-health baseline, anomaly and hysteresis states;
- supervisory-control state machine with logical run-permit boundary;
- authenticated sessions, authorization roles and anti-replay controls;
- robustness campaigns, malformed-frame testing, fuzz targets and fault
  injection;
- portable signed-firmware lifecycle, version policy and anti-rollback model;
- STM32F401CDU6 hardware contract, Keil manifests and read-only qualification
  plan.

These statements describe software, architecture, testing or qualification-plan
surfaces.

They do not state that every firmware module is fuzzed.

They do not state that the physical board is qualified.

They do not establish production readiness or certification.

RT-M15-003 remains authoritative for hardware-validation limitations.

RT-M15-004 remains authoritative for fuzzing-coverage limitations.

Disposition:

`SUPPORTED_WITH_BOUNDARY`

## CHANGELOG — Verified in software

The heading:

`Verified in software`

is followed by:

- protocol, simulator, console and portable firmware test suites;
- strict host compilation gates for portable C and STM32F401-facing
  translation units;
- deterministic robustness and mutation campaigns in supported environments.

The heading and supporting bullets explicitly scope the verification to
software.

No physical-hardware interpretation is authorized.

Disposition:

`SUPPORTED_WITH_BOUNDARY`

## CITATION.cff

The citation abstract describes Guardian F401 as an experimental STM32F401CDU6
embedded research platform combining:

- deterministic signal acquisition;
- DSP and spectral analysis;
- machine-health state evaluation;
- supervisory control;
- authenticated command handling;
- robustness testing;
- portable signed-firmware lifecycle;
- bounded software-assurance behavior.

The abstract immediately states:

`Software validation is documented; physical board qualification and safety
certification are not claimed.`

This explicit boundary prevents the software-capability description from being
interpreted as physical qualification or safety certification.

Disposition:

`SUPPORTED_WITH_BOUNDARY`

## Release evidence

The v0.14.2 release evidence explicitly records physical hardware validation as:

`PENDING`

It also preserves restrictions including:

- no physical hardware validation claim;
- no production deployment authorization;
- no safety or cybersecurity certification claim;
- no flashing qualification claim.

Release-integrity statements remain distinct from hardware and safety claims.

Disposition:

`SUPPORTED_WITH_BOUNDARY`

## Cross-record authority

The following adjudication records remain authoritative for their respective
claim boundaries:

- RT-M15-001 — AI/advisory and actuation authority;
- RT-M15-003 — hardware validation;
- RT-M15-004 — fuzzing claims and coverage;
- RT-M15-005 — prior art and related work;
- RT-M15-006 — SBOM and dependency accounting;
- RT-M15-007 — license and adoption;
- RT-M15-008 — multi-node / heterogeneous architecture;
- RT-M15-009 — independent review;
- RT-M15-010 — release/version reconciliation;
- RT-M15-011 — Technical Destruction terminology.

This RT record does not override open findings in those records.

## Known overbroad public claims

After contextual adjudication of the reviewed public and release surfaces:

`0`

known material claims remain classified as:

`OVERBROAD`

within the reviewed claim set.

This statement is bounded to the repository state and surfaces inspected by
this review.

It is not proof that no inaccurate statement can exist anywhere in the
repository.

## Decision

RT-M15-012 is:

`CLOSED_FOR_CLAIM_BOUNDARY`

No known material claim in the reviewed README, CHANGELOG, CITATION or v0.14.2
release-evidence surfaces remains identified as exceeding its demonstrated
evidence after the current adjudication pass.

Existing technical gaps remain gaps.

They are not converted into completed capabilities by this decision.

## Normative claim rule

Guardian F401 shall preserve the following evidence discipline:

`IMPLEMENTED != PHYSICALLY VALIDATED`

`SOFTWARE VERIFIED != HARDWARE QUALIFIED`

`BUILD SUCCESS != PRODUCTION READINESS`

`FUZZ TARGETS EXIST != COMPLETE FIRMWARE FUZZ COVERAGE`

`DOCUMENTED ARCHITECTURE != IMPLEMENTED ARCHITECTURE`

`INTERNAL REVIEW != INDEPENDENT EXTERNAL REVIEW`

`PUBLICATION != CERTIFICATION`

`PUBLIC REPOSITORY != OPEN SOURCE`

`PLANNED CAPABILITY != CURRENT CAPABILITY`

Every material future claim must remain traceable to appropriate evidence and
its applicable limitations.

## Re-open conditions

RT-M15-012 must be reopened if:

- public-facing capability wording materially changes;
- a new release is prepared;
- previously planned capabilities become implemented;
- AI/advisory functionality is introduced;
- multi-node functionality is introduced;
- physical hardware validation is completed;
- certification or external-review wording is introduced;
- a new claim cannot be traced to evidence.

## Residual restrictions

This adjudication does not establish:

- physical hardware qualification;
- complete fuzz coverage;
- formal SBOM completion;
- independent external review;
- multi-node implementation;
- AI containment validation;
- production readiness;
- functional-safety certification;
- cybersecurity certification.
