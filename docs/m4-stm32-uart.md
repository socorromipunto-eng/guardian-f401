# M4 — STM32F401 Physical UART Transport

## Objective

M4 moves Guardian Protocol v0.1 from software-only transport into physical STM32F401 firmware while preserving the host command contract.

```text
guardianctl
    |
    | USB-to-UART
    v
PA3 RX     PA2 TX
    \       /
     USART2
       |
       v
IRQ byte queues
       |
       v
Guardian Embedded Link
       |
       v
Guardian Protocol Parser
       |
       v
Device Service
```

## Reference Hardware Configuration

Target ordering code:

```text
STM32F401CDU6
```

ST's STM32F401xD/xE datasheet identifies:

```text
PA2 -> USART2_TX
PA3 -> USART2_RX
```

For the 48-pin UQFN/UFQFPN package, PA2 is pin 12 and PA3 is pin 13.

The alternate-function table assigns USART2 to AF7 on PA2 and PA3.

The official ST CMSIS `stm32f4xx.h` groups STM32F401CD under the `STM32F401xE` device define.

## Keil Configuration

Use the STM32F4 Device Family Pack/CMSIS device files supplied by Keil/ST.

The target compiler preprocessor must include:

```text
STM32F401xE
```

M4's USART2 adapter uses CMSIS register access directly and does not require HAL runtime calls.

## Files To Add To the Keil Target

```text
firmware/Protocol/Src/guardian_protocol.c
firmware/Protocol/Src/guardian_parser.c
firmware/App/Src/guardian_device_service.c
firmware/App/Src/guardian_firmware_app.c
firmware/Platform/Src/guardian_embedded_link.c
firmware/Platform/STM32F401/Src/stm32f401_uart2.c
firmware/Platform/STM32F401/Src/stm32f401_uart2_irq.c
```

Include paths:

```text
firmware/Protocol/Inc
firmware/App/Inc
firmware/Platform/Inc
firmware/Platform/STM32F401/Inc
```


If the existing firmware already defines `USART2_IRQHandler`, do **not** add
`stm32f401_uart2_irq.c` to the Keil target. Instead, call:

```text
guardian_stm32f401_uart2_irq_handler();
```

from the existing USART2 interrupt handler. This avoids duplicate vector
symbols while preserving the current firmware interrupt architecture.

## Electrical Connection

Use a **3.3 V TTL UART** USB adapter.

```text
STM32 PA2 / USART2_TX  -> USB-UART RX
STM32 PA3 / USART2_RX  <- USB-UART TX
STM32 GND              -- USB-UART GND
```

Do not connect RS-232 voltage levels directly to the MCU.

## UART Configuration

M4 uses:

```text
115200 baud
8 data bits
No parity
1 stop bit
No hardware flow control
```

The driver derives APB1 clock from `SystemCoreClock` and RCC `PPRE1`.

## Interrupt Architecture

The interrupt handler does not parse Guardian messages.

It performs bounded byte movement only:

```text
USART2 IRQ
   |
   +--> RXNE -> RX ring queue
   |
   `--> TXE  <- TX ring queue
```

Foreground code performs:

```text
main loop
   |
   v
guardian_firmware_app_poll()
   |
   v
Guardian incremental parser
   |
   v
command service
   |
   v
response encoder
   |
   v
TX ring queue
```

## Existing Main Loop Integration

After the application's clock setup:

```text
guardian_firmware_app_init(115200U, 5U);
```

Inside the main loop:

```text
guardian_firmware_app_poll();
```

From an existing one-millisecond system tick:

```text
guardian_firmware_app_tick_1ms();
```

Do not add a second `SysTick_Handler` when the existing firmware already owns it.

## Host Setup

Install the optional serial dependency:

```text
python -m pip install -r console/requirements-serial.txt
```

Then use the operating-system serial port, for example:

```text
python tools/guardianctl.py --serial-port COM5 ping
python tools/guardianctl.py --serial-port COM5 info
python tools/guardianctl.py --serial-port COM5 status
```

## Validation Boundary

Portable M4 middleware and host serial abstractions are automatically tested.

The target-specific register adapter is based on ST documentation but still requires a Keil target build and real STM32F401CDU6 hardware validation before hardware completion is claimed.

## References

- STMicroelectronics DS10086 — STM32F401xD/xE datasheet.
- STMicroelectronics RM0368 — STM32F401xB/C and STM32F401xD/E reference manual.
- STMicroelectronics `cmsis-device-f4` — official STM32F4 CMSIS device headers.
