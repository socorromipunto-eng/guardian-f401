# Third-Party Notices and Dependency Register

This register distinguishes original Guardian F401 material from external tools
and components. It is a release-governance inventory, not a legal conclusion.
The authoritative terms are those distributed by each third party.

| Component | Use | Included in repository | Governing terms / action |
| --- | --- | --- | --- |
| Arm Cortex-M4 / CMSIS interfaces | Target architecture and declarations | Official CMSIS/device-pack files are not included; a project-specific compile stub is included for CI | Obtain and use the official Arm/ST terms applicable to the selected device pack. |
| STM32F401 device support and STM32CubeMX | Target configuration and generated local workspace | `.ioc` metadata is included; generated vendor libraries are excluded | Subject to STMicroelectronics terms. Review before redistributing generated content. |
| Arm Keil MDK / Arm Compiler 6 | Local proprietary target build | Toolchain is not included | Subject to Arm/Keil license terms. |
| Python | Simulator, console and tooling runtime | Runtime is not included | Python Software Foundation License. |
| pySerial | Optional physical serial transport | Package is not included; version range is recorded in `console/requirements-serial.txt` | BSD-3-Clause according to the upstream project; verify at release time. |
| GitHub Actions checkout/setup-python | CI automation | Workflow references only | Subject to each action repository's terms. Pinning by immutable commit is pending supply-chain hardening. |

## Project-created compatibility material

`firmware/Tests/CMSISStub/stm32f4xx.h` is a project-created, CMSIS-shaped compile
contract used only by host CI. It must not be represented as an official Arm or
ST header and must never be included in the physical firmware target.

## Release gate

Before adding any external source, binary, generated vendor package or model to
the repository, record its origin, version, license, intended use and
redistribution status here or in a generated SBOM.
