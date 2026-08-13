# Guardian F401

Guardian F401 is an STM32F401CDU6 embedded machine-health, supervisory-control and secure command platform.

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
M10 authenticated privileged command channel
  |
  +--> HMAC-SHA-256
  +--> mutual PSK proof
  +--> role authorization
  `--> strict uint64 anti-replay counter
```

## M10 Security Demo

Start the software simulator with security enforcement:

```text
python tools/run_simulator.py --secure
```

Set the intentionally public demo-only key:

```text
GUARDIAN_PSK_HEX =
00112233445566778899aabbccddeeff102132435465768798a9bacbdcedfe0f
```

Then:

```text
python tools/guardianctl.py security authenticate
python tools/guardianctl.py baseline start --samples 64
python tools/guardianctl.py control arm
python tools/guardianctl.py security status
```

The demo key must never be used on physical hardware.

Production firmware does not embed a PSK in the repository.

## Protected Commands

```text
BASELINE_CONTROL -> OPERATOR
CONTROL_COMMAND  -> OPERATOR
```

M10 protects privileged command authenticity, authorization and replay.

It does not encrypt payloads.

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

M10 authenticated sessions + authorization + anti-replay — implemented.

M11 robustness / fuzzing / fault injection — next.

See `docs/m10-security.md`.
