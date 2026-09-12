# C5-R3C-E Physical Persistence Backend Evidence and Human Adjudication

Status: LOCAL_EVIDENCE_COMPLETE
Milestone: C5-R3C-E
Generated UTC: 2026-09-12T18:09:04.413Z

## 1. Purpose

This document records the evidence and human adjudication for the C5-R3C-E physical persistence backend boundary.

R3C-E materializes a physical persistence backend beneath the previously governed R3C-C persistence transaction contract and R3C-D canonical serialization/integrity codec.

This evidence does not redefine R3C-C or R3C-D semantics.

## 2. Source of truth

Branch:

    feature/m16-audit-c5-r3c-e-physical-persistence

Source-of-truth HEAD before R3C-E local mutation:

    d587be5252f56a2f608a7d760572783f2d39b0a9

Target MCU:

    STM32F401CDU6

Correct CMSIS device macro:

    STM32F401xE

Selected logical physical layout:

    Application governed candidate range:
      0x08000000 .. 0x0800BFFF

    Slot A:
      STM32F401 Sector 3
      Base: 0x0800C000
      Capacity: 16384 bytes

    Slot B:
      STM32F401 Sector 4
      Base: 0x08010000
      Capacity: 65536 bytes

The linker reservation of Sector 3 and Sector 4 has not been demonstrated.

## 3. Governing semantic boundaries

The following non-equivalences are mandatory:

    SHA256_INTEGRITY != AUTHENTICATED_STORAGE

    VALID_SLOT != CURRENT_SLOT_PROOF

    HIGHER_VISIBLE_GENERATION != STRONG_ROLLBACK_RESISTANCE

    DUAL_SLOT_RECOVERY != ANTI_ROLLBACK

    FLASH_PERSISTENCE != PERSISTENT_ANTI_REPLAY

    HOST_TEST_PASS != HARDWARE_VALIDATED

    HAL_SIMULATION_PASS != FLASH_HARDWARE_VALIDATED

    ARM_SYNTAX_PASS != TARGET_BINARY_VALIDATED

    CI_STEP_DEFINED != CI_EXECUTED

    CI_EXECUTED != PROTECTED_CI_ENFORCED

    PERSISTENCE != FRESHNESS

    FRESHNESS != AUTHORITY

    AUTHORITY != ACTUATION_AUTHORITY

## 4. R3C-E implementation evidence

Portable backend:

    IMPLEMENTED=YES

STM32F401 HAL flash adapter:

    IMPLEMENTED=YES

STM32F401 HAL adapter properties demonstrated locally:

    TARGET_MCU_BINDING=STM32F401CDU6
    DEVICE_MACRO=STM32F401xE
    FREESTANDING_ARM_SYNTAX=PASS
    REAL_LOCAL_STM32_HAL_HEADERS_USED=YES
    PROGRAM_UNIT=FLASH_TYPEPROGRAM_BYTE
    ERASE_VOLTAGE_RANGE_HARDCODED=NO
    ERASE_VOLTAGE_RANGE_EXPLICIT_CONFIGURATION=YES

## 5. Portable backend Technical Destruction

O0:

    TD_TOTAL=27
    TD_PASS_COUNT=27
    TD_FAIL_COUNT=0
    RESULT=PASS

O2:

    TD_TOTAL=27
    TD_PASS_COUNT=27
    TD_FAIL_COUNT=0
    RESULT=PASS

The exercised backend cases include:

- erased media;
- initial establishment;
- generation continuation;
- adjacent dual-valid recovery;
- reboot recovery;
- uncommitted stage recovery;
- torn candidate recovery;
- partial program behavior;
- partial commit-marker behavior;
- corruption handling;
- backend read failure;
- backend erase failure;
- NOR 1-to-0 programming enforcement.

HOST_POWER_LOSS_FAULT_INJECTION_VALIDATED=YES

This is host fault injection and is not hardware power-loss validation.

## 6. Governed R3C regression

The previously governed R3A/R3B/R3C regression remained passing.

Observed evidence:

    C5_R3C_B_PERSISTENCE_CLASSIFICATION_HOST_TEST=PASS
    C5_R3A_FRESHNESS_STATE_HOST_TEST=PASS
    C5_R3B_FRESHNESS_EVALUATOR_HOST_TEST=PASS
    C5_R3C_D_PERSISTENCE_CODEC_HOST_TEST=PASS
    R3C_D_TD_TOTAL=32
    R3C_D_TD_PASS_COUNT=32

Historical R3C-D output stating PHYSICAL_STORAGE_IMPLEMENTED=NO remains historically correct for the governed R3C-D boundary and is not retroactively modified by R3C-E.

## 7. STM32F401 adapter evidence

ARM-target freestanding syntax:

    RESULT=PASS

Host HAL adapter hostile-contract tests:

    TD_TOTAL=30
    TD_PASS_COUNT=30
    TD_FAIL_COUNT=0
    RESULT=PASS

The adapter contract tests include:

- valid and invalid initialization;
- explicit erase voltage propagation;
- Sector 3 mapping;
- Sector 4 mapping;
- unlock failure;
- erase failure;
- sector-error propagation;
- byte-program sequencing;
- exact address sequencing;
- exact byte-data sequencing;
- slot bounds;
- invalid-slot rejection;
- programming failure;
- lock failure;
- fail-closed error propagation.

HAL_ERROR_PROPAGATION_VALIDATED=YES

HARDWARE_FLASH_WRITE_VALIDATED=NO

## 8. CI integration evidence

Existing governed workflow reused:

    .github/workflows/firmware-middleware-tests.yml

New workflow created:

    NO

R3C-E steps defined:

    BACKEND_O0_CI_STEP_DEFINED=YES
    BACKEND_O2_CI_STEP_DEFINED=YES
    HAL_ADAPTER_CONTRACT_CI_STEP_DEFINED=YES

Local CI-equivalent execution:

    PASS

Workflow semantic claim boundaries:

    PASS

Devil's Advocate:

    PASS

Scope escape:

    NO

Semantic overclaim:

    NO

Duplicate R3C-E CI block:

    NO

Unexpected staging:

    NO

Git diff whitespace/error validation:

    PASS

YAML parser validation:

    NOT_DEMONSTRATED

Reason:

    Local Python installations do not currently provide PyYAML.

GitHub CI execution:

    NOT_DEMONSTRATED

Protected CI enforcement:

    NOT_DEMONSTRATED

## 9. Workflow provenance

Workflow SHA-256 before R3C-E CI mutation:

    D39EE862211CDDEC1AB5776DC0FA992083FDFCDD655C822B5D1E4566BD107C60

Workflow SHA-256 after R3C-E CI mutation:

    5B2015D599867B0F836452508C60CC423887DE01A6C1679502212EFDFCAB048C

## 10. R3C-E file provenance

    firmware/NodeLink/Inc/guardian_node_link_freshness_persistence_backend.h
    SHA256=6D0D2AC58CB069F8263D53820DDB7BAC829805EC518FB5A6778CD060B1739F19
    firmware/NodeLink/Inc/guardian_node_link_freshness_persistence_stm32f401.h
    SHA256=8DBF3CD0CC548716CE622045DDFB38AFF1ACB9D80050308078085E527C066372
    firmware/NodeLink/Src/guardian_node_link_freshness_persistence_backend.c
    SHA256=DCC5891B82B9CE104C5D47B2CBFAFFA94CEBBD462A4D58371BC78E0B9EDE25CA
    firmware/NodeLink/Src/guardian_node_link_freshness_persistence_stm32f401.c
    SHA256=6992D00C4CBC51F7EEB4A5EAE8F58BB6590F2EF9C39C646C3A1B90998395CE6D
    firmware/Tests/node_link_freshness_persistence_backend_host_test.c
    SHA256=97B33B1D4F6C6109FCC5F3EC318D69AC819E846F1BFF18F5EBEBB5E532468B86
    firmware/Tests/node_link_freshness_persistence_stm32f401_adapter_host_test.c
    SHA256=B2525C116F9F6C33B6BFF80CF2BCC660D6D38B1BB224BD404961EE6C4767F55E
## 11. Claims demonstrated by current evidence

    PHYSICAL_BACKEND_PORTABLE_IMPLEMENTED=YES

    STM32_HAL_FLASH_ADAPTER_IMPLEMENTED=YES

    HOST_POWER_LOSS_FAULT_INJECTION_VALIDATED=YES

    HAL_ERROR_PROPAGATION_VALIDATED=YES

    STM32F401_ARM_FREESTANDING_SYNTAX=PASS

    GOVERNED_R3C_REGRESSION=PASS

    R3C_E_FULL_LOCAL_TECHNICAL_DESTRUCTION=PASS

    R3C_E_CI_INTEGRATION_LOCAL=PASS

    R3C_E_CI_DEVILS_ADVOCATE=PASS

## 12. Claims explicitly NOT demonstrated

    STM32_LINKER_RESERVATION_DEMONSTRATED=NO

    TARGET_BINARY_VALIDATED=NO

    HARDWARE_FLASH_WRITE_VALIDATED=NO

    HARDWARE_POWER_LOSS_VALIDATED=NO

    PHYSICAL_DURABILITY_DEMONSTRATED=NO

    AUTHENTICATED_STORAGE_DEMONSTRATED=NO

    ROLLBACK_ANCHOR_DEMONSTRATED=NO

    MONOTONIC_HARDWARE_COUNTER_DEMONSTRATED=NO

    PERSISTENT_ANTI_REPLAY_DEMONSTRATED=NO

    YAML_PARSE_VALIDATION_DEMONSTRATED=NO

    GITHUB_CI_EXECUTION_DEMONSTRATED=NO

    PROTECTED_CI_ENFORCEMENT_DEMONSTRATED=NO

    REJOIN_IMPLEMENTED=NO

    BOOTSTRAP_AUTHORITY_IMPLEMENTED=NO

    EPOCH_TRANSITION_AUTHORITY_IMPLEMENTED=NO

    NODE_SUPERVISOR_CONSUMPTION=NO

    FRESHNESS_GRANTED_BY_BACKEND=NO

    AUTHORITY_GRANTED_BY_BACKEND=NO

    ACTUATION_GRANTED_BY_BACKEND=NO

    AI_AUTHORITY=NO

## 13. Known limitations

1. Both slots being erased cannot by themselves distinguish factory-blank media from total loss of previously established persistent state. Higher-level lifecycle policy must prevent automatic bootstrap after established-state loss.

2. Dual-slot flash state does not provide a strong anti-rollback anchor against an attacker capable of restoring an older physically valid flash image.

3. Physical generation metadata is not a monotonic hardware counter.

4. SHA-256 integrity detects corruption but is not authenticated storage.

5. STM32 flash endurance and production update-frequency suitability have not yet been demonstrated with hardware evidence.

6. Persistence preservation across programmer operations, mass erase, recovery tooling, or firmware deployment has not yet been governed.

7. Linker reservation of Sector 3 and Sector 4 is not demonstrated.

8. A current production target binary has not been built against the reserved memory layout.

9. Real hardware flash programming has not been demonstrated.

10. Real hardware power-loss recovery has not been demonstrated.

## 14. Human adjudication

Architecture consistency:

    PASS

Source-of-truth continuity:

    PASS

Requirements-to-implementation traceability:

    PASS

Positive and hostile test coverage:

    PASS

Technical Destruction:

    PASS

Governed R3C regression:

    PASS

CI local integration:

    PASS

Human diff review:

    PASS

Devil's Advocate:

    PASS

Claim strength within evidence:

    PASS

Semantic boundary preservation:

    PASS

R3C-E local evidence status:

    PASS

R3C-E production hardware evidence status:

    PENDING

## 15. Boundary decision

C5-R3C-E software implementation and local evidence are accepted for progression to repository staging/commit review.

This adjudication does not close hardware durability evidence.

Remaining production evidence boundary:

    R3C-E-E

R3C-E-E remains responsible for hardware durability evidence, target/linker evidence, real flash execution, and real power-loss recovery evidence.

No NodeSupervisor implementation is authorized by this record.

No authority or actuation authority is granted by this record.

## 16. Standards and assurance framing

This work is designed and reviewed using high-assurance engineering discipline informed by selected practices from:

- NIST SP 800-160 Volume 1 and Volume 2;
- NIST SP 800-218;
- CISA Secure by Design / Secure by Default;
- NASA NPR 7150.2D;
- NASA-STD-8739.8;
- applicable secure-by-design principles under the EU Cyber Resilience Act.

These references are engineering and audit benchmarks.

They do not constitute certification, compliance approval, accreditation, endorsement, or third-party validation.

## 17. Repository action status

At generation of this evidence record:

    STAGING_PERFORMED=NO
    COMMIT_PERFORMED=NO
    PUSH_PERFORMED=NO
    PR_CREATED=NO
    MERGE_PERFORMED=NO
    TAG_CREATED=NO
    RELEASE_CREATED=NO
