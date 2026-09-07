# Guardian F401 - Scripting Guardrails and Anti-Regression Rules

Status: NORMATIVE CANDIDATE - v1.3.1

Scope: Windows PowerShell 5.1, Git, GitHub CLI, m365 CLI, repository automation, validation, release and governance tooling.

Applies to: Guardian F401 engineering work and repository automation within the governed project boundary.

Supersedes: v1.3.0.

Relationship to other controls:

- This document governs repository scripting and automation behavior.
- Higher-level engineering and authority policies remain outside the scope of this document.
- `governance/scripting-incident-register.json` records known scripting failure classes and their prevention controls.

---

## 1. Premise

```text
ONE FAILURE = FIX THE FAILURE
TWO SIMILAR FAILURES = FIX THE METHOD

ASSUMPTION -> VALIDATE
AMBIGUITY -> STOP
UNKNOWN -> FAIL_CLOSED
MUTATION -> VERIFY_WITH_A_DIFFERENT_MEASUREMENT
RECOVERY -> DISCOVER_FIRST

SCRIPT_FAILURE != MUTATION_FAILURE
STDERR_OUTPUT != COMMAND_FAILURE
PRINTED_HEADER != RESULT
REMOTE_MERGE_SUCCESS != LOCAL_STATE_NORMALIZED
EXPECTED_RECOVERY_ARTIFACTS != UNSAFE_DIRTY_WORKTREE

KNOWN_FAILURE_REINTRODUCED = PROCESS_NONCONFORMANCE

AUTOMATION = CONTROLLED_ACCELERATION_NOT_REMOVAL_OF_GOVERNANCE
```

A script SHALL NOT be issued while a matching documented failure class is unresolved.

---

## 2. Mandatory script-generation gate

Before issuing any PowerShell, Git, GitHub CLI, governance, recovery, release or repository automation script:

1. Read this file.
2. Read `governance/scripting-incident-register.json`.
3. Identify all historical failure classes applicable to the requested operation.
4. Determine the current repository state from evidence, not memory.
5. State the exact human-authorized mutation boundary.
6. Run the preflight in section 4.
7. If a known failure class is reintroduced, set `SCRIPT_ISSUANCE=PROHIBITED`.
8. Only then issue the script.

```text
MEMORY = NAVIGATION_AID
SOURCE_OF_TRUTH = GOVERNING_RULE
CURRENT_REPOSITORY_STATE = EVIDENCE
HUMAN_APPROVAL = MUTATION_AUTHORITY
```

---

## 3. Normative rules

| ID | Rule | Origin |
|---|---|---|
| R-01 | Native command failure is determined by `$LASTEXITCODE`, never by STDERR content or `NativeCommandError`. Capture or redirect STDERR deliberately. | SG-001 |
| R-02 | CLI JSON is structured data. Normalize null/empty to `@()`, wrap results with `@(...)`, count with `.Count`, never cast an object collection to `[int]`. Explicitly flatten nested arrays when the CLI shape requires it. | SG-002, SG-003, SG-005 |
| R-03 | Git text identity under EOL normalization uses `git hash-object --path` against the index object ID, not raw worktree SHA alone. | historical EOL incident |
| R-04 | Every mutating script declares one exact authorized boundary and stops before the next boundary. | governance |
| R-05 | After any failure that may have followed mutation, perform read-only discovery first and execute only the missing operation. | SG-004, SG-006 |
| R-06 | Before stage/commit/push/PR/merge/tag/release verify branch, HEAD, local main, origin/main, worktree and staged scope against expected values. Any unexpected movement is STOP. | SG-004 |
| R-07 | A required state must either be proven or established by an explicitly authorized transition. `REMOTE_MERGE_SUCCESS != LOCAL_MAIN_CURRENT`. | SG-004 |
| R-08 | Assert cardinality on both sides of mutations: staged files, commit files, PR files/commits and destination inventory. | SG-006 |
| R-09 | Validate PR properties separately. `PR_EXISTS != PR_CORRECT`; `CI_PASS != HUMAN_APPROVAL`. | governance |
| R-10 | Never issue executable placeholders such as `<path>`, `<branch>` or `C:\ruta`. Use discovered or explicit real values. | SG-007 |
| R-11 | Multi-line executable code is delivered as a file. Console instructions remain one-line invocations. | SG-008 |
| R-12 | Verification measures content or semantic outcome, not presentation form. | SG-009 |
| R-13 | Signing capability is proven with a verifiable artifact. Repository identity and signing key remain local to the project configuration. | SG-010 |
| R-14 | Server-relative URLs are manipulated as URL strings, never with Windows path functions such as `Split-Path`. | SG-011 |
| R-15 | Prior verification may be reused only if repository SHA, input SHA/OID, schema version, tool version, runtime assumptions, authority boundary, platform and semantic profile remain bound and unchanged. Unknown binding means revalidate. | evidence reuse |
| R-16 | Independent read-only checks may be bundled or parallelized. Mutations sharing worktree, index, branch pointer, remote ref or release boundary are serialized. | governance |
| R-17 | Script output uses explicit semantic labels such as `DISCOVERY=`, `MUTATION_PERFORMED=`, `POSTCONDITION=`. Never emit bare `SUCCESS`. | governance |
| R-18 | Before first use of a CLI subcommand/option, read local help and capture the CLI version. Before executing a downloaded script, verify file identity and a content marker. | SG-012 |
| R-19 | Reintroduction of a documented failure class is process non-conformance. Stop mutation-script issuance until the generation method is corrected. | anti-regression |
| R-20 | Every generated mutating script declares `SCRIPT_NAME`, `SCRIPT_VERSION`, `EXPECTED_BASELINE`, `AUTHORIZED_BOUNDARY`, `GENERATION_TIMESTAMP_UTC` and `SCRIPT_SHA256` in its issued evidence or header metadata. | stale-script prevention |
| R-21 | Dirty worktree state is classified before stopping. Exact expected recovery artifacts are `EXPECTED`, not automatically `UNSAFE`. Unexpected modifications remain fail-closed. | SG-013 |
| R-22 | Windows PowerShell 5.1 executable source is ASCII-only by default. Smart quotes, em dash, en dash and other non-ASCII punctuation are prohibited in `.ps1` source unless an explicitly proven encoding-safe path is part of the boundary. | SG-014 |
| R-23 | Exact multiline matching is prohibited when newline representation or byte identity is not proven. Normalize EOL or use bounded semantic parsing/regex and assert exactly one intended match before mutation. | SG-015 |
| R-24 | A controlled document with an explicit recognized lifecycle-bearing self-status SHALL be semantically coherent with its `document-register` lifecycle state. Recognized aliases may normalize to the governed lifecycle; ambiguity or contradiction fails closed. | SG-016 |
| R-25 | A controlled-document mutation is not closed until its register binding is verified against the canonical Git blob at `source_commit:path`, including SHA-256 identity and current-HEAD coherence where applicable. | SG-016 |
| R-26 | Structured native-tool output consumed as JSON MUST come from a dedicated stdout channel. STDERR is diagnostic only and MUST NOT be merged into the JSON parse stream. Native success remains determined by exit code. | SG-017 |
| R-27 | Local Git graph or object validation MUST NOT assume that a remotely identified commit already exists in the local object database. Prove or materialize the object locally, normally by fetch, before merge-base, show, cat-file or equivalent graph validation. | SG-017 |
| R-28 | Windows PowerShell automatic variables and reserved runtime variables MUST NOT be reused as script parameters or mutable state variables. Script generation SHALL preflight identifiers that can alter invocation or runtime semantics. | SG-017 |
| R-29 | Every Windows PowerShell 5.1 script MUST pass a mechanical parser gate before issuance. The parser result MUST contain exactly zero parse errors; otherwise `SCRIPT_ISSUANCE=PROHIBITED`. | SG-017 |
| R-30 | Expandable PowerShell strings MUST NOT rely on ambiguous variable interpolation next to syntactically significant characters. Prefer the format operator, explicit concatenation, or delimited variable syntax. Ambiguous forms such as a variable immediately followed by a colon are prohibited unless parser-safe delimitation is proven. | SG-017 |
| R-31 | Remote ref existence or absence SHALL be treated as state, not success or failure. The script SHALL classify the observed remote ref state against the expected operation before deciding whether to stop, reuse, update, create or escalate. | SG-018 |
| R-32 | Native command output SHALL be parsed with the minimum structure required by the operation. Collection or wrapper abstraction for scalar or line-oriented output is prohibited unless expected cardinality and return shape are explicitly proven. | SG-018 |

---

## 4. Preflight and quality gate

Run before issuing every script.

```text
G0 INTENT
  C-01 exact requested final state stated
  C-02 exact human-authorized boundary stated
  C-03 no convenience mutation added

G1 DISCOVERY
  C-04 current branch and HEAD identified
  C-05 local main and origin/main identified when relevant
  C-06 worktree and staged scope discovered
  C-07 worktree entries classified EXPECTED / UNEXPECTED
  C-08 expected baseline written explicitly

G2 PLAN
  C-09 runtime target declared; Windows PowerShell 5.1 assumed unless proved otherwise
  C-10 historical incidents applicable to this task reviewed
  C-11 native STDERR/exit-code behavior reviewed
  C-12 CLI JSON shape and cardinality reviewed
  C-13 Git EOL/filter behavior reviewed where text identity matters
  C-14 no placeholders
  C-15 multi-line executable delivered as a file
  C-16 PS5.1 source ASCII check passes
  C-17 CLI version/help check included when first using an option/subcommand
  C-18 expected pre-state and expected post-state both defined
  C-18A multiline matching is EOL-tolerant or byte identity is explicitly proven
  C-18B controlled-document self-status and register lifecycle semantics are coherent when lifecycle-bearing self-status is present
  C-18C controlled-document mutations include canonical source_commit:path and SHA-256 register-rebind verification
  C-18D structured native-tool JSON is captured from stdout only; stderr remains a separate diagnostic channel
  C-18E remote Git identity is not treated as local object availability; required objects are proven/materialized before local graph validation
  C-18F PowerShell automatic/reserved runtime variable names are not reused as parameters or mutable state
  C-18G Windows PowerShell 5.1 parser validation completed with exactly zero parser errors before script issuance
  C-18H remote refs required by the operation are classified as ABSENT / EXPECTED / STALE_EXPECTED / UNEXPECTED before mutation
  C-18I function and native-command return shapes are explicitly declared and cardinality-tested before indexing or property access

G3 APPLY
  C-19 script performs only the authorized mutation
  C-20 script stops before stage/commit/push/PR/merge/tag/release unless that exact boundary is authorized
  C-21 failure after possible mutation routes to discovery-first recovery

G4 VERIFY
  C-22 postcondition is measured independently
  C-23 cardinality checked before and after
  C-24 target content identity verified
  C-25 unexpected worktree/staged changes cause STOP

G5 RECORD
  C-26 output labels preserve discovery/mutation/postcondition distinctions
  C-27 script identity/version/baseline/boundary recorded
  C-28 no known incident class reintroduced

IF C-28 FAILS:
  SCRIPT_ISSUANCE=PROHIBITED
```

---

## 5. Task categories

```text
A STATE_DISCOVERY
B BASELINE_VERIFICATION
C AUTHORITY_SCOPE_CHECK
D LOCAL_STATE_NORMALIZATION
E CONTENT_MATERIALIZATION
F SEMANTIC_VALIDATION
G GIT_NORMALIZATION_CHECK
H STAGING_VALIDATION
I SIGNED_COMMIT
J LOCAL_COMMIT_VERIFICATION
K REMOTE_PUSH
L REMOTE_HEAD_VERIFICATION
M PR_CREATE_UPDATE
N CI_VERIFICATION
O HUMAN_MERGE_GATE
P MERGE
Q POST_MERGE_AUDIT
R LOCAL_POST_MERGE_NORMALIZATION
S RELEASE_TAG_DOI
```

Every task declares:

```text
TASK_ID
PURPOSE
INPUTS
EXPECTED_STATE
READ_ONLY_OR_MUTATING
DEPENDENCIES
OUTPUT_EVIDENCE
FAILURE_BOUNDARY
AUTHORIZED_SCOPE
NEXT_ON_PASS
```

---

## 6. Windows PowerShell 5.1 source rule

Default policy:

```text
TARGET_RUNTIME=WINDOWS_POWERSHELL_5_1
SCRIPT_SOURCE_ENCODING=ASCII
NON_ASCII_SOURCE_CHARACTERS=0
SMART_QUOTES=0
EM_DASH=0
EN_DASH=0
UNICODE_PUNCTUATION=0
```

If the script must manipulate Unicode document content, do not embed that Unicode as executable source literals. Prefer:

- byte/Base64 payloads;
- ASCII-safe markers;
- explicit UTF-8 read/write operations;
- hash verification of expected input and output.

A script-generation process SHALL inspect the final `.ps1` bytes and prove all bytes are `<= 0x7F` before issuance when the target is Windows PowerShell 5.1.

---

## 6A. Multiline semantic matching

For text mutation in files whose EOL representation may vary:

```text
EXACT_MULTILINE_MATCH
REQUIRES
BYTE_IDENTITY_PROVEN
```

Otherwise:

```text
NORMALIZE_EOL
OR
USE_BOUNDED_SEMANTIC_PARSING
OR
USE_BOUNDED_REGEX
```

The script SHALL assert the intended match count before mutation.

```text
EXPECTED_MATCH_COUNT=1
ACTUAL_MATCH_COUNT!=1 -> STOP
```

This rule prevents SG-015.

---

## 7. Recovery-state classification

Before treating a non-clean worktree as unsafe:

```text
DISCOVER
-> ENUMERATE_CHANGES
-> CLASSIFY_EACH_CHANGE
   -> EXPECTED_RECOVERY_ARTIFACT
   -> EXPECTED_PREEXISTING_ARTIFACT
   -> UNEXPECTED_CHANGE
-> STOP_ONLY_IF_UNEXPECTED_OR_AMBIGUOUS
```

Exact expected untracked artifacts may be preserved across recovery steps.

```text
EXPECTED_RECOVERY_ARTIFACTS != UNSAFE_DIRTY_WORKTREE
```

---

## 8. Signing proof

Do not alter project history merely to prove signing capability.

Preferred proof:

1. detached signature over a temporary file; or
2. a throwaway temporary Git repository outside the project repository.

A signing test SHALL NOT use `reset --hard` against the Guardian repository.

---

## 9. Human authority boundary

This document governs script generation and execution only. It authorizes nothing.

```text
SCRIPT_VALIDATION != HUMAN_AUTHORIZATION
METHOD_GUARDRAIL != MUTATION_AUTHORITY
```

No implementation, staging, commit, push, PR, merge, tag, release, publication or architecture approval follows merely from this file.

---

## 10. Maintenance

For every new failure class:

```text
OBSERVE
-> RECORD_INCIDENT
-> IDENTIFY_ROOT_CAUSE
-> DEFINE_PREVENTION
-> ADD_OR_AMEND_RULE
-> UPDATE_PREFLIGHT
-> VERIFY_GENERATION_METHOD
-> ONLY_THEN_REISSUE_SCRIPT
```

Promotion from `NORMATIVE CANDIDATE` to `NORMATIVE` requires a complete governed cycle under these rules with no reintroduced incident class.

