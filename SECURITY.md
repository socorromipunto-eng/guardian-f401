# Security Policy

## Supported status

Guardian F401 is experimental research software. No released version is
currently represented as a certified, production-supported security product.

## Reporting a vulnerability

Do not disclose an unpatched vulnerability through a public issue if doing so
would create avoidable risk. Use the repository owner's private GitHub contact
or security-reporting mechanism when one is published. Never include passwords,
private keys, production credentials, personal data or unsafe physical-control
instructions in a report.

A useful report should contain:

- the affected release or commit;
- the affected component;
- reproducible steps using a safe simulator or isolated test environment;
- expected and observed behavior;
- potential impact;
- suggested mitigation, if known.

Receipt of a report does not guarantee a remediation date. Valid reports will
be evaluated, documented and handled through controlled change management.

## Security boundaries

- CRC32 is an error-detection mechanism, not authentication.
- Repository demo keys and deterministic test material are not production keys.
- The portable firmware-lifecycle model is not a complete physical secure-boot
  or secure-update implementation.
- The STM32F401 target requires a separately engineered root of trust, protected
  key storage, flash ownership, recovery path and production verifier.
- Passing tests or fuzz campaigns does not prove cryptographic correctness,
  memory safety, functional safety or resistance to all attacks.
- Physical deployment requires independent safety and security review.

## Disclosure and release integrity

Published releases should be tagged, archived, hashed and linked to their
evidence record. A release must not be described as hardware validated until a
physical qualification report exists for the exact hardware, firmware and test
configuration.

## Release signing identity

Release-signing primary-key fingerprint:
`E5D6D18B609971D36B96898185C13CE4778382E1`

Author: Antonio Jose Socorro Marin (AJSM).
Release-integrity contact: socorromipunto@gmail.com.
Repository owner: https://github.com/socorromipunto-eng.
Do not send private keys, passwords, credentials or sensitive exploit data
through ordinary email. This contact publishes the author's existing public
identity; it does not promise a private disclosure service or response time.

The v0.16.0 signed tag and detached checksum signature were checked against
this fingerprint. The release public-key asset is a verification input, not
an independent trust anchor. A fingerprint in this repository improves
consistency checking but is not an independent, out-of-band identity proof.
Verify author-key trust through a separately authenticated channel before
accepting a release. A signing-key change requires a documented transition
and independent confirmation; never automatically trust a replacement key
because it accompanies a release.
