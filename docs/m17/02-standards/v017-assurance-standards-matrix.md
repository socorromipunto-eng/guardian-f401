# Guardian F401 v0.17.0 Assurance Standards Matrix

Status: CONTROLLED CANDIDATE
Source freshness snapshot: 2026-10-04

This matrix defines how public standards, regulations, guidance, and licensed references are used during v0.17.0 closure. It deliberately separates reference mapping from conformance, certification, approval, and deployment authorization.

| Source | Current project use | Authority / source treatment | Allowed claim | Prohibited claim without additional evidence |
| --- | --- | --- | --- | --- |
| NIST SP 800-218 SSDF v1.1 | Secure-development lifecycle, vulnerability prevention, supply-chain and release practices. | Public NIST final publication. | Practices mapped as assurance reference. | NIST certification/approval. |
| NIST SP 800-193 | Firmware protection, detection, recovery and resilience lens. | Public NIST final publication. | Firmware resiliency requirements and test rationale mapped. | Full platform conformance based only on host tests. |
| NIST SP 800-160 Vol.1 Rev.1 | Systems security engineering, trustworthiness, requirements/architecture/verification discipline. | Public NIST final publication. | SSE principles used as engineering reference. | NIST endorsement or formal certification. |
| NIST SP 800-57 Part 1 Rev.5 | Key lifecycle, custody, cryptoperiod, trust-anchor management. | Public NIST final publication. | Key-management design reference. | Production key provenance merely because a key exists. |
| NIST SP 800-133 Rev.2 | Generation of cryptographic keys. | Public NIST final publication. | Ceremony generation requirements reference. | Claim that an unqualified generator produced an approved key. |
| FIPS 186-5 | EdDSA/Ed25519 algorithm baseline. | Public FIPS. | Ed25519 is an allowed signature algorithm baseline. | Provider/module FIPS validation. |
| FIPS 140-3 / CMVP | Cryptographic module assurance and validation context. | Public program/reference. | Module assurance reference only. | Validated-module claim without an exact CMVP certificate and applicable configuration evidence. |
| CISA Secure by Design | Secure defaults, threat ownership, reduction of customer security burden, transparency. | Public CISA guidance. | Design and review lens. | CISA approval/certification. |
| NASA-STD-8739.8B | Software assurance and software safety discipline; evidence that intended behavior occurs and unintended behavior is constrained. | Public NASA assurance reference. | Assurance activities used as reference. | NASA certification/approval. |
| IEEE 1012-2024 | System/software/hardware V&V integrity and independence concepts. | Licensed normative standard; public catalog metadata only is not the normative text. | High-level V&V reference; detailed mapping only from licensed text. | IEEE conformance without licensed clause-level evidence. |
| FAA AC 20-115D | Airborne software assurance reference and public recognition of DO-178C/DO-330 family. | Active FAA advisory circular; acceptable means context, not project certification. | Aerospace-style assurance model used as reference. | Airworthiness, DAL, FAA approval, or DO-178C compliance. |
| EASA AMC 20-115D | European airborne software assurance reference. | EASA AMC reference. | Aerospace-style assurance model used as reference. | EASA certification or ED-12C compliance. |
| RTCA DO-178C / EUROCAE ED-12C | Requirements-based software assurance concepts recognized by FAA/EASA. | Licensed normative texts. | Reference family named; detailed mapping requires licensed text. | DAL or compliance claim absent complete applicable evidence. |
| RTCA DO-330 / EUROCAE ED-215 | Tool qualification concept reference. | Licensed normative texts recognized by FAA AC 20-115D. | Tool-qualification risk concept. | Qualified-tool claim without applicable qualification evidence. |
| ISO/IEC 9899:2024 | C language baseline. | Published language standard. | C implementation baseline. | Certification of code merely from compiler acceptance. |
| SEI CERT C | Secure C coding guidance. | Public/controlled coding guidance. | Secure coding review reference. | Formal certification unless independently obtained. |
| MISRA C | Safety/security coding discipline where licensed and applicable. | Licensed source required. | Mapping only from verified licensed source. | Detailed compliance claim from memory or summaries. |
| IEC 62443 family | Industrial automation/control-system security reference where scope applies. | Licensed normative text required for detailed mapping. | Architecture/security reference after applicability decision. | IEC 62443 conformance/certification absent evidence. |
| Regulation (EU) 2024/2847 CRA | Cybersecurity requirements for products with digital elements; applicability must be adjudicated. | EU law, official EUR-Lex source. | Applicability/risk mapping when scope is established. | Product compliance claim without legal/applicability and conformity evidence. |
| Directive (EU) 2022/2555 NIS2 | Entity/jurisdiction cybersecurity obligations. | EU directive; national transposition matters. | Applicability review only. | Product-level certification claim. |
| Regulation (EU) 2024/1689 AI Act | Future AI/advisory plane applicability; v0.17 has no AI authority/actuation path. | EU law, current consolidated text. | Applicability review for future v0.18 AI role. | AI Act compliance claim for v0.17 without applicable facts. |

## Public source catalog

- NIST SP 800-218: https://csrc.nist.gov/pubs/sp/800/218/final
- NIST SP 800-193: https://csrc.nist.gov/pubs/sp/800/193/final
- NIST SP 800-160 Vol.1 Rev.1: https://csrc.nist.gov/pubs/sp/800/160/v1/r1/final
- NIST SP 800-57 Part 1 Rev.5: https://csrc.nist.gov/pubs/sp/800/57/pt1/r5/final
- NIST SP 800-133 Rev.2: https://csrc.nist.gov/pubs/sp/800/133/r2/final
- FIPS 186-5: https://csrc.nist.gov/pubs/fips/186-5/final
- CISA Secure by Design: https://www.cisa.gov/securebydesign
- NASA-STD-8739.8B public software assurance resource: https://swehb.nasa.gov/spaces/SITE/pages/119242809/NASA-STD-8739.8B
- FAA AC 20-115D: https://www.faa.gov/airports/resources/advisory_circulars/index.cfm/go/document.information/documentNumber/20-115D
- EASA AMC-20 Easy Access Rules, including AMC 20-115D: https://www.easa.europa.eu/en/document-library/easy-access-rules/online-publications/easy-access-rules-acceptable-means-2
- EU CRA: https://eur-lex.europa.eu/legal-content/EN/TXT/?uri=CELEX:32024R2847
- NIS2: https://eur-lex.europa.eu/eli/dir/2022/2555/oj/eng
- EU AI Act: https://eur-lex.europa.eu/eli/reg/2024/1689

## Claim firewall

REFERENCE != CONFORMANCE
CONFORMANCE != CERTIFICATION
CERTIFICATION != DEPLOYMENT AUTHORIZATION
MAPPING != LEGAL APPLICABILITY
HOST EVIDENCE != PHYSICAL HARDWARE EVIDENCE
ALGORITHM ALLOWED != CRYPTOGRAPHIC MODULE VALIDATED

## EU machinery source review candidate - 2026-10-08

Status: CONTROLLED CANDIDATE; no gate or compliance closure.
The original document bytes above are preserved.
See eu-machinery-applicability-and-assurance-candidate.md and ../03-requirements/eu-machinery-requirements-candidate.json for the sixteen-row focused mapping and exact five-PDF identities.
The supplied chain includes 2023/1230, its 2023 corrigendum, 2024/2748 and 2026/1744; consolidation 27 July 2026 is documentary only.
Applicability and exhaustive currentness remain review items. Five-year upload/intervention tracing and conditional one-year safety decision retention remain distinct.
NASA/CISA/IEEE/aerospace references above are unchanged; mapping does not establish conformity or certification.
