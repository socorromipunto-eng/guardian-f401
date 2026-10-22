# M17 CI integration candidate and evidence boundary

Status: EXTERNAL_CANDIDATE_NOT_ADMITTED. Gate16 and Gate01 remain open.

## Architecture and existing evidence
F401 remains the deterministic reference/control node. The MCXN947 supervisor, ST measurement, security witness and high-compute roles have portable host implementations; no physical heterogeneous deployment is inferred. Device selections for three additional roles remain NOT_SELECTED. AI remains advisory and no silent-peer takeover is authorized.

05NA covers eleven host recipes at O0/O2; 05O plus 05OA covers fifteen selected existing-CI recipes at O0/O2. This candidate fills eight source-membership omissions in existing CI: six MultiNode/signed-evidence tests, the Monocypher provider test and persistent anti-replay continuation. Three existing key/adapter/lifecycle integration tests are already covered by the lifecycle workflow and are not duplicated here.

## Candidate files
New workflow .github/workflows/m17-portable-host-tests.yml, tools/run_m17_host_tests.py, tools/m17-host-test-recipes.json and this document. Sixteen jobs are planned: eight recipes at O0/O2. The manifest binds every participating source to the reviewed worktree SHA-256, rejects stale/missing/extra membership, duplicate translation units and the known Monocypher resolver-owner conflict. This is a maintenance lock: source changes require explicit manifest review, not automatic hash refresh.

The helper uses GCC already on the runner, records its absolute resolved path/hash/version, compiles each source with strict diagnostics and assertions enabled, records command stdout/stderr and artifact hashes, executes each test with its exact result marker, and checks source/tool/manifest preservation. Timeouts and nonzero results preserve evidence and fail. No tool installation is performed. These records are generated host evidence, not signed release evidence or independently qualified toolchain evidence.

## Policy and review
The new test set does not contain TD-016's intentional const-contract violation; all selected translation units use -Werror without exception. Existing middleware tests and their diagnostic policy are preserved. The 05OA exception remains external host validation evidence, not an undocumented global CI suppression.

The workflow follows the current checkout action reference @v7 and ubuntu-latest platform policy. Neither is an immutable supply-chain lock; Gate17 remains open. Logs preserve readable command evidence, but no artifact-upload dependency is introduced; durable CI artifact retention must be separately decided. This candidate does not make the historical CI identical to Windows Clang results. Linux GCC validation is separately classified.

Human Readability: explicit roles, portable scope, warning policy and maintenance cost. Devil's Advocate: host role tests cannot establish silicon isolation, real flash durability, actual peer links, authorized production-key ceremony or semantic compatibility on hardware. Technical Destruction: manifest/schema/path/duplicate/resolver-conflict rejection is checked before compilation; further sanitizer, mutation, partition, fuzz and call-graph validation remains separately required.

## Normative and release limits
No applicability, compliance, certification or agency endorsement is claimed. Sixteen EU requirement candidates remain subject to legal source/applicability adjudication and claim-to-control-to-evidence mapping. NASA/CISA/aerospace benchmarks are not approvals. No release readiness, Gate01/Gate16 closure, hardware stack, production key or production signing qualification follows from host PASS.

## Admission and next boundary
No repository files have been written by this candidate. Current HEAD 717e53c717af1dc31af540bbcadd9b426f9c9535 and branch feature/m17-heterogeneous-multinode-assurance remain the admission baseline. A worktree-only admission runner must preflight all current 554-file/index/ref locks and four new-path absences; no staging, commit, push, merge, tag or release is implied. The existing uncommitted M17 implementation must be admitted and reviewed in its own eventual commit scope; workflow-only commit would reference absent source at HEAD.

## Local candidate validation, 2026-10-08
The exact candidate helper and manifest were executed with local Linux GCC in a separate source copy: all eight recipes passed at O0/O2 (16 cases). Six unsafe-path and four invalid-manifest cases were rejected (10 negative checks). The workflow matrix was parsed and its 8-by-2 membership checked. These are local results, not GitHub Actions execution, Windows PowerShell parser qualification or physical hardware evidence. Complete command records and compiled artifacts accompany the external validation archive.
