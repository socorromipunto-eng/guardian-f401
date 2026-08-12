# Guardian F401

Guardian F401 is an advanced embedded systems portfolio project built around the STM32F401CDU6.

## Current System

M5 adds asynchronous machine telemetry to the existing protocol, simulator, host console and STM32 middleware.

```text
Application measurements
         |
         v
Telemetry scheduler
         |
         v
Guardian TELEMETRY frame
         |
    +----+----+
    |         |
    v         v
 STM32 UART   TCP Simulator
    |         |
    +----+----+
         |
         v
guardianctl telemetry
```

## Commands

Synchronous diagnostics:

```text
python tools/guardianctl.py ping
python tools/guardianctl.py info
python tools/guardianctl.py status
```

Live simulator telemetry:

```text
python tools/run_simulator.py
python tools/guardianctl.py telemetry --period-ms 500 --count 10
```

JSON Lines:

```text
python tools/guardianctl.py --json telemetry --period-ms 250 --count 20
```

Physical UART after target validation:

```text
python tools/guardianctl.py --serial-port COM5 telemetry --period-ms 500 --count 10
```

## M5 Telemetry Contract

Control command:

```text
0x20 SET_TELEMETRY
```

Asynchronous channel:

```text
0x21 MACHINE_TELEMETRY
message_type = TELEMETRY
```

Published machine sample fields:

- device state
- monotonic timestamp
- temperature
- vibration RMS
- current
- RPM
- supply voltage
- status flags

The physical firmware exposes an application API for updating these fields.

M6 will connect real ADC, timer and DMA acquisition to that API.

The simulator values are deterministic synthetic data for protocol and UI testing only.

## Design Constraints

- telemetry is disabled by default;
- period is bounded to 100–60000 ms;
- the UART ISR remains byte-only;
- CRC, parsing and telemetry packing execute in foreground code;
- missed periods are dropped instead of creating catch-up bursts;
- no dynamic allocation is required by the embedded telemetry path;
- CRC detects accidental corruption but is not a security mechanism.

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
Source implemented; Keil and physical board validation remain a hardware gate.

### M5 — Asynchronous Telemetry
Implemented.

### M6 — Acquisition
Next: ADC, timers, DMA and deterministic sampling.

### M7 — DSP
RMS, FFT and spectral features.

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

See `docs/m5-telemetry.md`.
