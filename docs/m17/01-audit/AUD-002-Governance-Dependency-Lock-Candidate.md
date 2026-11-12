# AUD-002: candidate correction and execution checklist

Author: Antonio José Socorro Marín. Status: EXTERNAL CANDIDATE; AUD-002 OPEN.

## Evidence and exact diagnosis

09B Windows evidence ZIP SHA256:
C4BB6A9FEB7189ACC178131DEBDCAA2B884106588D52FFD36EDE95AFEEAC6DF3.
All 570 sealed entries plus seal.json were checked against lengths and SHA256.
93 native commands were captured. Original lock failed Linux wheel verification as expected;
three additional hostile pip cases rejected wrong hash, missing hash and corrupted wheel.
Native Windows installation and schema controls passed. Linux cross resolution passed;
09B did not execute Linux binaries. Separate Linux qualification passed on CPython 3.12.14.
Neither qualification establishes full repository governance regression coverage.

## Architecture and decision candidate

The current workflow ignores requirements.lock and upgrades pip without a fixed identity.
The lock's rpds-py hash covers the Windows wheel, not the Linux wheel used by CI.
Preserve all six package versions and the Windows wheel hash. Add the reviewed Linux hash.
Replace only the workflow's two installation commands with one explicit installer call.
The new installer binds the seven Linux/pip wheels by exact URL, size and SHA256, checks
HTTPS redirect hosts and bounds responses, then invokes the reviewed pip 25.2 wheel directly.
It installs the six locked dependencies offline with --require-hashes and --only-binary.
It does not upgrade the job's installed pip. Existing governance validation steps are retained.
Pip 25.2 is a candidate tool choice tied to qualification, not a claim of latest version,
security audit, package authenticity beyond the captured provenance, or production approval.
The helper installs into the disposable GitHub job interpreter; do not run it on local hosts.
No cryptographic key, firmware, NodeLink contract or physical authority behavior changes.

## Exact proposed scope

- Replace .github/workflows/m15-semantic-governance.yml.
- Replace governance/requirements.lock.
- Create tools/install_governance_dependencies.py.
- Create docs/m17/01-audit/AUD-002-Governance-Dependency-Lock-Candidate.md.

## Ordered checklist

- [x] 09A: verified clean 604-file baseline and equal-tree branch.
- [x] 09B: reviewed Windows install, schema controls and three hostile cases.
- [x] Separate Linux native dependency/schema qualification.
- [x] 09C preparation: reversible exact workflow substitution and four candidate files.
- [ ] 09C Windows execution and archive adjudication.
- [ ] 09D: full governance regressions in a separate external fixture using these exact bytes.
- [ ] Exercise the installer with controlled positive, redirect, hash, truncation and subprocess failures.
- [ ] Validate YAML semantics, preserve workflow triggers/permissions and all validation commands.
- [ ] Human readability, devil's advocate and technical destruction review of actual candidate.
- [ ] Exact worktree admission after successful validation.
- [ ] Separate exact staging and signed commit; verify fingerprint and committed path set.
- [ ] Exact push, PR, required CI, protected merge and new-main CI adjudication.
- [ ] Close AUD-002 only against the resulting integrated evidence and acceptance criteria.

## Limits and continuity

09C performs no install, network, repository mutation or governance suite execution.
Source baseline: c18dbbac244b2394686a50756cf0ca495e6c8fe3;
branch fix/m17-aud002-governance-dependency-lock; 604 files; clean state.
AUD-003 action SHA pinning is a separate correction. No NASA, CISA, NIST, aviation,
EU machinery/AI, PQC, hardware or release conformance is established by this change.
Gates 01/13/14/16 remain open. AI/advisory does not gain authority or actuation.
