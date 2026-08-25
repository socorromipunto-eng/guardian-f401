# RT-M15-004 — Fuzzing Claims and Coverage Adjudication

## Status

PARTIAL

## Classification

Robustness-evidence and public-claim accuracy finding.

## Finding

Guardian F401 contains real M11 fuzzing and sanitizer infrastructure.

However, the public milestone wording previously stated:

`M11 robustness + fuzzing + fault injection — completed.`

That wording could reasonably be interpreted as broader firmware fuzz coverage
than the repository demonstrates.

The public README wording was therefore qualified to match the evidenced M11
scope.

## Confirmed M11 fuzzing evidence

The M11 robustness implementation includes:

- parser mutation driver;
- security mutation driver;
- parser libFuzzer harness;
- security libFuzzer harness;
- parser corpus;
- security corpus;
- ASan/UBSan campaigns;
- bounded parser-recovery validation;
- malformed-input robustness testing;
- authenticated-envelope mutation testing.

The M11 documentation already states that successful fuzz/sanitizer campaigns
do not prove memory safety, functional safety, or cryptographic correctness.

## Critical later-module fuzz coverage

Repository fuzz references were inspected for:

### guardian_control

Fuzz references:

`0`

Status:

`NOT_DEMONSTRATED`

### guardian_firmware_lifecycle

Fuzz references:

`0`

Status:

`NOT_DEMONSTRATED`

### guardian_embedded_link

Fuzz references:

`0`

Status:

`NOT_DEMONSTRATED`

## Ordinary test coverage

The absence of fuzz coverage must not be confused with absence of testing.

Observed ordinary test references:

- `guardian_control`: 105
- `guardian_firmware_lifecycle`: 8
- `guardian_embedded_link`: 52

These tests provide useful functional and fail-closed evidence but are not
equivalent to structure-aware fuzzing.

## Public claim correction

The README milestone statement was changed from:

`M11 robustness + fuzzing + fault injection — completed.`

to:

`M11 bounded parser/security robustness + fuzzing + fault injection — completed for the documented M11 scope.`

The README now explicitly states that M11 completion applies to the documented
bounded parser/security campaigns and does not claim fuzz coverage of later:

- control;
- embedded-link;
- firmware-lifecycle;
- physical-I/O;
- hardware execution paths.

## Decision

RT-M15-004 remains:

`PARTIAL`

The existing M11 fuzzing implementation and evidence are confirmed.

The previous public wording was broader than the demonstrated fuzz target
coverage and has been qualified.

No claim of complete firmware fuzz coverage is authorized.

## Open technical work

Future hardening must separately address structure-aware fuzzing for:

1. `guardian_control`;
2. `guardian_firmware_lifecycle`;
3. `guardian_embedded_link`.

That work must preserve each module's state-machine and fail-closed semantics
rather than relying only on arbitrary-byte mutation.

## Residual restrictions

This record does not establish:

- complete firmware fuzz coverage;
- memory-safety proof;
- physical-hardware fault-injection coverage;
- functional-safety qualification;
- production readiness.

RT-M15-004 may become fully CLOSED only when the remaining critical-module
coverage is implemented, executed, evidenced, and independently reviewed.