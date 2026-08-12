# Guardian F401

Guardian F401 is an embedded machine-health and supervisory-control platform built around the STM32F401CDU6.

## Current Architecture

M6 connects deterministic STM32 acquisition to the asynchronous M5 telemetry path.

```text
         machine sensors
              |
      +-------+-------+
      |       |       |
 vibration current   RPM
      |       |       |
      v       v       v
    ADC1 scan       TIM3 capture
      |
      v
TIM2 trigger @ 4 kHz
      |
      v
DMA2 double buffer
      |
      v
portable acquisition processor
      |
      v
engineering measurements
      |
      v
Guardian M5 telemetry
      |
   +--+--+
   |     |
 UART   TCP simulator
   |     |
   +--+--+
      |
      v
guardianctl
```

## Physical Reference Mapping

```text
PA0 -> ADC1_IN0 vibration
PA1 -> ADC1_IN1 current
PA4 -> ADC1_IN4 supply divider
PA6 -> TIM3_CH1 RPM
PA2 -> USART2_TX
PA3 -> USART2_RX
```

ADC scan also includes the internal temperature sensor and VREFINT.

## Acquisition Properties

- TIM2 hardware-triggered ADC scan
- 4 kHz reference scan-frame rate
- five ADC conversions per frame
- DMA2 Stream0 Channel 0
- hardware double-buffer mode
- 64 scan frames per DMA target
- VREFINT supply compensation
- factory temperature calibration interpolation
- integer vibration RMS
- current and external supply conversion
- TIM3 input-capture RPM
- explicit acquisition quality flags
- foreground error recovery
- no dynamic allocation

## Calibration Boundary

The STM32 factory VREFINT and temperature calibration values are used directly.

Vibration, current, supply-divider and RPM sensor calibration are board-specific.

The reference configuration therefore publishes the `DEFAULT_CALIBRATION` quality flag until those values are replaced with real sensor calibration.

## Existing Host Demo

The simulator remains available:

```text
python tools/run_simulator.py
python tools/guardianctl.py telemetry --period-ms 500 --count 10
```

On real hardware, the same M5 telemetry schema will carry M6 measurements after Keil and physical-board validation.

## Development Phases

### M0 — Repository Foundation
Completed.

### M1 — Guardian Protocol v0.1
Completed.

### M2 — Device Simulator
Completed.

### M3 — guardianctl
Completed.

### M4 — STM32 UART Transport
Source implemented; physical validation remains a hardware gate.

### M5 — Asynchronous Telemetry
Completed.

### M6 — ADC + Timers + DMA Acquisition
Implemented in source with portable conversion tests.

### M7 — DSP
Next: RMS pipeline expansion, FFT and spectral features.

### M8 — Machine Health
Baseline modeling and anomaly detection.

### M9 — Supervisory Control
State machine, faults and outputs.

### M10 — Security
Authentication, authorization, sessions and anti-replay.

### M11 — Robustness
Fuzzing and fault injection.

### M12 — Firmware Lifecycle
Signed firmware updates and rollback protection.

See `docs/m6-acquisition.md`.
