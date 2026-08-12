# Guardian F401

Guardian F401 is an STM32F401CDU6 embedded machine-health platform.

## Current Pipeline

```text
Sensors
  |
  v
M6 ADC + timers + DMA
  |
  v
calibrated 64-sample vibration blocks
  |
  v
M7 DSP
  |
  +--> RMS
  +--> peak
  +--> crest factor
  +--> 64-point FFT
  +--> dominant frequency
  +--> spectral centroid
  `--> low/mid/high spectral energy
  |
  v
Guardian Protocol
  |
  v
guardianctl dsp
```

## Host Commands

```text
python tools/guardianctl.py ping
python tools/guardianctl.py info
python tools/guardianctl.py status
python tools/guardianctl.py telemetry --period-ms 500 --count 10
python tools/guardianctl.py dsp
python tools/guardianctl.py --json dsp
```

## Milestones

M0 repository foundation — completed.

M1 Guardian Protocol v0.1 — completed.

M2 device simulator — completed.

M3 guardianctl — completed.

M4 STM32 UART transport — source implemented.

M5 asynchronous telemetry — completed.

M6 ADC + timers + DMA acquisition — implemented.

M7 DSP / FFT / spectral features — implemented.

M8 machine-health baseline and anomaly detection — next.

See `docs/m7-dsp.md`.
