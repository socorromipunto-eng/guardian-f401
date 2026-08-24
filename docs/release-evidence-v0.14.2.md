# Guardian F401 v0.14.2 Release Evidence

Status: POST-PUBLICATION CORRECTION REVIEW
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
- Previous GitHub-integrated Zenodo release: `v0.14.1`
- GitHub-integration concept DOI: `10.5281/zenodo.21981233`
- Historical/manual concept DOI: `10.5281/zenodo.21923858`

During preparation, the v0.14.2 commit, tag and version DOI were intentionally
left pending until they existed and could be independently verified. Their
verified published identities are recorded below.

## Publication-family adjudication

Two Zenodo concept families exist.

Historical/manual family:

- Concept DOI: `10.5281/zenodo.21923858`
- v0.13.0 DOI: `10.5281/zenodo.21923859`
- v0.14.0 DOI: `10.5281/zenodo.21980859`

GitHub-integration family:

- Concept DOI: `10.5281/zenodo.21981233`
- v0.14.0 DOI: `10.5281/zenodo.21981234`
- v0.14.1 DOI: `10.5281/zenodo.22062543`
- v0.14.2 DOI: `10.5281/zenodo.22075322`

Version v0.14.0 exists in both families as separate published deposits. No
record was deleted, rewritten or represented as the other.

The immutable v0.14.1 Git tag contains `CITATION.cff` metadata identifying
v0.14.0. The GitHub-integrated Zenodo record identifies the archived GitHub
release as v0.14.1.

The earlier pre-publication conclusion that v0.14.1 had no Zenodo DOI resulted
from inspecting only the historical/manual family. This record corrects that
conclusion using the complete two-family inventory.

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
- Post-publication corrections require independent review of the manifest,
  commit, GitHub release notes and Zenodo evidence.

## Published release identity

- Release merge commit:
  `8cce72a6fbf176d9a57ff31c7900ebd8a7dccb2a`
- Annotated tag: `v0.14.2`
- GitHub release:
  `https://github.com/socorromipunto-eng/guardian-f401/releases/tag/v0.14.2`
- Zenodo version DOI: `10.5281/zenodo.22075322`
- Zenodo concept DOI: `10.5281/zenodo.21981233`
- Archived file:
  `socorromipunto-eng/guardian-f401-v0.14.2.zip`
- Archived file size: 547,465 bytes
- Zenodo-reported MD5:
  `108FBF40B7D564F4AD9D527C4B447AE8`
- Independently calculated SHA-256:
  `B37DE78BD4F628B88A8913EFDB5BBD8B752E88FB43E29916D40A6F94258B0F0B`
- ZIP entries: 304
- Required release entries missing: 0
- Pull-request checks: 10 of 10 successful
- Post-merge workflows: 8 of 8 successful
- Release manifest rows: 221
- Release-manifest SHA-256:
  `22EE67CCD07E03E8AA79E4CF55E69AE919E4E98ABABB4F50C29B8AAB42257347`

## Immutable publication boundary

The v0.14.2 tag and Zenodo archive contain the pre-publication documentation
that was available when they were created. This post-publication correction
does not rewrite those immutable artifacts. It establishes the corrected
publication map on the default branch and in editable GitHub release notes.
