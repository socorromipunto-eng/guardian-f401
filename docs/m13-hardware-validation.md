# M13 — Hardware Integration + Keil/STM32F401 Validation

## Objective

M13 turns the previously implemented STM32-specific source into a repeatable hardware bring-up contract.

The milestone does **not** claim physical completion automatically.

It provides:

```text
compile-time STM32F401CDU6 target contract
startup clock/UID/calibration preflight
Keil source/include manifests
minimal CMSIS main-loop template
committed CMSIS-shaped target-source compile contract
read-only physical serial qualification tool
JSON qualification evidence
GitHub Actions target-source gate
```

## Target

Ordering code:

```text
STM32F401CDU6
```

Reference package:

```text
UFQFPN48
```

Guardian's reference hardware map remains:

```text
PA0  ADC1_IN0   vibration
PA1  ADC1_IN1   current
PA2  USART2_TX
PA3  USART2_RX
PA4  ADC1_IN4   supply divider
PA6  TIM3_CH1   RPM pulse input
```

The ST CMSIS device header groups `STM32F401CD` under:

```text
STM32F401xE
```

M13 now makes that device define a compile-time requirement for STM32-specific Guardian translation units.

## Target Contract

`guardian_stm32f401_target.h` centralizes:

```text
STM32F401xE device requirement
84 MHz HCLK ceiling
42 MHz APB1 ceiling
84 MHz APB2 ceiling
384 KiB STM32F401CD flash capacity
96 KiB SRAM capacity
USART2 PA2/PA3 AF7 mapping
ADC PA0/PA1/PA4 mapping
TIM3_CH1 PA6 AF2 mapping
factory calibration addresses
UART baud-error tolerance
```

Previously duplicated pin/calibration constants in the UART/acquisition adapters now consume this target contract.

## Startup Preflight

`guardian_stm32f401_preflight_run()` executes before Guardian enables USART2 or acquisition peripherals.

It is intentionally non-invasive.

It checks:

```text
SystemCoreClock is non-zero
HCLK <= 84 MHz
PCLK1 <= 42 MHz
PCLK2 <= 84 MHz
requested USART2 baud has a representable BRR
calculated UART baud error <= 20000 ppm
96-bit factory UID is not all zero/all erased
VREFINT factory calibration is present
temperature calibration points are present and distinct
```

Failure flags are stored in:

```text
guardian_stm32f401_preflight_report_t
```

The application preserves the most recent report and exposes:

```text
guardian_firmware_app_preflight()
```

`guardian_firmware_app_init()` returns failure before peripheral initialization when preflight fails.

This makes incorrect clock/device-pack assumptions visible early instead of allowing a partially working hardware target.

## Firmware Version

The physical hardware integration target now reports:

```text
0.13.0
```

through `DEVICE_INFO`.

## Keil Project Creation

Create a Keil MDK/uVision project using the STM32F401CDU6 device entry from the installed STM32F4 Device Family Pack.

The Device Pack should provide the CMSIS device/startup files.

Use:

```text
STM32F401xE
```

as the target device preprocessor definition.

Do not copy random CMSIS headers from another STM32F4 device into the project.

### Source Files

The exact Guardian source manifest is:

```text
firmware/MDK-ARM/guardian-f401-keil-sources.txt
```

The standalone reference target includes:

```text
firmware/MDK-ARM/Templates/main_guardian.c
```

The Keil/ST Device Pack supplies:

```text
startup_stm32f401xe.s
system_stm32f4xx.c
CMSIS core headers
STM32F401 device headers
```

### Include Paths

Use every path in:

```text
firmware/MDK-ARM/guardian-f401-keil-includes.txt
```

### Interrupt Ownership

For a standalone Guardian target, include:

```text
stm32f401_uart2_irq.c
stm32f401_acquisition_irq.c
main_guardian.c
```

Those files provide:

```text
USART2_IRQHandler
DMA2_Stream0_IRQHandler
ADC_IRQHandler
TIM3_IRQHandler
SysTick_Handler
```

If an existing application already owns one of those vectors, do not add the corresponding wrapper that would duplicate it.

Instead call the Guardian handler from the existing vector.

UART:

```text
guardian_stm32f401_uart2_irq_handler()
```

Acquisition:

```text
guardian_stm32f401_acquisition_dma_irq_handler()
guardian_stm32f401_acquisition_adc_irq_handler()
guardian_stm32f401_acquisition_tim3_irq_handler()
```

Existing one-millisecond tick:

```text
guardian_firmware_app_tick_1ms()
```

## Minimal Main Loop

The reference file:

```text
firmware/MDK-ARM/Templates/main_guardian.c
```

performs:

```text
SystemCoreClockUpdate
SysTick_Config(1 ms)
guardian_firmware_app_init(115200, 5)
foreground guardian_firmware_app_poll loop
```

On startup failure it disables interrupts and remains in a debugger-visible fail-stop loop.

Inspect:

```text
guardian_firmware_app_preflight()
```

before changing clock/peripheral code.

## M13 CI Target Contract

GitHub Actions cannot replace a Keil target build.

M13 therefore checks a narrower invariant automatically:

```text
all STM32F401 direct-register translation units
+
the exact Keil main template
```

must compile under strict C11 diagnostics against a committed CMSIS-shaped declaration contract while:

```text
STM32F401xE
```

is defined.

The compile stub exists only under:

```text
firmware/Tests/CMSISStub
```

It is not part of production firmware.

The real Keil target must use ST's official Device Pack headers.

## Physical UART Wiring

Use a 3.3 V TTL USB-to-UART adapter:

```text
STM32 PA2 / USART2_TX  -> adapter RX
STM32 PA3 / USART2_RX  <- adapter TX
STM32 GND              -- adapter GND
```

Do not apply RS-232 voltage levels directly to the MCU.

Reference UART configuration:

```text
115200
8 data bits
no parity
1 stop bit
no hardware flow control
```

## Host Serial Dependency

Install the existing optional serial dependency:

```text
python -m pip install -r console/requirements-serial.txt
```

## Physical Qualification Tool

M13 adds:

```text
python tools/hardware_validation.py --serial-port COM5
```

Linux example:

```text
python tools/hardware_validation.py --serial-port /dev/ttyUSB0
```

The default plan is intentionally read-only with respect to machine control and secure update.

It executes:

```text
PING
DEVICE_INFO
GET_STATUS
GET_SECURITY_STATUS
GET_FIRMWARE_STATUS
GET_CONTROL_STATUS
bounded telemetry observation
GET_DSP_FEATURES
GET_HEALTH_STATUS
```

It does **not** execute:

```text
baseline start/reset
control arm/disarm/clear-fault
security authenticate
firmware upload/activate
```

The tool writes:

```text
hardware-validation.json
```

The JSON artifact records:

```text
UTC timestamp
serial port
baud rate
host/Python metadata
exact guardianctl command per step
return code
elapsed milliseconds
stdout
stderr
pass/fail
```

Use another output path when desired:

```text
python tools/hardware_validation.py \
  --serial-port COM5 \
  --output artifacts/bench-001.json
```

## Qualification Sequence

Recommended first physical session:

```text
1. Confirm 3.3 V rail and common ground.
2. Leave external analog/machine wiring disconnected or safely conditioned.
3. Flash the Keil target.
4. Confirm execution reaches the foreground loop.
5. Inspect M13 preflight failure_flags == 0.
6. Connect the 3.3 V USB-UART adapter.
7. Install pyserial.
8. Run tools/hardware_validation.py.
9. Preserve the JSON report with the test date/board identifier.
10. Only then connect calibrated sensor front-ends.
```

## Debugger Watch Targets

Useful read-only functions:

```text
guardian_firmware_app_preflight()
guardian_firmware_app_acquisition_stats()
guardian_firmware_app_control_status()
guardian_firmware_app_security_status()
guardian_firmware_app_firmware_status()
```

The M9 run permit remains a logical application shadow in this reference integration.

M13 does not assign a physical actuator GPIO.

That prevents hardware bring-up from energizing a machine output unexpectedly.

## Expected Early Hardware Results

Before production provisioning:

```text
security status -> unprovisioned/unauthenticated is acceptable
firmware status -> lifecycle present but physical storage backend may remain unconfigured
control status  -> safe-off / interlock open is expected
```

Before actual sensor calibration:

```text
telemetry may carry DEFAULT_CALIBRATION
```

That flag is expected until the physical vibration/current/supply front-end is characterized.

## M12 Hardware Boundary

The M12 portable lifecycle is linked into the target, but this M13 bring-up template does not invent a flash partition or embed a signing secret.

Physical secure update still requires a board-specific boot/update backend:

```text
staging flash layout
trusted Ed25519 public key
bounded erase/write callbacks
staged image hashing
atomic pending-image metadata
atomic rollback-floor persistence
power-loss recovery
boot selection/fallback policy
```

Do not enable physical firmware activation until those pieces are reviewed together.

## Pass Criteria

M13 software/target contract:

```text
Python regressions PASS
portable C regressions PASS
STM32 target-source compile contract PASS
Keil source/include manifest complete
```

Physical board:

```text
Keil build succeeds for STM32F401CDU6
preflight failure_flags == 0
hardware-validation JSON reports PASS
UART diagnostics do not continuously accumulate errors
DMA blocks complete
no persistent ADC/DMA recovery storm
control remains safe-off during read-only qualification
```

Sensor calibration and real machine control remain separate hardware gates.

## Primary References

- STMicroelectronics DS10086 — STM32F401xD/xE datasheet.
- STMicroelectronics RM0368 — STM32F401xB/C and STM32F401xD/E reference manual.
- STMicroelectronics `cmsis-device-f4` — official STM32F4 CMSIS device headers.

<!-- M13-KEIL-BUILD-PASS-START -->

## Keil build qualification - 2026-08-13

Status: **PASS**

- Device: `STM32F401CDUx`
- CMSIS device define: `STM32F401xE`
- Toolchain: Arm Compiler `6.24`
- Guardian sources compiled: `19/19`
- Guardian application entry point: `main_guardian.c`
- CubeMX `main.c`: excluded from the Guardian target
- CubeMX `stm32f4xx_it.c`: excluded from the Guardian target
- CI-only `CMSISStub`: absent from the physical target
- Program size: Code=`34528`, RO-data=`1292`, RW-data=`8`, ZI-data=`5800`
- HEX generation: PASS
- Compiler/linker result: `0 Error(s), 0 Warning(s)`

This completes the Keil build portion of M13.

Physical M13 completion remains pending until a real STM32F401 target is
flashed, `guardian_firmware_app_preflight().failure_flags == 0`, and the
physical `hardware_validation.json` report returns PASS.

<!-- M13-KEIL-BUILD-PASS-END -->
