# M5 — Asynchronous Telemetry

## Objective

M5 adds bounded device-to-host streaming without waiting for one request per sample.

```text
Sensors / application values
           |
           v
latest measurement snapshot
           |
           v
M5 telemetry scheduler
           |
           v
MACHINE_TELEMETRY frame
           |
      +----+----+
      |         |
      v         v
   UART       TCP simulator
      |         |
      +----+----+
           |
           v
guardianctl telemetry
```

## Control Command

M5 adds:

```text
0x20 SET_TELEMETRY
```

Request and response payload:

```text
Offset  Size  Field
------  ----  ----------------
0       1     SCHEMA_VERSION = 1
1       1     ENABLED (0 or 1)
2       2     PERIOD_MS
```

`PERIOD_MS` is big-endian and must be between 100 and 60000 ms.

This bound prevents accidental command-channel flooding.

## Telemetry Channel

M5 adds:

```text
0x21 MACHINE_TELEMETRY
```

The frame uses:

```text
message_type = TELEMETRY
command      = MACHINE_TELEMETRY
sequence     = independent telemetry sequence
```

Payload schema v1:

```text
Offset  Size  Field
------  ----  -------------------------
0       1     SCHEMA_VERSION = 1
1       1     DEVICE_STATE
2       4     TIMESTAMP_MS
6       2     TEMPERATURE_CENTI_C (i16)
8       2     VIBRATION_MG_RMS
10      2     CURRENT_MA
12      2     RPM
14      2     SUPPLY_MV
16      2     STATUS_FLAGS
```

All multi-byte fields use big-endian order.

## Scheduling Policy

Firmware emits at most one due telemetry frame per foreground poll.

If the main loop is delayed, missed periods are dropped rather than transmitted as a catch-up burst.

The UART ISR remains byte-only.

CRC calculation, telemetry packing and command handling remain in foreground code.

## Measurement Boundary

M5 does **not** claim that ADC/DMA sensor acquisition exists yet.

The physical firmware API receives the latest application-provided measurement snapshot:

```text
guardian_firmware_app_update_telemetry(...)
```

M6 will provide those values from real ADC, timers and DMA.

The software simulator generates deterministic synthetic measurements only for transport and UI testing.

## Host Demonstration

Simulator:

```text
python tools/run_simulator.py
python tools/guardianctl.py telemetry --period-ms 500 --count 10
```

JSON Lines:

```text
python tools/guardianctl.py --json telemetry --period-ms 250 --count 20
```

Physical UART after Keil/hardware validation:

```text
python tools/guardianctl.py --serial-port COM5 telemetry --period-ms 500 --count 10
```

## Security Boundary

Telemetry is CRC-protected against accidental corruption but is not cryptographically authenticated in M5.

Authentication, authorization and anti-replay protection remain planned for M10.

## Next Milestone

M6 connects real STM32F401 acquisition to these telemetry fields using ADC, timers and DMA.
