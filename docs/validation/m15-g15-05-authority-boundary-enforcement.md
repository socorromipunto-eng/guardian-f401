# Guardian F401 M15 - G15-05 Authority Boundary Enforcement

## Status

IMPLEMENTED_AND_LOCALLY_VALIDATED

REMOTE_CI_EXECUTION_NOT_DEMONSTRATED

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

Remote GitHub Actions execution remains NOT_DEMONSTRATED because no push has been performed.

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

Remote CI execution: NOT_DEMONSTRATED

Physical actuation: NOT_DEMONSTRATED

## Disposition

G15-05 implementation is complete and locally validated.

G15-05 must remain OPEN_FOR_REMOTE_CI_EVIDENCE until the committed authority-boundary workflow is executed by the repository CI service and produces passing evidence.

RT-M15-001 must not yet be rewritten as fully closed solely from local evidence.

After remote CI PASS, G15-05 may be reconsidered for closure of the logical/mechanical authority-enforcement finding while physical actuation remains separately open.
