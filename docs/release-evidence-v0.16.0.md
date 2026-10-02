# Guardian F401 v0.16.0 Release Evidence

Status: PREPUBLICATION_CANDIDATE
Planned tag: v0.16.0
Publication performed: NO
Version DOI allocation: PENDING
Publication date allocation: PENDING
PhysicalHardwareValidation: PENDING
TargetGeometryQualification: NOT_DEMONSTRATED
RuntimeStackSufficiency: NOT_DEMONSTRATED
StorageOwnershipQualification: NOT_DEMONSTRATED
BackupChipDeployment: NOT_DEMONSTRATED
ProductionSecurityQualification: NOT_DEMONSTRATED
CertificationClaimAuthorized: NO
Zenodo concept DOI: `10.5281/zenodo.21981233`

## Decision and publication architecture

The author requested preparation of v0.16.0 as a bounded research software
release. A version number identifies a source increment; it does not close the
physical-strengthening or backup-chip milestones in the research roadmap.
The F401 remains the deterministic authority node. Heterogeneous peers and AI
remain subject to the existing explicit authority and advisory boundaries.

Release preparation and completed publication are separate evidence states.
PREPUBLICATION_CANDIDATE permits coherent candidate metadata without fabricating
a release date, tag object, release URL state, or Zenodo version DOI. The
existing POST-PUBLICATION EVIDENCE RECORD state requires the observed release
date and valid version/concept DOI identities. Unknown or duplicate states
fail closed. A successful coherence check does not prove publication or target
qualification; --require-published rejects this candidate state.

The published M15 governance closure belongs to v0.15.0 and is immutable
historical evidence. Later version identifiers must not replace its DOI. The
checker verifies that closure against docs/release-evidence-v0.15.0.md while
validating the current version separately. Candidate citation metadata retains
only the concept DOI, omits date-released, and labels the candidate explicitly.

## Established baseline and limits

- Reviewed source baseline: `c32832dca44f49c80786d3169444fb46c896aef6`.
- Pull request: https://github.com/socorromipunto-eng/guardian-f401/pull/76.
- Baseline CI: ten workflow runs completed successfully for that exact PR head.
  A metadata commit requires fresh CI; baseline success is not inherited proof.
- Startup configuration: 4096 bytes. Complete callback/interrupt stack bounds
  and physical stack-watermark evidence remain pending.
- Isolated Keil 6.24 build: zero errors, three ST HAL unused-parameter warnings;
  load ROM 22720 bytes within the configured 49152-byte region. This was an
  external source snapshot, not a physical-board qualification.
- The GNU runtime-matched ABI candidate is separate toolchain evidence; it is
  not interchangeable with the Keil AXF or a qualified deployment artifact.
- B03 provider, adapter and lifecycle integration evidence is host evidence.
  Test-double signature verification is not production Ed25519 qualification.
- HAL and NodeLink project changes establish source/build contracts, not
  demonstrated target execution, authentication provisioning or failover.
- MCU identity, xE/CDUx configuration consistency, sector ownership and S6
  binding remain unresolved. No flash operation is authorized by this record.

## Publication gates and evidence ownership

1. Validate candidate metadata, positive/negative regression tests, unchanged
   historical evidence and unchanged production firmware before signed commit.
2. Push only the feature branch, verify the new PR head and adjudicate its CI.
3. Review the bounded claims, complete the PR review and protected-main merge,
   and adjudicate CI on the actual resulting main commit.
4. Create and verify the signed v0.16.0 tag on that reviewed commit; verify
   remote tag identity and create the bounded GitHub release under an explicit
   publication authorization. Never retarget an existing tag automatically.
5. Observe Zenodo ingestion. Only then record the actual version DOI, date,
   tag/release identity and archive checksums in a separate post-publication
   evidence update on main. The tagged prepublication snapshot stays immutable.
   Regenerate and verify a release manifest from the immutable tag's Git blobs.

No tag, merge, release, DOI allocation or hardware qualification is performed
by the metadata runner. NASA, CISA, European and other assurance references
remain bounded by the repository's applicability and evidence records; this
release record grants no certification, approval, compliance or endorsement.
