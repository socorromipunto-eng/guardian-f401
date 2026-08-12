# Firmware

Target:

```text
STM32F401CDU6
```

## M5 Telemetry

M5 adds a transport-independent asynchronous telemetry engine.

```text
application measurement snapshot
            |
            v
guardian_telemetry
            |
            v
guardian_embedded_link
            |
            v
bounded TX queue
            |
            v
USART2 ISR
```

The application supplies the newest measurement snapshot:

```text
guardian_firmware_app_update_telemetry(...)
```

The existing one-millisecond application tick calls:

```text
guardian_firmware_app_tick_1ms()
```

The main loop calls:

```text
guardian_firmware_app_poll()
```

The UART interrupt handler still performs byte movement only.

M5 does not implement sensor acquisition.

M6 will populate the measurement snapshot from ADC, timers and DMA.

See `docs/m5-telemetry.md`.
