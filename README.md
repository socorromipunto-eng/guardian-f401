# Guardian F401

Guardian F401 is an STM32F401CDU6 embedded machine-health,
supervisory-control and secure-command platform.

## Current pipeline

```text
M6 acquisition
  |
M7 DSP / FFT
  |
M8 machine-health baseline
  |
M9 supervisory control
  |
M10 authenticated privileged commands
  |
M11 robustness / fuzzing / fault injection
  |
M12 signed firmware lifecycle / anti-rollback
  |
M13 STM32F401 hardware / Keil qualification
  |
M14 bounded assurance software slice
```

## M13 hardware qualification

M13 adds a compile-time STM32F401CDU6 contract, fail-closed startup
preflight, Keil bring-up manifests and a read-only serial qualification
report.

Keil integration files:

```text
firmware/MDK-ARM/Templates/main_guardian.c
firmware/MDK-ARM/guardian-f401-keil-sources.txt
firmware/MDK-ARM/guardian-f401-keil-includes.txt
firmware/MDK-ARM/guardian-f401-keil-checklist.md
```

Physical serial qualification:

```text
python -m pip install -r console/requirements-serial.txt
python tools/hardware_validation.py --serial-port COM5 --output hardware-validation.json
```

The default physical qualification does not arm control, change baselines,
authenticate privileged sessions or upload firmware.

## M14 bounded assurance software slice

The current M14 feature branch adds a software-only deterministic assurance
boundary for strict JSON validation and RFC 8785 canonicalization. It includes
closed observation, decision and witness payload contracts, explicit resource
bounds and an exact 87-test CPython 3.12 CI gate.

```text
untrusted bytes
      |
      v
strict parse + duplicate rejection
      |
      v
bounded closed envelope and payload schema
      |
      v
RFC 8785 canonical bytes
      |
      v
software evidence only — no physical authority
```

Validated feature-branch commit:
`072446f8e0d633095406bb34601a6e3f3d2836e0`.

This slice does not establish hardware qualification, real-time behavior,
signatures, freshness, attestation, distributed failover, production
readiness or certification. Canonical or schema-valid data cannot authorize a
physical action.

See `docs/m14-assurance.md`.

## Milestones

M0 repository foundation — completed.

M1 Guardian Protocol v0.1 — completed.

M2 device simulator — completed.

M3 guardianctl — completed.

M4 STM32 UART transport — source implemented.

M5 asynchronous telemetry — completed.

M6 ADC + timers + DMA acquisition — implemented.

M7 DSP / FFT / spectral features — completed.

M8 machine-health baseline + anomaly detection — completed.

M9 supervisory control + fault policy — completed.

M10 authenticated sessions + authorization + anti-replay — completed.

M11 robustness + fuzzing + fault injection — completed.

M12 secure firmware lifecycle + rollback protection — completed.

M13 hardware integration + Keil/STM32F401 validation — implemented.

M14 distributed Guardian architecture — partial software-only assurance slice
validated; distributed architecture, simulation and hardware gates remain
open.

Physical completion still requires a real Keil target build,
STM32F401CDU6 board, bench wiring and a generated hardware-validation JSON
report.

See `docs/m13-hardware-validation.md`.

## Release and citation

Project release: v0.13.0
Guardian Protocol: v0.1
Firmware semantic version: 0.13.0

Author: Antonio José Socorro Marín
ORCID: https://orcid.org/0009-0007-9089-9222

Archived release: https://doi.org/10.5281/zenodo.21923859
All versions: https://doi.org/10.5281/zenodo.21923858
GitHub release: https://github.com/socorromipunto-eng/guardian-f401/releases/tag/v0.13.0

Citation metadata is provided in `CITATION.cff`. This release is governed by
the proprietary terms in `LICENSE`; public visibility does not create an
open-source license. Physical board qualification and safety certification are
not claimed. See `docs/release-evidence-v0.13.0.md`.

## Reproducible software validation

```text
python tools/validate_keil_manifest.py
PYTHONPATH=protocol/python python -m unittest discover -s protocol/python/tests -v
PYTHONPATH=protocol/python:simulator/src python -m unittest discover -s simulator/tests -v
PYTHONPATH=protocol/python:simulator/src:console/src python -m unittest discover -s console/tests -v
```

The GitHub Actions workflows also compile and execute the portable C suites,
STM32F401 compile contract and bounded robustness campaigns. These checks do
not replace a Keil target build or physical hardware qualification.

<!-- GUARDIAN-F401-RIGHTS-START -->

## Licensing and commercial use

Copyright (c) 2026 **Antonio José Socorro Marín**. All rights reserved.

The original Guardian F401 code is **not released under an open-source
license**. No general commercial license is granted.

Public visibility of this repository does not grant additional permission for
commercial use, redistribution, modification, sublicensing, or incorporation
of the original Guardian F401 code into another product or service, except for
rights provided by applicable law and GitHub's Terms of Service for public
repositories.

Commercial licensing may be available separately from
**Antonio José Socorro Marín**.

Third-party components remain subject to their own licenses and copyright
notices. See `COPYRIGHT` and `NOTICE` for additional information.

## Safety and deployment status

Guardian F401 is an experimental engineering and research project. It is not a
certified functional-safety or industrial-safety controller.

Hardware deployment requires independent electrical, firmware, security,
failure-mode, and regulatory validation.

<!-- GUARDIAN-F401-RIGHTS-END -->
