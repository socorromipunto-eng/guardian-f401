# Zenodo Manual Deposit Plan - Guardian F401 v0.13.0

Do not publish until the release gate is approved.

## Metadata

- Resource type: Software
- Title: Guardian F401: Deterministic Embedded Decision Assurance for Machine Health, Cybersecurity, and Supervisory Control
- Version: 0.13.0
- Publication date: 2026-08-13
- Creator: Antonio José Socorro Marín
- ORCID: 0009-0007-9089-9222
- Affiliation: leave blank unless a formal affiliation exists
- Language: English
- Access: Public metadata and files, subject to the proprietary license

## Rights configuration - mandatory

Zenodo defaults to CC BY 4.0. Do not leave that default in place.

Create a custom license with:

- Title: Guardian F401 Proprietary License - All Rights Reserved
- Description: Publicly archived for citation and technical review. No permission is granted for copying, modification, redistribution, derivative works, commercial use or physical deployment except by applicable law or prior written permission.
- Link: the LICENSE file in the tagged GitHub release

Confirm in Preview that CC BY 4.0 does not appear anywhere in the record.

## Description

Guardian F401 is an experimental STM32F401CDU6 embedded research platform for
machine-health analysis, cybersecurity and deterministic supervisory control.
It combines sensor acquisition, DSP/FFT features, anomaly-state evaluation,
authenticated command handling, robustness testing and a portable signed-
firmware lifecycle. The architecture treats analytics and future AI inference
as recommendations; deterministic policy retains operational authority.

Software tests and a locally reported Keil compilation milestone are
documented. Physical STM32F401 board qualification, production secure-boot
integration and safety/cybersecurity certification remain pending and are not
claimed.

## Keywords

STM32F401; embedded systems; cybersecurity; firmware integrity; deterministic
control; decision assurance; Edge AI; digital signal processing; machine
condition monitoring; supervisory control

## Publication gate

1. Review staged changes and license decision.
2. Commit the approved files.
3. Require all CI workflows to pass on the exact commit.
4. Tag 0.13.0 and create the GitHub release.
5. Download and hash the GitHub-generated release archive.
6. Create a manual Zenodo draft and upload the single release ZIP.
7. Enter the metadata above and apply the custom proprietary license.
8. Save and preview; do not publish on the first pass.
9. Perform human-readability, devil's-advocate and technical-destruction review.
10. Publish only after explicit approval.
11. Add the resulting DOI to README and ORCID Works in a subsequent version.

## Automation decision

Do not enable automatic GitHub-to-Zenodo ingestion for this first proprietary
release. Manual deposit provides a review checkpoint for the custom license and
metadata. Reassess automation after the first record is verified.
