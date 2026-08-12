# Architecture

## Core Rule

UART is only a transport.

The Guardian Protocol defines messages.

The application decides whether a valid command is allowed to affect the machine.

```text
Application
    |
    v
Control / Diagnostics / DSP
    |
    v
Protocol / Security
    |
    v
Platform
    |
    v
STM32 Drivers
    |
    v
Hardware
```

## Separation of Responsibilities

- Drivers access STM32 peripherals.
- Platform code exposes hardware-dependent services.
- Protocol code parses and serializes messages.
- Security code validates identity, permissions and replay state.
- Application code owns system behavior.
- The host console never depends on the STM32 memory layout.
