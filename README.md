# Guardian F401

Guardian F401 is an advanced embedded systems portfolio project built around the STM32F401CDU6.

## Purpose

The project demonstrates:

- modular STM32 firmware architecture
- transport-independent binary communication
- defensive stream parsing
- host/device protocol contracts
- software device simulation
- UART-ready framing
- telemetry and diagnostics
- ADC, timers, interrupts and DMA
- DSP and machine-health analysis
- deterministic state machines
- authentication and authorization architecture
- anti-replay protection
- automated testing and fuzzing

## Architecture

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
      `--> Device Simulator
```

## Implemented Vertical Slice

M2 now provides a real software device behind the protocol.

```text
Host bytes
    |
    v
Guardian Protocol v0.1
    |
    v
Incremental parser
    |
    v
GuardianDevice
    |
    +--> PING
    +--> DEVICE_INFO
    `--> GET_STATUS
    |
    v
Guardian response bytes
```

The simulator currently uses local TCP as a development transport.

The same binary frames are intended for STM32 UART later.

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

## Run the Simulator

From the repository root:

```text
python tools/run_simulator.py
```

Default endpoint:

```text
127.0.0.1:9401
```

The default loopback bind avoids exposing the development simulator to the LAN.

## Development Phases

### M0 — Repository Foundation

Completed.

### M1 — Guardian Protocol v0.1

Completed.

### M2 — Device Simulator

Completed.

Implemented command dispatch, binary metadata/status payloads, diagnostics, real loopback TCP transport and integration tests.

### M3 — guardianctl

Next.

The host command-line console will implement:

```text
guardianctl ping
guardianctl info
guardianctl status
```

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

M2 — Device Simulator implemented.

Next milestone: M3 — `guardianctl`.
