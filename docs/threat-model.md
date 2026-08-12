# Threat Model

## Status

Initial design draft.

No production-security claims are made at this stage.

## Trust Boundary

```text
Untrusted Transport
        |
        v
Protocol Parser
        |
        v
Message Validation
        |
        v
Security Policy
        |
        v
Application State Machine
        |
        v
Hardware
```

## Threats To Address

- malformed packets
- invalid lengths
- oversized payloads
- parser desynchronization
- corrupted frames
- unauthorized commands
- replayed commands
- invalid state transitions
- buffer exhaustion
- firmware modification
- firmware rollback
- credential exposure

## Fundamental Principle

Transport connectivity never implies authorization.
