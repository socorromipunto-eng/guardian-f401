# Firmware

M13 adds the STM32F401CDU6 hardware qualification layer around the existing M4/M6 physical adapters.

New target contract:

```text
firmware/Platform/STM32F401/Inc/guardian_stm32f401_target.h
```

New startup preflight:

```text
firmware/Platform/STM32F401/Inc/guardian_stm32f401_preflight.h
firmware/Platform/STM32F401/Src/guardian_stm32f401_preflight.c
```

The preflight validates:

```text
HCLK/APB clock limits
USART2 baud representability/error
factory UID presence
VREFINT calibration presence
temperature calibration presence/order
```

`guardian_firmware_app_init()` now fails before peripheral startup when the preflight contract fails.

The physical hardware firmware reports:

```text
0.13.0
```

Keil source/include manifests and the minimal standalone `main()` template are under:

```text
firmware/MDK-ARM
```

The committed `firmware/Tests/CMSISStub` directory is only a host CI compile contract.

Never add that test stub to the physical Keil target.

See `docs/m13-hardware-validation.md`.
