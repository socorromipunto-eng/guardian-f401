/* Include the public M5 telemetry declarations. */
#include "guardian_telemetry.h"

/* Include memory initialization and copy support. */
#include <string.h>

/* Decode one unsigned 16-bit integer using Guardian big-endian order. */
static uint16_t guardian_telemetry_read_u16_be(
    const uint8_t *input)
{
    /* Decode the most-significant byte. */
    uint16_t value = (uint16_t)((uint16_t)input[0] << 8U);

    /* Merge the least-significant byte. */
    value |= (uint16_t)input[1];

    /* Return the decoded value. */
    return value;
}

/* Encode one unsigned 16-bit integer using Guardian big-endian order. */
static void guardian_telemetry_write_u16_be(
    uint8_t *output,
    uint16_t value)
{
    /* Write the most-significant byte first. */
    output[0] = (uint8_t)((value >> 8U) & 0xFFU);

    /* Write the least-significant byte last. */
    output[1] = (uint8_t)(value & 0xFFU);
}

/* Encode one signed 16-bit integer using its two's-complement wire representation. */
static void guardian_telemetry_write_i16_be(
    uint8_t *output,
    int16_t value)
{
    /* Preserve the exact 16-bit representation before byte extraction. */
    uint16_t encoded = (uint16_t)value;

    /* Reuse the unsigned big-endian writer. */
    guardian_telemetry_write_u16_be(
        output,
        encoded);
}

/* Encode one unsigned 32-bit integer using Guardian big-endian order. */
static void guardian_telemetry_write_u32_be(
    uint8_t *output,
    uint32_t value)
{
    /* Write the most-significant byte first. */
    output[0] = (uint8_t)((value >> 24U) & 0xFFU);

    /* Write the second byte. */
    output[1] = (uint8_t)((value >> 16U) & 0xFFU);

    /* Write the third byte. */
    output[2] = (uint8_t)((value >> 8U) & 0xFFU);

    /* Write the least-significant byte last. */
    output[3] = (uint8_t)(value & 0xFFU);
}

/* Build one request-correlated Guardian ERROR frame. */
static void guardian_telemetry_make_error(
    const guardian_frame_t *request,
    guardian_frame_t *response,
    guardian_error_code_t error_code)
{
    /* Preserve the original command identifier. */
    response->command = request->command;

    /* Preserve the original request sequence. */
    response->sequence = request->sequence;

    /* Mark the frame explicitly as an ERROR. */
    response->message_type = GUARDIAN_MESSAGE_ERROR;

    /* Use the only flags value defined by protocol v0.1. */
    response->flags = GUARDIAN_SUPPORTED_FLAGS;

    /* Encode the frozen one-byte error payload. */
    response->payload_length = 1U;

    /* Store the published error identifier. */
    response->payload[0] = (uint8_t)error_code;
}

/* Build one successful SET_TELEMETRY response. */
static void guardian_telemetry_make_config_response(
    const guardian_telemetry_t *telemetry,
    const guardian_frame_t *request,
    guardian_frame_t *response)
{
    /* Preserve the telemetry configuration command identifier. */
    response->command = request->command;

    /* Preserve request/response sequence correlation. */
    response->sequence = request->sequence;

    /* Mark the frame as a successful synchronous response. */
    response->message_type = GUARDIAN_MESSAGE_RESPONSE;

    /* Use the only flags value defined by protocol v0.1. */
    response->flags = GUARDIAN_SUPPORTED_FLAGS;

    /* Encode the fixed four-byte configuration payload. */
    response->payload_length = 4U;

    /* Encode the M5 telemetry schema revision. */
    response->payload[0] = GUARDIAN_TELEMETRY_SCHEMA_VERSION;

    /* Encode the normalized Boolean enabled value. */
    response->payload[1] = telemetry->enabled;

    /* Encode the active bounded period in big-endian order. */
    guardian_telemetry_write_u16_be(
        &response->payload[2],
        telemetry->period_ms);
}

/* Initialize deterministic telemetry state. */
void guardian_telemetry_init(
    guardian_telemetry_t *telemetry)
{
    /* Ignore a missing telemetry pointer defensively. */
    if (telemetry == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Clear scheduler and measurement state. */
    (void)memset(
        telemetry,
        0,
        sizeof(*telemetry));

    /* Select the documented default telemetry period. */
    telemetry->period_ms =
        GUARDIAN_TELEMETRY_DEFAULT_PERIOD_MS;

    /* Reserve sequence zero and begin asynchronous telemetry at one. */
    telemetry->next_sequence = 1U;
}

/* Advance monotonic telemetry time by one millisecond. */
void guardian_telemetry_tick_1ms(
    guardian_telemetry_t *telemetry)
{
    /* Ignore a missing telemetry pointer defensively. */
    if (telemetry == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Allow the published timestamp to wrap naturally modulo 2^32. */
    telemetry->timestamp_ms += 1U;

    /* Accumulate scheduling time only while telemetry is enabled. */
    if (telemetry->enabled != 0U)
    {
        /* Saturate elapsed time so long stalls cannot wrap below the due threshold. */
        if (telemetry->elapsed_ms != 0xFFFFFFFFUL)
        {
            /* Advance the enabled scheduling clock. */
            telemetry->elapsed_ms += 1U;
        }
    }
}

/* Replace the latest application measurement snapshot. */
void guardian_telemetry_set_measurements(
    guardian_telemetry_t *telemetry,
    const guardian_machine_measurements_t *measurements)
{
    /* Ignore missing storage defensively. */
    if ((telemetry == NULL) || (measurements == NULL))
    {
        /* Return without changing the previous valid sample. */
        return;
    }

    /* Copy the complete bounded measurement snapshot by value. */
    telemetry->measurements = *measurements;
}

/* Process one SET_TELEMETRY request. */
guardian_protocol_result_t guardian_telemetry_handle_request(
    guardian_telemetry_t *telemetry,
    const guardian_frame_t *request,
    guardian_frame_t *response)
{
    /* Store the decoded requested period. */
    uint16_t requested_period_ms = 0U;

    /* Reject missing required storage. */
    if ((telemetry == NULL) ||
        (request == NULL) ||
        (response == NULL))
    {
        /* Report the canonical missing-argument failure. */
        return GUARDIAN_PROTOCOL_ERROR_NULL_ARGUMENT;
    }

    /* Reject non-request traffic at the command boundary. */
    if (request->message_type != GUARDIAN_MESSAGE_REQUEST)
    {
        /* Build a deterministic semantic error response. */
        guardian_telemetry_make_error(
            request,
            response,
            GUARDIAN_ERROR_MALFORMED_FRAME);

        /* Report successful ERROR frame construction. */
        return GUARDIAN_PROTOCOL_OK;
    }

    /* Reject accidental dispatch of an unrelated command. */
    if (request->command !=
        (uint8_t)GUARDIAN_COMMAND_SET_TELEMETRY)
    {
        /* Build a deterministic unknown-command response. */
        guardian_telemetry_make_error(
            request,
            response,
            GUARDIAN_ERROR_UNKNOWN_COMMAND);

        /* Report successful ERROR frame construction. */
        return GUARDIAN_PROTOCOL_OK;
    }

    /* Require the exact frozen four-byte configuration payload. */
    if (request->payload_length != 4U)
    {
        /* Reject malformed command-specific payloads. */
        guardian_telemetry_make_error(
            request,
            response,
            GUARDIAN_ERROR_INVALID_PAYLOAD);

        /* Report successful ERROR frame construction. */
        return GUARDIAN_PROTOCOL_OK;
    }

    /* Require the first telemetry payload schema revision. */
    if (request->payload[0] !=
        GUARDIAN_TELEMETRY_SCHEMA_VERSION)
    {
        /* Reject unsupported command-payload semantics. */
        guardian_telemetry_make_error(
            request,
            response,
            GUARDIAN_ERROR_INVALID_PAYLOAD);

        /* Report successful ERROR frame construction. */
        return GUARDIAN_PROTOCOL_OK;
    }

    /* Require the frozen Boolean encoding. */
    if (request->payload[1] > 1U)
    {
        /* Reject ambiguous enabled values. */
        guardian_telemetry_make_error(
            request,
            response,
            GUARDIAN_ERROR_INVALID_PAYLOAD);

        /* Report successful ERROR frame construction. */
        return GUARDIAN_PROTOCOL_OK;
    }

    /* Decode the requested big-endian telemetry period. */
    requested_period_ms =
        guardian_telemetry_read_u16_be(
            &request->payload[2]);

    /* Enforce the published rate limit. */
    if ((requested_period_ms <
         GUARDIAN_TELEMETRY_MIN_PERIOD_MS) ||
        (requested_period_ms >
         GUARDIAN_TELEMETRY_MAX_PERIOD_MS))
    {
        /* Reject rates outside the bounded command-channel policy. */
        guardian_telemetry_make_error(
            request,
            response,
            GUARDIAN_ERROR_INVALID_PAYLOAD);

        /* Report successful ERROR frame construction. */
        return GUARDIAN_PROTOCOL_OK;
    }

    /* Store the normalized enabled state. */
    telemetry->enabled = request->payload[1];

    /* Store the validated transmission period. */
    telemetry->period_ms = requested_period_ms;

    /* Restart the scheduling interval whenever configuration changes. */
    telemetry->elapsed_ms = 0U;

    /* Return the normalized active configuration. */
    guardian_telemetry_make_config_response(
        telemetry,
        request,
        response);

    /* Report successful response construction. */
    return GUARDIAN_PROTOCOL_OK;
}

/* Create one due asynchronous machine telemetry frame. */
int guardian_telemetry_make_due_frame(
    guardian_telemetry_t *telemetry,
    guardian_device_state_t state,
    guardian_frame_t *frame)
{
    /* Reject missing storage. */
    if ((telemetry == NULL) || (frame == NULL))
    {
        /* Report that no frame was produced. */
        return 0;
    }

    /* Do not emit while asynchronous telemetry is disabled. */
    if (telemetry->enabled == 0U)
    {
        /* Report that no frame was produced. */
        return 0;
    }

    /* Wait until the configured bounded period has elapsed. */
    if (telemetry->elapsed_ms <
        (uint32_t)telemetry->period_ms)
    {
        /* Report that no frame is due yet. */
        return 0;
    }

    /* Drop missed periods instead of creating an unbounded catch-up burst. */
    telemetry->elapsed_ms = 0U;

    /* Mark the frame as asynchronous telemetry. */
    frame->message_type = GUARDIAN_MESSAGE_TELEMETRY;

    /* Identify the machine telemetry channel in the command field. */
    frame->command =
        (uint8_t)GUARDIAN_COMMAND_MACHINE_TELEMETRY;

    /* Use the only flags value defined by protocol v0.1. */
    frame->flags = GUARDIAN_SUPPORTED_FLAGS;

    /* Publish the current independent telemetry sequence. */
    frame->sequence = telemetry->next_sequence;

    /* Wrap the asynchronous sequence to one so zero remains reserved. */
    if (telemetry->next_sequence == 0xFFFFFFFFUL)
    {
        /* Restart the non-zero sequence space. */
        telemetry->next_sequence = 1U;
    }
    else
    {
        /* Advance monotonically for the next telemetry frame. */
        telemetry->next_sequence += 1U;
    }

    /* Publish the fixed M5 machine telemetry payload size. */
    frame->payload_length = 18U;

    /* Encode the telemetry payload schema revision. */
    frame->payload[0] = GUARDIAN_TELEMETRY_SCHEMA_VERSION;

    /* Encode the application state at emission time. */
    frame->payload[1] = (uint8_t)state;

    /* Encode the monotonic sample timestamp. */
    guardian_telemetry_write_u32_be(
        &frame->payload[2],
        telemetry->timestamp_ms);

    /* Encode signed temperature. */
    guardian_telemetry_write_i16_be(
        &frame->payload[6],
        telemetry->measurements.temperature_centi_c);

    /* Encode RMS vibration magnitude. */
    guardian_telemetry_write_u16_be(
        &frame->payload[8],
        telemetry->measurements.vibration_mg_rms);

    /* Encode machine current. */
    guardian_telemetry_write_u16_be(
        &frame->payload[10],
        telemetry->measurements.current_ma);

    /* Encode shaft speed. */
    guardian_telemetry_write_u16_be(
        &frame->payload[12],
        telemetry->measurements.rpm);

    /* Encode supply voltage. */
    guardian_telemetry_write_u16_be(
        &frame->payload[14],
        telemetry->measurements.supply_mv);

    /* Encode application-defined status flags. */
    guardian_telemetry_write_u16_be(
        &frame->payload[16],
        telemetry->measurements.status_flags);

    /* Report that exactly one telemetry frame was produced. */
    return 1;
}
