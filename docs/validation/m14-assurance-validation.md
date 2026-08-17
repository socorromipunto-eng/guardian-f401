# M14 assurance validation record

## Classification

`CONFIRMED_BY_EVIDENCE` for the enumerated software tests and repository gates.
Hardware and production claims remain `PENDING_VALIDATION` or not claimed.

## Commit chain

| Purpose | Commit |
|---|---|
| Post-publication baseline | `50ebf92f4aa2cf99e257a5a82e1717354fdacb53` |
| S1 assurance slice | `e3f209d7b7aa2ca2f15ba61398fcf9d9b8797d25` |
| S2 closed contracts | `4349aa37489df1364b03a08aa610c0d0ffa9c795` |
| Python 3.12 workflow | `3e862ab7699abca30c9bc5582d0d160142901e15` |
| EOF-only correction | `072446f8e0d633095406bb34601a6e3f3d2836e0` |

## Final validation

- Local suite: 87 discovered, 87 executed, 0 failures.
- Final commit Git integrity: passed.
- Final commit working tree: clean.
- Push type: ordinary fast-forward; no force push.
- Remote branch tip: `072446f8e0d633095406bb34601a6e3f3d2836e0`.
- GitHub Actions run: <https://github.com/socorromipunto-eng/guardian-f401/actions/runs/31983989907>.
- Remote conclusion: success; failed jobs: 0.
- Final evidence-manifest SHA-256:
  `0E3715DF6F80FECD9C804F0FA87FCF9AC22EFE917EFAC18F3E8C902B7C3B626A`.

## Additional bounded evidence

- 87 individual test executions passed during S2 validation.
- 10 seeded randomized complete suites passed.
- One ordered complete suite passed.
- A bounded Java oracle completed 10 checks with zero divergences.
- Eight coverage groups were reviewed with zero recorded gaps after correction.
- RFC 8785 evaluation passed nine targeted checks and a 1,000-run byte
  determinism vector.

## Error and correction disclosure

Control tooling encountered and preserved orchestration defects involving
native stderr handling, manifest file locking, strict-mode empty results,
PowerShell JSON-array enumeration, a stale expected test count and EOF
formatting. These were tooling or evidence-processing defects unless otherwise
stated; they were corrected and revalidated. The final EOF commit contains only
nine one-line deletions and received a new successful local and remote run.

## Interpretation boundary

A passing result demonstrates only the executed vectors on the recorded
software environments. It is not proof of exhaustive correctness, hardware
qualification, real-time behavior, production readiness, safety or
cybersecurity certification.
