# Guardian Protocol Specification

## Status

Version `0.1` is frozen for implementation.

The protocol is intentionally transport-independent. UART, USB and the software simulator may carry the same byte stream.

## Wire Format

All multi-byte integers use **big-endian** byte order.

```text
Offset  Size  Field
------  ----  ----------------
0       2     MAGIC
2       1     VERSION
3       1     MESSAGE_TYPE
4       1     COMMAND
5       1     FLAGS
6       4     SEQUENCE
10      2     PAYLOAD_LENGTH
12      N     PAYLOAD
12+N    4     CRC32
```

The fixed header is 12 bytes.

The CRC field is 4 bytes.

The maximum payload is 256 bytes.

The maximum complete frame is therefore 272 bytes.

## Field Definitions

| Field | Width | v0.1 rule |
|---|---:|---|
| `MAGIC` | 2 bytes | ASCII `GF`, hexadecimal `47 46` |
| `VERSION` | 1 byte | `0x01` |
| `MESSAGE_TYPE` | 1 byte | Defined in the message type registry |
| `COMMAND` | 1 byte | Defined in the command registry |
| `FLAGS` | 1 byte | Must be `0x00` in v0.1 |
| `SEQUENCE` | 4 bytes | Unsigned request correlation identifier |
| `PAYLOAD_LENGTH` | 2 bytes | `0..256` |
| `PAYLOAD` | variable | Command-specific bytes |
| `CRC32` | 4 bytes | IEEE CRC-32 of header + payload |

## CRC32

Guardian Protocol v0.1 uses the reflected IEEE CRC-32 algorithm:

```text
Polynomial:   0xEDB88320
Initial:      0xFFFFFFFF
Final XOR:    0xFFFFFFFF
Input:        MAGIC through the final PAYLOAD byte
CRC encoding: big-endian
```

CRC detects accidental transmission corruption.

CRC is **not** cryptographic authentication.

## Message Types

| Value | Name | Purpose |
|---:|---|---|
| `0x01` | `REQUEST` | Host requests an operation |
| `0x02` | `RESPONSE` | Device returns a successful response |
| `0x03` | `EVENT` | Device reports an asynchronous event |
| `0x04` | `ERROR` | Device reports a protocol/application error |
| `0x05` | `TELEMETRY` | Device publishes asynchronous measurements |

## Sequence Rules

A host chooses the `SEQUENCE` value for each request.

A `RESPONSE` or `ERROR` associated with that request copies the same sequence value.

Sequence numbers are correlation identifiers in v0.1.

Cryptographic anti-replay semantics are reserved for the later security milestone.

## Parser Requirements

A conforming parser must:

- search for the `GF` magic bytes;
- accept fragmented input;
- accept multiple frames in one input block;
- reject payload lengths greater than 256 bytes;
- reject unsupported versions;
- reject unsupported message types;
- reject non-zero flags in v0.1;
- reject CRC mismatches;
- recover and search for the next valid magic sequence;
- avoid dynamic memory allocation in the embedded implementation.

## Canonical Test Vectors

### PING request

```text
Message type: REQUEST
Command:      PING
Flags:        0
Sequence:     1
Payload:      empty
```

Encoded frame:

```text
47 46 01 01 01 00 00 00 00 01 00 00 34 02 5E 68
```

Compact hexadecimal:

```text
47460101010000000001000034025e68
```

### PONG response

```text
Message type: RESPONSE
Command:      PING
Flags:        0
Sequence:     1
Payload:      ASCII "PONG"
```

Encoded frame:

```text
47 46 01 02 01 00 00 00 00 01 00 04 50 4F 4E 47 D4 B5 2C 94
```

Compact hexadecimal:

```text
474601020100000000010004504f4e47d4b52c94
```

## Security Boundary

Frame decoding proves only that a frame is structurally valid and has an intact CRC.

It does not prove:

- sender identity;
- authorization;
- freshness;
- confidentiality.

Those properties belong to the later security layer.
