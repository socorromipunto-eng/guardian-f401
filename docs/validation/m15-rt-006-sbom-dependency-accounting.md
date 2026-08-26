# RT-M15-006 — SBOM and Dependency Accounting

## Status

PARTIAL

## Classification

Software-supply-chain, dependency-accounting and release-governance finding.

This record does not establish software-supply-chain certification, complete
dependency provenance, production readiness, or formal SBOM compliance.

## RT-06A — Dependency identification and version accounting

Status:

`PARTIAL`

Repository inspection identified one explicit dependency requirements file:

`console/requirements-serial.txt`

Observed requirement:

`pyserial>=3.5,<4`

The repository also contains references to external packages including:

- `rfc8785`;
- `cryptography`.

However, this audit did not demonstrate one complete authoritative dependency
manifest containing every runtime, build, test and tooling dependency.

The absence of such a manifest from this audit must not be converted into a
claim that every referenced dependency is necessarily unpinned. The supported
finding is that complete dependency pinning is not demonstrated by the current
repository evidence.

## RT-06B — Third-party license accounting

Status:

`PARTIAL`

The repository contains:

- `LICENSE`;
- `NOTICE`;
- `THIRD_PARTY_NOTICES.md`.

`THIRD_PARTY_NOTICES.md` identifies at least:

- Arm Cortex-M4 / CMSIS interfaces;
- STM32F401 device support / STM32CubeMX;
- Arm Keil MDK / Arm Compiler 6;
- Python;
- pySerial;
- GitHub Actions checkout/setup-python.

The register distinguishes repository-included material from external
toolchains and vendor packages.

Residual actions explicitly recorded by the repository include:

- release-time verification of pySerial governing terms;
- review of applicable Arm/ST vendor terms;
- immutable GitHub Actions pinning under future supply-chain hardening.

Therefore third-party accounting is substantial but not complete.

## RT-06C — Formal SBOM

Status:

`NOT_DEMONSTRATED`

Repository inspection found:

`0`

formal SBOM artifacts matching SPDX, CycloneDX or explicit SBOM naming.

No claim of formal SBOM availability is authorized.

## RT-06D — Automated SBOM / provenance generation

Status:

`NOT_DEMONSTRATED`

CI inspection found:

`0`

matches demonstrating automated:

- SBOM generation;
- SPDX generation;
- CycloneDX generation;
- Syft generation;
- provenance generation;
- supply-chain attestation generation.

## RT-06E — Release integrity manifests

Status:

`PARTIAL / SEPARATE CONTROL`

Guardian F401 has separate release-integrity and SHA-256 evidence in the
repository and release workflow.

Integrity hashes are useful release evidence.

They are not equivalent to:

- a dependency inventory;
- an SPDX SBOM;
- a CycloneDX SBOM;
- dependency provenance;
- software-supply-chain attestation.

Release integrity must therefore remain distinct from SBOM completion.

## Decision

RT-M15-006 remains globally:

`PARTIAL`

The repository demonstrates meaningful third-party accounting and some explicit
dependency version constraints.

It does not yet demonstrate a complete, canonical dependency inventory.

It does not contain a demonstrated formal SPDX/CycloneDX SBOM.

It does not demonstrate automated SBOM or provenance generation in CI.

## Required future work

The open work maps to:

`G15-07 — Build and Supply-Chain Security`

Required controls include:

1. one authoritative dependency inventory;
2. exact dependency versions where reproducibility requires them;
3. dependency hashes where technically applicable;
4. complete third-party license mapping;
5. CycloneDX or SPDX SBOM generation;
6. reproducible SBOM generation in CI;
7. immutable pinning of third-party CI actions;
8. build provenance;
9. signed release artifacts where the release model requires them;
10. verification that generated SBOM content matches the actual release tree.

## Validation requirement

A future RT-06 closure requires more than producing an SBOM file once.

Closure requires demonstrating that the SBOM:

- is generated from the intended source/release baseline;
- is reproducible or otherwise deterministically attributable;
- contains the expected direct dependencies;
- correctly represents applicable tool/runtime dependencies according to the
  defined SBOM scope;
- carries license and version information appropriate to that scope;
- is generated or validated by the controlled build/release workflow.

## Residual restrictions

This record does not authorize claims of:

- complete dependency provenance;
- complete third-party license closure;
- SPDX compliance;
- CycloneDX compliance;
- SLSA compliance;
- software-supply-chain certification;
- production readiness.

RT-M15-006 may become CLOSED only after G15-07 controls are implemented,
evidenced and independently reviewed.