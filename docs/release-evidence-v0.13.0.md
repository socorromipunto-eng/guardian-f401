# Guardian F401 v0.13.0 - Release Evidence Record

## Classification

| Assertion | Status | Evidence / limitation |
| --- | --- | --- |
| Repository structure reviewed | CONFIRMED | Pre-Zenodo preparation validates required paths. |
| Protocol/simulator/console tests | PENDING RE-RUN ON RELEASE COMMIT | Run the commands below and retain CI URLs/results. |
| Portable C host tests | PENDING RE-RUN ON RELEASE COMMIT | Run GitHub Actions on the exact release candidate. |
| STM32F401 compile contract | PENDING RE-RUN ON RELEASE COMMIT | This uses a CMSIS-shaped stub and is not a Keil or physical build. |
| Keil Arm Compiler 6.24 build | CONFIRMED_BY_ATTACHED_ARTIFACTS | Build-log SHA-256: a7be9fb7b201a5d85b266e12d0ac8955624699087e1db3dea67c93272a3a8faf |
| Generated HEX | CONFIRMED_BY_ATTACHED_ARTIFACTS | HEX SHA-256: 1f5157a6e176f8b3342c3d408c77787e109739ccb2433a5266fc3e2bb68520db |
| Physical board qualification | PENDING VALIDATION | No physical hardware-validation JSON is included. |
| Functional-safety certification | NOT CLAIMED | Guardian F401 is experimental research software. |
| Cybersecurity certification | NOT CLAIMED | Tests do not constitute certification. |

## Locally reported Keil milestone

- Target: STM32F401CDUx
- Compiler: Arm Compiler 6.24
- Guardian sources: 19/19
- Code: 34528 bytes
- RO-data: 1292 bytes
- RW-data: 8 bytes
- ZI-data: 5800 bytes
- HEX generation: PASS
- Compiler/linker: 0 errors / 0 warnings

Unless -ConfirmKeilEvidence was used with retained artifacts, these values are
classified as a local build report rather than independently reproduced
evidence.

## Required release validation

Run on the exact proposed release commit:

`powershell
python tools/validate_keil_manifest.py
$env:PYTHONPATH = 'protocol/python'
python -m unittest discover -s protocol/python/tests -v
$env:PYTHONPATH = 'protocol/python;simulator/src'
python -m unittest discover -s simulator/tests -v
$env:PYTHONPATH = 'protocol/python;simulator/src;console/src'
python -m unittest discover -s console/tests -v
`

Then confirm every GitHub Actions workflow passes on the release commit. Retain
the commit SHA, workflow run URLs, UTC timestamps and tool versions.

## Release identity

- Project version: 0.13.0
- Protocol version: 0.1
- Firmware semantic version: 0.13.0
- Author: Antonio José Socorro Marín
- ORCID: https://orcid.org/0009-0007-9089-9222
- Repository: https://github.com/socorromipunto-eng/guardian-f401

## Evidence privacy sanitization

The public Keil build log is a sanitized derivative of the original local
build log. Local Windows user paths were replaced with <USER_PROFILE>.
Compiler results, program size, target information, error and warning counts,
and build outcomes were not changed.

- Original Keil build log SHA-256: A7BE9FB7B201A5D85B266E12D0AC8955624699087E1DB3DEA67C93272A3A8FAF
- Published sanitized log SHA-256: 0612CDF844E2AC98D8E6FBBBE3E357227335A10D6F5E24B84AAA8ACFBD3A706A
- Original evidence remains preserved outside the publication package.