# Guardian M17: 05Y admission and corroboration CI candidate

## Source and evidence

05Y admission archive SHA256: CBFDC3A0470BBB9494EA007B3831CAB529544B3F4F43032313BA680230E65EB0.
Inspected archive CRC, exact four postimages against 05W and 05X, the 562 unchanged file records, HEAD, branch, refs and index identities. Status delta is exactly the four untracked additions; captured total is 566. Evidence supports admission only. The subsequent repeated 05Y invocation stopped against its obsolete 562-file precondition with no write attempts, as expected. Admission runners are single-boundary operations, not rerunnable synchronization tools. No reset or deletion is justified.

## Requirement, control and implementation impact

Existing M17-PART-001 through M17-PART-012 remain OPEN. Corroboration is portable logic supporting the partition/corroboration investigation; host tests do not close these requirements. The ST authority node and planned NXP supervisor retain distinct responsibilities. This increment does not choose or operate additional chips and grants no actuator authority.

The current workflow has eight recipes at O0/O2. Its manifest validator enforces the same eight-member set. Adding only a matrix item would fail validation; changing only the manifest would leave the matrix without the new case. Candidate updates these three existing files together: tools/m17-host-test-recipes.json, tools/run_m17_host_tests.py and .github/workflows/m17-portable-host-tests.yml. Append recipe 12-corroboration with two exact source hashes, one include directory, PASS marker and required 544-case marker. Preserve existing eight recipes and source hashes. Extend validator membership to nine; require unique exact success markers and the additional required line. Extend the workflow matrix to nine by one item. Include this audit record as the fourth candidate path.

## Validation performed by this reviewer

In an external Linux fixture, the candidate GCC driver compiled, linked and executed all nine recipes at O0 and O2: 18 passing executions. Actual result.json and stdout/stderr retained. Four manifest regressions were rejected: missing recipe, duplicate recipe, corrupted corroboration source hash and duplicate translation unit. Original eight recipe objects and their source hashes are identical to preimages. This tests the modified host orchestration and matrix membership consistency, not GitHub Actions itself. No Windows execution occurs in 05Z. No target firmware build, NXP SDK integration, physical communication, certification or regulatory conformance is demonstrated.

## Human readability and adversarial review

A passing matching-commitment test does not prove true observations, global partition absence or authorized actuation. Upstream authentication, persistent freshness and acknowledgement attribution remain assumptions in this component. Gates 01, 13, 14 and 16 remain open. Full runtime/link-library qualification, header hashing in CI, action SHA pinning and physical timing/persistence remain separate open work. actions/checkout@v7 is preserved by this limited increment; it is not an immutable action pin. Compiler and runner semantics differ between local GCC and Windows Clang evidence.

## Change boundary

05Z writes only a new external candidate folder and ZIP. It creates no repository files, updates no existing worktree bytes and stages nothing. A separate exact admission boundary follows candidate adjudication; commit/push/PR and remote CI are later boundaries. Existing pending M17 work must be preserved. No automatic recovery or repeated mutation runner. Ignored files are outside the captured preservation proof.
