# Portable corroboration API — candidate

Basis: the hash-locked M17 partition contract and M17-PART-001..012. This increment is a candidate implementation, not a gate closure, node-to-node deployment or legal conformity.

Capacity is five fixed peer slots. Roles are control=0, supervisor=1, measurement=2, witness=3, analysis=4. All slots have unique nonzero IDs. Threshold AND required-role coverage are mandatory. Generation, policy, profile and round are nonzero fixed uint32 identities. Commitments are opaque 32-byte values; this module neither canonicalizes nor signs them.

## Precedence, before implementation

| Event | Result | Accepted evidence changes |
| --- | --- | --- |
| Null/unconfigured API input | Reject | None |
| Malformed identity/flags/context/sequence/replay | Reject | None |
| Invalid local clock or rollback after otherwise valid input | REJOIN_REQUIRED, deny | Previous records retained for review |
| Changed authenticated peer epoch | REJOIN_REQUIRED, deny | Previous records retained |
| Non-control authority claim or commitment disagreement | CONFLICT_LATCHED, deny | Conflicting report retained separately |
| Valid matching record | Store one slot, assess live age/coverage | Its cursor advances |
| Required evidence missing, expired or lacks same-round acknowledgement | INSUFFICIENT | No positive eligibility |
| Fresh agreement meets count AND role mask | CORROBORATED | Eligibility for separate local review only |
| Conflict followed by ordinary packets/timeout | Remain CONFLICT_LATCHED | No clearing |
| Explicit link loss | Clear availability, retain anti-replay cursor | Reassess; no authority transfer |
| Reset/rejoin request | REJOIN_REQUIRED unless conflict already latched | No clearing |

Conflict dominates rejoin. A malformed packet cannot clear an existing finding. Link loss retains the cursor and accepted report, distinguishing availability from history. Expiration uses local reception age, with subtraction only after a future-time check; a caller must supply a validated monotonic clock. Freshness of upstream observations, persistent epoch/sequence validation, authentication, semantic canonicalization and attribution of bidirectional acknowledgements are integration obligations. The acceptance flags are explicit host assumptions, not verification services.

No reconciliation/clear-conflict API is supplied. Configure is allowed only in an initialized UNCONFIGURED object; configure cannot clear a latch. Reinitializing RAM is not authorized reconciliation: production use requires independently persisted findings and a governed reconciliation service before any positive state can influence local policy. Single-owner, non-reentrant access only; no thread/ISR guarantee. Caller must not alias report/output memory with the context. No actuator callbacks, takeover, networking, storage, heap or recursion.

Synthetic timeout values in tests are not a deployment selection. The output carries the fixed configuration, per-peer accepted report, local reception time, availability, count, role coverage and the report that triggered conflict. Agreement is not truth; local visibility cannot prove global partition absence. An authenticated liar agreeing with others is outside this mechanism's detection capability.

Host tests and selected runtime guard-removal mutants are evidence for this candidate only. Exhaustive guard mutation, Windows execution, CI registration/header locking, actual cryptographic integration, persistence, target builds, physical operation, WCET/stack and plant response remain pending. Gates01/13/14/16 remain open. ST F401 retains reference control authority; NXP and additional roles acquire none from this API.

## Candidate host evidence

Exact candidate header/source/test, strict GCC C11 with warnings as errors: O0 and O2 each executed 544 cases successfully. AddressSanitizer and UndefinedBehaviorSanitizer execution passed with `ASAN_OPTIONS=detect_leaks=0`; the initial default LeakSanitizer attempt failed due to this execution environment's process inspection restrictions. Leak checking is NOT_EXECUTED, not a passing property. No dynamic allocation exists in this component.

Four selected denial predicates were separately removed: authentication acceptance, required-role coverage, local reception age and non-control actuation claim. All four mutants compiled and were killed by runtime assertions at both O0 and O2 (8/8 executions). This is selected mutation coverage, not all-guard or exhaustive destruction. Tests assume upstream acceptance and do not prove real signatures, persistent counters, true observations or an actual board. Windows and GitHub execution are not performed by this candidate runner.

The 05VA archive shows exactly three already-present proposed documents and an unchanged original 559-file set, HEAD, branch, refs and index; current baseline is 562. Their creating process and chronology remain UNKNOWN. 05V failed before attempting writes. The admission runner must not be replayed.
