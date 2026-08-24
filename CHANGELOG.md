# Changelog

All notable changes to Guardian F401 are documented in this file.

## [0.14.2] - 2026-08-24

### Added

- Added the portable Guardian F401 µVision structural-review project.
- Added the M15/G15-01D AXF adjudication incident record.
- Added reproducibility evidence showing identical loadable images between
  absolute-path and portable project builds.
- Added narrowly scoped ignore rules for local Keil-generated artifacts.

### Changed

- Replaced machine-specific paths in the structural-review project with
  portable relative paths.
- Updated release and citation metadata for the v0.14.2 corrective release.
- Regenerated the tracked-file integrity manifest after the M15 integration.

### Corrected

- Corrected README DOI attribution: the version DOI
  `10.5281/zenodo.21923859` belongs to v0.13.0.
- Recorded the v0.14.0 version DOI:
  `10.5281/zenodo.21980859`.
- Initially documented v0.14.1 as having no Zenodo DOI based on the historical
  manual concept family alone. Post-publication inspection established that
  the GitHub integration maintains a second concept family.
- Verified GitHub-integrated Zenodo records:
  - v0.14.0: `10.5281/zenodo.21981234`
  - v0.14.1: `10.5281/zenodo.22062543`
  - v0.14.2: `10.5281/zenodo.22075322`
  - Concept DOI: `10.5281/zenodo.21981233`
- Preserved the historical/manual concept family:
  `10.5281/zenodo.21923858`.
- Recorded that v0.14.0 exists in both concept families as two distinct,
  already-published deposits.
- Preserved the immutable v0.14.1 tag despite its stale v0.14.0
  `CITATION.cff` metadata.

### Validation

- Guardian F401 structural-review target: 21 source entries and 13 include
  paths.
- Arm Compiler 6.24 rebuild: 0 errors and 0 warnings.
- `CreateHexFile`: 0.
- Two portable AXF rebuilds produced SHA-256
  `A9A0383BDE722753ACD663F571B848A4696D4D7101FFA80D1E3FC101C912EDA8`.
- Baseline and portable loadable images were identical: 36,088 bytes,
  SHA-256
  `89217348214D73CFA5011A8DD6AFACDD479336AF9F120886952CBEFA793FE5D7`.
- Existing tracked HEX was confirmed as unchanged v0.13.0 evidence after
  CRLF/LF normalization; no HEX was generated for this release preparation.

### Limitations

- Physical STM32F401 hardware validation remains pending.
- No flashing, deployment, production certification or safety certification
  is authorized or claimed.
- The tracked historical HEX is evidence from v0.13.0 and is not presented
  as a newly generated or hardware-qualified v0.14.2 firmware artifact.
## [0.14.1] - 2026-08-22

### Fixed

- A missing `rfc8785` dependency now raises an explicit environment error
  instead of being reclassified as `CANONICALIZATION`. The `AssuranceError`
  taxonomy remains reserved for input rejection.
- Documented the `assurance/requirements.lock` installation step and the
  assurance suite in the reproducible-validation instructions.
- Corrected stale M14 review-candidate wording and paths in
  `assurance/README.md`, which described the published component as an
  unauthorized external candidate.

### Unchanged

- No experimental result, contract, bound, limit or conclusion is modified.
- The immutable v0.14.0 tag, its published artifact and its Zenodo deposit
  are not rewritten.

## [0.14.0] - 2026-08-16

### Added — M14 bounded assurance software slice

- Added a bounded Python assurance component with strict UTF-8 JSON parsing,
  duplicate-member rejection, domain separation and closed envelopes.
- Added versioned observation, decision and witness payload contracts.
- Added explicit depth, node, member, array, string and raw-input bounds.
- Added safe-integer validation and rejection of booleans, floating-point
  values, exponent forms and negative zero in the accepted contract.
- Added RFC 8785 canonicalization using locked `rfc8785==0.1.4`.
- Added 87 software tests and a least-privilege CPython 3.12 workflow.
- Added M14 architecture, threat-model, decision, validation, evidence and
  open-gate documentation.

### Validated

- M14 merge commit on `main`:
  `e0cd5d11424de2acdec595e81438a935d38dc0c9`.
- Exact post-merge validation: 9 of 9 GitHub checks completed successfully.
- Corrective post-merge evidence-manifest SHA-256:
  `321092306757B170D6E722F704AD1B23885E0437C03EA6D52D27ED111FC821BE`.

### Limitations

- M14 remains a partial software-only research milestone.
- Physical STM32F401 qualification, distributed-controller simulation,
  attestation, freshness, failover/recovery and production certification are
  not established.

### Post-publication metadata

- Recorded the Zenodo DOI for Guardian F401 v0.13.0:
  https://doi.org/10.5281/zenodo.21923859
- Recorded the concept DOI representing all Guardian F401 versions:
  https://doi.org/10.5281/zenodo.21923858
- Recorded the public GitHub release and ORCID authorship linkage.
- Added a consolidated publication record without modifying the immutable
  v0.13.0 tag or its published ZIP artifact.

## [0.13.0] - 2026-08-13

### Added

- Guardian Protocol v0.1 framing, CRC32 validation and strict parsing.
- Python device simulator and guardianctl console.
- STM32F401 USART2 transport and ADC/timer/DMA acquisition architecture.
- RMS, peak, Hann-window, FFT, dominant-frequency and spectral-energy processing.
- Deterministic machine-health baseline, anomaly and hysteresis states.
- Supervisory-control state machine with logical run-permit boundary.
- Authenticated sessions, authorization roles and anti-replay controls.
- Robustness campaigns, malformed-frame testing, fuzz targets and fault injection.
- Portable signed-firmware lifecycle, version policy and anti-rollback model.
- STM32F401CDU6 hardware contract, Keil manifests and read-only qualification plan.
- Proprietary release license, citation metadata, security policy and evidence record.

### Verified in software

- Protocol, simulator, console and portable firmware test suites.
- Strict host compilation gates for portable C and STM32F401-facing translation units.
- Deterministic robustness and mutation campaigns in supported environments.

### Locally reported build milestone

- Arm Compiler 6.24 target build for STM32F401CDUx.
- Guardian sources: 19/19.
- Program size: Code 34528; RO-data 1292; RW-data 8; ZI-data 5800 bytes.
- HEX generation reported as PASS with 0 errors and 0 warnings.

The local Keil result is not independently reproducible from this repository
without the matching proprietary toolchain/device pack and retained build
evidence. See `docs/release-evidence-v0.13.0.md`.

### Pending validation

- Physical STM32F401CDU6 board qualification.
- Electrical, sensor, isolation and actuator-interface validation.
- Production root-of-trust, key provisioning and secure bootloader integration.
- Functional-safety, industrial-safety and cybersecurity certification.
