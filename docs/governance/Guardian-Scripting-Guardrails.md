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
| R-33 | Validation logic SHALL resolve governance objects using the schema version, canonical container, and canonical identifier field actually observed in the artifact. Similar or legacy field names SHALL NOT be treated as equivalent without an explicit schema mapping. | SG-019 |
| R-34 | Cross-register coherence SHALL be validated bidirectionally and with exact cardinality before a claim, evidence item, assessment, or transition is accepted as coherent. | SG-019 |
| R-35 | Native process invocation SHALL preserve each logical argument as an independent argument boundary. A governed runner SHALL NOT reconstruct an argument vector by naive whitespace joining when any argument can contain whitespace, quoting, metacharacters, or escaping-sensitive content. Invocation design SHALL use a mechanically proven quoting and escaping function or an argument-boundary mechanism whose semantics are verified for the target runtime. | governance |
| R-36 | Asynchronous task completion state SHALL NOT be promoted directly into a protocol or transport root-cause classification. Timeout, cancellation, fault, and successful completion SHALL be distinguished explicitly. Wrapper or aggregate exception state SHALL be unwrapped to the underlying exception before assigning a lower-layer cause. | governance |
| R-37 | Every variable consumed by a governed Windows PowerShell 5.1 runner SHALL be initialized or mechanically proven to exist on every reachable execution path before use. Parser success alone does not prove runtime variable validity. StrictMode variable-reference failures SHALL be classified as runner defects unless independent project evidence demonstrates otherwise. | governance |
| R-38 | A governed Windows PowerShell 5.1 runner SHALL prove that an object property exists through PSObject.Properties before dereferencing it when property presence is not already mechanically guaranteed by an exact validated schema. A null comparison performed after direct property dereference is not a valid existence guard under StrictMode. | governance |
| R-39 | Native output whose leading or trailing characters encode state SHALL be preserved character-for-character until its grammar has been parsed. Generic Trim(), TrimStart(), or TrimEnd() SHALL NOT be applied to fixed-column formats such as git status --porcelain before semantic interpretation. | governance |

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
  C-18J governance validation discovers and binds schema version, canonical container, and canonical identifier field before object resolution
  C-18K cross-register references are validated bidirectionally with exact cardinality before coherence is accepted

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

### E037 - Revision-qualified Git paths must be normalized before repository-relative membership comparison

**Trigger/context:** A read-only target-membership adjudicator consumed `git grep` output generated against `HEAD`.

**Observed symptom:** Paths shaped as `HEAD:firmware/...` were compared directly with target-member paths shaped as `firmware/...`, producing false `TARGET_MEMBER=False` results.

**Root cause:** The Git revision qualifier was retained as though it were part of the repository-relative path.

**Classification:** Runner path-normalization and target-membership defect.

**Mutation status:** Read-only adjudication only. No repository mutation occurred.

**Safe handling:** Preserve repository state, strip the revision qualifier, normalize path separators, and repeat the read-only target-membership adjudication.

**Prohibited automatic recovery:** Do not modify project membership, source files, project files, index state, commits, or remote state based on unnormalized membership results.

**Rule:**

`REVISION_QUALIFIED_GIT_PATH != REPOSITORY_RELATIVE_PATH`

`HEAD:firmware/foo.c -> firmware/foo.c before target-membership comparison`

`NORMALIZE_REVISION_PREFIX + NORMALIZE_SEPARATORS -> TARGET_MEMBERSHIP`

### E038 - Broad provider-candidate heuristics must not be promoted into concrete-provider evidence

**Trigger/context:** Initial B02 discovery classified `B02_CONCRETE_TARGET_PROVIDER_CANDIDATE=True` from broad entropy/token and assignment-pattern hits.

**Observed symptom:** Focused adjudication later demonstrated `RANDOM_ASSIGNMENT_TARGET_HIT_COUNT=0` and no concrete target random callback assignment.

**Root cause:** The broad heuristic treated correlated textual evidence as though it proved an exact `guardian_security_config.random` assignment to a concrete entropy provider.

**Classification:** Runner heuristic false-positive provider-classification defect.

**Mutation status:** Read-only adjudication only. No repository mutation occurred.

**Safe handling:** Treat broad searches as candidate generation only. Require exact assignment resolution, symbol definition, target membership, concrete entropy source, and runtime wiring before provider classification.

**Prohibited automatic recovery:** Do not enable HAL RNG, modify project membership, implement provider code, or promote release state solely because a broad heuristic returned `candidate=True`.

**Rules:**

`BROAD_TOKEN_HITS != CONCRETE_PROVIDER`

`CALLBACK_SURFACE != PROVIDER`

`PROVIDER_EVIDENCE_REQUIRES_EXACT_ASSIGNMENT + SYMBOL_RESOLUTION + TARGET_MEMBERSHIP + CONCRETE_SOURCE + RUNTIME_WIRING`

### E039 - Git grep extended-regex patterns must not use .NET/PCRE inline case flags

**Trigger/context:** A governed read-only B01/B02 provider adjudication invoked `git grep -E` with a pattern containing the inline case-insensitive token `(?i)`.

**Symptom:** `git grep` rejected the pattern and the adjudication controlled-stopped at the first Ed25519 discovery query.

**Root cause:** The runner mixed .NET/PCRE inline regular-expression syntax with Git extended regular-expression mode. `git grep -E` does not accept `(?i)` as a portable ERE case-control construct.

**Classification:** Runner regex-dialect / native-tool-contract defect; no Guardian source, project, provider, or release defect demonstrated.

**Mutation status:** No repository mutation occurred.

**Safe handling:** Preserve repository state. Use `git grep -i` for case-insensitive matching, keep `-E` patterns free of inline .NET/PCRE case flags, and repeat the same read-only adjudication from Phase 0.

**Prohibited automatic recovery:** Do not modify source, project membership, index, commit history, or remote refs because a read-only grep pattern was rejected. Do not interpret grep syntax failure as zero matches or as absence of provider evidence.

**Rule:** A governed runner using `git grep -E` SHALL use Git-supported ERE syntax only. Case-insensitive matching SHALL be requested with `-i`, not with inline .NET/PCRE case-control tokens.

`GIT_GREP_ERE_PATTERN != DOTNET_PCRE_PATTERN`

`CASE_INSENSITIVE_GIT_GREP = git grep -i -E`

`GREP_SYNTAX_FAILURE != ZERO_MATCHES`

`READ_ONLY_QUERY_RUNNER_DEFECT = CORRECT_RUNNER + REPEAT_READ_ONLY_GATE`

**Reusable example:**

Bad:

```text
git grep -E '(?i)ed25519'
```

Good:

```text
git grep -i -E 'ed25519'
```
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


---

## E040 - Helper symbol was invoked without a valid function definition/call contract

**Trigger/context:** During governed defect-ledger adjudication, the master attempted to use a boolean-formatting helper while no callable helper with that exact name was available in the executing scope.

**Observed symptom:** Windows PowerShell raised CommandNotFoundException for the missing helper before any repository mutation or network activity.

**Root cause:** Runner generation referenced a formatting/helper symbol without mechanically proving that the helper was defined and callable in the emitted script. The source parser can accept an unresolved command name, so parser success alone did not prove runtime helper resolution.

**Classification:** Runner helper-resolution/runtime-contract defect; no Guardian project failure.

**Mutation status:** Read-only execution only. No repository mutation occurred.

**Safe handling:** Prefer direct deterministic expressions for trivial boolean formatting. If a helper is necessary, define it before first use and include a source-quality assertion that the required helper definition exists exactly once.

**Prohibited automatic recovery:** Do not create aliases dynamically, import unrelated modules, weaken StrictMode, mutate project files, or retry a mutation boundary merely to resolve a missing runner-local helper.

**Corrected design rule:** Every non-built-in helper referenced by a governed runner SHALL be defined in the emitted runner before first use and SHALL be covered by source-quality/static checks appropriate to that helper contract. Parser success is not proof of runtime symbol resolution.

PARSER_PASS != HELPER_RESOLUTION_PROVEN

HELPER_REFERENCE -> DEFINITION_PRESENT + CALL_CONTRACT_VALID
---

## E041 - No-op branch retained mutation-path postconditions

**Trigger/context:** After adjudication showed that prior defect records were already represented in the governance ledger, a corrected master changed to a no-write path but still retained postconditions from the earlier write design.

**Observed symptom:** Static review demonstrated that the no-op branch would have expected two worktree paths and referenced a mutation-specific expected ledger postimage even though no ledger write should occur. The defect was found before those postconditions executed.

**Root cause:** Mutation and no-op branches shared stale state expectations instead of deriving postconditions from the actual branch outcome (write performed versus no write).

**Classification:** Runner branch-state/postcondition design defect; no Guardian project failure.

**Mutation status:** No repository mutation occurred as a result of this defect.

**Safe handling:** Define expected path counts, expected postimage identity, and final-state assertions from explicit mutation outcome flags. In a no-op branch, require the ledger bytes to equal the preimage and preserve the original worktree cardinality.

**Prohibited automatic recovery:** Do not fabricate a write to satisfy a two-path expectation, do not invent an expected postimage for an unchanged file, and do not clean/reset/restore valid pre-existing worktree state.

**Corrected design rule:** Branch-specific postconditions SHALL be selected from mechanically demonstrated mutation outcome. A no-op branch SHALL assert no-write semantics; a write branch SHALL assert the exact authorized mutation semantics.

NO_WRITE_BRANCH -> POSTIMAGE_EQ_PREIMAGE + PATH_COUNT_UNCHANGED

WRITE_BRANCH -> EXACT_AUTHORIZED_PATH_DELTA + POSTIMAGE_VERIFIED
---

## E042 - Pending adjudication was conflated with demonstrated engineering gap

**Trigger/context:** A selector-binding reconvergence runner summarized one demonstrated engineering gap while also selecting an earlier binding that merely required focused adjudication as the first true blocker.

**Observed symptom:** The same result reported ENGINEERING_GAP_COUNT=1 for the trusted-key resolver/storage lifecycle gap, yet printed FIRST_TRUE_BINDING_BLOCKER=B01 because B01 was the earliest unresolved adjudication.

**Root cause:** The runner used one priority field for two different semantic classes: unresolved evidence/adjudication and mechanically demonstrated implementation absence.

**Classification:** Runner decision-classification defect; no Guardian implementation regression was demonstrated by the inconsistent priority label.

**Mutation status:** The defect was discovered during read-only adjudication. No repository mutation occurred in the defective run.

**Safe handling:** Preserve both dimensions independently. Report the first pending adjudication separately from the first demonstrated engineering gap and keep each binding status unchanged until its own evidence supports promotion.

**Prohibited automatic recovery:** Do not implement code, rewrite a binding, or promote an unresolved adjudication to an engineering defect solely because it appears earlier in binding order.

**Rule:** Governed binding summaries SHALL maintain separate ordered fields for pending adjudication and demonstrated engineering gaps.

`PENDING_ADJUDICATION != ENGINEERING_GAP`

`FIRST_PENDING_ADJUDICATION != FIRST_DEMONSTRATED_ENGINEERING_GAP`

`UNRESOLVED_EVIDENCE != IMPLEMENTATION_ABSENCE`
---

## E043 - PowerShell automatic variable `$Args` shadowed governed native argument parameter

**Trigger/context:** The v0.16 single-file release orchestrator declared native Git wrapper parameters as `[string[]]$Args` and then invoked the wrapper with concrete Git tokens.

**Symptom:** Read-only `Status`, B04/B06 materialization preflight, and B03 preflight all stopped before useful work with `git failed:  | exit=1 | stderr=`. The emitted Git command text was empty.

**Root cause:** `$Args` is an automatic PowerShell variable. Reusing that name as the governed wrapper parameter caused the native argument vector to be lost/empty at runtime under Windows PowerShell 5.1.

**Classification:** Runner PowerShell automatic-variable / native-argv binding defect; no Guardian repository or Git defect demonstrated.

**Mutation status:** No repository mutation occurred in the failed invocations. B04/B06 materialization did not begin; index, HEAD, and worktree remained preserved.

**Safe handling:** Rename wrapper parameters to a non-automatic name such as `$GitArgs` or `$NativeArgs`, fail closed on zero arguments, and rerun read-only preflight from Phase 0.

**Prohibited automatic recovery:** Do not reset, restore, clean, stage, retry a mutation, or reinterpret empty Git argv as repository failure.

**Rule:** Governed PowerShell helpers SHALL NOT use automatic-variable names for explicit parameters. Native-command wrappers SHALL prove a non-empty argument vector before process launch.

`POWERSHELL_AUTOMATIC_VARIABLE != GOVERNED_PARAMETER_NAME`

`NATIVE_ARGV_COUNT_ZERO = RUNNER_DEFECT + CONTROLLED_STOP`

---

## E044 - Runner sidecar state write exceeded exact authorized mutation path

**Trigger/context:** The same v0.16 release orchestrator's B04/B06 materialization phase was authorized to mutate only `docs/architecture/m16-r3d-persistent-anti-replay-rollback-anchor-and-recovery.md`, but its design also called `Save-State`, which would create or modify `.guardian-release/v0.16-orchestrator-state.json`.

**Symptom:** Static adjudication of the runner showed a second repository write path outside the exact human-authorized mutation set. The earlier E043 failure prevented this sidecar write from occurring.

**Root cause:** Orchestrator convenience state was modeled as an implicit repository mutation rather than as derived read-only state or an independently authorized artifact.

**Classification:** Runner authorization-scope design defect; no unauthorized sidecar mutation actually occurred in the observed failed runs.

**Mutation status:** No sidecar state file was written by the failed executions. Repository state must remain preserved.

**Safe handling:** Remove implicit repository sidecar state writes. Derive phase state mechanically from Git/working-tree evidence on every invocation, or authorize any persistent state artifact as its own exact mutation boundary.

**Prohibited automatic recovery:** Do not create the sidecar retrospectively, do not broaden B04/B06 authorization to include it, and do not use later approval as retroactive authorization.

**Rule:** A single-file orchestrator does not create authority to persist hidden or convenience state inside the repository. Exact mutation path authorization remains controlling.

`ORCHESTRATOR_STATE_CONVENIENCE != AUTHORIZED_REPOSITORY_MUTATION`

`AUTHORIZED_PATH_SET_EXACT = NO_IMPLICIT_SIDECAR_WRITE`
---

## E045 - Cross-phase preimage lock was not advanced after an authorized prior-phase mutation

**Trigger/context:** The v0.16 release orchestrator successfully materialized E043/E044 into the scripting guardrails, changing that file's SHA-256. The immediately following B04/B06 phase still called a preflight that required the older pre-E043/E044 guardrails SHA.

**Symptom:** B04/B06 stopped before its authorized architecture write with `Guardrails preimage hash mismatch`, even though the observed guardrails SHA exactly matched the prior authorized E043/E044 postimage.

**Root cause:** The orchestrator treated an earlier phase baseline as a timeless invariant instead of advancing the exact state lock across authorized phase transitions.

**Classification:** Runner cross-phase state-handoff / stale-preimage-lock defect; no Guardian architecture, firmware, Git, or B04/B06 project defect demonstrated.

**Mutation status:** The failed B04/B06 invocation performed no B04/B06 file write, staging, commit, push, network action, reset, restore, or cleanup. The earlier authorized E043/E044 ledger mutation remains preserved.

**Safe handling:** Every phase SHALL lock against the exact postimage demonstrated by the immediately preceding authorized mutation. When a later phase depends on a mutable governance artifact, its expected SHA SHALL be supplied or otherwise mechanically derived from the prior phase result, never silently inherited from the original baseline.

**Prohibited automatic recovery:** Do not restore the guardrails file to its older hash, do not remove E043/E044, do not bypass the hash lock, and do not retry B04/B06 until E045 is durable.

**Rule:** Cross-phase orchestration SHALL distinguish immutable baseline identity from authorized evolving state and SHALL make the expected prior-phase postimage explicit at every mutation boundary.

`AUTHORIZED_PRIOR_PHASE_POSTIMAGE != ORIGINAL_BASELINE_PREIMAGE`

`NEXT_PHASE_EXPECTED_SHA = PRIOR_PHASE_DEMONSTRATED_POSTIMAGE`

`STALE_PREIMAGE_LOCK = RUNNER_DEFECT + CONTROLLED_STOP`

---

## E046 - PowerShell interpolated variable followed by colon is parsed as a scoped variable reference

**Trigger/context:** A governed PowerShell 5.1 runner contained an interpolated diagnostic string with a variable immediately followed by a colon, for example `"$i: actual=..."`.

**Symptom:** Windows PowerShell stopped at parse time with `Variable reference is not valid.
'':'
 was not followed by a valid variable name character` and `FullyQualifiedErrorId : InvalidVariableReferenceWithDrive`.

**Root cause:** In an expandable PowerShell string, a colon immediately following an unbraced variable token can be parsed as part of scoped/provider variable syntax rather than as literal punctuation.

**Classification:** Runner parser-compatibility defect; no Guardian project defect demonstrated.

**Mutation status:** The defective runner failed during parsing before its script body executed. No repository, key-file, index, network, commit, or push mutation occurred.

**Safe handling:** Preserve repository state. Correct only the runner text, re-preflight the same governed boundary, and use explicit braced interpolation or string concatenation.

**Prohibited automatic recovery:** Do not reset, restore, clean, unstage, delete candidates, or infer a project failure from a parser error.

**Rule:** When literal punctuation immediately follows an interpolated PowerShell variable, delimit the variable explicitly or concatenate the punctuation.

`$i: text` = PROHIBITED_IN_GOVERNED_EXPANDABLE_STRING

`${i}: text` = SAFE_EXPLICIT_DELIMITATION

`$Label +
':'
 + $Value` = SAFE_CONCATENATION

---

## E047 - ProcessStartInfo.ArgumentList is unavailable in Windows PowerShell 5.1 / .NET Framework

**Trigger/context:** A read-only governed runner used `System.Diagnostics.ProcessStartInfo.ArgumentList` while executing under Windows PowerShell 5.1.

**Symptom:** The runner controlled-stopped with `The property
'ArgumentList'
 cannot be found on this object` before repository mutation.

**Root cause:** `ProcessStartInfo.ArgumentList` belongs to newer .NET implementations and is not available on the .NET Framework surface used by Windows PowerShell 5.1.

**Classification:** Runner runtime/API compatibility defect; no Guardian source, provider, test, or repository failure demonstrated.

**Mutation status:** Read-only boundary. No repository, index, key-file, network, commit, or push mutation occurred.

**Safe handling:** Preserve state. Use `ProcessStartInfo.Arguments` with deterministic governed quoting, keep `UseShellExecute = $false`, redirect stdout/stderr separately, drain both asynchronously, and adjudicate the native process exit code plus resulting state.

**Prohibited automatic recovery:** Do not mutate repository state, switch shells, or automatically retry a mutation merely to work around the unavailable property.

**Rule:** Windows PowerShell 5.1 governed runners SHALL NOT use `ProcessStartInfo.ArgumentList`. Native argv must be rendered through an explicit PowerShell-5.1-compatible argument builder into `ProcessStartInfo.Arguments`.

`WINDOWS_POWERSHELL_5_1 -> ProcessStartInfo.Arguments`

`ProcessStartInfo.ArgumentList -> PROHIBITED_UNLESS_RUNTIME_CAPABILITY_IS_EXPLICITLY_PROVEN`

`NATIVE_EXIT_STATUS != STDERR_TEXT`


---

## E048 - Safe native argv grammar and spaced search predicates

  - Trigger/context: A read-only master passed the fixed search text RELEASE_READY_EQUALS to a native helper whose governed safe-token grammar intentionally rejects spaces.
  - Observed symptom: The runner stopped before release convergence with a safe-token validation error; no repository mutation or network action occurred.
  - Root cause: A PowerShell-side search/filter requirement was incorrectly transported as a spaced native argv token instead of using a safe native token and filtering exact text in PowerShell.
  - Classification: Runner argv-contract defect; no Guardian project failure.
  - Mutation status: Read-only execution only; no repository mutation occurred.
  - Safe handling: Preserve repository state. Search natively with a safe token such as RELEASE_READY, then apply the spaced/exact predicate inside PowerShell.
  - Prohibited automatic recovery: Do not relax the native safe-token grammar, add ad hoc quoting, retry mutations, or reinterpret the stop as a project defect.
  - Corrected design rule: Native helpers constrained to safe-token argv SHALL NOT receive arguments outside that grammar. Requirements containing spaces or shell-sensitive content SHALL be resolved with safe candidate retrieval plus in-process filtering.
  - Invariant: SAFE_NATIVE_ARGV + POWERSHELL_FILTERING, never SAFE_TOKEN_BYPASS_FOR_SPACED_PATTERN.

---

## E049 - Optional .NET property access under StrictMode

Related incident: E047. This record preserves the previously misplaced observation without replacing the historical E038 identity.

  - Trigger/context: A PowerShell 5.1 read-only adjudicator accessed ProcessStartInfo.ArgumentList while StrictMode was enabled.
  - Observed symptom: The runner raised PropertyNotFoundException because the optional property is not present on the observed PowerShell 5.1/.NET object; no repository mutation or network action occurred.
  - Root cause: The runner accessed an optional/version-dependent property directly instead of probing property existence first or using the PowerShell 5.1-compatible Arguments path.
  - Classification: Runner runtime-compatibility/StrictMode defect; no Guardian project failure.
  - Mutation status: Read-only execution only; no repository mutation occurred.
  - Safe handling: Probe PSObject.Properties for ArgumentList before access. When absent, use the already-governed PowerShell 5.1-compatible argument transport without broadening the accepted argv grammar.
  - Prohibited automatic recovery: Do not disable StrictMode, assume modern .NET members exist, relax argv controls, or mutate repository state to compensate for an environment/API-shape mismatch.
  - Corrected design rule: Every optional or version-dependent property SHALL be existence-tested through PSObject.Properties before direct access under StrictMode.
  - Invariant: PROPERTY_EXISTS_BEFORE_ACCESS; POWERSHELL_5_1_COMPATIBILITY_IS_A_PREFLIGHT_CONTRACT.

---

## E050 - Runner-local variable collided with automatic PowerShell Matches

Trigger/context: 04Z signing preflight stored a Regex MatchCollection in `$matches` and subsequently evaluated a scalar `-cmatch` predicate.
Observed symptom: `The property 'Groups' cannot be found on this object` at `$matches[0].Groups[1].Value`.
Root cause: PowerShell variable names are case-insensitive. Successful scalar matching replaced the automatic `$Matches` value with a hashtable, overwriting the runner-local binding.
Classification: runner runtime/parser-state defect; not a Guardian source or signing-key failure.
Mutation status: the original 04Z output records STAGE_ATTEMPTED=False and COMMIT_ATTEMPTED=False. The external signing proof directory was created and preserved.
Safe handling: preserve the signing proof, HEAD, index and worktree; correct the helper under a new runner identity. 04ZA used `$signatureMatches` and `[regex]::IsMatch`, passed four synthetic parser cases, and subsequently produced a verified signed commit.
Prohibited automatic recovery: no repeat of 04Z, reset, restore, clean, amend, automatic restage or retry after partial mutation.
Design rule: never assign to automatic/reserved variables, including `$Matches` and `$Args`. Keep extraction collections in uniquely named variables and use `[regex]::IsMatch` for boolean predicates when match state must be preserved.
Validation limit: synthetic parser cases prove parser behavior only; cryptographic validity requires the independent GPG verification result.

---

## E051 - Staged whitespace defect in 04ZD ledger remediation

Trigger/context: 04ZD relocated incident records and appended E050 without validating the complete candidate diff before writing and staging the ledger.
Observed symptom: the staged whitespace check reported trailing spaces at lines 576, 596 and 606, and stopped with STAGED_WHITESPACE_CHECK_FAILED_PRESERVE_INDEX.
Root cause: three trailing spaces already present in the reviewed ledger were carried into the candidate; the runner checked Git diff whitespace only after repository mutation.
Classification: runner candidate-validation ordering defect; not a firmware or signing-key failure.
Mutation status: LEDGER_WRITE_ATTEMPTED=True, STAGE_ATTEMPTED=True, COMMIT_ATTEMPTED=False. HEAD remained 760978bd28290d601c66c181cb64b62dcb3b2872; one ledger path remained staged and the architecture document remained unstaged.
Safe handling: preserve the stopped state. A distinct continuation locks that exact staged blob and raw ledger hash, removes only the three recorded trailing spaces, appends this incident, stages only the ledger and attempts one signed commit.
Design rule: validate the complete candidate bytes and diff before repository writes or index changes; preserve all unrelated work and verify signed commit scope, parent and signer.
Prohibited automatic recovery: no repeat of 04ZD, reset, restore, clean, amend, blanket staging or automatic retry after a partial mutation.
Validation limit: static checks of a generated continuation do not establish successful Windows execution or release readiness.
