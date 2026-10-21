# M17 EU Machinery applicability and assurance candidate — revision 2

Status: CONTROLLED CANDIDATE / NOT ADMITTED. Gate01 remains open.
This supersedes the external 05D EU supplement as a candidate; it does not change the repository or historical records.

## Exact sources and review

Five PDFs have been captured and hash-verified. The supplied consolidated edition lists the original, C1, M1 and M2 matching the received set. The focused 1.1.9/1.2.1 original-to-consolidated normalized-text comparison passed. Cryptographic OJ authenticity and exhaustive present-day currentness are not established. Source identities are recorded in the companion JSON. The consolidated text is documentary, with no independent legal effect.

The corrigendum corrects the general application date to 20 January 2027; individual provisions have separate dates. The 2024 amendment adds conditional emergency procedures. The 2026 amendment changes articles 8, 20 and 47 regarding high-risk AI and delegated requirements; it does not directly replace 1.1.9/1.2.1.

## Architecture and applicability

STM32F401 retains deterministic reference/control authority. MCXN947 is the supervisory peer, without automatic actuator authority. Additional ST measurement, security witness and high-compute roles remain planned with exact parts NOT_SELECTED. Semantic contracts must preserve meaning, provenance, uncertainty, policy and authority context. No silence, quorum, compute or AI advice grants actuation authority.

The machine, intended use, product classification, safety functions, hazards, responsible economic operator and market/service context must be specified before applicability is approved. Research status is not an automatic laboratory exclusion. AI/advisory architecture is not a legal AI-risk classification.

NASA, CISA, IEEE and aerospace assurance references remain separate benchmarks. This candidate does not certify their use, remove their existing content or establish EU conformity. Device identity is distinct from source integrity; signed envelopes are distinct from persisted events and retention.

## Revised requirement decomposition — proposed, not admitted

The following is a focused firmware/control mapping, not an exhaustive Annex III or CE-marking checklist. Legal obligation summaries and proposed engineering implementations are separate. All rows remain REVIEW_REQUIRED; no compliance claim is authorized.

| ID | Source locator | Source obligation / conditional topic | Proposed Guardian requirement and acceptance evidence |
| --- | --- | --- | --- |
| EU-M17-001 | Articles 2–3; Annex III general principles | Product scope and risk-based requirements | Record product/use/safety role and hazard register; justify applicability and exclusions. Charter alone is insufficient. |
| EU-M17-002 | Annex III 1.1.9, paragraphs 3–4; consolidated p62 | Identify critical software/data and installed software needed for safe operation; provide accessible identity | Distinguish source, image, installed firmware and configuration. Target readback, authorized/unauthorized configuration-change tests. A source manifest is not installed identity. |
| EU-M17-003 | 1.1.9, paragraphs 2–3 | Protect relevant hardware, critical software and data against accidental/intentional corruption | Define protected assets and interfaces; test corrupt firmware/configuration, access paths and hardware faults on target. Signature verification covers only part of this requirement. |
| EU-M17-004 | 1.1.9, paragraph 5 | Collect evidence of legitimate/illegitimate software interventions or software/configuration modifications | Record real target events, including approved changes and rejected attempts, with attributable identity/context and durable retrieval. A signed envelope is not an event store. |
| EU-M17-005 | 1.2.1 first list (a)–(e); consolidated p63 | Prevent hazardous effects of stresses, external influences, foreseeable malicious attempts, hardware/logic faults and foreseeable human errors | Machine-specific safe response to hostile peers, reset, partitions and sensor faults; hardware tests and hazard-linked acceptance limits. Fail-closed must be defined physically, not equated automatically with removing power. |
| EU-M17-006 | 1.2.1 first list (f); p63 | Enable tracing log of intervention data and safety-software versions uploaded after market/service, for five years after upload, for the stated conformity purpose and authority-request conditions | Define per-upload retention, export and storage responsibility; measure capacity, wear, clock/epoch, exhaustion, recovery and retrieval. Do not relabel this as five years for every telemetry sample. |
| EU-M17-007 | Annex III general principles | Risk assessment and risk reduction over intended use and foreseeable misuse | Link hazards to functions, controls, residual risks and evidence; adversarial software review is not a machine hazard analysis. |
| EU-M17-008 | Project governance derived from 1.1.9/1.2.1 | Evidence/claim discipline; project assurance rule, not a verbatim statutory obligation | Keep host, target-build and physical evidence distinct; no claim closure without applicability and accepted evidence. |
| EU-M17-009 | 1.1.9 paragraph 1 | Connecting another device or communicating remote device must not create a hazardous situation | Node admission/disagreement/partition contracts plus fault injection; measure the actuator outcome. More nodes can add attack and fault paths. |
| EU-M17-010 | 1.1.9 paragraph 2 | Collect legitimate/illegitimate intervention evidence for relevant hardware components | Enumerate debug, boot, communication and access hardware within scope; implement and test relevant intervention detection and recording. The earlier software-only evidence row omitted this surface. |
| EU-M17-011 | 1.2.1 self-evolving/autonomy list (a), (c); pp63–64 | Conditional: bound task/movement space and maintain ability to correct for inherent safety | Classify actual evolving/autonomous behaviour. If applicable, demonstrate deterministic limits and correction under faults. No automatic exemption from an advisory label. |
| EU-M17-012 | 1.2.1 self-evolving/autonomy list (b); p63 | Conditional: enable recording of safety decision data and retain for one year after collection, with stated purpose/request conditions | Separate this retention class from EU-M17-006; define covered decisions, persistent recording and retrieval tests. Do not apply automatically to every AI output. |
| EU-M17-013 | 1.2.1 particular-attention list (a)–(h); p64 | Prevent unsafe restart, uncontrolled parameters and hazardous rule changes; preserve stopping/protection and coherent assembly safety | Test restart authorization, update/recovery, stop priority and assembly-level consistency. This includes physical control behaviour beyond host quorum logic. |
| EU-M17-014 | 1.2.1 final paragraph; p64 | Wireless-control communication/connection failure must not create a hazardous situation | Conditional on wireless control. Define link fault response and test; transport is not selected merely by a NodeLink abstraction. |
| EU-M17-015 | Articles 8, 20(10), 47 as amended by 2026/1744 | Relevant high-risk AI requirements, delegated acts and standards framework | Create AI use/safety classification and source-monitoring decision. Review AI Act text and applicable instruments before approving a claim. No AI regulatory closure in this review. |
| EU-M17-016 | Articles 10–11 and Annex IV, subject to product classification | Manufacturer technical documentation and conformity responsibilities | Expand beyond firmware clauses into the applicable technical file, integration information and assessment route. A signed release is supply-chain evidence, not conformity assessment. |

## Admission and validation

Every row is REVIEW_REQUIRED. Preserve the original eight IDs; extend the decomposition to sixteen. Implementation/test references retain their original hashes and indicate review surfaces, not satisfied requirements. This mapping is not a complete legal checklist. No production, physical, standards, compliance or release claim is closed.

Next: review all five candidate paths and source dependencies, then preflight current exact repository state before a separate documentation-only admission. No staging, commit, firmware edit, build or release is authorized here.
