# Guardian F401

Guardian F401 is an STM32F401CDU6 embedded machine-health and supervisory-control platform.

## Current Pipeline

```text
Sensors
  |
  v
M6 acquisition
  |
  v
M7 DSP / FFT
  |
  v
M8 healthy baseline + anomaly model
  |
  v
M9 supervisory-control policy
  |
  +--> local interlock
  +--> local-only run request
  +--> degraded operation
  +--> fault latching
  `--> logical safe run permit
```

## M9 Safety Property

The host has no RUN or START command.

`control arm` only arms supervision and leaves the logical permit safe-off.

The actual run request remains local to firmware/application integration.

## Host Demo

```text
python tools/run_simulator.py
python tools/guardianctl.py baseline start --samples 64
python tools/guardianctl.py control arm
python tools/guardianctl.py control status
```

Safe disable:

```text
python tools/guardianctl.py control disarm
```

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

M9 supervisory control + fault policy — implemented.

M10 authenticated sessions + authorization + anti-replay — next.

See `docs/m9-control.md`.
