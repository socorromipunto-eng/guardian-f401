# Guardian Protocol Specification

## Status

Draft 0.1.

## Design Goals

The protocol must be:

- binary
- deterministic
- bounded
- versioned
- transport-independent
- testable
- simple to parse on a constrained MCU

## Initial Frame Concept

```text
+-------+---------+------+-------+----------+--------+---------+-------+
| MAGIC | VERSION | TYPE | FLAGS | SEQUENCE | LENGTH | PAYLOAD | CRC32 |
+-------+---------+------+-------+----------+--------+---------+-------+
```

## Next Decisions

Before implementation, M1 will freeze:

- exact magic bytes
- field widths
- byte order
- maximum payload length
- CRC algorithm
- request/response semantics
- error codes
- sequence-number rules

## Security Note

CRC detects accidental corruption.

CRC is not message authentication.
