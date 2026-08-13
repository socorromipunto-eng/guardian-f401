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

/* Include the M8 runtime baseline and anomaly model. */
#include "guardian_health.h"

/* Include the M9 supervisory-control policy and safe-output abstraction. */
#include "guardian_control.h"

/* Include the M10 authenticated-session and anti-replay policy. */
#include "guardian_security.h"

/* Include the M12 secure firmware lifecycle policy. */
#include "guardian_firmware_lifecycle.h"

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

    /* Store the complete M8 runtime baseline and anomaly model. */
    guardian_health_t health;

    /* Store the complete M9 supervisory-control runtime. */
    guardian_control_t control;

    /* Store the complete M10 authenticated-session runtime. */
    guardian_security_t security;

    /* Store the complete M12 secure firmware lifecycle runtime. */
    guardian_firmware_lifecycle_t firmware;

    /* Require protected wrapping for privileged commands when non-zero. */
    uint8_t security_required;

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

/* Change legacy application state only while M9 supervision remains disabled. */
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


/* Return the current immutable M8 machine-health snapshot by value. */
guardian_health_status_t guardian_embedded_link_health_status(
    const guardian_embedded_link_t *link);


/* Install the application/board-specific logical run-permit output boundary. */
guardian_control_result_t guardian_embedded_link_configure_control_output(
    guardian_embedded_link_t *link,
    const guardian_control_output_t *output);

/* Update the local-only machine run request consumed by M9 policy. */
void guardian_embedded_link_set_local_run_request(
    guardian_embedded_link_t *link,
    uint8_t requested);

/* Update the local safety-interlock input consumed by M9 policy. */
void guardian_embedded_link_set_interlock(
    guardian_embedded_link_t *link,
    uint8_t closed);

/* Return the current immutable M9 supervisory-control snapshot by value. */
guardian_control_status_t guardian_embedded_link_control_status(
    const guardian_embedded_link_t *link);

/* Return the currently applied logical M9 run permit. */
uint8_t guardian_embedded_link_run_permit(
    const guardian_embedded_link_t *link);


/* Install M10 PSK provisioning and cryptographic nonce callback. */
guardian_security_result_t guardian_embedded_link_configure_security(
    guardian_embedded_link_t *link,
    const guardian_security_config_t *config);

/* Enable or disable direct privileged-command rejection. */
void guardian_embedded_link_require_security(
    guardian_embedded_link_t *link,
    uint8_t required);

/* Return public M10 security diagnostics by value. */
guardian_security_status_t guardian_embedded_link_security_status(
    guardian_embedded_link_t *link);

/* Install M12 staging, signature-verification and rollback-persistence callbacks. */
guardian_firmware_result_t guardian_embedded_link_configure_firmware(
    guardian_embedded_link_t *link,
    const guardian_firmware_config_t *config);

/* Return public M12 firmware lifecycle diagnostics by value. */
guardian_firmware_status_t guardian_embedded_link_firmware_status(
    const guardian_embedded_link_t *link);

/* Confirm one successfully booted pending image and advance rollback floor. */
guardian_firmware_result_t guardian_embedded_link_confirm_firmware_boot(
    guardian_embedded_link_t *link,
    uint32_t booted_version_counter);

/* Record one failed pending image boot without advancing rollback floor. */
guardian_firmware_result_t guardian_embedded_link_report_firmware_boot_failure(
    guardian_embedded_link_t *link,
    uint32_t attempted_version_counter);

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
