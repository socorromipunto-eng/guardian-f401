# Guardian F401 v0.16 Static Analysis Gate

## Current status

The Guardian F401 v0.16 static-analysis remediation gate is closed under the governed evidence recorded in this document.

Closure is limited to the defined v0.16 static-analysis and remediation boundary. It does not claim CI enforcement, required-check enforcement, release readiness, production readiness, hardware validation, certification, compliance, approval, endorsement, or broader assurance beyond the evidence explicitly recorded below.

## Governed analyzer identity

- Tool: Cppcheck
- Version: 2.21.0
- Official repository: `cppcheck-opensource/cppcheck`
- Tag: `2.21.0`
- Commit: `e73bf44c3e49686b7495fab352d03a6c6075516b`

The upstream selected commit is not claimed to be cryptographically signed.

Guardian identifies this analyzer revision using:

1. official repository identity;
2. official release identity;
3. exact tag;
4. exact commit;
5. local executable version;
6. local executable SHA-256.

Version promotion is never automatic.

## Analysis boundary

Primary analysis scope is first-party Guardian C implementation code.

Vendor CMSIS and STM32 HAL implementation source is not part of the primary
finding domain.

Vendor headers remain parsing context where required by Guardian target code.

Required target defines:

- `STM32F401xE`
- `USE_HAL_DRIVER`

## Evidence

The governed runner emits:

- Cppcheck version;
- Cppcheck executable SHA-256;
- analyzed-file count;
- deterministic analyzed-file-set SHA-256;
- raw analyzer XML;
- structured JSON summary;
- finding count;
- severity counts;
- analyzer exit code.

## Failure semantics

Analyzer execution failure is not equivalent to analyzer findings.

The runner fails closed on:

- unexpected Cppcheck version;
- missing executable;
- invalid repository root;
- missing required first-party roots;
- empty analysis scope;
- missing required target include roots;
- unparseable analyzer XML;
- unexpected analyzer process exit code.

Finding policy is adjudicated separately.

## Negative testing

A later authorized boundary must prove that the analyzer/gate detects a
synthetic known defect without modifying production firmware.

## Non-claims

Closure of this gate does not demonstrate:

- zero analyzer findings;
- negative-test effectiveness using a synthetic known defect;
- protected CI enforcement;
- required-check integration;
- release readiness;
- production readiness;
- hardware validation;
- certification, compliance, approval, or endorsement;
- completion of the broader R3D implementation boundary.

## ArmClang compiler model for static analysis

The V0.16 Cppcheck runner uses a pinned compiler-model input for the validated Guardian F401 ArmClang toolchain.

- Compiler model: `ARMCLANG_6_24`
- Compiler-model define: `__ARMCC_VERSION=6240002`
- Functional firmware defines remain `STM32F401xE` and `USE_HAL_DRIVER`.
- Compiler-model and functional defines are intentionally treated as separate concerns.
- No Cppcheck suppression is introduced for this model alignment.
- The pinned value was derived from the authoritative Arm Compiler for Embedded 6.24 builtin macro query and validated by the controlled V4 experiment.
- CMSIS dispatch evidence demonstrates that `__ARMCC_VERSION=6240002` selects `cmsis_armclang.h`; the baseline `cmsis_gcc.h` `comparePointers` findings at lines 151 and 157 were adjudicated as compiler-model false positives.
- A toolchain-version change requires explicit compiler-model revalidation; the runner does not dynamically query a host ArmClang installation.
- Expected suppression count: `0`.

## Gate closure adjudication

The following values record the governed v0.16 static-analysis gate closure. External machine-generated evidence remains outside the repository at this boundary and is identified here by exact SHA-256.

### Closure result

- `STATIC_ANALYSIS_GATE_CLOSED=YES`
- `V016_STATIC_ANALYSIS_GATE_CLOSURE_ADJUDICATION=PASS`
- `REFERENCE_SUBSTANTIVE_FINDING_COUNT=41`
- `FINAL_SUBSTANTIVE_FINDING_COUNT=34`
- `REMOVED_SUBSTANTIVE_FINDING_COUNT=7`
- `ADDED_SUBSTANTIVE_FINDING_COUNT=0`
- `TARGET_FINDING_REMAINING_COUNT=0`
- `ALL_SEVEN_TARGET_FINDINGS_REMOVED=YES`
- `FIRST_PARTY_STYLE_FINDING_COUNT=0`
- `CPPCHECK_WARNING_COUNT=0`
- `CPPCHECK_ERROR_COUNT=0`

### Governed pytest result

- `PYTEST_PASSED_COUNT=6`
- `PYTEST_VERSION=8.4.2`
- `PYTEST_RESULT_SOURCE=PRIOR_GOVERNED_GATE_OUTPUT`
- Pytest was not rerun during the final closure adjudication.

### Static-analysis execution

- Analyzer: Cppcheck 2.21.0
- Compiler model: `ARMCLANG_6_24`
- Analyzed first-party file count: `27`
- Final analyzed-file-set SHA-256: `17E4DF3565E142551FE741754D98BCBB040665D02B385E35DED22264D0A5703B`
- Analyzer exit code: `2` under the governed finding-policy semantics.
- Suppression count: `0`
- `STATIC_XML_SHA256=CF9F59B96F248A1466A965866780DF67A1DA1335D1AFEFFB771A45EE84048249`
- `STATIC_SUMMARY_SHA256=018BDC0ED2AD07789AF7A36D40C7340F8D0085258202B8398550AB9CA32114BC`

### Build validation

- Project: `firmware/MDK-ARM/Guardian-F401.uvprojx`
- Target: `Guardian-F401-Structural-Review`
- Device: `STM32F401CDUx`
- Compiler: ArmClang 6.24
- The governed build compiled each of the five remediated source files exactly once.
- `BUILD_ERROR_COUNT=0`
- `BUILD_WARNING_COUNT=0`
- `BUILD_LOG_SHA256=D7002BBAD7D53109DD5BC7BB7C71057F350A32108AE9F1872CB7AE31E872A7FB`
- `AXF_SHA256=082CB13AF89B938A2A4AE483BC882329CF7C359DFEF02646C7F299DC26D38E2C`
- `MAP_SHA256=D2309E5244804026DF45F458074BB2FCE515FBA1ADD4F5B988E622353053BD70`

The AXF and MAP are generated build products. They remain untracked and ignored under the repository policy; their hashes are recorded as evidence and are not an authorization to version those generated artifacts.

### Closure evidence

- `CLOSURE_JSON_SHA256=AA90B073A6BE5645F520C892BEA6812244EA0D46E0D6B3677E61ABB5D2F75587`
- `CLOSURE_JSON_REPOSITORY_STATUS=EXTERNAL_EVIDENCE_REFERENCE_BY_HASH`

The closure JSON, raw Cppcheck XML, Cppcheck summary JSON, and Keil build log were not copied into the repository by this boundary. Their hashes provide exact correlation to the governed external evidence used for adjudication.

### Scope limitation

This gate closure demonstrates the defined v0.16 static-analysis remediation result and the corresponding successful governed Keil build. It is not a claim of certification, compliance, hardware validation, production readiness, release authorization, or completion of unrelated Guardian assurance boundaries.
