# M16 NodeLink Protocol v0.1

Status: CANDIDATE

## Purpose

Guardian NodeLink is the bounded node-to-node supervision protocol introduced
for the M16 heterogeneous Guardian pair.

It is independent of Guardian Protocol v0.1.

## Wire format

All multi-byte integer fields are unsigned big-endian.

Fixed header:

| Offset | Size | Field |
|---:|---:|---|
| 0 | 1 | magic_0 = 0x47 ('G') |
| 1 | 1 | magic_1 = 0x4E ('N') |
| 2 | 1 | protocol_version = 0x01 |
| 3 | 1 | message_type |
| 4 | 1 | flags = 0x00 |
| 5 | 1 | node_state |
| 6 | 2 | payload_length |
| 8 | 4 | sequence |
| 12 | 4 | sender_epoch |
| 16 | 4 | sender_node_id |

Header size: 20 bytes.

Payload bound: 64 bytes.

Trailer: IEEE CRC32 over header + payload, encoded as 4-byte big-endian.

Maximum frame: 88 bytes.

## sender_node_id

`sender_node_id` is a transport-level numeric node identifier.

It is not cryptographic proof of identity.

It must not be interpreted as a replacement for M15 `producer_id`, `key_id`,
or a future trusted credential binding.

## Message types

- 0x01 HELLO
- 0x02 CHALLENGE
- 0x03 RESPONSE
- 0x04 HEARTBEAT
- 0x05 HEALTH
- 0x06 SUPERVISION_STATE
- 0x7F ERROR

Unknown values are rejected.

## States

- 0x00 BOOT
- 0x01 DISCOVERING
- 0x02 ACTIVE
- 0x03 DEGRADED
- 0x04 SAFE_HOLD
- 0x05 FAULT

Unknown values are rejected.

## Sequence

Sequence zero is reserved and rejected.

The Block 0 helper accepts only a sequence greater than the previous accepted
sequence within one initialized runtime session.

No wraparound acceptance is defined.

This is deliberately conservative.

## Integrity versus authentication

CRC32 detects accidental frame corruption.

CRC32 is not authentication.

Block 0 does not claim cryptographic node authentication.

Future M16 work may bind the exact accepted NodeLink transcript to the M15
signing provider boundary without changing this distinction:

CRC-valid != authenticated != fresh != truthful != authorized

## Authority

No NodeLink message directly authorizes physical actuation.

A SUPERVISION_STATE message communicates a remote supervisory observation or
recommendation only.

Local deterministic policy remains responsible for any authority decision.

## Initial validation targets

Host validation shall cover at least:

- encode/decode round trip;
- maximum payload;
- malformed magic;
- unsupported version;
- unsupported flags;
- unknown message type;
- invalid node state;
- zero sequence;
- oversized payload;
- length mismatch;
- CRC corruption;
- strict sequence duplicate rejection;
- strict sequence regression rejection.
