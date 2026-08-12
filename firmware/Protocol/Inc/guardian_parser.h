#ifndef GUARDIAN_PARSER_H
#define GUARDIAN_PARSER_H

/* Include the canonical frame and protocol result declarations. */
#include "guardian_protocol.h"

/* Define stream parser outcomes independently from complete-frame codec errors. */
typedef enum
{
    /* Indicate that more transport bytes are required. */
    GUARDIAN_PARSER_NO_FRAME = 0,

    /* Indicate that one validated frame was produced. */
    GUARDIAN_PARSER_FRAME_READY = 1,

    /* Indicate that a complete candidate failed CRC validation. */
    GUARDIAN_PARSER_ERROR_CRC = -1,

    /* Indicate that a complete candidate violated another protocol rule. */
    GUARDIAN_PARSER_ERROR_PROTOCOL = -2,

    /* Indicate that a declared payload exceeded the parser bound. */
    GUARDIAN_PARSER_ERROR_OVERSIZE = -3
} guardian_parser_result_t;

/* Define the internal synchronization stages required by the byte parser. */
typedef enum
{
    /* Wait for the first synchronization byte. */
    GUARDIAN_PARSER_WAIT_MAGIC_0 = 0,

    /* Wait for the second synchronization byte while preserving overlap. */
    GUARDIAN_PARSER_WAIT_MAGIC_1,

    /* Collect the remaining fixed header, payload and CRC bytes. */
    GUARDIAN_PARSER_READ_FRAME
} guardian_parser_state_t;

/* Store parser state and diagnostics without using dynamic allocation. */
typedef struct
{
    /* Store the current synchronization/collection state. */
    guardian_parser_state_t state;

    /* Store the bounded candidate frame bytes. */
    uint8_t buffer[GUARDIAN_MAX_FRAME_SIZE];

    /* Store the number of currently collected bytes. */
    size_t index;

    /* Store the complete frame size once the fixed header is available. */
    size_t expected_size;

    /* Count validated frames returned to the application. */
    uint32_t frames_received;

    /* Count candidates rejected because their CRC was incorrect. */
    uint32_t crc_errors;

    /* Count candidates rejected by another complete-frame protocol rule. */
    uint32_t protocol_errors;

    /* Count headers declaring payloads above the compile-time bound. */
    uint32_t oversize_errors;

    /* Preserve the latest complete-frame decoder result for diagnostics. */
    guardian_protocol_result_t last_protocol_result;
} guardian_parser_t;

/* Initialize every parser field to a deterministic idle state. */
void guardian_parser_init(guardian_parser_t *parser);

/* Discard only incomplete stream state while preserving diagnostic counters. */
void guardian_parser_reset_stream(guardian_parser_t *parser);

/* Push one transport byte and optionally produce one validated frame. */
guardian_parser_result_t guardian_parser_push_byte(
    guardian_parser_t *parser,
    uint8_t byte,
    guardian_frame_t *frame);

#endif
