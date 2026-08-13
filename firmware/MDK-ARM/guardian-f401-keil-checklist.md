# Guardian F401 Keil Bring-Up Checklist

## Target

```text
MCU: STM32F401CDU6
CMSIS define: STM32F401xE
UART: USART2 PA2/PA3 AF7, 115200 8-N-1
```

## Project

```text
[ ] Select STM32F401CDU6 in the installed STM32F4 Device Family Pack.
[ ] Use ST/Keil CMSIS startup and system files for STM32F401xE.
[ ] Define STM32F401xE.
[ ] Add every path from guardian-f401-keil-includes.txt.
[ ] Add every Guardian source from guardian-f401-keil-sources.txt.
[ ] Build with warnings enabled.
[ ] Resolve every compiler/linker error before flashing.
```

## IRQ Ownership

```text
[ ] Exactly one USART2_IRQHandler.
[ ] Exactly one DMA2_Stream0_IRQHandler.
[ ] Exactly one ADC_IRQHandler.
[ ] Exactly one TIM3_IRQHandler.
[ ] Exactly one SysTick_Handler.
```

When another module owns a vector, remove the Guardian wrapper and forward to the Guardian handler explicitly.

## First Flash

```text
[ ] No external machine-level voltage is connected directly to MCU analog pins.
[ ] No physical actuator output is connected to the logical M9 run-permit shadow.
[ ] 3.3 V supply and ground verified.
[ ] Debugger reaches guardian_firmware_app_poll().
[ ] guardian_firmware_app_preflight().failure_flags == 0.
```

## Serial Qualification

```text
python -m pip install -r console/requirements-serial.txt
python tools/hardware_validation.py --serial-port COM5 --output hardware-validation.json
```

Expected:

```text
Hardware qualification: PASS
```

Keep the JSON report as bench evidence.
