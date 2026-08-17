# M14 assurance software-slice architecture

## Purpose and scope

This document describes only the software component implemented on
`feature/m14-assurance-first-slice`. It does not describe an implemented
distributed controller, hardware root of trust or physical failover system.

## Processing boundary

```text
Untrusted UTF-8 bytes
        |
        v
Raw-size gate -> strict JSON parser -> duplicate-member rejection
        |                                  |
        +----------------+-----------------+
                         v
                 Structural limits
                         |
                         v
          Closed envelope and domain binding
                         |
                         v
       Object-type-specific closed payload schema
                         |
                         v
             RFC 8785 canonical JSON bytes
                         |
                         v
            Bounded software result/evidence
```

No arrow in this diagram represents physical authority.

## Envelope invariants

The envelope is closed: missing, additional or incorrectly typed members are
rejected. Domain binding prevents an accepted object type from being evaluated
under a different domain. Duplicate JSON members, including escaped-equivalent
member names, are rejected before schema validation.

## Closed payload contracts

The implemented object types are observation, decision and witness. Each uses
an exact versioned schema and an explicit result enumeration. Arrays have
cardinality and uniqueness constraints. Identifiers are intentionally limited
to a stable ASCII subset; evidence and object identifiers use lowercase
hexadecimal representations where specified.

## Bounded structural model

- Root JSON value depth: 1.
- Maximum value depth: 16.
- Maximum value nodes: 2,048.
- Maximum members per object: 128.
- Maximum items per array: 256.
- Maximum decoded UTF-8 bytes per key or string: 4,096.
- Maximum raw input: 65,536 bytes.

Object member names are constrained strings but do not count as JSON value
nodes and do not independently increase value depth.

These values are software research bounds. They are not demonstrated embedded
capacity or real-time limits.

## Numeric model

Only JSON integers from `-9007199254740991` through `9007199254740991` are
accepted. Booleans are not integers. Fractions, exponent forms, negative zero
and values outside the safe range are rejected by the accepted contract.

## Dependency and CI boundary

RFC 8785 canonicalization uses `rfc8785==0.1.4`, locked by version and hash.
The dedicated workflow uses read-only repository permission, immutable action
commit references, CPython 3.12 and an exact 87-test gate.

## Relationship to future M14 architecture

This component can become a message-validation boundary for future primary,
secondary and independent Guardian nodes. That integration is
`PENDING_VALIDATION`. Node identities, signatures, freshness, quorum, network
partition behavior, failover and recovery are not implemented by this slice.
