# Guardian F401 v0.15.0 Release Evidence

Status: POST-PUBLICATION EVIDENCE RECORD
Publication date: 2026-09-01
PhysicalHardwareValidation: PENDING

## Release identity

- Release merge commit: `18ec274b156e3fc5b53095fe45f38243a520a2dc`
- M15 closure feature commit: `f9b7b9cd32e8dff09c8cf40696d53f682afd17a6`
- Pull request: `#30`
- Annotated tag: `v0.15.0`
- Annotated tag object: `02ee922e8380482a6f088e06abddae390a5cb4e8`
- Tag target commit: `18ec274b156e3fc5b53095fe45f38243a520a2dc`
- Tag signature state: `UNSIGNED`
- GitHub release: `https://github.com/socorromipunto-eng/guardian-f401/releases/tag/v0.15.0`
- GitHub release state: `PUBLISHED`

## Zenodo publication identity

- Ingestion mode: `GITHUB_INTEGRATION`
- Zenodo version DOI: `10.5281/zenodo.22218923`
- Zenodo concept DOI: `10.5281/zenodo.21981233`
- Archived file: `socorromipunto-eng/guardian-f401-v0.15.0.zip`
- Manual deposit used for v0.15.0: `NO`

Archive size, Zenodo-reported checksum and an independently calculated archive
SHA-256 were not captured in the available evidence and remain `NOT_VERIFIED`.
Values from prior releases must not be reused.

## Deterministic release manifest

The post-publication manifest
`release-evidence/release-file-sha256-v0.15.0.csv` is generated from the
immutable `v0.15.0` Git tree using canonical Git blob bytes. It does not hash
platform-dependent working-tree bytes. Paths are repository paths with `/`
separators. Verification requires exact path coverage, SHA-256, byte count,
Git blob identity and resolved release commit.

This supersedes the portability assumptions of the historical
`release-evidence/release-file-sha256.csv`; the historical file remains
unchanged as evidence of the earlier release process.

## Validation

- M15 semantic CI integration Technical Destruction: `23/23 PASS`.
- M15 closure Technical Destruction: `22/22 PASS`.
- Governance regression: `PASS`.
- Semantic registry validation: `PASS`.
- Precommit: `PASS`.
- Pre-PR: `PASS`.
- PR #30 changed files: `5`.
- PR-triggered workflows observed before merge: `10/10 SUCCESS`.

## Publication-family map

GitHub-integrated family:

- Concept DOI: `10.5281/zenodo.21981233`
- v0.14.0: `10.5281/zenodo.21981234`
- v0.14.1: `10.5281/zenodo.22062543`
- v0.14.2: `10.5281/zenodo.22075322`
- v0.15.0: `10.5281/zenodo.22218923`

Historical/manual family:

- Concept DOI: `10.5281/zenodo.21923858`
- v0.13.0: `10.5281/zenodo.21923859`
- v0.14.0: `10.5281/zenodo.21980859`

The two families remain distinct.

## Git identity adjudication

The published merge commit records author name `Antonio Jose Socoro marin`,
while other project authorship metadata uses Antonio José Socorro Marín /
Antonio Jose Socorro Marin. The published commit is not rewritten. `.mailmap`
provides a canonical historical display mapping for repository tooling.
Future Git/GitHub identity configuration must use the canonical project author
identity. This mapping does not alter commit signatures or object IDs.

## Preserved non-claims

- ADR-M15-001 full implementation remains `NOT_ADJUDICATED`.
- Full ADR-M15-002/003/004 conformance remains `NOT_DEMONSTRATED`.
- Governance completeness remains `NOT_DEMONSTRATED`.
- Physical STM32F401 qualification remains `NOT_DEMONSTRATED`.
- Production readiness remains `NOT_DEMONSTRATED`.
- External cybersecurity and functional-safety certification remain `NOT_DEMONSTRATED`.
- NASA/NIST/CIS mappings remain `NOT_ADJUDICATED`.
- AI/advisory output does not grant actuation authority.
