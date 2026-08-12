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
