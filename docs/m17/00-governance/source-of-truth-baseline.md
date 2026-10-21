> Guardian F401 M17 candidate artifact.
>
> Status: CANDIDATE / NOT YET NORMATIVE.
>
> Registration does not grant approval.
> Documentation does not grant implementation.
> Implementation does not grant validation.
> Validation does not grant authority.
> Authentication does not grant authority.
> Authority does not automatically grant actuation.
>
> AI/advisory != authority != actuation.
>
> Unknown or ambiguous authority context fails closed.
>
> No certification, regulatory conformity, production-readiness,
> hardware-qualification or external-approval claim is made here.
# M17 Source of Truth Baseline

## Repository baseline

Repository:

socorromipunto-eng/guardian-f401

M17 development branch:

feature/m17-heterogeneous-multinode-assurance

Entry HEAD:

717e53c717af1dc31af540bbcadd9b426f9c9535

Entry tree:

d16f9ffed29c4f3c1b7dd445902aeafe42a489c0

Entry VERSION:

0.16.0

## Baseline release limits

The v0.16.0 release evidence explicitly preserves:

PhysicalHardwareValidation = PENDING

TargetGeometryQualification = NOT_DEMONSTRATED

RuntimeStackSufficiency = NOT_DEMONSTRATED

StorageOwnershipQualification = NOT_DEMONSTRATED

BackupChipDeployment = NOT_DEMONSTRATED

ProductionSecurityQualification = NOT_DEMONSTRATED

CertificationClaimAuthorized = NO

These are M17 inputs.

They are not silently promoted.

## Governance state

The current capability register is empty.

The current M16 requirements register contains sixteen requirements whose
recorded status is NOT_DEMONSTRATED.

At least one M16 claim, session sequence rejection, is separately recorded as
DEMONSTRATED and ACCEPTED with supporting HOST_EXECUTION evidence.

Therefore the requirement, claim and evidence surfaces require formal
reconciliation.

The reconciliation result must not be guessed from identifier similarity.

## Semantic state

The current semantic register contains two CANDIDATE profiles:

STM32F401 / DETERMINISTIC_REFERENCE_CONTROL_NODE

MCXN947 / INDEPENDENT_SUPERVISORY_PEER

The STM32F401 profile records firmware_version 0.15.0.

This value is historical governed state and shall not be silently rewritten
to the current product version.

The F401-MCXN947 compatibility contract currently records:

pre_freshness_compatibility = UNKNOWN

full_message_semantic_compatibility = UNKNOWN

authority_effect = NONE

actuation_effect = NONE

Persistent freshness is not currently represented as implemented.

## Historical permanence

M17 shall add new semantic profiles or change boundaries when needed.

M17 shall not reinterpret historical evidence under a later profile without an
explicit governed migration or compatibility rule.
