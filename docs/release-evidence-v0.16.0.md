# Guardian F401 v0.16.0 Release Evidence

Status: POST-PUBLICATION EVIDENCE RECORD
Publication performed: YES
Publication date: 2026-10-02
Publication UTC: 2026-10-02T20:56:39Z
Zenodo version DOI: `10.5281/zenodo.23111025`
Zenodo concept DOI: `10.5281/zenodo.21981233`
PhysicalHardwareValidation: PENDING
TargetGeometryQualification: NOT_DEMONSTRATED
RuntimeStackSufficiency: NOT_DEMONSTRATED
StorageOwnershipQualification: NOT_DEMONSTRATED
BackupChipDeployment: NOT_DEMONSTRATED
ProductionSecurityQualification: NOT_DEMONSTRATED
CertificationClaimAuthorized: NO

## Publication identity and evidence ownership

GitHub release: https://github.com/socorromipunto-eng/guardian-f401/releases/tag/v0.16.0
Zenodo record: https://zenodo.org/records/23111025
Source merge commit: `51beb5f208d8f32ff7cf21928edc02defd74f1d3`
Signed annotated tag object: `4c3a5940629bd9b3c36e979f378204608a0f976e`
Tag and checksum signer: `E5D6D18B609971D36B96898185C13CE4778382E1`

The Windows 04ZM publication runner verified the local tag signature, exact
remote tag object and commit, twelve successful merge check runs across ten
push workflows, release identity and all seven uploaded asset hashes by
subsequently downloading them. Source/index/working-tree state was preserved.
This document records that observed result; it does not rerun the target.

Zenodo's public record identifies version v0.16.0, date 2026-10-02 and the
GitHub v0.16.0 tree as its related software identifier. Its archive is
`socorromipunto-eng/guardian-f401-v0.16.0.zip`, 1449391 bytes, with service
checksum `md5:9bd231d2596176b465a13d30f82ac9ca`. That MD5 is the service's
reported archive metadata, not an independently recomputed cryptographic
integrity proof. The Zenodo archive and uploaded canonical source ZIP are
different packaging artifacts and must not be treated as byte-identical.

The immutable signed tag retains its PREPUBLICATION_CANDIDATE snapshot. This
post-publication record updates main through a separate reviewed PR. It does
not retag, amend the release or rewrite any deposit. The historical M15 closure
continues to identify v0.15.0 and DOI 10.5281/zenodo.22218923. The new version
DOI must not replace historical identifiers.

## Published GitHub asset SHA256 values

| Asset | SHA256 |
| --- | --- |
| guardian-f401-v0.16.0-source.zip | `D57B988AE88CAFA780D1F49BF81777CC0FDA81C181281749B551558B6F9AD1F0` |
| release-file-sha256-v0.16.0.csv | `A93EAF4F531E70FFCDFBC297DD611D77D559A07D571CED16814277A421940F21` |
| RELEASE-NOTES-v0.16.0.md | `AC1DC0F589BA383D8497C140F711D96CC615850632F378388E2DACAB1E1A7895` |
| Guardian-signing-public-key.asc | `F4015B23793D98E1AD24B3A9136D3CB12ADDED59068C3D750D3E2C5FDFE588AA` |
| release-identity-v0.16.0.json | `0FC68F9622E97765BB53791680B894939884932A6733E34FDE2B609D639F822E` |
| SHA256SUMS.txt | `5D6EAC807320EDE92C2F57A3C6D92336BE3E996B8A930B331708B0937347F44B` |
| SHA256SUMS.txt.asc | `50432937D9B5507BF17F0A1073454E3C2880B6DF02034674FD0E135101DAA640` |

The source manifest covers 498 canonical Git blobs and independently verified
without errors. It is an integrity inventory, not an SBOM or an independent
build attestation. Published assets do not include a newly built qualified
firmware image. Public key export alone does not establish author-key trust.

## Software build evidence and open qualification limits

- Reviewed source baseline: `c32832dca44f49c80786d3169444fb46c896aef6`.
- Pull request: https://github.com/socorromipunto-eng/guardian-f401/pull/76.
- Baseline CI: ten workflow runs completed successfully for that exact PR head.
  A metadata commit requires fresh CI; baseline success is not inherited proof.
- Startup configuration: 4096 bytes. Complete callback/interrupt stack bounds
  and physical stack-watermark evidence remain pending.
- The earlier candidate cited operator-provided isolated Keil 6.24 results.
  The corresponding log and map are retained outside the repository and are
  not provided by this record as public reproducible build evidence. No new
  Keil build or HEX is demonstrated by this documentation update. Public
  build-evidence qualification remains open; a missing HEX must not be
  fabricated or substituted with a historical image.
- The GNU runtime-matched ABI candidate is separate toolchain evidence; it is
  not interchangeable with the Keil AXF or a qualified deployment artifact.
- B03 provider, adapter and lifecycle integration evidence is host evidence.
  Test-double signature verification is not production Ed25519 qualification.
- HAL and NodeLink project changes establish source/build contracts, not
  demonstrated target execution, authentication provisioning or failover.
- MCU identity, xE/CDUx configuration consistency, sector ownership and S6
  binding remain unresolved. No flash operation is authorized by this record.

## Roadmap and assurance scope

This is a bounded research software source release. A version number does not
close the physical-strengthening or backup-chip milestones in the roadmap.
The F401 remains the deterministic authority node. AI/advisory, peer evidence,
authority and actuation remain distinct under the established contracts.
NASA, CISA, European and other assurance references retain their controlled
applicability and evidence boundaries. No certification, regulatory compliance,
approval, production readiness or endorsement is authorized by publication.

## Publication runner incident

04ZL stopped after manifest verification because git archive output at
COPYRIGHT differed from the raw Git blob manifest. No tag/release was created
in that attempt. Git archive may apply checkout conversions; the user's exact
old-ZIP byte delta was not separately measured. 04ZM generated the archive
directly from binary Git blob reads and passed the unchanged independent ZIP
verifier before publication. The old evidence remains preserved. See incident
RUNNER-04ZL-ARCHIVE-BYTE-001 in the governed scripting ledger.

## Target security integration remains open

The published main entry point does not call the application's security or
firmware configuration APIs. Keil source membership does not prove those
paths survive linking or execute. The trusted-signature adapter calls an
Ed25519 backend interface and is not included in the published Keil project.
Production backend selection, provisioning, key ownership, negative tests and
complete target call-path evidence remain a separate implementation gate.
No source change or production verifier qualification is performed here.
