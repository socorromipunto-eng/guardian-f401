# Guardian F401 Keil Workspace

This directory intentionally tracks only portable project metadata.

The full Keil and STM32CubeMX workspace is generated locally and excluded from
Git. Build objects, AXF/HEX files, logs, IDE state, copied STM32Cube libraries,
and other generated artifacts remain local.

## Verified build

Guardian F401 was successfully rebuilt for `STM32F401CDUx` using Arm Compiler
6.24.

- CMSIS target define: `STM32F401xE`
- Guardian sources: `19/19`
- Guardian entry point: `main_guardian.c`
- CubeMX `main.c`: excluded
- CubeMX `stm32f4xx_it.c`: excluded
- CI-only `CMSISStub`: absent
- Code: `34528` bytes
- RO-data: `1292` bytes
- RW-data: `8` bytes
- ZI-data: `5800` bytes
- HEX generation: PASS
- Compiler/linker result: `0 Error(s), 0 Warning(s)`

## Recreating the local Keil project

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

The physical Keil target must never include `firmware/Tests/CMSISStub`.

See `../guardian-f401-keil-checklist.md` and
`../../../docs/m13-hardware-validation.md` for the complete validation contract.
