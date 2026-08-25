# Guardian F401 Keil Workspace

This directory intentionally tracks only portable project metadata.

The full Keil and STM32CubeMX workspace is generated locally and excluded from
Git. Build objects, AXF/HEX/BIN files, logs, IDE state, copied STM32Cube
libraries, and other generated artifacts remain local.

## Structural-review project

The portable project is:

`../Guardian-F401.uvprojx`

Validated configuration:

- Target: `Guardian-F401-Structural-Review`
- Device: `STM32F401CDUx`
- Device family define: `STM32F401xE`
- Compiler: Arm Compiler 6.24
- Project source entries: `21`
- Guardian sources: `19/19`
- Vendor support sources: `startup_stm32f401xe.s` and `system_stm32f4xx.c`
- Include paths: `13`
- Guardian entry point: `main_guardian.c`
- CubeMX `main.c`: excluded
- CubeMX `stm32f4xx_it.c`: excluded
- CI-only `firmware/Tests/CMSISStub`: absent
- Code: `34706` bytes
- RO-data: `1374` bytes
- RW-data: `8` bytes
- ZI-data: `5816` bytes
- `CreateHexFile`: `0`
- Compiler/linker result: `0 Error(s), 0 Warning(s)`
- Flashing: not performed
- Physical hardware validation: pending

## Reproducibility evidence

Two consecutive portable rebuilds produced the same AXF:

`A9A0383BDE722753ACD663F571B848A4696D4D7101FFA80D1E3FC101C912EDA8`

The earlier absolute-path AXF produced:

`499A7B94940AD34BD8DD042FA64AD4D70FF00574EF80CD26A17703966DE9412F`

The complete AXF hashes differ in non-loadable content. AXF files include debug
metadata that may reflect source paths and dependency metadata. The loadable
images extracted from both AXF files were identical:

- Loadable image length: `36088` bytes
- SHA-256:
  `89217348214D73CFA5011A8DD6AFACDD479336AF9F120886952CBEFA793FE5D7`
- Binary comparison: `IDENTICAL`

No HEX file was generated during this structural-review validation.

## Recreating the local Keil workspace

1. Open `GuardianF401.ioc` with STM32CubeMX.
2. Select `MDK-ARM` as the toolchain.
3. Generate the STM32F401 project locally.
4. Use `../guardian-f401-keil-sources.txt` for the Guardian source list.
5. Use `../guardian-f401-keil-includes.txt` for Guardian include directories.
6. Define `STM32F401xE`.
7. Use Arm Compiler 6.
8. Exclude CubeMX `main.c` from the Guardian target.
9. Exclude CubeMX `stm32f4xx_it.c` from the Guardian target.
10. Add `../Templates/main_guardian.c` as the application entry point.
11. Open `../Guardian-F401.uvprojx` for the portable structural-review target.

The physical Keil target must never include `firmware/Tests/CMSISStub`.

See `../guardian-f401-keil-checklist.md` and
`../../../docs/m13-hardware-validation.md` for the complete validation contract.
