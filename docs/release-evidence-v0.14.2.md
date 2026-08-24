# Guardian F401 v0.14.2 Release Evidence

Status: PRE-PUBLICATION REVIEW
Date: 2026-08-24
PhysicalHardwareValidation: PENDING

## Purpose

This corrective release preserves the M15/G15-01D AXF adjudication evidence,
adds the portable Guardian F401 µVision structural-review project and corrects
the published release-metadata chain without rewriting immutable historical
tags or Zenodo deposits.

## Source state

- Base main commit:
  `cb16dcd0e769d9c2bb5e01070161de6bc002eea2`
- Release branch: `release/v0.14.2`
- Previous GitHub release: `v0.14.1`
- Previous Zenodo release: `v0.14.0`
- Zenodo concept DOI: `10.5281/zenodo.21923858`

The v0.14.2 commit, tag and version DOI are not recorded until they exist and
are independently verified.

## Historical publication adjudication

- v0.13.0 DOI: `10.5281/zenodo.21923859`
- v0.14.0 DOI: `10.5281/zenodo.21980859`
- v0.14.1 Zenodo version DOI: NOT ISSUED
- The immutable v0.14.1 Git tag contains `CITATION.cff` metadata identifying
  v0.14.0.
- Historical tags and deposits were not modified.

## Structural-review validation

- Target: `Guardian-F401-Structural-Review`
- Device: `STM32F401CDUx`
- Compiler: Arm Compiler 6.24
- Source entries: 21
- Guardian sources: 19/19
- Include paths: 13
- Entry point: `main_guardian.c`
- `CreateHexFile`: 0
- Build result: 0 errors and 0 warnings
- Code: 34,706 bytes
- RO-data: 1,374 bytes
- RW-data: 8 bytes
- ZI-data: 5,816 bytes

Two consecutive portable rebuilds produced AXF SHA-256:

`A9A0383BDE722753ACD663F571B848A4696D4D7101FFA80D1E3FC101C912EDA8`

The earlier absolute-path build produced AXF SHA-256:

`499A7B94940AD34BD8DD042FA64AD4D70FF00574EF80CD26A17703966DE9412F`

The complete AXF files differ in non-loadable content. Their extracted
loadable images were identical:

- Length: 36,088 bytes
- SHA-256:
  `89217348214D73CFA5011A8DD6AFACDD479336AF9F120886952CBEFA793FE5D7`
- Comparison: IDENTICAL

## Historical HEX boundary

The tracked `release-evidence/guardian-f401.hex` remains the historical
v0.13.0 evidence artifact.

- Git blob:
  `5a6cd3e4d7aebd176863233d2d5a4a165ab0f3b6`
- Canonical LF SHA-256:
  `C113A9E5FAE2D9881AF67C541AB8AC5DB16134BECAD5267A2F4386A72D4BA0A8`
- Original CRLF SHA-256:
  `1F5157A6E176F8B3342C3D408C77787E109739CCB2433A5266FC3E2BB68520DB`
- Normalized line comparison: IDENTICAL
- Intel HEX lines compared: 2,243
- Substantive line differences: 0

No HEX was generated during the v0.14.2 preparation or structural-review
validation. This historical file is not presented as newly generated,
hardware-qualified or authorized for flashing.

## Safety boundaries

- Physical hardware validation remains pending.
- No flashing or hardware programming was performed.
- No production deployment is authorized.
- No safety or cybersecurity certification is claimed.
- No CI-only `firmware/Tests/CMSISStub` content is included in the physical
  Keil target.
- Formal publication requires independent review of the final manifest,
  commit, tag, GitHub release and Zenodo metadata.

## Pending publication evidence

The following remain intentionally pending:

- Final v0.14.2 release commit
- Final tracked-file manifest
- Git tag
- GitHub release
- GitHub Actions results
- Zenodo version DOI
- Archived source-file checksum
