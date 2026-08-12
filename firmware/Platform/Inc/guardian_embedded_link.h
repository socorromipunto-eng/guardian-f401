#ifndef GUARDIAN_EMBEDDED_LINK_H
#define GUARDIAN_EMBEDDED_LINK_H

/* Include the transport-independent device command service. */
#include "guardian_device_service.h"

/* Include the incremental Guardian byte-stream parser. */
#include "guardian_parser.h"

/* Include the asynchronous telemetry engine. */
#include "guardian_telemetry.h"

/* Include the M7 DSP feature snapshot and request handler. */
#include "guardian_dsp.h"

/* Include size_t for bounded byte budgets and writer lengths. */
#include <stddef.h>

/* Include fixed-width integer types for callbacks and diagnostics. */
#include <stdint.h>

/* Define the maximum number of RX bytes processed by one default poll call. */
#define GUARDIAN_EMBEDDED_DEFAULT_RX_BUDGET ((size_t)64U)

/* Read one transport byte without blocking. */
typedef int (*guardian_embedded_read_byte_fn)(
    void *context,
    uint8_t *byte);

/* Queue a byte block for non-blocking transmission. */
typedef size_t (*guardian_embedded_write_fn)(
    void *context,
    const uint8_t *data,
    size_t length);

/* Read monotonic firmware uptime in whole seconds. */
typedef uint32_t (*guardian_embedded_uptime_fn)(
    void *context);

/* Store transport callbacks required by the middleware link. */
typedef struct
{
    /* Store the non-blocking RX callback. */
    guardian_embedded_read_byte_fn read_byte;

    /* Store the non-blocking bounded TX queue callback. */
    guardian_embedded_write_fn write;

    /* Store the monotonic uptime callback. */
    guardian_embedded_uptime_fn uptime_seconds;

    /* Store opaque platform state passed back to every callback. */
    void *context;
} guardian_embedded_io_t;

/* Store middleware diagnostics exposed by GET_STATUS and debug tooling. */
typedef struct
{
    /* Count validated frames delivered to command handlers. */
    uint32_t rx_frames;

    /* Count response, error and telemetry frames accepted by the TX queue. */
    uint32_t tx_frames;

    /* Count parser, semantic and TX queue failures. */
    uint32_t protocol_errors;

    /* Count complete frames that could not enter the TX queue. */
    uint32_t tx_queue_failures;

    /* Store the latest Guardian application or protocol error identifier. */
    uint8_t last_error;
} guardian_embedded_link_stats_t;

/* Store the complete transport-independent embedded communication middleware. */
typedef struct
{
    /* Store byte-stream parser state across main-loop iterations. */
    guardian_parser_t parser;

    /* Store immutable device command-service identity. */
    guardian_device_service_t service;

    /* Store asynchronous telemetry scheduling and measurements. */
    guardian_telemetry_t telemetry;

    /* Store the latest successfully analyzed M7 feature snapshot. */
    guardian_dsp_features_t dsp_features;

    /* Store whether the latest DSP feature snapshot is ready for host requests. */
    uint8_t dsp_features_valid;

    /* Store platform transport callbacks. */
    guardian_embedded_io_t io;

    /* Store the application state exposed by status and telemetry. */
    guardian_device_state_t state;

    /* Store cumulative communication diagnostics. */
    guardian_embedded_link_stats_t stats;
} guardian_embedded_link_t;

/* Initialize middleware, parser, telemetry, callbacks and device identity. */
guardian_protocol_result_t guardian_embedded_link_init(
    guardian_embedded_link_t *link,
    const guardian_embedded_io_t *io,
    const guardian_device_identity_t *identity);

/* Change the application state published by status and telemetry. */
void guardian_embedded_link_set_state(
    guardian_embedded_link_t *link,
    guardian_device_state_t state);

/* Replace the latest application-provided telemetry measurements. */
void guardian_embedded_link_update_telemetry(
    guardian_embedded_link_t *link,
    const guardian_machine_measurements_t *measurements);


/* Replace the latest M7 DSP feature snapshot exposed by GET_DSP_FEATURES. */
void guardian_embedded_link_update_dsp(
    guardian_embedded_link_t *link,
    const guardian_dsp_features_t *features);

/* Advance asynchronous telemetry scheduling by one millisecond. */
void guardian_embedded_link_tick_1ms(
    guardian_embedded_link_t *link);

/* Process bounded RX work and emit at most one due telemetry frame. */
void guardian_embedded_link_poll(
    guardian_embedded_link_t *link,
    size_t rx_budget);

/* Return a copy of cumulative middleware diagnostics. */
guardian_embedded_link_stats_t guardian_embedded_link_stats(
    const guardian_embedded_link_t *link);

#endif
