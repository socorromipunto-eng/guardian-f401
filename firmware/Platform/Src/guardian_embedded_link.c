/* Include the public embedded middleware declarations. */
#include "guardian_embedded_link.h"

/* Include memory initialization support for deterministic startup. */
#include <string.h>

/* Saturating-increment one unsigned 32-bit diagnostic counter. */
static void guardian_embedded_increment_u32(
    uint32_t *value)
{
    /* Ignore a missing counter pointer defensively. */
    if (value == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Avoid unsigned wrap because diagnostics should remain monotonic. */
    if (*value != 0xFFFFFFFFUL)
    {
        /* Increment only while representable. */
        *value += 1U;
    }
}

/* Map M9 supervisory state into the pre-existing device state published by M4/M5. */
static guardian_device_state_t guardian_embedded_control_device_state(
    const guardian_control_status_t *status)
{
    /* Map active normal supervision into RUNNING. */
    if (status->state ==
        GUARDIAN_CONTROL_STATE_ACTIVE)
    {
        /* Publish active machine monitoring/control state. */
        return GUARDIAN_DEVICE_STATE_RUNNING;
    }

    /* Map warning-level active supervision into DEGRADED. */
    if (status->state ==
        GUARDIAN_CONTROL_STATE_DEGRADED)
    {
        /* Publish non-fatal degraded operation. */
        return GUARDIAN_DEVICE_STATE_DEGRADED;
    }

    /* Map every latched M9 control fault into FAULT. */
    if (status->state ==
        GUARDIAN_CONTROL_STATE_FAULT_LATCHED)
    {
        /* Publish the existing application fault state. */
        return GUARDIAN_DEVICE_STATE_FAULT;
    }

    /* Keep DISABLED and ARMED supervision represented as IDLE. */
    return GUARDIAN_DEVICE_STATE_IDLE;
}

/* Synchronize legacy device-state output only after M9 supervision becomes authoritative. */
static void guardian_embedded_sync_control_state(
    guardian_embedded_link_t *link)
{
    /* Read one immutable M9 control snapshot. */
    guardian_control_status_t status =
        guardian_control_status(
            &link->control);

    /* Preserve legacy caller-selected state while supervision remains completely disabled. */
    if ((status.supervision_enabled == 0U) &&
        (status.state !=
         GUARDIAN_CONTROL_STATE_FAULT_LATCHED))
    {
        /* Return without overwriting the M4 compatibility state. */
        return;
    }

    /* Map M9 control state into GET_STATUS and M5 telemetry state. */
    link->state =
        guardian_embedded_control_device_state(
            &status);
}

/* Feed the latest M8 snapshot into M9 and synchronize legacy device state when engaged. */
static void guardian_embedded_update_control_health(
    guardian_embedded_link_t *link)
{
    /* Read the current M8 machine-health snapshot. */
    guardian_health_status_t health =
        guardian_health_status(
            &link->health);

    /* Enforce automatic M9 health-driven safety policy. */
    guardian_control_update_health(
        &link->control,
        &health);

    /* Reflect active M9 policy through the existing status/telemetry state field. */
    guardian_embedded_sync_control_state(
        link);
}

/* Build a coherent runtime snapshot before creating a response. */
static guardian_device_runtime_t guardian_embedded_runtime(
    const guardian_embedded_link_t *link)
{
    /* Create deterministic zero-initialized runtime storage. */
    guardian_device_runtime_t runtime = {0};

    /* Publish the current application state. */
    runtime.state = link->state;

    /* Read monotonic uptime from the platform callback. */
    runtime.uptime_seconds =
        link->io.uptime_seconds(link->io.context);

    /* Publish validated received-frame count. */
    runtime.rx_frames = link->stats.rx_frames;

    /* Publish frames transmitted before the current response. */
    runtime.tx_frames = link->stats.tx_frames;

    /* Publish accumulated protocol and transport failures. */
    runtime.protocol_errors = link->stats.protocol_errors;

    /* Publish the most recent Guardian error identifier. */
    runtime.last_error = link->stats.last_error;

    /* Return the coherent snapshot by value. */
    return runtime;
}

/* Record semantic ERROR frames produced by a command handler. */
static void guardian_embedded_record_response_error(
    guardian_embedded_link_t *link,
    const guardian_frame_t *response)
{
    /* Ignore successful response and telemetry classes. */
    if (response->message_type != GUARDIAN_MESSAGE_ERROR)
    {
        /* Return without changing error diagnostics. */
        return;
    }

    /* Count the semantic command failure. */
    guardian_embedded_increment_u32(
        &link->stats.protocol_errors);

    /* Preserve the published one-byte error code when present. */
    if (response->payload_length == 1U)
    {
        /* Store the most recent Guardian error identifier. */
        link->stats.last_error = response->payload[0];
    }
}

/* Encode and queue one complete Guardian frame atomically. */
static void guardian_embedded_transmit(
    guardian_embedded_link_t *link,
    const guardian_frame_t *frame)
{
    /* Store the largest legal encoded Guardian frame. */
    uint8_t encoded[GUARDIAN_MAX_FRAME_SIZE] = {0};

    /* Store the exact encoded frame length. */
    size_t encoded_size = 0U;

    /* Store the number of bytes accepted by the platform TX queue. */
    size_t written = 0U;

    /* Store the canonical frame-encoding outcome. */
    guardian_protocol_result_t result = GUARDIAN_PROTOCOL_OK;

    /* Encode the complete frame using the M1 canonical codec. */
    result =
        guardian_protocol_encode(
            frame,
            encoded,
            sizeof(encoded),
            &encoded_size);

    /* Record unexpected frame-encoding failures. */
    if (result != GUARDIAN_PROTOCOL_OK)
    {
        /* Count the internal communication failure. */
        guardian_embedded_increment_u32(
            &link->stats.protocol_errors);

        /* Publish the generic internal error identifier. */
        link->stats.last_error =
            (uint8_t)GUARDIAN_ERROR_INTERNAL;

        /* Return without transmitting invalid bytes. */
        return;
    }

    /* Queue the complete encoded frame through the platform writer. */
    written =
        link->io.write(
            link->io.context,
            encoded,
            encoded_size);

    /* Require atomic acceptance of the complete Guardian frame. */
    if (written != encoded_size)
    {
        /* Count the bounded TX capacity failure. */
        guardian_embedded_increment_u32(
            &link->stats.tx_queue_failures);

        /* Count the failed communication attempt. */
        guardian_embedded_increment_u32(
            &link->stats.protocol_errors);

        /* Publish BUSY because bounded TX capacity prevented transmission. */
        link->stats.last_error =
            (uint8_t)GUARDIAN_ERROR_BUSY;

        /* Return without blocking or retry bursts. */
        return;
    }

    /* Count every complete response, error or telemetry frame accepted by TX. */
    guardian_embedded_increment_u32(
        &link->stats.tx_frames);
}

/* Initialize the complete transport-independent communication path. */
guardian_protocol_result_t guardian_embedded_link_init(
    guardian_embedded_link_t *link,
    const guardian_embedded_io_t *io,
    const guardian_device_identity_t *identity)
{
    /* Store device-service initialization status. */
    guardian_protocol_result_t result = GUARDIAN_PROTOCOL_OK;

    /* Reject missing middleware, callback or identity storage. */
    if ((link == NULL) || (io == NULL) || (identity == NULL))
    {
        /* Report the canonical missing-argument error. */
        return GUARDIAN_PROTOCOL_ERROR_NULL_ARGUMENT;
    }

    /* Reject missing mandatory transport callbacks. */
    if ((io->read_byte == NULL) ||
        (io->write == NULL) ||
        (io->uptime_seconds == NULL))
    {
        /* Report the canonical missing-argument error. */
        return GUARDIAN_PROTOCOL_ERROR_NULL_ARGUMENT;
    }

    /* Clear every middleware field before subsystem initialization. */
    (void)memset(
        link,
        0,
        sizeof(*link));

    /* Copy platform callback configuration by value. */
    link->io = *io;

    /* Start the application in its initialized IDLE state. */
    link->state = GUARDIAN_DEVICE_STATE_IDLE;

    /* Initialize deterministic byte-stream parser state. */
    guardian_parser_init(
        &link->parser);

    /* Initialize asynchronous telemetry disabled with safe defaults. */
    guardian_telemetry_init(
        &link->telemetry);

    /* Initialize the M8 runtime health model in the explicit UNTRAINED state. */
    guardian_health_init(
        &link->health);

    /* Initialize M9 supervision disabled, output-safe and interlock-open. */
    guardian_control_init(
        &link->control);

    /* Initialize immutable command-service identity. */
    result =
        guardian_device_service_init(
            &link->service,
            identity);

    /* Return the command-service initialization result. */
    return result;
}

/* Change the legacy application state only while M9 supervision is not authoritative. */
void guardian_embedded_link_set_state(
    guardian_embedded_link_t *link,
    guardian_device_state_t state)
{
    /* Ignore a missing middleware pointer defensively. */
    if (link == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Read the current M9 supervisory snapshot. */
    guardian_control_status_t control_status =
        guardian_control_status(
            &link->control);

    /* Refuse manual state overrides while M9 is armed, active, degraded or fault-latched. */
    if ((control_status.supervision_enabled != 0U) ||
        (control_status.state ==
         GUARDIAN_CONTROL_STATE_FAULT_LATCHED))
    {
        /* Preserve the safety-owned state mapping. */
        return;
    }

    /* Preserve M4 compatibility while supervision remains disabled. */
    link->state = state;
}

/* Replace the latest application-provided telemetry measurements. */
void guardian_embedded_link_update_telemetry(
    guardian_embedded_link_t *link,
    const guardian_machine_measurements_t *measurements)
{
    /* Ignore missing middleware storage defensively. */
    if (link == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Forward the bounded snapshot into the telemetry engine. */
    guardian_telemetry_set_measurements(
        &link->telemetry,
        measurements);
}

/* Replace the latest M7 DSP feature snapshot exposed by GET_DSP_FEATURES. */
void guardian_embedded_link_update_dsp(
    guardian_embedded_link_t *link,
    const guardian_dsp_features_t *features)
{
    /* Ignore missing middleware or feature storage defensively. */
    if ((link == NULL) || (features == NULL))
    {
        /* Return without modifying the previous valid snapshot. */
        return;
    }

    /* Copy the complete bounded feature snapshot by value. */
    link->dsp_features = *features;

    /* Mark the copied feature snapshot ready for host requests. */
    link->dsp_features_valid = 1U;

    /* Ingest the exact same feature snapshot into the frozen-baseline health model. */
    guardian_health_ingest(
        &link->health,
        features);

    /* Enforce M9 policy using the newly updated M8 health snapshot. */
    guardian_embedded_update_control_health(
        link);
}

/* Return the current immutable M8 machine-health snapshot by value. */
guardian_health_status_t guardian_embedded_link_health_status(
    const guardian_embedded_link_t *link)
{
    /* Return deterministic untrained status for a missing middleware pointer. */
    if (link == NULL)
    {
        /* Delegate null handling to the health module. */
        return guardian_health_status(
            NULL);
    }

    /* Return the current bounded model snapshot by value. */
    return guardian_health_status(
        &link->health);
}

/* Install the application/board-specific logical run-permit output boundary. */
guardian_control_result_t guardian_embedded_link_configure_control_output(
    guardian_embedded_link_t *link,
    const guardian_control_output_t *output)
{
    /* Reject a missing middleware pointer. */
    if (link == NULL)
    {
        /* Report invalid caller storage. */
        return GUARDIAN_CONTROL_ERROR_NULL_ARGUMENT;
    }

    /* Configure and immediately safe-off the M9 output boundary. */
    guardian_control_result_t result =
        guardian_control_configure_output(
            &link->control,
            output);

    /* Reflect any output-configuration fault into legacy device state. */
    guardian_embedded_sync_control_state(
        link);

    /* Return the canonical M9 configuration result. */
    return result;
}

/* Update the local-only machine run request consumed by M9 policy. */
void guardian_embedded_link_set_local_run_request(
    guardian_embedded_link_t *link,
    uint8_t requested)
{
    /* Ignore a missing middleware pointer defensively. */
    if (link == NULL)
    {
        /* Return without changing control state. */
        return;
    }

    /* Forward the local-only run request into the M9 state machine. */
    guardian_control_set_local_run_request(
        &link->control,
        requested);

    /* Reflect active M9 supervision through existing state output. */
    guardian_embedded_sync_control_state(
        link);
}

/* Update the local safety-interlock input consumed by M9 policy. */
void guardian_embedded_link_set_interlock(
    guardian_embedded_link_t *link,
    uint8_t closed)
{
    /* Ignore a missing middleware pointer defensively. */
    if (link == NULL)
    {
        /* Return without changing control state. */
        return;
    }

    /* Forward the local interlock state into M9 policy. */
    guardian_control_set_interlock(
        &link->control,
        closed);

    /* Reflect any resulting active/fault state through existing device state. */
    guardian_embedded_sync_control_state(
        link);
}

/* Return the current immutable M9 supervisory-control snapshot by value. */
guardian_control_status_t guardian_embedded_link_control_status(
    const guardian_embedded_link_t *link)
{
    /* Delegate deterministic null handling to the control module. */
    if (link == NULL)
    {
        /* Return the safe default M9 snapshot. */
        return guardian_control_status(
            NULL);
    }

    /* Return the current bounded control snapshot by value. */
    return guardian_control_status(
        &link->control);
}

/* Return the currently applied logical M9 run permit. */
uint8_t guardian_embedded_link_run_permit(
    const guardian_embedded_link_t *link)
{
    /* Return safe-off for a missing middleware pointer. */
    if (link == NULL)
    {
        /* Publish safe logical output. */
        return 0U;
    }

    /* Return the successfully applied M9 logical run permit. */
    return guardian_control_run_permit(
        &link->control);
}

/* Advance asynchronous telemetry scheduling by one millisecond. */
void guardian_embedded_link_tick_1ms(
    guardian_embedded_link_t *link)
{
    /* Ignore a missing middleware pointer defensively. */
    if (link == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Advance the independent telemetry scheduler and timestamp. */
    guardian_telemetry_tick_1ms(
        &link->telemetry);
}

/* Process bounded RX work and emit at most one due telemetry frame. */
void guardian_embedded_link_poll(
    guardian_embedded_link_t *link,
    size_t rx_budget)
{
    /* Store one byte retrieved from the platform RX queue. */
    uint8_t byte = 0U;

    /* Store one validated request produced by the stream parser. */
    guardian_frame_t request = {0};

    /* Store one response or asynchronous telemetry frame. */
    guardian_frame_t output = {0};

    /* Store one coherent runtime snapshot for a request. */
    guardian_device_runtime_t runtime = {0};

    /* Store the stream parser outcome for one received byte. */
    guardian_parser_result_t parser_result =
        GUARDIAN_PARSER_NO_FRAME;

    /* Store command-handler outcomes. */
    guardian_protocol_result_t protocol_result =
        GUARDIAN_PROTOCOL_OK;

    /* Track bounded foreground RX work completed by this poll call. */
    size_t processed = 0U;

    /* Ignore a missing middleware pointer defensively. */
    if (link == NULL)
    {
        /* Return without dereferencing caller memory. */
        return;
    }

    /* Convert a zero budget into the documented default workload. */
    if (rx_budget == 0U)
    {
        /* Use the project default foreground RX budget. */
        rx_budget = GUARDIAN_EMBEDDED_DEFAULT_RX_BUDGET;
    }

    /* Process available RX bytes until the queue is empty or budget is consumed. */
    while (processed < rx_budget)
    {
        /* Attempt to read exactly one byte without blocking. */
        if (link->io.read_byte(
                link->io.context,
                &byte) == 0)
        {
            /* Stop immediately when the platform RX queue is empty. */
            break;
        }

        /* Count the foreground work unit just consumed. */
        processed += 1U;

        /* Feed the byte into the transport-independent parser. */
        parser_result =
            guardian_parser_push_byte(
                &link->parser,
                byte,
                &request);

        /* Ignore incomplete frame progress. */
        if (parser_result == GUARDIAN_PARSER_NO_FRAME)
        {
            /* Continue consuming bytes within the remaining budget. */
            continue;
        }

        /* Record parser failures that cannot reach command semantics safely. */
        if (parser_result != GUARDIAN_PARSER_FRAME_READY)
        {
            /* Count the rejected transport/protocol candidate. */
            guardian_embedded_increment_u32(
                &link->stats.protocol_errors);

            /* Preserve a malformed-frame diagnostic for status output. */
            link->stats.last_error =
                (uint8_t)GUARDIAN_ERROR_MALFORMED_FRAME;

            /* Continue searching for the next valid request. */
            continue;
        }

        /* Count the validated frame delivered to command semantics. */
        guardian_embedded_increment_u32(
            &link->stats.rx_frames);

        /* Route M5 telemetry configuration to the telemetry engine. */
        if (request.command ==
            (uint8_t)GUARDIAN_COMMAND_SET_TELEMETRY)
        {
            /* Process and normalize the telemetry configuration. */
            protocol_result =
                guardian_telemetry_handle_request(
                    &link->telemetry,
                    &request,
                    &output);
        }
        else if (request.command ==
                 (uint8_t)GUARDIAN_COMMAND_GET_DSP_FEATURES)
        {
            /* Return the latest immutable M7 feature snapshot or BUSY before the first analysis. */
            protocol_result =
                guardian_dsp_handle_request(
                    &link->dsp_features,
                    link->dsp_features_valid,
                    &request,
                    &output);
        }
        else if ((request.command ==
                  (uint8_t)GUARDIAN_COMMAND_GET_HEALTH_STATUS) ||
                 (request.command ==
                  (uint8_t)GUARDIAN_COMMAND_BASELINE_CONTROL))
        {
            /* Route M8 baseline lifecycle and health queries to the health model. */
            protocol_result =
                guardian_health_handle_request(
                    &link->health,
                    &request,
                    &output);

            /* Re-evaluate M9 when a baseline command may have changed M8 readiness. */
            if ((protocol_result ==
                 GUARDIAN_PROTOCOL_OK) &&
                (request.command ==
                 (uint8_t)GUARDIAN_COMMAND_BASELINE_CONTROL) &&
                (output.message_type ==
                 GUARDIAN_MESSAGE_RESPONSE))
            {
                /* Enforce safe-off if baseline reset/start made health unready. */
                guardian_embedded_update_control_health(
                    link);
            }
        }
        else if ((request.command ==
                  (uint8_t)GUARDIAN_COMMAND_GET_CONTROL_STATUS) ||
                 (request.command ==
                  (uint8_t)GUARDIAN_COMMAND_CONTROL_COMMAND))
        {
            /* Route M9 status and safety-gated host actions to supervisory control. */
            protocol_result =
                guardian_control_handle_request(
                    &link->control,
                    &request,
                    &output);

            /* Reflect successful or faulting M9 command effects through legacy device state. */
            guardian_embedded_sync_control_state(
                link);
        }
        else
        {
            /* Build runtime diagnostics before the current response increments TX. */
            runtime = guardian_embedded_runtime(link);

            /* Dispatch the existing deterministic device commands. */
            protocol_result =
                guardian_device_service_handle(
                    &link->service,
                    &runtime,
                    &request,
                    &output);
        }

        /* Record an internal middleware failure if response construction failed. */
        if (protocol_result != GUARDIAN_PROTOCOL_OK)
        {
            /* Count the response-construction failure. */
            guardian_embedded_increment_u32(
                &link->stats.protocol_errors);

            /* Publish the generic internal Guardian error identifier. */
            link->stats.last_error =
                (uint8_t)GUARDIAN_ERROR_INTERNAL;

            /* Continue without transmitting uninitialized output state. */
            continue;
        }

        /* Record semantic ERROR responses before transmission. */
        guardian_embedded_record_response_error(
            link,
            &output);

        /* Encode and queue the complete response or error frame. */
        guardian_embedded_transmit(
            link,
            &output);
    }

    /* Emit at most one due telemetry frame after bounded request processing. */
    if (guardian_telemetry_make_due_frame(
            &link->telemetry,
            link->state,
            &output) != 0)
    {
        /* Queue the asynchronous frame without blocking the main loop. */
        guardian_embedded_transmit(
            link,
            &output);
    }
}

/* Return a safe copy of cumulative middleware diagnostics. */
guardian_embedded_link_stats_t guardian_embedded_link_stats(
    const guardian_embedded_link_t *link)
{
    /* Create deterministic zero diagnostics for a missing pointer. */
    guardian_embedded_link_stats_t empty = {0};

    /* Return zero diagnostics when caller storage is invalid. */
    if (link == NULL)
    {
        /* Return the deterministic empty value. */
        return empty;
    }

    /* Return middleware diagnostics by value. */
    return link->stats;
}
