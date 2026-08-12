/* Include the public incremental parser declarations. */
#include "guardian_parser.h"

/* Decode a big-endian unsigned 16-bit value needed by the fixed header parser. */
static uint16_t guardian_parser_read_u16_be(const uint8_t *input)
{
    /* Promote and place the most-significant byte. */
    uint16_t high = (uint16_t)((uint16_t)input[0] << 8U);

    /* Promote the least-significant byte. */
    uint16_t low = (uint16_t)input[1];

    /* Combine both bytes into the host value. */
    return (uint16_t)(high | low);
}

/* Return the parser to magic-byte synchronization without changing diagnostics. */
static void guardian_parser_return_to_sync(guardian_parser_t *parser)
{
    /* Wait for the first synchronization byte on the next input byte. */
    parser->state = GUARDIAN_PARSER_WAIT_MAGIC_0;

    /* Forget every byte from the completed or rejected candidate. */
    parser->index = 0U;

    /* Clear the previously calculated complete-frame target size. */
    parser->expected_size = 0U;
}

/* Initialize a parser instance before the first transport byte arrives. */
void guardian_parser_init(guardian_parser_t *parser)
{
    /* Ignore initialization safely when the caller passes a null pointer. */
    if (parser == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Start by searching for the first magic byte. */
    parser->state = GUARDIAN_PARSER_WAIT_MAGIC_0;

    /* Start with an empty candidate buffer. */
    parser->index = 0U;

    /* Start without a known complete-frame target size. */
    parser->expected_size = 0U;

    /* Clear the validated-frame diagnostic counter. */
    parser->frames_received = 0U;

    /* Clear the CRC failure diagnostic counter. */
    parser->crc_errors = 0U;

    /* Clear the generic protocol failure diagnostic counter. */
    parser->protocol_errors = 0U;

    /* Clear the oversized payload diagnostic counter. */
    parser->oversize_errors = 0U;

    /* Initialize the last codec result to success. */
    parser->last_protocol_result = GUARDIAN_PROTOCOL_OK;
}

/* Discard incomplete bytes while preserving lifetime parser diagnostics. */
void guardian_parser_reset_stream(guardian_parser_t *parser)
{
    /* Ignore reset requests safely when the caller passes a null pointer. */
    if (parser == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Restore only the stream synchronization state. */
    guardian_parser_return_to_sync(parser);
}

/* Consume one byte from UART, USB or any equivalent ordered byte transport. */
guardian_parser_result_t guardian_parser_push_byte(
    guardian_parser_t *parser,
    uint8_t byte,
    guardian_frame_t *frame)
{
    /* Store the payload length as soon as the complete fixed header exists. */
    uint16_t payload_length = 0U;

    /* Store the complete-frame decoder result for the finished candidate. */
    guardian_protocol_result_t decode_result = GUARDIAN_PROTOCOL_OK;

    /* Reject missing parser or output-frame storage. */
    if ((parser == NULL) || (frame == NULL))
    {
        /* Report a generic parser protocol failure without dereferencing null memory. */
        return GUARDIAN_PARSER_ERROR_PROTOCOL;
    }

    /* Handle synchronization and frame collection according to the current state. */
    switch (parser->state)
    {
        /* Search for the first Guardian magic byte. */
        case GUARDIAN_PARSER_WAIT_MAGIC_0:

            /* Check whether this byte can begin a new frame. */
            if (byte == GUARDIAN_MAGIC_0)
            {
                /* Preserve the first synchronization byte. */
                parser->buffer[0] = byte;

                /* Record that one candidate byte is now buffered. */
                parser->index = 1U;

                /* Wait specifically for the second magic byte. */
                parser->state = GUARDIAN_PARSER_WAIT_MAGIC_1;
            }

            /* No complete frame can exist after only the first synchronization byte. */
            return GUARDIAN_PARSER_NO_FRAME;

        /* Wait for the second synchronization byte while handling overlapping magic candidates. */
        case GUARDIAN_PARSER_WAIT_MAGIC_1:

            /* Check whether the synchronization sequence is now complete. */
            if (byte == GUARDIAN_MAGIC_1)
            {
                /* Preserve the second synchronization byte. */
                parser->buffer[1] = byte;

                /* Record both buffered synchronization bytes. */
                parser->index = 2U;

                /* Begin collecting the remaining frame bytes. */
                parser->state = GUARDIAN_PARSER_READ_FRAME;
            }
            else if (byte == GUARDIAN_MAGIC_0)
            {
                /* Preserve this byte as a new possible first synchronization byte. */
                parser->buffer[0] = byte;

                /* Keep exactly one synchronization candidate byte buffered. */
                parser->index = 1U;
            }
            else
            {
                /* Return to the initial synchronization state after unrelated input. */
                guardian_parser_return_to_sync(parser);
            }

            /* No complete frame exists while synchronization has only just completed. */
            return GUARDIAN_PARSER_NO_FRAME;

        /* Collect the remainder of the bounded frame candidate. */
        case GUARDIAN_PARSER_READ_FRAME:

            /* Defensively reject an impossible buffer index before writing a byte. */
            if (parser->index >= GUARDIAN_MAX_FRAME_SIZE)
            {
                /* Count the bounded-buffer violation. */
                parser->oversize_errors += 1U;

                /* Return to synchronization without writing outside the fixed buffer. */
                guardian_parser_return_to_sync(parser);

                /* Report the explicit oversize parser outcome. */
                return GUARDIAN_PARSER_ERROR_OVERSIZE;
            }

            /* Append this transport byte to the bounded candidate buffer. */
            parser->buffer[parser->index] = byte;

            /* Advance the number of collected candidate bytes. */
            parser->index += 1U;

            /* Check whether the complete fixed header has just become available. */
            if (parser->index == GUARDIAN_HEADER_SIZE)
            {
                /* Decode the declared payload length directly from its fixed header offset. */
                payload_length =
                    guardian_parser_read_u16_be(
                        &parser->buffer[GUARDIAN_OFFSET_PAYLOAD_LENGTH]);

                /* Reject declared payloads that cannot fit in the fixed frame storage. */
                if ((size_t)payload_length > GUARDIAN_MAX_PAYLOAD_SIZE)
                {
                    /* Count the explicit payload-bound violation. */
                    parser->oversize_errors += 1U;

                    /* Preserve a useful codec-style diagnostic value. */
                    parser->last_protocol_result =
                        GUARDIAN_PROTOCOL_ERROR_PAYLOAD_TOO_LARGE;

                    /* Return to synchronization immediately instead of buffering attacker-selected lengths. */
                    guardian_parser_return_to_sync(parser);

                    /* Report the bounded-length parser outcome. */
                    return GUARDIAN_PARSER_ERROR_OVERSIZE;
                }

                /* Calculate exactly how many bytes complete this candidate. */
                parser->expected_size =
                    GUARDIAN_HEADER_SIZE +
                    (size_t)payload_length +
                    GUARDIAN_CRC_SIZE;
            }

            /* Wait until the declared complete frame candidate has been collected. */
            if ((parser->expected_size == 0U) ||
                (parser->index < parser->expected_size))
            {
                /* Report that additional transport bytes are still required. */
                return GUARDIAN_PARSER_NO_FRAME;
            }

            /* Validate the complete candidate using the canonical frame decoder. */
            decode_result =
                guardian_protocol_decode(
                    parser->buffer,
                    parser->expected_size,
                    frame);

            /* Preserve the detailed decoder result before resetting stream state. */
            parser->last_protocol_result = decode_result;

            /* Return to synchronization regardless of candidate validity. */
            guardian_parser_return_to_sync(parser);

            /* Check whether the frame passed every structural and CRC validation step. */
            if (decode_result == GUARDIAN_PROTOCOL_OK)
            {
                /* Count the frame exposed to the application. */
                parser->frames_received += 1U;

                /* Report that the caller may now consume the output frame. */
                return GUARDIAN_PARSER_FRAME_READY;
            }

            /* Classify CRC failures separately for link diagnostics. */
            if (decode_result == GUARDIAN_PROTOCOL_ERROR_CRC)
            {
                /* Count the CRC-protected corruption event. */
                parser->crc_errors += 1U;

                /* Report the dedicated stream parser CRC outcome. */
                return GUARDIAN_PARSER_ERROR_CRC;
            }

            /* Count every other complete-candidate validation failure. */
            parser->protocol_errors += 1U;

            /* Report a generic complete-frame protocol failure. */
            return GUARDIAN_PARSER_ERROR_PROTOCOL;

        /* Recover safely if parser memory contains an unknown state value. */
        default:

            /* Count the internal state corruption as a protocol-side diagnostic. */
            parser->protocol_errors += 1U;

            /* Restore deterministic synchronization behavior. */
            guardian_parser_return_to_sync(parser);

            /* Report the recovered internal parser error. */
            return GUARDIAN_PARSER_ERROR_PROTOCOL;
    }
}
