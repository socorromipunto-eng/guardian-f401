# Guardian F401 M15 - G15-05 Authority Boundary Enforcement

## Status

IMPLEMENTED_AND_LOCALLY_VALIDATED

REMOTE_CI_EXECUTION_CONFIRMED

## Purpose

This record adjudicates the implementation state of G15-05 after architecture definition, mechanical enforcement, hostile validation, regression testing and CI-gate integration.

It does not claim physical actuation validation.

It does not claim remote GitHub Actions execution because the reviewed branch has not been pushed.

## Controlled lineage

Architecture baseline:

808db44 - docs(m15): define g15-05 authority reachability model

Enforcement implementation:

cf7df6f - feat(m15): enforce authority boundary policy and regression

CI gate integration:

a2ae6f3 - ci(m15): enforce authority boundary gate

## Implemented controls

G15-05 now includes:

- machine-readable authority policy;
- Guardian-specific authority validator;
- canonical run-permit ownership enforcement;
- authority-output callback invocation ownership enforcement;
- approved output-configuration path enforcement;
- production runtime-heap prohibition;
- comment and string-literal filtering;
- multiline authority-sensitive matching;
- callback-alias detection;
- permanent hostile regression tests;
- dedicated platform-neutral CI workflow.

## Technical Destruction

Validator v1 result:

3 / 7 robust cases

4 / 7 weak, bypassed or false-positive cases

Validator v2 result against the same hostile set:

7 / 7 robust cases

0 / 7 weak or bypassed cases

The hostile set demonstrated detection of:

- multiline run-permit mutation;
- aliased run-permit mutation;
- callback alias invocation;
- unauthorized output-configuration wrapper;
- macro-based run-permit mutation;
- multiline runtime-heap introduction.

The validator also demonstrated false-positive resistance for authority-like text appearing only inside comments and string literals.

## Permanent regression

Permanent G15-05 regression count:

9 tests

The permanent regression includes baseline acceptance and hostile rejection cases derived from Technical Destruction.

## Full assurance regression

The locked assurance environment was reconstructed from the repository dependency lock.

The repository-defined assurance/src import path was used.

The complete assurance regression was green after G15-05 Technical Destruction findings were promoted to permanent tests.

No G15-05 regression was demonstrated.

## CI enforcement

Dedicated workflow:

.github/workflows/authority-boundary-tests.yml

The workflow:

- uses locked assurance dependencies;
- executes the authority-boundary validator;
- executes the permanent G15-05 regression suite;
- remains separate from STM32F401-specific hardware validation.

The workflow is committed to the local integration branch.

Remote GitHub Actions execution is CONFIRMED for the reviewed integration branch.

Remote evidence:

- reviewed head SHA: `3e3318f491b3ca371a9a4dd25ba6cf80d5b37dda`;
- pull request: `#11`;
- Authority Boundary Tests run: `32922326472`;
- Authority Boundary Tests status: `completed`;
- Authority Boundary Tests conclusion: `success`;
- M14 Assurance Python 3.12 run: `32922326454`;
- M14 Assurance Python 3.12 status: `completed`;
- M14 Assurance Python 3.12 conclusion: `success`.

The reviewed PR check set completed successfully after correction of the stale M14 historical test-discovery scope.

## Platform portability

G15-05 enforcement is defined around Guardian authority roles rather than STM32F401 physical-output implementation.

The intended contract remains:

Guardian Core Authority
    -> Platform Authority Adapter
    -> Target-Specific Physical I/O

A future target may replace the Platform Authority Adapter without changing Guardian Core authority semantics.

## Physical-actuation boundary

Physical output remains NOT_IMPLEMENTED / NOT_DEMONSTRATED in the reviewed baseline.

Closure of logical authority enforcement must not be interpreted as physical actuator validation.

Physical hardware and actuator validation remain owned by their separate validation gates.

## Current adjudication

Architecture: CONFIRMED

Machine-readable policy: CONFIRMED

Mechanical authority validator: CONFIRMED

Technical Destruction 7/7: CONFIRMED

Permanent hostile regression: CONFIRMED

Locked full regression: CONFIRMED

CI workflow committed: CONFIRMED

Remote CI execution: CONFIRMED

Physical actuation: NOT_DEMONSTRATED

## Disposition

G15-05 implementation is complete and locally validated.

The remote-CI closure condition for G15-05 has been satisfied.

Final Human Readability and Devil's Advocate review found no demonstrated basis to withhold closure of the G15-05 logical/mechanical authority-enforcement boundary.

G15-05 disposition: CLOSED_FOR_LOGICAL_MECHANICAL_AUTHORITY_BOUNDARY.

RT-M15-001 must not yet be rewritten as fully closed solely from local evidence.

Remote CI PASS has now been demonstrated for the reviewed head.

G15-05 logical/mechanical authority-enforcement closure is confirmed for the reviewed evidence set.

Physical actuation remains separately open and NOT_DEMONSTRATED.

RT-M15-001 must be reconciled separately before its disposition is changed.
