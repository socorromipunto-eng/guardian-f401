# Guardian F401 v0.17.0 Release Closure Requirements

Status: CONTROLLED CANDIDATE
Requirement namespace: V017-CR

## Governance and evidence

- V017-CR-001: Every closure gate shall identify an exact entry branch, HEAD, tree, worktree/index state, and dependency hashes before mutation.
- V017-CR-002: Every repository mutation boundary shall require one explicit human authorization token.
- V017-CR-003: Automatic reset, restore, clean, retry, unstage, amend, revert, force-push, or hidden mutation is prohibited after a controlled stop.
- V017-CR-004: Every release claim shall identify evidence, limitations, and whether the evidence is host, target, physical, documentary, CI, or human-review evidence.
- V017-CR-005: Unchanged components shall reuse prior PASS evidence by exact hash; changed dependencies shall trigger impact revalidation.
- V017-CR-006: Stage, commit, push, PR, merge, tag, and release shall remain separate human-authorized boundaries after technical closure.

## Authority and actuation

- V017-CR-010: STM32F401 shall remain deterministic reference/control authority for v0.17.
- V017-CR-011: Heterogeneous peers shall not obtain automatic actuator authority through supervision, witness, quorum, compute capacity, or failover.
- V017-CR-012: Unknown or ambiguous authority state shall fail closed.
- V017-CR-013: AI/advisory output shall not be authority and shall not directly actuate.
- V017-CR-014: Valid signature, trust, authorization, acceptance, and actuation authorization shall remain distinct states.

## Cryptography, identity, and keys

- V017-CR-020: Production signature verification shall use the locked Ed25519 provider boundary and shall fail closed on invalid input, invalid signature, unknown key, non-production key, or unavailable backend.
- V017-CR-021: A production trust anchor shall require ACTIVE state, exact algorithm match, PRODUCTION_APPROVED provenance, and a nonzero evidence digest.
- V017-CR-022: Legacy key_id=2 shall not be retroactively promoted solely because it was historically present or operational.
- V017-CR-023: Production private key material shall not be stored in the repository or on the STM32F401 target.
- V017-CR-024: Production key generation shall occur only after ceremony environment qualification, role separation, and human authorization.
- V017-CR-025: Production key proof-of-possession shall be domain separated and shall not by itself establish trust or authorization.

## Freshness, replay, persistence, rollback

- V017-CR-030: Authenticated shall not imply fresh; persisted shall not imply fresh.
- V017-CR-031: Persistent anti-replay initial provisioning shall distinguish first provision from continuation and shall fail closed on ambiguous prior state.
- V017-CR-032: Rollback-anchor advancement and persistent-state promotion shall preserve ordering that prevents silent rollback acceptance.
- V017-CR-033: Physical rollback resistance shall not be claimed until a target-backed anchor is demonstrated.
- V017-CR-034: Power-loss and torn-write validation shall exercise defined interruption points and preserve fail-closed behavior.

## Distributed safety and failover

- V017-CR-040: Quorum shall not be equivalent to authority.
- V017-CR-041: Partition and split-brain conditions shall not create competing actuation authority.
- V017-CR-042: Failover shall be bounded and shall not silently transfer or expand actuation authority.
- V017-CR-043: Witness, supervisory, measurement, and high-compute roles shall remain semantically separate from reference-control authority.

## Firmware lifecycle

- V017-CR-050: Firmware update acceptance shall require expected algorithm, approved production trust anchor, correct signed transcript, image integrity, and rollback policy.
- V017-CR-051: Boot confirmation shall not advance rollback state before required persistent evidence is safely committed.
- V017-CR-052: Failed boot/recovery behavior shall preserve authority and rollback invariants.
- V017-CR-053: Update, boot, rollback, recovery, signing, trust, and version semantics shall be cross-integrated before release closure.

## Verification and destruction

- V017-CR-060: Every new or changed security surface shall have positive, negative/hostile, and technical-destruction verification as applicable.
- V017-CR-061: Strict C compilation shall treat warnings as errors on changed portable C surfaces unless a separately adjudicated exception exists.
- V017-CR-062: Optimization-sensitive security behavior shall be compared across required build profiles such as O0/O2 when relevant.
- V017-CR-063: Tests shall prove absence of bypass/fallback paths, not only the expected positive path.
- V017-CR-064: Test doubles shall be clearly separated from production providers and shall never become evidence of a real production key or physical target.

## CI, supply chain, and release integrity

- V017-CR-070: M17 CI shall execute the release-critical changed-surface tests and shall fail closed when required test inputs or dependencies are absent.
- V017-CR-071: Third-party cryptographic source, version, commit/blob identity, license, local patch state, and hashes shall remain captured.
- V017-CR-072: Provider substitution shall require a new governed boundary and impact review.
- V017-CR-073: Release artifacts shall have reproducible identities/hashes and a machine-readable evidence manifest.
- V017-CR-074: Supply-chain evidence shall distinguish source provenance, build-tool provenance, build result identity, and release authorization.

## Physical and release closure

- V017-CR-080: Host validation shall not be represented as physical hardware qualification.
- V017-CR-081: Physical hardware qualification shall identify the actual board/device configuration and evidence scope.
- V017-CR-082: Candidate peer hardware shall not be called final unless official evidence supports the claim.
- V017-CR-083: Final release review shall challenge every release-critical claim for stale, missing, circular, self-referential, or mismatched evidence.
- V017-CR-084: v0.17.0 release readiness shall remain HOLD until every release-critical domain is DEMONSTRATED or evidenced NOT_APPLICABLE.
- V017-CR-085: Technical release closure shall not itself authorize Git stage, commit, push, PR, merge, tag, or publication.