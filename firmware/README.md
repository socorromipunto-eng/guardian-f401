# Firmware

Target MCU:

```text
STM32F401CDU6
```

Primary environment:

- ARM Cortex-M4
- Keil MDK / uVision
- C
- ST/Keil CMSIS device pack
- Guardian Protocol v0.1

## M4 Physical Command Channel

M4 adds a reference USART2 implementation:

```text
PA2 -> USART2_TX
PA3 -> USART2_RX
AF7
115200 8-N-1
```

Architecture:

```text
USART2 ISR
   |
   +--> bounded RX/TX queues
   |
main loop
   |
   v
guardian_firmware_app_poll()
   |
   v
Guardian parser
   |
   v
device service
   |
   v
Guardian response
```

The ISR does not execute protocol parsing or command dispatch.

See `docs/m4-stm32-uart.md` before integrating sources into Keil.
