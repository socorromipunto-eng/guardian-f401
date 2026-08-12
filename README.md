# Guardian F401

Guardian F401 is an advanced embedded systems portfolio project built around the STM32F401CDU6.

## Purpose

The project demonstrates:

- modular STM32 firmware architecture
- transport-independent binary communication
- defensive stream parsing
- host/device protocol contracts
- software device simulation
- operator-facing host tooling
- UART-ready framing
- telemetry and diagnostics
- ADC, timers, interrupts and DMA
- DSP and machine-health analysis
- deterministic state machines
- authentication and authorization architecture
- anti-replay protection
- automated testing and fuzzing

## Current End-to-End System

M3 provides the first complete operator-to-device software path:

```text
Operator
   |
   v
guardianctl
   |
   v
GuardianClient
   |
   v
GuardianTcpTransport
   |
   v
Guardian Protocol v0.1
   |
   v
M2 Device Simulator
```

Available commands:

```text
python tools/guardianctl.py ping
python tools/guardianctl.py info
python tools/guardianctl.py status
```

The same host command model will later use UART to communicate with the STM32F401.

## Guardian Protocol v0.1

```text
+-------+---------+------+---------+-------+----------+--------+---------+-------+
| MAGIC | VERSION | TYPE | COMMAND | FLAGS | SEQUENCE | LENGTH | PAYLOAD | CRC32 |
+-------+---------+------+---------+-------+----------+--------+---------+-------+
```

Implemented properties:

- fixed 12-byte header
- maximum 256-byte payload
- big-endian multi-byte fields
- IEEE CRC32
- incremental stream parsing
- bounded STM32 memory
- canonical C/Python compatibility vectors
- command-specific payload codecs
- automated Python and portable C tests

## Quick Demonstration

Start the software device:

```text
python tools/run_simulator.py
```

In another terminal:

```text
python tools/guardianctl.py ping
python tools/guardianctl.py info
python tools/guardianctl.py status
```

JSON is available for automation:

```text
python tools/guardianctl.py --json status
```

## Development Phases

### M0 — Repository Foundation

Completed.

### M1 — Guardian Protocol v0.1

Completed.

### M2 — Device Simulator

Completed.

### M3 — guardianctl

Completed.

Implemented typed host operations, request sequence allocation, bounded TCP I/O, response correlation, remote-error handling, human output, JSON output and end-to-end simulator tests.

### M4 — STM32 UART Transport

Next.

The first physical STM32F401 integration will reuse Guardian Protocol v0.1 instead of inventing a new command path.

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

M3 — `guardianctl` implemented.

Next milestone: M4 — STM32 UART Transport.
