# Guardian F401

Guardian F401 is an advanced embedded systems portfolio project built around the STM32F401CDU6.

## Purpose

The project is designed to demonstrate:

- modular STM32 firmware architecture
- UART communication
- transport-independent binary protocols
- deterministic state machines
- telemetry and diagnostics
- ADC, timers, interrupts and DMA
- DSP and machine-health analysis
- defensive protocol parsing
- authentication and authorization architecture
- anti-replay protection
- host-side tooling
- device simulation
- automated testing and fuzzing

## System Architecture

```text
Physical Process
      |
      v
Sensors
      |
      v
STM32F401CDU6
      |
      +--> Acquisition
      +--> DSP
      +--> Diagnostics
      +--> State Machine
      +--> Supervisory Control
      +--> Security
      |
      v
Guardian Protocol
      |
      +--> guardianctl
      |
      +--> Device Simulator
```

## Guardian Protocol v0.1

M1 now defines a real byte-level contract shared by C firmware and Python host tools.

```text
+-------+---------+------+---------+-------+----------+--------+---------+-------+
| MAGIC | VERSION | TYPE | COMMAND | FLAGS | SEQUENCE | LENGTH | PAYLOAD | CRC32 |
+-------+---------+------+---------+-------+----------+--------+---------+-------+
```

Key properties:

- fixed 12-byte header
- maximum 256-byte payload
- big-endian multi-byte fields
- IEEE CRC32
- incremental stream parsing
- bounded STM32 memory
- canonical C/Python compatibility vectors
- automated Python and portable C tests

## Repository Structure

```text
guardian-f401/
├── firmware/
│   ├── Core/
│   ├── App/
│   ├── Platform/
│   ├── Protocol/
│   ├── Security/
│   ├── Diagnostics/
│   ├── Acquisition/
│   ├── DSP/
│   ├── Control/
│   ├── Tests/
│   └── MDK-ARM/
├── console/
├── simulator/
├── protocol/
├── docs/
├── tests/
├── tools/
├── fuzz/
└── .github/
```

## Development Phases

### M0 — Repository Foundation

Completed.

### M1 — Guardian Protocol v0.1

Completed.

Binary framing, bounded parsing, sequence correlation, CRC32, C/Python codecs and tests.

### M2 — Device Simulator

Next.

A software implementation of the future embedded device.

### M3 — guardianctl

A host command-line console using the same protocol as the physical STM32.

### M4 — STM32 UART Transport

Real communication with the STM32F401CDU6.

### M5 — Telemetry

Asynchronous measurements, diagnostics and event reporting.

### M6 — Acquisition

ADC, timers, DMA and deterministic data capture.

### M7 — DSP

RMS, FFT, spectral features and signal processing.

### M8 — Machine Health

Baseline modeling and anomaly detection.

### M9 — Supervisory Control

State machine, faults and controlled outputs.

### M10 — Security

Authentication, authorization, session management and anti-replay controls.

### M11 — Robustness

Fuzzing, malformed frames, fault injection and recovery testing.

### M12 — Firmware Lifecycle

Signed firmware update architecture and rollback protection.

## Current Milestone

M1 — Guardian Protocol v0.1 implemented.

Next milestone: M2 — Device Simulator.
