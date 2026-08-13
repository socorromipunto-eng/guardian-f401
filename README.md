# Guardian F401

Guardian F401 is an STM32F401CDU6 embedded machine-health, supervisory-control and secure-command platform.

## Current Pipeline

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
```

## M13 Hardware Qualification

M13 adds a compile-time STM32F401CDU6 contract, fail-closed startup preflight, Keil bring-up manifests and a read-only serial qualification report.

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

The default physical qualification does not arm control, change baselines, authenticate privileged sessions or upload firmware.

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

Physical completion still requires a real Keil target build, STM32F401CDU6 board, bench wiring and a generated hardware-validation JSON report.

See `docs/m13-hardware-validation.md`.
