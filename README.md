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
```

## M12 Simulator Demo

Start secure mode:

```text
python tools/run_simulator.py --secure
```

Configure the public simulator M10 PSK:

```text
$env:GUARDIAN_PSK_HEX="00112233445566778899aabbccddeeff102132435465768798a9bacbdcedfe0f"
```

Build a simulator-only signed package:

```text
python tools/build_firmware_package.py demo --output demo-m12.gfu
```

Upload it with ADMIN authorization and mark it pending activation:

```text
python tools/guardianctl.py --role admin firmware upload demo-m12.gfu --activate
```

Inspect lifecycle state:

```text
python tools/guardianctl.py firmware status
```

The demo signing backend is intentionally test-only.

Production uses the M12 signature-verification callback with trusted public-key storage.

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

M12 secure firmware lifecycle + rollback protection — implemented.

The next engineering gate is physical STM32F401 integration: Keil target build, bootloader/flash layout, trusted Ed25519 verification, persistent metadata and power-loss testing.

See `docs/m12-firmware-lifecycle.md`.
