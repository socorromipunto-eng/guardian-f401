# Guardian F401

Guardian F401 is an STM32F401CDU6 embedded machine-health and supervisory-control platform.

## Current Pipeline

```text
Sensors
  |
  v
M6 ADC + timers + DMA
  |
  v
M7 DSP / FFT / spectral features
  |
  v
M8 runtime healthy baseline
  |
  v
weighted anomaly scoring + hysteresis
  |
  v
health state
  |
  v
Guardian Protocol / guardianctl
```

## M8 Host Commands

Start an explicit healthy baseline:

```text
python tools/guardianctl.py baseline start --samples 64
```

Read health:

```text
python tools/guardianctl.py health
python tools/guardianctl.py --json health
```

Reset:

```text
python tools/guardianctl.py baseline reset
```

## M8 States

```text
UNTRAINED
LEARNING
READY
WARNING
ALARM
```

The baseline is runtime-only and frozen after learning.

Bad ADC/DMA feature blocks are rejected rather than learned.

The health score is an engineering condition indicator, not a probability of failure.

## Milestones

M0 repository foundation — completed.

M1 Guardian Protocol v0.1 — completed.

M2 device simulator — completed.

M3 guardianctl — completed.

M4 STM32 UART transport — source implemented.

M5 asynchronous telemetry — completed.

M6 ADC + timers + DMA acquisition — implemented.

M7 DSP / FFT / spectral features — completed.

M8 machine-health baseline + anomaly detection — implemented.

M9 supervisory control — next.

See `docs/m8-health.md`.
