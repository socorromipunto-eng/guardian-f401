# Guardian F401

Guardian F401 is an advanced embedded systems portfolio project built around the STM32F401CDU6.

## Current System

M4 adds the first physical STM32F401 transport while preserving the Guardian Protocol contract used by the simulator.

```text
                       GuardianClient
                            |
             +--------------+--------------+
             |                             |
             v                             v
     TCP development                 Physical serial
        transport                      transport
             |                             |
             v                             v
      M2 Simulator                  USB-to-UART adapter
                                           |
                                           v
                                  STM32F401 USART2
                                    PA2 TX / PA3 RX
                                           |
                                           v
                                    IRQ byte queues
                                           |
                                           v
                                  Guardian Embedded Link
                                           |
                                           v
                                   Device Command Service
```

## Host Commands

Simulator:

```text
python tools/guardianctl.py ping
python tools/guardianctl.py info
python tools/guardianctl.py status
```

Physical UART:

```text
python tools/guardianctl.py --serial-port COM5 ping
python tools/guardianctl.py --serial-port COM5 info
python tools/guardianctl.py --serial-port COM5 status
```

## Physical UART Reference

```text
STM32F401CDU6
USART2
PA2 = TX
PA3 = RX
AF7
115200
8-N-1
```

The ISR performs bounded byte movement only.

CRC, parsing and command dispatch execute in foreground code.

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
Implemented in source.

Physical Keil build and board validation remain required before declaring the hardware path verified.

### M5 — Telemetry
Next.

### M6 — Acquisition
ADC, timers and DMA.

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

See `docs/m4-stm32-uart.md`.
