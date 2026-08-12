/* Include the embedded middleware declarations. */
#include "guardian_embedded_link.h"

/* Include memory initialization support. */
#include <string.h>

/* Saturating-increment one diagnostic counter. */
static void guardian_embedded_increment_u32(uint32_t *value)
{
    /* Ignore a missing pointer. */
    if (value == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Avoid diagnostic wrap. */
    if (*value != 0xFFFFFFFFUL)
    {
        /* Increment while representable. */
        *value += 1U;
    }
}

/* Build a coherent runtime snapshot. */
static guardian_device_runtime_t guardian_embedded_runtime(
    const guardian_embedded_link_t *link)
{
    /* Create deterministic runtime storage. */
    guardian_device_runtime_t runtime = {0};

    /* Publish current state. */
    runtime.state = link->state;

    /* Publish platform uptime. */
    runtime.uptime_seconds =
        link->io.uptime_seconds(link->io.context);

    /* Publish RX count. */
    runtime.rx_frames = link->stats.rx_frames;

    /* Publish TX count before the current response. */
    runtime.tx_frames = link->stats.tx_frames;

    /* Publish protocol error count. */
    runtime.protocol_errors = link->stats.protocol_errors;

    /* Publish latest error. */
    runtime.last_error = link->stats.last_error;

    /* Return the snapshot. */
    return runtime;
}

/* Initialize the complete transport-independent embedded communication path. */
guardian_protocol_result_t guardian_embedded_link_init(
    guardian_embedded_link_t *link,
    const guardian_embedded_io_t *io,
    const guardian_device_identity_t *identity)
{
    /* Store device-service initialization status. */
    guardian_protocol_result_t result = GUARDIAN_PROTOCOL_OK;

    /* Reject missing storage. */
    if ((link == NULL) || (io == NULL) || (identity == NULL))
    {
        /* Report the canonical missing-argument error. */
        return GUARDIAN_PROTOCOL_ERROR_NULL_ARGUMENT;
    }

    /* Reject missing callbacks. */
    if ((io->read_byte == NULL) ||
        (io->write == NULL) ||
        (io->uptime_seconds == NULL))
    {
        /* Report the canonical missing-argument error. */
        return GUARDIAN_PROTOCOL_ERROR_NULL_ARGUMENT;
    }

    /* Clear middleware state. */
    (void)memset(link, 0, sizeof(*link));

    /* Copy platform callbacks. */
    link->io = *io;

    /* Start in IDLE. */
    link->state = GUARDIAN_DEVICE_STATE_IDLE;

    /* Initialize parser state. */
    guardian_parser_init(&link->parser);

    /* Initialize public identity. */
    result =
        guardian_device_service_init(
            &link->service,
            identity);

    /* Return initialization status. */
    return result;
}

/* Change the application state exposed by GET_STATUS. */
void guardian_embedded_link_set_state(
    guardian_embedded_link_t *link,
    guardian_device_state_t state)
{
    /* Ignore a missing pointer. */
    if (link == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Store the new state. */
    link->state = state;
}

/* Process bounded foreground protocol work. */
void guardian_embedded_link_poll(
    guardian_embedded_link_t *link,
    size_t rx_budget)
{
    /* Store one received byte. */
    uint8_t byte = 0U;

    /* Store one validated request. */
    guardian_frame_t request = {0};

    /* Store one response. */
    guardian_frame_t response = {0};

    /* Store one runtime snapshot. */
    guardian_device_runtime_t runtime = {0};

    /* Store one maximum encoded response. */
    uint8_t encoded[GUARDIAN_MAX_FRAME_SIZE] = {0};

    /* Store encoded response length. */
    size_t encoded_size = 0U;

    /* Store TX accepted bytes. */
    size_t written = 0U;

    /* Store parser outcome. */
    guardian_parser_result_t parser_result = GUARDIAN_PARSER_NO_FRAME;

    /* Store service/codec outcome. */
    guardian_protocol_result_t protocol_result = GUARDIAN_PROTOCOL_OK;

    /* Track bounded work. */
    size_t processed = 0U;

    /* Ignore a missing link pointer. */
    if (link == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Apply default budget when caller passes zero. */
    if (rx_budget == 0U)
    {
        /* Use the documented default budget. */
        rx_budget = GUARDIAN_EMBEDDED_DEFAULT_RX_BUDGET;
    }

    /* Process bytes until queue empty or budget consumed. */
    while (processed < rx_budget)
    {
        /* Attempt to read one non-blocking byte. */
        if (link->io.read_byte(link->io.context, &byte) == 0)
        {
            /* Stop when RX queue is empty. */
            break;
        }

        /* Count one consumed byte. */
        processed += 1U;

        /* Feed the byte into the incremental parser. */
        parser_result =
            guardian_parser_push_byte(
                &link->parser,
                byte,
                &request);

        /* Continue while the frame remains incomplete. */
        if (parser_result == GUARDIAN_PARSER_NO_FRAME)
        {
            /* Continue within the remaining budget. */
            continue;
        }

        /* Record parser failures. */
        if (parser_result != GUARDIAN_PARSER_FRAME_READY)
        {
            /* Count the rejected candidate. */
            guardian_embedded_increment_u32(
                &link->stats.protocol_errors);

            /* Publish a malformed-frame diagnostic. */
            link->stats.last_error =
                (uint8_t)GUARDIAN_ERROR_MALFORMED_FRAME;

            /* Continue searching for the next frame. */
            continue;
        }

        /* Count the validated request frame. */
        guardian_embedded_increment_u32(
            &link->stats.rx_frames);

        /* Snapshot status before current TX count changes. */
        runtime = guardian_embedded_runtime(link);

        /* Dispatch command semantics. */
        protocol_result =
            guardian_device_service_handle(
                &link->service,
                &runtime,
                &request,
                &response);

        /* Handle unexpected service failure. */
        if (protocol_result != GUARDIAN_PROTOCOL_OK)
        {
            /* Count the internal failure. */
            guardian_embedded_increment_u32(
                &link->stats.protocol_errors);

            /* Publish an internal error. */
            link->stats.last_error =
                (uint8_t)GUARDIAN_ERROR_INTERNAL;

            /* Continue without transmitting invalid state. */
            continue;
        }

        /* Record semantic ERROR frames. */
        if (response.message_type == GUARDIAN_MESSAGE_ERROR)
        {
            /* Count the semantic protocol/application error. */
            guardian_embedded_increment_u32(
                &link->stats.protocol_errors);

            /* Preserve the one-byte error code when available. */
            if (response.payload_length == 1U)
            {
                /* Publish the error code. */
                link->stats.last_error = response.payload[0];
            }
        }

        /* Encode the response. */
        protocol_result =
            guardian_protocol_encode(
                &response,
                encoded,
                sizeof(encoded),
                &encoded_size);

        /* Handle unexpected encoding failure. */
        if (protocol_result != GUARDIAN_PROTOCOL_OK)
        {
            /* Count the internal failure. */
            guardian_embedded_increment_u32(
                &link->stats.protocol_errors);

            /* Publish an internal error. */
            link->stats.last_error =
                (uint8_t)GUARDIAN_ERROR_INTERNAL;

            /* Continue without transmitting invalid bytes. */
            continue;
        }

        /* Queue the complete response atomically. */
        written =
            link->io.write(
                link->io.context,
                encoded,
                encoded_size);

        /* Require complete frame acceptance. */
        if (written != encoded_size)
        {
            /* Count the TX queue failure. */
            guardian_embedded_increment_u32(
                &link->stats.tx_queue_failures);

            /* Count the communication failure. */
            guardian_embedded_increment_u32(
                &link->stats.protocol_errors);

            /* Publish BUSY. */
            link->stats.last_error =
                (uint8_t)GUARDIAN_ERROR_BUSY;

            /* Continue without blocking. */
            continue;
        }

        /* Count complete accepted TX frames. */
        guardian_embedded_increment_u32(
            &link->stats.tx_frames);
    }
}

/* Return a safe copy of middleware diagnostics. */
guardian_embedded_link_stats_t guardian_embedded_link_stats(
    const guardian_embedded_link_t *link)
{
    /* Create deterministic empty diagnostics. */
    guardian_embedded_link_stats_t empty = {0};

    /* Handle a missing pointer. */
    if (link == NULL)
    {
        /* Return zero diagnostics. */
        return empty;
    }

    /* Return diagnostics by value. */
    return link->stats;
}
