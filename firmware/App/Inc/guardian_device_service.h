#ifndef GUARDIAN_DEVICE_SERVICE_H
#define GUARDIAN_DEVICE_SERVICE_H

/* Include the canonical Guardian Protocol frame declarations. */
#include "guardian_protocol.h"

/* Include fixed-width integer types for deterministic payload fields. */
#include <stdint.h>

/* Define the maximum model name encoded by DEVICE_INFO schema v1. */
#define GUARDIAN_DEVICE_MODEL_MAX_LENGTH ((uint8_t)32U)

/* Define the command-payload schema revision implemented by M4 firmware. */
#define GUARDIAN_DEVICE_PAYLOAD_SCHEMA_VERSION ((uint8_t)0x01U)

/* Define application states published by GET_STATUS schema v1. */
typedef enum
{
    /* Identify firmware initialization before normal services are available. */
    GUARDIAN_DEVICE_STATE_BOOT = 0x00,

    /* Identify initialized firmware waiting for application work. */
    GUARDIAN_DEVICE_STATE_IDLE = 0x01,

    /* Identify active machine monitoring or control. */
    GUARDIAN_DEVICE_STATE_RUNNING = 0x02,

    /* Identify active operation with a non-fatal degraded condition. */
    GUARDIAN_DEVICE_STATE_DEGRADED = 0x03,

    /* Identify a latched application fault. */
    GUARDIAN_DEVICE_STATE_FAULT = 0x04
} guardian_device_state_t;

/* Store immutable device identity exposed by DEVICE_INFO. */
typedef struct
{
    /* Point to a stable zero-terminated model string owned by the application. */
    const char *model;

    /* Store the semantic firmware major version. */
    uint8_t firmware_major;

    /* Store the semantic firmware minor version. */
    uint8_t firmware_minor;

    /* Store the semantic firmware patch version. */
    uint8_t firmware_patch;

    /* Store a display identifier that is not a security credential. */
    uint32_t device_id;
} guardian_device_identity_t;

/* Store one coherent runtime snapshot exposed by GET_STATUS. */
typedef struct
{
    /* Store the current application state. */
    guardian_device_state_t state;

    /* Store monotonic firmware uptime in whole seconds. */
    uint32_t uptime_seconds;

    /* Store the number of validated frames delivered to the application. */
    uint32_t rx_frames;

    /* Store the number of response/error frames accepted by the TX transport. */
    uint32_t tx_frames;

    /* Store parser, semantic and transport protocol failures. */
    uint32_t protocol_errors;

    /* Store the most recent Guardian error code. */
    uint8_t last_error;
} guardian_device_runtime_t;

/* Store immutable service configuration used by every request. */
typedef struct
{
    /* Store public device identity fields. */
    guardian_device_identity_t identity;
} guardian_device_service_t;

/* Initialize the device service with immutable public identity. */
guardian_protocol_result_t guardian_device_service_init(
    guardian_device_service_t *service,
    const guardian_device_identity_t *identity);

/* Dispatch one validated request and create one response or error frame. */
guardian_protocol_result_t guardian_device_service_handle(
    const guardian_device_service_t *service,
    const guardian_device_runtime_t *runtime,
    const guardian_frame_t *request,
    guardian_frame_t *response);

#endif
