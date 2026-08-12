# Firmware

Target:

```text
STM32F401CDU6
```

## Current Physical Pipeline

```text
TIM2 4 kHz
   |
   v
ADC1 five-channel scan
   |
   v
DMA2 Stream0 Channel0
double buffer
   |
   v
guardian_acquisition
   |
   +--> vibration RMS
   +--> current
   +--> supply
   +--> internal temperature
   +--> VREF compensation
   `--> TIM3 RPM
   |
   v
M5 telemetry
```

Reference pins:

```text
PA0 -> vibration analog
PA1 -> current analog
PA4 -> supply-divider analog
PA6 -> TIM3_CH1 RPM
PA2 -> USART2_TX
PA3 -> USART2_RX
```

M6 keeps parsing, engineering-unit conversion and telemetry packing out of interrupt context.

DMA/ADC/TIM3 IRQ handlers perform bounded state updates only.

External sensor calibration remains explicit and must be replaced for the final physical front-end.

See `docs/m6-acquisition.md`.
