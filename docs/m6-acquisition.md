# M6 — Deterministic ADC, Timer and DMA Acquisition

## Objective

M6 connects real STM32F401 acquisition infrastructure to the machine telemetry path introduced by M5.

```text
TIM2 @ 4 kHz
     |
     v
ADC1 regular scan
     |
     +--> PA0  vibration
     +--> PA1  current
     +--> PA4  supply divider
     +--> IN16 internal temperature
     `--> IN17 VREFINT
     |
     v
DMA2 Stream0 / Channel0
double-buffer M0 <-> M1
     |
     v
portable acquisition processor
     |
     +--> VREF compensation
     +--> temperature calibration
     +--> vibration RMS
     +--> current
     +--> supply
     `--> RPM from TIM3_CH1
     |
     v
M5 machine telemetry snapshot
```

## STM32F401 Reference Mapping

Target:

```text
STM32F401CDU6
```

Reference external signals:

```text
PA0 / ADC1_IN0  -> analog vibration front-end
PA1 / ADC1_IN1  -> analog current front-end
PA4 / ADC1_IN4  -> external supply resistor divider
PA6 / TIM3_CH1  -> digital RPM pulse input
```

The UART transport remains:

```text
PA2 / USART2_TX
PA3 / USART2_RX
```

For the UFQFPN48 package, the STM32F401xD/xE datasheet places PA0, PA1, PA4 and PA6 on package pins 10, 11, 14 and 16 respectively.

## ADC Scan

The regular scan order is frozen for M6:

```text
slot 1 -> ADC1_IN0  vibration
slot 2 -> ADC1_IN1  current
slot 3 -> ADC1_IN4  supply divider
slot 4 -> ADC1_IN16 internal temperature sensor
slot 5 -> ADC1_IN17 VREFINT
```

TIM2 update is configured as TRGO.

ADC1 selects TIM2_TRGO as the rising-edge regular conversion trigger.

The reference scan-frame rate is:

```text
4000 Hz
```

The allowed configuration range is:

```text
100 .. 10000 Hz
```

The ADC common prescaler is selected at runtime from:

```text
PCLK2 / 2
PCLK2 / 4
PCLK2 / 6
PCLK2 / 8
```

M6 chooses the fastest option that keeps the ADC kernel clock at or below:

```text
18 MHz
```

That ceiling is intentionally conservative across the STM32F401xD/xE datasheet VDDA range.

Initialization also verifies that the selected ADC clock can finish the complete five-conversion sequence before the next TIM2 trigger.

Reference sample times:

```text
external channels -> 56 ADC cycles
temperature       -> 480 ADC cycles
VREFINT            -> 480 ADC cycles
```

The long internal-channel sample time is deliberate. The STM32F401xD/xE datasheet specifies a 10 us minimum sampling requirement for the internal temperature sensor and VREFINT when accuracy matters.

## DMA Double Buffer

ADC1 uses the STM32F401 DMA2 Channel 0 request mapping.

M6 selects:

```text
DMA2 Stream0
Channel 0
Peripheral -> memory
16-bit peripheral width
16-bit memory width
memory increment
double-buffer mode
64 scan frames per target
```

One target contains:

```text
64 frames * 5 channels = 320 half-words
```

At 4 kHz, one target completes every:

```text
16 ms
```

When one target completes, DMA switches to the other target.

The foreground code snapshots the completed target into a dedicated processing buffer while hardware fills the other DMA target.

The driver checks the hardware `CT` target before and after that bounded copy. A target that has already become active again is discarded rather than processed concurrently with DMA.

If foreground processing misses a target before it is reused, M6 increments `dma_blocks_dropped` and publishes the `DMA_DROP` quality bit with the next trustworthy measurement.

DMA transfer errors fail closed. The potentially incomplete target is discarded and the ADC/DMA stream is reconstructed in foreground code.

## RPM Capture

M6 uses:

```text
PA6 -> AF2 -> TIM3_CH1
```

TIM3 runs near:

```text
10 kHz
```

Each rising edge captures the hardware 16-bit counter.

TIM3 update interrupts extend that counter in software, producing a 32-bit modulo capture timestamp. This allows low-speed periods to span multiple hardware counter wraps without corrupting the RPM calculation.

The period between two extended timestamps is converted using:

```text
RPM = timer_hz * 60 / (period_ticks * pulses_per_revolution)
```

Default:

```text
pulses_per_revolution = 1
```

RPM is marked stale after:

```text
2000 ms
```

The actual sensor must provide a clean logic signal appropriate for the MCU input.

## Factory Calibration

M6 reads the STM32 factory calibration half-words documented for STM32F401xD/xE:

```text
VREFIN_CAL -> 0x1FFF7A2A
TS_CAL1    -> 0x1FFF7A2C
TS_CAL2    -> 0x1FFF7A2E
```

`VREFIN_CAL` is used to estimate actual VDDA.

Temperature ADC data is normalized to the 3.3 V factory calibration domain before interpolation between the 30 C and 110 C factory points.

## External Sensor Calibration

The repository cannot know the analog front-end attached to a future physical board.

Therefore M6 ships explicit **reference defaults** for:

```text
vibration zero code
vibration mg/code scale
current zero code
current mA/code scale
PA4 resistor-divider ratio
RPM pulses/revolution
```

The telemetry `status_flags` field includes:

```text
DEFAULT_CALIBRATION
```

until those values are replaced with actual board/sensor calibration.

This prevents reference values from being mistaken for laboratory calibration.

## Quality Flags

M6 uses M5 `status_flags` for acquisition quality:

```text
0x0001 VALID
0x0002 ADC_SATURATED
0x0004 RPM_STALE
0x0008 DMA_DROP
0x0010 DEFAULT_CALIBRATION
0x0020 ADC_ERROR
0x0040 DMA_ERROR
```

## Keil Target Files

Add:

```text
firmware/Acquisition/Src/guardian_acquisition.c
firmware/Platform/STM32F401/Src/stm32f401_acquisition.c
firmware/App/Src/guardian_firmware_app.c
```

For a standalone target that does not already own the vectors, also add:

```text
firmware/Platform/STM32F401/Src/stm32f401_acquisition_irq.c
```

Include paths:

```text
firmware/Protocol/Inc
firmware/App/Inc
firmware/Telemetry/Inc
firmware/Acquisition/Inc
firmware/Platform/Inc
firmware/Platform/STM32F401/Inc
```

The target device define remains:

```text
STM32F401xE
```

## Existing Interrupt Ownership

If existing firmware already defines any of these vectors:

```text
DMA2_Stream0_IRQHandler
ADC_IRQHandler
TIM3_IRQHandler
```

do not add `stm32f401_acquisition_irq.c`.

Call the corresponding Guardian handler from the existing vector:

```text
guardian_stm32f401_acquisition_dma_irq_handler()
guardian_stm32f401_acquisition_adc_irq_handler()
guardian_stm32f401_acquisition_tim3_irq_handler()
```

This avoids duplicate vector symbols.

## Main Application Integration

M6 is already integrated into:

```text
guardian_firmware_app_init()
guardian_firmware_app_poll()
guardian_firmware_app_tick_1ms()
```

The foreground loop now:

```text
process one ready DMA block
        |
        v
convert raw acquisition
        |
        v
update M5 telemetry snapshot
        |
        v
process Guardian commands/telemetry
```

## Electrical Boundary

External analog signals connected to PA0, PA1 or PA4 must be conditioned for the ADC input range.

Do not connect an unknown machine-level voltage directly to the MCU.

The PA4 supply measurement requires a resistor divider appropriate for the voltage being measured and the ADC input requirements.

The vibration and current channels require sensor-specific analog front-ends and calibration before their engineering-unit values can be considered accurate.

## Verification Boundary

Automated host verification covers:

- raw interleaved block framing;
- VREFINT compensation;
- factory-style temperature interpolation;
- integer vibration RMS;
- current and supply conversion;
- stale RPM flags;
- ADC saturation flags;
- previous M4/M5 middleware regression tests;
- strict C compiler warnings;
- target-specific source syntax against CMSIS-shaped declarations.

Physical completion still requires:

```text
Keil target build
real STM32F401CDU6
actual clock tree
actual sensor front-end
oscilloscope/logic-analyzer timing verification
ADC calibration measurements
RPM sensor validation
```

## Primary References

- STMicroelectronics RM0368, STM32F401xB/C and STM32F401xD/E reference manual.
- STMicroelectronics DS10086, STM32F401xD/xE datasheet.
- STMicroelectronics STM32F4 CMSIS device headers.

## Next Milestone

M7 will consume deterministic M6 sample blocks for DSP:

```text
DC removal
RMS
peak
crest factor
frequency-domain analysis
FFT / spectral energy
```
