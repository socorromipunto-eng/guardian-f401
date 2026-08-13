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
