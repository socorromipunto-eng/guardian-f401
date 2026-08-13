# Guardian Protocol Command Registry

## Version

Guardian Protocol `0.1`.

## Commands

| ID | Name | Request payload | Successful response |
|---:|---|---|---|
| `0x01` | `PING` | Empty | ASCII `PONG` |
| `0x02` | `DEVICE_INFO` | Empty | Binary DeviceInfo schema v1 |
| `0x10` | `GET_STATUS` | Empty | Binary DeviceStatus schema v1 |
| `0x11` | `GET_DSP_FEATURES` | Empty | Binary DspFeatures schema v1 |
| `0x12` | `GET_HEALTH_STATUS` | Empty | Binary HealthStatus schema v1 |
| `0x13` | `BASELINE_CONTROL` | Binary BaselineControl schema v1 | Normalized BaselineControl schema v1 |
| `0x14` | `GET_CONTROL_STATUS` | Empty | Binary ControlStatus schema v1 |
| `0x15` | `CONTROL_COMMAND` | Binary ControlCommand schema v1 | Binary ControlCommandResult schema v1 |
| `0x30` | `AUTH_BEGIN` | Binary AuthBegin schema v1 | Binary AuthChallenge schema v1 |
| `0x31` | `AUTH_FINISH` | Binary AuthFinish schema v1 | Binary AuthenticatedSession schema v1 |
| `0x32` | `GET_SECURITY_STATUS` | Empty | Binary SecurityStatus schema v1 |
| `0x33` | `SECURE_COMMAND` | Authenticated secure envelope | Authenticated secure envelope |
| `0x20` | `SET_TELEMETRY` | Binary TelemetryConfig schema v1 | Normalized TelemetryConfig schema v1 |
| `0x21` | `MACHINE_TELEMETRY` | Not a request command | Asynchronous TELEMETRY payload schema v1 |

## DEVICE_INFO Response Payload

All multi-byte integers use big-endian order.

```text
Offset  Size  Field
------  ----  ----------------
0       1     SCHEMA_VERSION = 1
1       1     FIRMWARE_MAJOR
2       1     FIRMWARE_MINOR
3       1     FIRMWARE_PATCH
4       4     DEVICE_ID
8       1     MODEL_LENGTH
9       N     MODEL_UTF8
```

`MODEL_LENGTH` must be between 1 and 32 bytes.

## GET_STATUS Response Payload

```text
Offset  Size  Field
------  ----  ----------------
0       1     SCHEMA_VERSION = 1
1       1     DEVICE_STATE
2       4     UPTIME_SECONDS
6       4     RX_FRAMES
10      4     TX_FRAMES
14      4     PROTOCOL_ERRORS
18      1     LAST_ERROR
```

Device states:

| ID | Name |
|---:|---|
| `0x00` | `BOOT` |
| `0x01` | `IDLE` |
| `0x02` | `RUNNING` |
| `0x03` | `DEGRADED` |
| `0x04` | `FAULT` |

## Error Codes

An `ERROR` frame uses the original command identifier and request sequence.

The first payload byte contains the error code.

| ID | Name |
|---:|---|
| `0x01` | `MALFORMED_FRAME` |
| `0x02` | `UNSUPPORTED_VERSION` |
| `0x03` | `UNSUPPORTED_MESSAGE_TYPE` |
| `0x04` | `UNSUPPORTED_FLAGS` |
| `0x05` | `UNKNOWN_COMMAND` |
| `0x06` | `INVALID_PAYLOAD` |
| `0x07` | `INTERNAL_ERROR` |
| `0x08` | `BUSY` |
| `0x09` | `UNAUTHORIZED` |
| `0x0A` | `REPLAY_DETECTED` |

`UNAUTHORIZED` and `REPLAY_DETECTED` remain reserved for the later security milestone.


## SET_TELEMETRY Request and Response Payload

```text
Offset  Size  Field
------  ----  ----------------
0       1     SCHEMA_VERSION = 1
1       1     ENABLED
2       2     PERIOD_MS
```

`ENABLED` must be `0` or `1`.

`PERIOD_MS` uses big-endian order and must be between `100` and `60000`.

The successful response echoes the normalized active configuration.

## MACHINE_TELEMETRY Payload

This identifier is used with `message_type = TELEMETRY`.

```text
Offset  Size  Field
------  ----  -------------------------
0       1     SCHEMA_VERSION = 1
1       1     DEVICE_STATE
2       4     TIMESTAMP_MS
6       2     TEMPERATURE_CENTI_C (i16)
8       2     VIBRATION_MG_RMS
10      2     CURRENT_MA
12      2     RPM
14      2     SUPPLY_MV
16      2     STATUS_FLAGS
```

The frame `SEQUENCE` field is an independent non-zero telemetry sequence.

All multi-byte values use big-endian order.


## GET_DSP_FEATURES Payload

The response payload is fixed at 32 bytes.

```text
Offset  Size  Field
0       1     SCHEMA_VERSION = 1
1       1     RESERVED = 0
2       4     BLOCK_SEQUENCE
6       2     SAMPLE_RATE_HZ
8       2     RMS_MG
10      2     PEAK_MG
12      2     CREST_FACTOR_MILLI
14      4     DOMINANT_FREQUENCY_CENTI_HZ
18      2     DOMINANT_PEAK_MG
20      4     SPECTRAL_CENTROID_CENTI_HZ
24      2     LOW_BAND_PERMILLE
26      2     MID_BAND_PERMILLE
28      2     HIGH_BAND_PERMILLE
30      2     ACQUISITION_STATUS_FLAGS
```


## BASELINE_CONTROL Payload

The request and successful response are fixed at 4 bytes.

```text
Offset  Size  Field
------  ----  ----------------
0       1     SCHEMA_VERSION = 1
1       1     ACTION
2       2     TARGET_SAMPLES
```

Actions:

```text
1 START
2 RESET
```

`START` requires `TARGET_SAMPLES` between `16` and `1024`.

`RESET` requires `TARGET_SAMPLES = 0`.

The baseline exists only in runtime RAM in M8.

## GET_HEALTH_STATUS Payload

The successful response is fixed at 36 bytes.

```text
Offset  Size  Field
------  ----  --------------------------------
0       1     SCHEMA_VERSION = 1
1       1     HEALTH_STATE
2       2     BASELINE_SAMPLES
4       2     BASELINE_TARGET
6       2     ANOMALY_SCORE
8       2     HEALTH_SCORE
10      2     MAX_DEVIATION_MILLI
12      1     DOMINANT_FEATURE
13      1     CONSECUTIVE_ANOMALOUS
14      2     QUALITY_FLAGS
16      4     BLOCK_SEQUENCE
20      2     CURRENT_RMS_MG
22      2     CURRENT_CREST_FACTOR_MILLI
24      4     CURRENT_DOMINANT_FREQUENCY_CENTI_HZ
28      2     BASELINE_RMS_MEAN_MG
30      2     BASELINE_RMS_STD_MG
32      2     EXCEEDED_FEATURE_MASK
34      2     REJECTED_INPUTS
```

Health states:

```text
0 UNTRAINED
1 LEARNING
2 READY
3 WARNING
4 ALARM
```

The M8 anomaly score is a deterministic statistical distance from an explicitly learned baseline. It is not a safety-certified diagnostic or a probability of failure.


## M9 CONTROL_COMMAND

The request is fixed at 2 bytes:

```text
Offset  Size  Field
------  ----  ----------------
0       1     SCHEMA_VERSION = 1
1       1     ACTION
```

Actions:

```text
1 ARM
2 DISARM
3 CLEAR_FAULT
```

The host protocol deliberately has **no RUN/START action in M9**.

`ARM` only enables supervisory evaluation. It requires:

```text
M8 health state == READY
local run request == false
local interlock == closed
safe-output adapter == available
no latched control fault
```

A successful `ARM` response therefore returns `run_permit = 0`.

Machine run permit can become active only after local firmware/application input later asserts the local run request.

The successful response is fixed at 4 bytes:

```text
Offset  Size  Field
------  ----  ----------------
0       1     SCHEMA_VERSION = 1
1       1     ACTION
2       1     RESULTING_CONTROL_STATE
3       1     RUN_PERMIT
```

## M9 GET_CONTROL_STATUS

The successful response is fixed at 28 bytes:

```text
Offset  Size  Field
------  ----  ---------------------------
0       1     SCHEMA_VERSION = 1
1       1     CONTROL_STATE
2       1     SUPERVISION_ENABLED
3       1     LOCAL_RUN_REQUEST
4       1     RUN_PERMIT
5       1     INTERLOCK_CLOSED
6       1     HEALTH_STATE
7       1     OUTPUT_AVAILABLE
8       2     LATCHED_FAULTS
10      2     ACTIVE_FAULTS
12      2     HEALTH_SCORE
14      2     ANOMALY_SCORE
16      4     TRANSITION_COUNT
20      4     FAULT_LATCH_COUNT
24      1     LAST_TRANSITION_REASON
25      1     RESERVED = 0
26      2     RESERVED = 0
```

Control states:

```text
0 DISABLED
1 ARMED
2 ACTIVE
3 DEGRADED
4 FAULT_LATCHED
```

Fault bits:

```text
0x0001 HEALTH_NOT_READY
0x0002 HEALTH_ALARM
0x0004 INTERLOCK_OPEN
0x0008 OUTPUT_FAILURE
0x0010 OUTPUT_UNAVAILABLE
```


## M10 Authenticated Sessions

M10 preserves the original Guardian frame format.

Security is carried inside command payloads.

The M10 reference profile uses:

```text
PSK:              32 bytes / 256 bits
Hash:             SHA-256
MAC:              HMAC-SHA-256
Transmitted tag:  first 16 bytes / 128 bits
Client nonce:     16 bytes
Device nonce:     16 bytes
Session counter:  unsigned 64-bit
Byte order:       big-endian
```

The PSK is never transmitted.

### AUTH_BEGIN request

```text
Offset  Size  Field
------  ----  ----------------
0       1     SCHEMA_VERSION = 1
1       1     REQUESTED_ROLE
2       16    CLIENT_NONCE
```

Roles:

```text
0 NONE
1 OBSERVER
2 OPERATOR
3 ADMIN
```

### AUTH_BEGIN response

```text
Offset  Size  Field
------  ----  ----------------
0       1     SCHEMA_VERSION = 1
1       1     GRANTED_ROLE
2       4     SESSION_ID
6       16    DEVICE_NONCE
22      16    SERVER_PROOF
```

`SERVER_PROOF` is the first 16 bytes of HMAC-SHA-256 over the domain-separated handshake transcript.

The client verifies this proof before sending its own proof.

### AUTH_FINISH request

```text
Offset  Size  Field
------  ----  ----------------
0       1     SCHEMA_VERSION = 1
1       1     ROLE
2       4     SESSION_ID
6       16    CLIENT_PROOF
```

### AUTH_FINISH response

```text
Offset  Size  Field
------  ----  ----------------
0       1     SCHEMA_VERSION = 1
1       1     ROLE
2       4     SESSION_ID
6       2     SESSION_TIMEOUT_SECONDS
```

A per-session 256-bit HMAC key is derived from the PSK, role, session identifier and both nonces using a separate HMAC domain label.

### SECURE_COMMAND request

```text
Offset  Size  Field
------  ----  ----------------
0       1     SCHEMA_VERSION = 1
1       4     SESSION_ID
5       8     COUNTER
13      1     INNER_COMMAND
14      2     INNER_PAYLOAD_LENGTH
16      N     INNER_PAYLOAD
16+N    16    TAG
```

The request tag authenticates:

```text
domain label
schema
session id
counter
outer Guardian sequence
inner command
inner payload length
inner payload
```

M10 uses a strict synchronous anti-replay policy:

```text
first counter = 1
accepted counter = exactly next_counter
duplicate counter -> REPLAY_DETECTED
skipped counter -> REPLAY_DETECTED
```

If delivery becomes ambiguous after a transport failure, the host establishes a fresh session rather than guessing the device counter.

### SECURE_COMMAND response

```text
Offset  Size  Field
------  ----  ----------------
0       1     SCHEMA_VERSION = 1
1       4     SESSION_ID
5       8     COUNTER
13      1     INNER_MESSAGE_TYPE
14      1     INNER_COMMAND
15      2     INNER_PAYLOAD_LENGTH
17      N     INNER_PAYLOAD
17+N    16    TAG
```

Application errors such as `BUSY` or `UNAUTHORIZED` can therefore be returned inside an authenticated response envelope.

### Protected M10 command policy

The reference protected surface is:

```text
BASELINE_CONTROL -> minimum role OPERATOR
CONTROL_COMMAND  -> minimum role OPERATOR
```

The STM32 application enables direct privileged-command rejection by default.

Read-only status queries remain available without authentication in M10.

Asynchronous telemetry remains CRC-protected rather than HMAC-protected in this milestone.

M10 provides authenticity, authorization and replay protection for privileged commands. It does not provide confidentiality/encryption.
