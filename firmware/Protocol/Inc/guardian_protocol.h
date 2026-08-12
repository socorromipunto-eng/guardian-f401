#ifndef GUARDIAN_PROTOCOL_H
#define GUARDIAN_PROTOCOL_H

/* Include size_t for bounded buffer APIs. */
#include <stddef.h>

/* Include fixed-width integer types for deterministic wire fields. */
#include <stdint.h>

/* Define the first protocol synchronization byte. */
#define GUARDIAN_MAGIC_0 ((uint8_t)0x47U)

/* Define the second protocol synchronization byte. */
#define GUARDIAN_MAGIC_1 ((uint8_t)0x46U)

/* Define the Guardian Protocol version implemented by this library. */
#define GUARDIAN_PROTOCOL_VERSION ((uint8_t)0x01U)

/* Define the fixed header size in bytes. */
#define GUARDIAN_HEADER_SIZE ((size_t)12U)

/* Define the encoded CRC32 trailer size in bytes. */
#define GUARDIAN_CRC_SIZE ((size_t)4U)

/* Bound payload storage so external input cannot request dynamic allocation. */
#define GUARDIAN_MAX_PAYLOAD_SIZE ((size_t)256U)

/* Define the maximum complete encoded frame size. */
#define GUARDIAN_MAX_FRAME_SIZE \
    (GUARDIAN_HEADER_SIZE + GUARDIAN_MAX_PAYLOAD_SIZE + GUARDIAN_CRC_SIZE)

/* Define the only flags value accepted by protocol version 0.1. */
#define GUARDIAN_SUPPORTED_FLAGS ((uint8_t)0x00U)

/* Identify byte offsets inside the fixed wire header. */
enum
{
    /* Identify the first synchronization byte offset. */
    GUARDIAN_OFFSET_MAGIC_0 = 0,

    /* Identify the second synchronization byte offset. */
    GUARDIAN_OFFSET_MAGIC_1 = 1,

    /* Identify the protocol version byte offset. */
    GUARDIAN_OFFSET_VERSION = 2,

    /* Identify the message type byte offset. */
    GUARDIAN_OFFSET_MESSAGE_TYPE = 3,

    /* Identify the command byte offset. */
    GUARDIAN_OFFSET_COMMAND = 4,

    /* Identify the flags byte offset. */
    GUARDIAN_OFFSET_FLAGS = 5,

    /* Identify the first sequence-number byte offset. */
    GUARDIAN_OFFSET_SEQUENCE = 6,

    /* Identify the first payload-length byte offset. */
    GUARDIAN_OFFSET_PAYLOAD_LENGTH = 10
};

/* Define every message type accepted by Guardian Protocol v0.1. */
typedef enum
{
    /* Identify a host-to-device request. */
    GUARDIAN_MESSAGE_REQUEST = 0x01,

    /* Identify a successful device response. */
    GUARDIAN_MESSAGE_RESPONSE = 0x02,

    /* Identify an asynchronous device event. */
    GUARDIAN_MESSAGE_EVENT = 0x03,

    /* Identify an error frame. */
    GUARDIAN_MESSAGE_ERROR = 0x04,

    /* Identify an asynchronous telemetry frame. */
    GUARDIAN_MESSAGE_TELEMETRY = 0x05
} guardian_message_type_t;

/* Define command identifiers published by the protocol registry. */
typedef enum
{
    /* Verify communication with the remote endpoint. */
    GUARDIAN_COMMAND_PING = 0x01,

    /* Request immutable device and firmware metadata. */
    GUARDIAN_COMMAND_DEVICE_INFO = 0x02,

    /* Request the current runtime system status. */
    GUARDIAN_COMMAND_GET_STATUS = 0x10,

    /* Request the latest analyzed DSP and spectral feature snapshot. */
    GUARDIAN_COMMAND_GET_DSP_FEATURES = 0x11,

    /* Configure asynchronous machine telemetry streaming. */
    GUARDIAN_COMMAND_SET_TELEMETRY = 0x20,

    /* Identify asynchronous machine telemetry frames. */
    GUARDIAN_COMMAND_MACHINE_TELEMETRY = 0x21
} guardian_command_t;

/* Define application and protocol error identifiers carried in ERROR payloads. */
typedef enum
{
    /* Report a frame that cannot be interpreted safely. */
    GUARDIAN_ERROR_MALFORMED_FRAME = 0x01,

    /* Report an unsupported protocol version. */
    GUARDIAN_ERROR_UNSUPPORTED_VERSION = 0x02,

    /* Report an unsupported message type. */
    GUARDIAN_ERROR_UNSUPPORTED_MESSAGE_TYPE = 0x03,

    /* Report unsupported flag bits. */
    GUARDIAN_ERROR_UNSUPPORTED_FLAGS = 0x04,

    /* Report an unknown command identifier. */
    GUARDIAN_ERROR_UNKNOWN_COMMAND = 0x05,

    /* Report an invalid command-specific payload. */
    GUARDIAN_ERROR_INVALID_PAYLOAD = 0x06,

    /* Report an unexpected internal failure. */
    GUARDIAN_ERROR_INTERNAL = 0x07,

    /* Report a valid operation that cannot execute now. */
    GUARDIAN_ERROR_BUSY = 0x08,

    /* Reserve authorization failure for the security milestone. */
    GUARDIAN_ERROR_UNAUTHORIZED = 0x09,

    /* Reserve replay detection for the security milestone. */
    GUARDIAN_ERROR_REPLAY_DETECTED = 0x0A
} guardian_error_code_t;

/* Define deterministic codec outcomes for callers and diagnostics. */
typedef enum
{
    /* Indicate successful validation or encoding. */
    GUARDIAN_PROTOCOL_OK = 0,

    /* Indicate a null pointer passed to a required API parameter. */
    GUARDIAN_PROTOCOL_ERROR_NULL_ARGUMENT,

    /* Indicate an output buffer that cannot contain the encoded frame. */
    GUARDIAN_PROTOCOL_ERROR_OUTPUT_TOO_SMALL,

    /* Indicate an input shorter than the smallest legal frame. */
    GUARDIAN_PROTOCOL_ERROR_FRAME_TOO_SHORT,

    /* Indicate invalid synchronization bytes. */
    GUARDIAN_PROTOCOL_ERROR_MAGIC,

    /* Indicate an unsupported protocol version. */
    GUARDIAN_PROTOCOL_ERROR_VERSION,

    /* Indicate an unsupported message type. */
    GUARDIAN_PROTOCOL_ERROR_MESSAGE_TYPE,

    /* Indicate unsupported v0.1 flags. */
    GUARDIAN_PROTOCOL_ERROR_FLAGS,

    /* Indicate a payload exceeding the compile-time bound. */
    GUARDIAN_PROTOCOL_ERROR_PAYLOAD_TOO_LARGE,

    /* Indicate disagreement between encoded length and actual input size. */
    GUARDIAN_PROTOCOL_ERROR_LENGTH_MISMATCH,

    /* Indicate a CRC mismatch. */
    GUARDIAN_PROTOCOL_ERROR_CRC
} guardian_protocol_result_t;

/* Store one validated protocol frame without dynamic allocation. */
typedef struct
{
    /* Store the validated wire-level message class. */
    guardian_message_type_t message_type;

    /* Store the command identifier. */
    uint8_t command;

    /* Store the v0.1 flags byte. */
    uint8_t flags;

    /* Store the request correlation sequence number. */
    uint32_t sequence;

    /* Store the number of meaningful bytes in payload. */
    uint16_t payload_length;

    /* Store bounded command-specific payload bytes inline. */
    uint8_t payload[GUARDIAN_MAX_PAYLOAD_SIZE];
} guardian_frame_t;

/* Calculate the Guardian Protocol IEEE CRC32 value for a byte block. */
uint32_t guardian_crc32(const uint8_t *data, size_t length);

/* Encode one validated in-memory frame into wire bytes. */
guardian_protocol_result_t guardian_protocol_encode(
    const guardian_frame_t *frame,
    uint8_t *output,
    size_t output_capacity,
    size_t *output_size);

/* Decode and validate exactly one complete wire frame. */
guardian_protocol_result_t guardian_protocol_decode(
    const uint8_t *input,
    size_t input_size,
    guardian_frame_t *frame);

/* Return a stable diagnostic string for one codec result. */
const char *guardian_protocol_result_string(guardian_protocol_result_t result);

#endif
