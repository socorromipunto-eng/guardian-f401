# M17 signed-envelope codec and transcript candidate

Status: EXTERNAL_CANDIDATE_NOT_ADMITTED. Version: research wire v2 candidate, not an operational NodeLink version. Baseline: 575 paths, HEAD 717e53c717af1dc31af540bbcadd9b426f9c9535, feature/m17-heterogeneous-multinode-assurance, after 06H/06I.

## Decision and compatibility

Use a separate bounded v2 signed-envelope codec rather than silently changing the v1 enum/parser or enlarging its payload to contain a detached signature. Existing v1 code and historical semantics are untouched. Magic GS differs from GN; v2 only carries proposed observation type 7 and ACK type 8. It must not fall back to v1 if validation fails. This choice is a candidate for review, not a compatibility approval or implemented migration.

## Exact binary fields

Little endian integers; no C struct serialization. Header size 32 bytes:

| Offset | Length | Field |
|---|---|---|
|0|2|magic 0x47 0x53|
|2|1|version 2|
|3|1|type 7 observation or 8 ACK|
|4|1|flags zero|
|5|1|state 0 through 5|
|6|1|signature algorithm 1, Ed25519 candidate|
|7|1|reserved zero|
|8|2|payload length 52 for observation, 60 for ACK|
|10|2|signature length 64|
|12|4|sender node ID, nonzero|
|16|4|producer ID, nonzero|
|20|4|key ID, nonzero|
|24|4|sender epoch, nonzero|
|28|4|sender sequence, nonzero|
|32|52 or 60|canonical payload, opaque to this layer|
|84 or 92|64|unverified signature bytes|

Packet sizes are exactly 148 or 156 bytes. API bound is 160 bytes; no other payload lengths accepted. The signature is stored in the same bounded packet as the signed header/payload. No CRC is introduced: transport error detection and signature verification are distinct concerns. Physical UART/SPI/CAN stream framing, fragmentation, resynchronization, queue bounds and timeout remain unimplemented.

## Exact signature transcript

ASCII GUARDIAN-F401:NODELINK:SIGNED-ENVELOPE:V2 without NUL, followed by the exact canonical 32-byte header and payload. Signature bytes are excluded. Version, type, algorithm, reserved fields, identity, epoch, sequence, lengths and all payload bytes are bound. No prehash mode is specified. This transcript differs from AUTH:V1 and must use a separately reviewed verification integration; no reuse of v1 verification as if transcripts were interchangeable.

## Security contract

Decode outputs gse_unverified only. Structural acceptance does not authenticate any field. Arbitrary and all-zero signature bytes are structurally representable; a later cryptographic verifier must reject invalid signatures. Signature substitution is not rejected by the codec. Host tests confirm substitution leaves the transcript unchanged; a real verification integration must subsequently prove rejection of substituted/invalid signatures against the exact transcript. Existing signature-provider keys/roles/semantics/freshness are not bypassed or replaced.

ACK sender epoch/sequence identify the ACK, while payload epoch/sequence reference the observation. The outer authenticated identity must be checked against locally configured pending identity before ACK consumption. Neither packet validity nor signature validity grants freshness, truth, authorization, quorum acceptance or actuator permission.

Encode/decode/transcript use fixed buffers and preserve output data on failure. Reported output length is zero on failure. The length pointer must not alias input/output; input/output data may overlap. No heap, recursion, mutable global state or actuator API is introduced. Stack/time bounds require target measurement. Signature bytes are not production secret keys, and this candidate neither creates nor provisions keys.

## Evidence and limitations

Linux GCC strict O0 and O2: 31,337 CHECK evaluations per execution, not 31,337 distinct test scenarios. UBSan additional run passed. Golden header bytes, payload/signature boundaries, exhaustive discriminant/reserved substitutions, all truncations, extra lengths, zero identity fields, every header/payload bit, signature substitution representation and capacity failure preservation are covered. Four selected mutants (version, reserved, payload transcript binding, key binding) were detected. No complete mutation campaign or ASan claim. Windows O0/O2 execution remains pending until runner evidence review.

The test plan covers ING-R01/R02/R03/R04/R06/R16 only at representation/transcript scope. It does not close these integration requirements; ING-R05 and remaining ingress requirements stay pending. No production Ed25519 verification, persistence, reset durability, pending-state transaction, target build/boot, physical ST-NXP interoperability, regulatory conformity or certification is demonstrated.

## Critical reviews and next dependency

Human Readability: a parsed signature is unverified data. Devil's Advocate: two packets can carry copied signatures; framing alone does not establish trust. Technical Destruction: tests detect malformed framing and changed transcript, while actual signature misuse awaits a real cryptographic integration test. Next candidate: verify the exact v2 transcript with the reviewed Ed25519 provider, configured peer identity, adversarial signature vectors and explicit v1/v2 rejection; then semantics, persistent freshness and transaction integration. All existing project debt and physical gates remain open.

## 07A continuation after the documentation merge and branch transition

Current execution baseline: HEAD 289eb429ef998af63e82946e300aab425bc7d485, branch feature/m17-authenticated-ingress-integration, 579 tracked/nonignored paths after 06Z. The earlier 575-path baseline above describes the original candidate preparation; it is retained as history, not the current execution lock.

The three C candidate files retain their original byte identities. Linux GCC strict O0/O2 and UBSan were re-executed for this candidate; each normal execution reports 31,337 CHECK evaluations. Four explicitly selected mutation cases were compiled separately and rejected. The new local-validation.json records this run. These are codec/transcript results, not Ed25519 verification or interoperability evidence. Windows O0/O2 results remain pending until the 07A ZIP is reviewed.

Proposed admission paths, not written by this runner: firmware/NodeLink/Inc/guardian_signed_envelope.h; firmware/NodeLink/Src/guardian_signed_envelope.c; firmware/Tests/m17_signed_envelope_host_test.c; docs/m17/04-architecture/ADR-M17-Signed-Envelope-Candidate.md. The packet remains an UNVERIFIED representation. No existing parser, enum, signature provider, authorization policy or CI workflow is changed.

Next dependency: connect the exact v2 transcript to a real Ed25519 provider with configured peer identity/key and hostile signature tests. Semantic compatibility, durable freshness, one-use ACK matching and transactional corroboration publication follow as separately evidenced integrations. An ML-DSA host artifact-signing experiment does not replace this 64-byte Ed25519 wire candidate. All physical and production gates remain open.
