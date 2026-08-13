# Changelog

All notable changes to Guardian F401 are documented in this file.

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
evidence. See docs/release-evidence-v0.13.0.md.

### Pending validation

- Physical STM32F401CDU6 board qualification.
- Electrical, sensor, isolation and actuator-interface validation.
- Production root-of-trust, key provisioning and secure bootloader integration.
- Functional-safety, industrial-safety and cybersecurity certification.
