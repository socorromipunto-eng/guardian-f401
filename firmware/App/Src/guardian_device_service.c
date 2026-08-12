/* Include the public Guardian device service declarations. */
#include "guardian_device_service.h"

/* Include size_t for bounded model-length validation. */
#include <stddef.h>

/* Include memory-copy support for deterministic payload construction. */
#include <string.h>

/* Encode one unsigned 32-bit integer using Guardian big-endian payload order. */
static void guardian_device_write_u32_be(uint8_t *output, uint32_t value)
{
    /* Write the most-significant byte first. */
    output[0] = (uint8_t)((value >> 24U) & 0xFFU);

    /* Write the second-most-significant byte. */
    output[1] = (uint8_t)((value >> 16U) & 0xFFU);

    /* Write the third byte. */
    output[2] = (uint8_t)((value >> 8U) & 0xFFU);

    /* Write the least-significant byte last. */
    output[3] = (uint8_t)(value & 0xFFU);
}

/* Calculate a bounded model-string length without unbounded reads. */
static size_t guardian_device_model_length(const char *model)
{
    /* Start before reading any model bytes. */
    size_t length = 0U;

    /* Reject a null model pointer without dereferencing it. */
    if (model == NULL)
    {
        /* Return zero so initialization can reject the invalid identity. */
        return 0U;
    }

    /* Scan only through one byte beyond the allowed maximum. */
    while (length <= (size_t)GUARDIAN_DEVICE_MODEL_MAX_LENGTH)
    {
        /* Stop when the required zero terminator is found. */
        if (model[length] == '\0')
        {
            /* Return the validated bounded length. */
            return length;
        }

        /* Advance to the next possible model byte. */
        length += 1U;
    }

    /* Return the over-limit value so initialization can reject it. */
    return length;
}

/* Build one Guardian ERROR frame correlated to the request. */
static void guardian_device_make_error(
    const guardian_frame_t *request,
    guardian_frame_t *response,
    guardian_error_code_t error_code)
{
    /* Preserve command correlation. */
    response->command = request->command;

    /* Preserve sequence correlation. */
    response->sequence = request->sequence;

    /* Mark the frame as an error. */
    response->message_type = GUARDIAN_MESSAGE_ERROR;

    /* Use the only supported v0.1 flags value. */
    response->flags = GUARDIAN_SUPPORTED_FLAGS;

    /* Encode exactly one error-code byte. */
    response->payload_length = 1U;

    /* Store the error identifier. */
    response->payload[0] = (uint8_t)error_code;
}

/* Build the frozen PING response. */
static void guardian_device_handle_ping(
    const guardian_frame_t *request,
    guardian_frame_t *response)
{
    /* Preserve command correlation. */
    response->command = request->command;

    /* Preserve sequence correlation. */
    response->sequence = request->sequence;

    /* Mark the frame as a response. */
    response->message_type = GUARDIAN_MESSAGE_RESPONSE;

    /* Use the only supported v0.1 flags value. */
    response->flags = GUARDIAN_SUPPORTED_FLAGS;

    /* Encode the four-byte PONG payload. */
    response->payload_length = 4U;

    /* Write the first PONG character. */
    response->payload[0] = (uint8_t)'P';

    /* Write the second PONG character. */
    response->payload[1] = (uint8_t)'O';

    /* Write the third PONG character. */
    response->payload[2] = (uint8_t)'N';

    /* Write the final PONG character. */
    response->payload[3] = (uint8_t)'G';
}

/* Build the DEVICE_INFO schema v1 response payload. */
static void guardian_device_handle_info(
    const guardian_device_service_t *service,
    const guardian_frame_t *request,
    guardian_frame_t *response)
{
    /* Calculate the already validated model-string length. */
    size_t model_length =
        guardian_device_model_length(service->identity.model);

    /* Preserve command correlation. */
    response->command = request->command;

    /* Preserve sequence correlation. */
    response->sequence = request->sequence;

    /* Mark the frame as a response. */
    response->message_type = GUARDIAN_MESSAGE_RESPONSE;

    /* Use the only supported v0.1 flags value. */
    response->flags = GUARDIAN_SUPPORTED_FLAGS;

    /* Encode the payload schema revision. */
    response->payload[0] = GUARDIAN_DEVICE_PAYLOAD_SCHEMA_VERSION;

    /* Encode firmware version fields. */
    response->payload[1] = service->identity.firmware_major;

    /* Encode firmware version fields. */
    response->payload[2] = service->identity.firmware_minor;

    /* Encode firmware version fields. */
    response->payload[3] = service->identity.firmware_patch;

    /* Encode the display identifier in big-endian order. */
    guardian_device_write_u32_be(
        &response->payload[4],
        service->identity.device_id);

    /* Encode the bounded model length. */
    response->payload[8] = (uint8_t)model_length;

    /* Copy exactly the validated model bytes. */
    (void)memcpy(
        &response->payload[9],
        service->identity.model,
        model_length);

    /* Publish the fixed prefix plus model bytes. */
    response->payload_length =
        (uint16_t)(9U + (uint16_t)model_length);
}

/* Build the fixed-width GET_STATUS schema v1 response payload. */
static void guardian_device_handle_status(
    const guardian_device_runtime_t *runtime,
    const guardian_frame_t *request,
    guardian_frame_t *response)
{
    /* Preserve command correlation. */
    response->command = request->command;

    /* Preserve sequence correlation. */
    response->sequence = request->sequence;

    /* Mark the frame as a response. */
    response->message_type = GUARDIAN_MESSAGE_RESPONSE;

    /* Use the only supported v0.1 flags value. */
    response->flags = GUARDIAN_SUPPORTED_FLAGS;

    /* Encode schema and application state. */
    response->payload[0] = GUARDIAN_DEVICE_PAYLOAD_SCHEMA_VERSION;

    /* Encode schema and application state. */
    response->payload[1] = (uint8_t)runtime->state;

    /* Encode uptime. */
    guardian_device_write_u32_be(
        &response->payload[2],
        runtime->uptime_seconds);

    /* Encode RX frame count. */
    guardian_device_write_u32_be(
        &response->payload[6],
        runtime->rx_frames);

    /* Encode TX frame count. */
    guardian_device_write_u32_be(
        &response->payload[10],
        runtime->tx_frames);

    /* Encode protocol error count. */
    guardian_device_write_u32_be(
        &response->payload[14],
        runtime->protocol_errors);

    /* Encode the latest error identifier. */
    response->payload[18] = runtime->last_error;

    /* Publish the frozen fixed-width payload size. */
    response->payload_length = 19U;
}

/* Initialize immutable device identity used by command responses. */
guardian_protocol_result_t guardian_device_service_init(
    guardian_device_service_t *service,
    const guardian_device_identity_t *identity)
{
    /* Store the bounded model length for validation. */
    size_t model_length = 0U;

    /* Reject missing storage. */
    if ((service == NULL) || (identity == NULL))
    {
        /* Report the canonical missing-argument error. */
        return GUARDIAN_PROTOCOL_ERROR_NULL_ARGUMENT;
    }

    /* Calculate bounded model length. */
    model_length = guardian_device_model_length(identity->model);

    /* Reject empty model strings. */
    if (model_length == 0U)
    {
        /* Report invalid configuration. */
        return GUARDIAN_PROTOCOL_ERROR_LENGTH_MISMATCH;
    }

    /* Reject oversized model strings. */
    if (model_length > (size_t)GUARDIAN_DEVICE_MODEL_MAX_LENGTH)
    {
        /* Report the bounded-payload violation. */
        return GUARDIAN_PROTOCOL_ERROR_PAYLOAD_TOO_LARGE;
    }

    /* Copy immutable identity by value. */
    service->identity = *identity;

    /* Report successful initialization. */
    return GUARDIAN_PROTOCOL_OK;
}

/* Dispatch one validated Guardian request into deterministic device behavior. */
guardian_protocol_result_t guardian_device_service_handle(
    const guardian_device_service_t *service,
    const guardian_device_runtime_t *runtime,
    const guardian_frame_t *request,
    guardian_frame_t *response)
{
    /* Reject missing storage. */
    if ((service == NULL) ||
        (runtime == NULL) ||
        (request == NULL) ||
        (response == NULL))
    {
        /* Report the canonical missing-argument error. */
        return GUARDIAN_PROTOCOL_ERROR_NULL_ARGUMENT;
    }

    /* Reject incoming non-request traffic. */
    if (request->message_type != GUARDIAN_MESSAGE_REQUEST)
    {
        /* Build a semantic error. */
        guardian_device_make_error(
            request,
            response,
            GUARDIAN_ERROR_MALFORMED_FRAME);

        /* Report successful ERROR frame construction. */
        return GUARDIAN_PROTOCOL_OK;
    }

    /* Require empty request payloads for M4 commands. */
    if (request->payload_length != 0U)
    {
        /* Build an invalid-payload error. */
        guardian_device_make_error(
            request,
            response,
            GUARDIAN_ERROR_INVALID_PAYLOAD);

        /* Report successful ERROR frame construction. */
        return GUARDIAN_PROTOCOL_OK;
    }

    /* Dispatch PING. */
    if (request->command == (uint8_t)GUARDIAN_COMMAND_PING)
    {
        /* Build PONG. */
        guardian_device_handle_ping(request, response);

        /* Report success. */
        return GUARDIAN_PROTOCOL_OK;
    }

    /* Dispatch DEVICE_INFO. */
    if (request->command == (uint8_t)GUARDIAN_COMMAND_DEVICE_INFO)
    {
        /* Build binary metadata. */
        guardian_device_handle_info(service, request, response);

        /* Report success. */
        return GUARDIAN_PROTOCOL_OK;
    }

    /* Dispatch GET_STATUS. */
    if (request->command == (uint8_t)GUARDIAN_COMMAND_GET_STATUS)
    {
        /* Build runtime diagnostics. */
        guardian_device_handle_status(runtime, request, response);

        /* Report success. */
        return GUARDIAN_PROTOCOL_OK;
    }

    /* Reject unpublished commands. */
    guardian_device_make_error(
        request,
        response,
        GUARDIAN_ERROR_UNKNOWN_COMMAND);

    /* Report successful ERROR frame construction. */
    return GUARDIAN_PROTOCOL_OK;
}
