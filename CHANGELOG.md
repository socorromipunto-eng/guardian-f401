# Changelog

All notable changes to Guardian F401 are documented in this file.

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
