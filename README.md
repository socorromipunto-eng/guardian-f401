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
```

## M11 Robustness Demo

Run one deterministic defensive campaign:

```text
python tools/run_robustness.py --seed 0xC0FFEE11 --iterations 1000
```

Expected summary:

```text
Guardian M11 robustness campaign: PASS
Parser recovery: 1000/1000
Secure tamper rejection: 1000/1000
Exact replay rejections: 1
Counter-gap rejections: 1
```

M11 also includes:

```text
GCC ASan/UBSan parser mutation driver
GCC ASan/UBSan security mutation driver
Clang libFuzzer parser harness
Clang libFuzzer secure-envelope harness
committed parser/security seed corpora
GitHub Actions robustness gate
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

M9 supervisory control + fault policy — completed.

M10 authenticated sessions + authorization + anti-replay — completed.

M11 robustness + fuzzing + fault injection — implemented.

M12 secure firmware lifecycle — next.

See `docs/m11-robustness.md`.
