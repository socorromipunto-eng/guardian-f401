# Guardian M17 — Post-quantum artifact signing candidate

Status: EXTERNAL_CANDIDATE_NOT_ADMITTED. Baseline: HEAD 717e53c717af1dc31af540bbcadd9b426f9c9535, feature/m17-heterogeneous-multinode-assurance, 575 paths after 06H and preserved by 06I. 06J execution has not been adjudicated.

## Research boundary

First experiment: detached pure ML-DSA signature over a domain-bound canonical manifest of a research artifact. This is not Windows Authenticode, an X.509 CA deployment, a bootloader or an on-chip verified update. The candidate parameter set is ML-DSA-87; final selection requires provider capabilities and resource review. Existing EdDSA signed release remains historical evidence and is never retagged by this work.

FIPS 204 defines ML-DSA. OpenSSL documents ML-DSA support introduced in 3.5. Version documentation does not prove the local binary's algorithms, integrity, provider configuration or validation. 06K only discovers executable locations and hashes; it executes no OpenSSL commands and reads no private keys or certificates. Zero observed tools is a valid inventory result, not proof none exist anywhere.

## Sources and status

- NIST FIPS 204 (Final, 2024-08-13): https://csrc.nist.gov/pubs/fips/204/final
- OpenSSL 3.5 ML-DSA signature implementation documentation: https://docs.openssl.org/3.5/man7/EVP_SIGNATURE-ML-DSA/
- NIST SP 800-208 (Final): https://csrc.nist.gov/pubs/sp/800/208/final
- NIST IR 8547 initial public draft: https://csrc.nist.gov/pubs/ir/8547/ipd
- NSA CNSA FAQ version 2.1 document landing page: https://www.nsa.gov/Press-Room/Digital-Media-Center/Document-Gallery/igphoto/2003071836/

Sources are research references, not fully acquired/hash-locked normative PDF evidence. Retrieved official search entries support FIPS 204 final status and OpenSSL support in 3.5; direct retrieval failed in this session. No CNSA timetable or complete approval-profile claim is promoted here. Current source identity/version/errata and deployment applicability remain subject to separate acquisition/adjudication.

## Manifest and trust design pending

The next experiment must specify a unique domain, exact canonical encoding, artifact type/hash/length, source/build identity, version, destination hardware, key/profile identity and policy binding. SHA-256 inventory hashes remain integrity tools, not a claim that the complete pipeline satisfies CNSA. The entire chosen manifest is signed; this is not an invented HashML-DSA mode. Policy must reject wrong destination, rollback and unknown keys independently of signature validity. A public key bundled with a release is not independently trusted.

A disposable research-key lifecycle must be authorized and designed before experiments; no production-key installation or ceremony is implied. LMS/XMSS require signer state controls and are alternative firmware paths, not chosen runtime messaging algorithms. NodeLink 06J signature field is classical-only and cannot silently accept larger PQC signatures. Artifact signing can advance independently without rewriting all Guardian code.

## Reviews and evidence limits

Human Readability: signing a file on a workstation is not an authenticated update on the chip. Devil's Advocate: perfect signature verification under an attacker-supplied key accepts an attacker artifact unless trust is independently provisioned. Technical Destruction: the 20 linked rows are planning placeholders, not implemented adversarial tests; concrete expected outcomes and test vectors are required before experiment execution. FIPS algorithm implementation is not FIPS module validation; software license is not certification.

Next boundary: adjudicate observed local tool identities and verify version/provider/ML-DSA capability under a separate read-only probe with locked binaries. Then finalize canonical manifest and disposable-key experiment. No install, network, OpenSSL execution, keys, firmware change, repository admission, staging, commit, push or flash in 06K. No gate or physical debt closed.


## Post-06M evidence update (2026-10-12)

The historical candidate above is preserved. 06M now has same-provider host research evidence for one pure ML-DSA-87 signature, two positive controls, four selected cryptographic negatives and a separate artifact-hash negative. The detailed adjudication is docs/m17/01-audit/m17-pqc-artifact-experiment-06m-adjudication.md and its JSON companion. Crypto verification was not re-executed by that reviewer. No independent implementation validation, production trust, target update or conformance is established. Requirement statuses remain candidate and no gate is closed. 06J Windows evidence remains pending.
