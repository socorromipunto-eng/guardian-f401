# M1 — Guardian Protocol v0.1

## Objective

M1 freezes and implements the first transport-independent communication contract shared by the STM32 firmware and host tools.

## Completed

- fixed 12-byte header;
- 256-byte maximum payload;
- big-endian multi-byte fields;
- IEEE CRC32;
- request/response sequence correlation;
- canonical PING/PONG vectors;
- Python encoder and decoder;
- Python incremental parser;
- portable C encoder and decoder;
- portable C byte-at-a-time parser;
- bounded embedded memory usage;
- Python unit tests;
- C host tests;
- GitHub Actions verification.

## Important Boundary

M1 provides framing and accidental-corruption detection.

M1 does **not** provide cryptographic security.

Authentication, authorization, anti-replay semantics and key management remain later milestones.

## Next Vertical Slice

M2 will add a software device that responds to:

```text
PING        -> PONG
DEVICE_INFO -> device metadata
GET_STATUS  -> runtime state
```

M3 will add the `guardianctl` command-line host.
