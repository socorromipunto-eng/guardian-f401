# M17 Assurance Normative Baseline v2

Status: GOVERNED ARCHITECTURE BASELINE
Snapshot verified: 2026-10-04
Release target: v0.17.0
Claim policy: MAPPING != CONFORMANCE != CERTIFICATION != DEPLOYMENT AUTHORIZATION

## 1. Purpose

This baseline defines the external assurance references used to review Guardian F401 M17 architecture, firmware, verification, evidence, and release decisions. It does not claim certification, regulatory approval, compliance, or endorsement by any listed authority.

## 2. Authority hierarchy

The project shall not silently merge sources with different legal or assurance roles. Decisions use this hierarchy:

1. Applicable law/regulation and competent-authority requirements.
2. Applicable certification/acceptable-means material when a certification target is explicitly selected.
3. Current normative engineering and V&V standards.
4. Government/industry secure-development guidance.
5. Guardian project requirements, ADRs, threat model, tests, and evidence.

If applicability is unknown, the claim state is HOLD/UNKNOWN rather than presumed applicable or satisfied.

## 3. Current reference snapshot

### NIST

- NIST SP 800-218, Secure Software Development Framework (SSDF) Version 1.1, final February 2022. A Version 1.2 revision is draft as of this snapshot and is not promoted to final-baseline status.
- NIST SP 800-193, Platform Firmware Resiliency Guidelines, final May 2018.
- NIST SP 800-160 Vol. 1 Rev. 1, Engineering Trustworthy Secure Systems, final November 2022.
- FIPS 186-5, Digital Signature Standard, final February 2023. EdDSA is an approved signature technique; implementation validation/certificate scope remains provider-specific and is not implied by algorithm selection.
- NIST SP 800-186 is the companion curve recommendation source for approved elliptic-curve domain parameters.

Official source family: https://csrc.nist.gov/

### CISA

- Secure by Design is used as product-security design guidance.
- CISA/FBI Product Security Bad Practices (2025) is voluntary guidance and is used as an anti-pattern benchmark for secure product engineering, including embedded/OT products where applicable.

Official source: https://www.cisa.gov/securebydesign

### NASA

- NASA-STD-8739.8B, Software Assurance and Software Safety Standard, active, dated 2022-09-08.
- NASA assurance concepts are used as an audit benchmark for lifecycle assurance, objective evidence, software safety/security coordination, and IV&V-style independence where justified.
- Guardian does not claim NASA approval or that NASA-STD-8739.8B is mandatory for this project.

Official source: https://standards.nasa.gov/standard/nasa/nasa-std-87398

### IEEE

- IEEE 1012-2024, Standard for System, Software, and Hardware Verification and Validation, active; published 2025-08-22 and superseding IEEE 1012-2016.
- Only publicly verified metadata and project-owned licensed content may be used. No unlicensed IEEE normative text shall be reproduced into the repository.

Official source: https://standards.ieee.org/ieee/1012/7324/

### Airborne software assurance benchmark

- FAA AC 20-115D is active and recognizes RTCA DO-178C and related supplements/tool-qualification material as an acceptable means for airborne software development assurance.
- EASA AMC 20-115D recognizes EUROCAE ED-12C / RTCA DO-178C for airborne software development assurance.
- Guardian uses the lifecycle discipline, traceability, verification independence, configuration control, evidence, and tool-confidence concepts as an audit benchmark only.
- Guardian is not an airborne certification applicant under this baseline and makes no DO-178C, DO-330, FAA, EASA, ED-12C, DAL, or ETSO certification claim.

FAA official source: https://www.faa.gov/airports/resources/advisory_circulars/index.cfm/go/document.information/documentNumber/20-115D
EASA official source: https://www.easa.europa.eu/en/document-library/agency-decisions/ed-decision-2017020r

### European legal/regulatory layer

- Regulation (EU) 2024/2847, Cyber Resilience Act (CRA), is an in-force legal act. Product-scope, role, transition date, conformity-assessment path, and market-placement applicability require separate legal/product adjudication.
- Directive (EU) 2022/2555, NIS 2, is an in-force directive. Entity/jurisdiction/transposition applicability is separate from product conformity.
- Regulation (EU) 2024/1689, Artificial Intelligence Act, is tracked for the future v0.18 AI advisory plane. AI applicability does not confer authority on AI and does not change the v0.17.0 deterministic authority model.

Official source family: https://eur-lex.europa.eu/

### Language / coding baseline

- ISO/IEC 9899:2024, Programming languages â€” C, Edition 5, is the current C language standard in this baseline.
- SEI CERT C may be used as a secure-coding benchmark.
- MISRA C requirements may be mapped only from a verified licensed source. No MISRA conformance claim is made by this baseline.
- IEC 62443 may be mapped where industrial/product context applies, but exact edition/profile applicability and licensed normative text must be independently verified before any conformance claim.

## 4. Guardian invariant model

The following separations are mandatory and mechanically testable where implemented:

- authenticated != fresh
- fresh != truthful
- truthful != authorized
- authorized != actuation-authorized
- valid signature != trusted
- trusted != authorized
- authorized != accepted
- accepted != actuation-authorized
- witness != authority
- quorum != authority
- high-compute != authority
- AI/advisory != authority != actuation
- mapping != conformance != certification != deployment authorization

## 5. Evidence-reuse rule

PREVIOUS_PASS + SAME_INPUT_HASHES = EVIDENCE_REUSED
PREVIOUS_PASS + CHANGED_DEPENDENCY = IMPACT_REVALIDATION_REQUIRED
UNCHANGED_COMPONENT = DO_NOT_REPEAT_TEST
NEW_COMPONENT = FULL_LIFECYCLE_REQUIRED
PHYSICAL_CLAIM = PHYSICAL_EVIDENCE_REQUIRED

## 6. Release claim rule

A critical v0.17.0 release area may be promoted only when it is DEMONSTRATED with evidence or genuinely governed NOT_APPLICABLE. UNKNOWN, REVIEW_REQUIRED, TBD, PLANNED, or NOT_DEMONSTRATED keeps release readiness on HOLD.

## EU machinery source review candidate - 2026-10-08

Status: CONTROLLED CANDIDATE; no gate or compliance closure.
The original document bytes above are preserved.
See eu-machinery-applicability-and-assurance-candidate.md and ../03-requirements/eu-machinery-requirements-candidate.json for the sixteen-row focused mapping and exact five-PDF identities.
The supplied chain includes 2023/1230, its 2023 corrigendum, 2024/2748 and 2026/1744; consolidation 27 July 2026 is documentary only.
Applicability and exhaustive currentness remain review items. Five-year upload/intervention tracing and conditional one-year safety decision retention remain distinct.
NASA/CISA/IEEE/aerospace references above are unchanged; mapping does not establish conformity or certification.
