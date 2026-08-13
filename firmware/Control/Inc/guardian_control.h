#ifndef GUARDIAN_CONTROL_H
#define GUARDIAN_CONTROL_H

/* Include M8 machine-health state consumed by supervisory policy. */
#include "guardian_health.h"

/* Include Guardian frame and protocol result types for control commands. */
#include "guardian_protocol.h"

/* Include fixed-width integer types for deterministic control fields. */
#include <stdint.h>

/* Define the first supervisory-control payload schema revision. */
#define GUARDIAN_CONTROL_SCHEMA_VERSION ((uint8_t)0x01U)

/* Define the fixed GET_CONTROL_STATUS response payload size. */
#define GUARDIAN_CONTROL_STATUS_PAYLOAD_SIZE ((uint16_t)28U)

/* Define the fixed CONTROL_COMMAND request payload size. */
#define GUARDIAN_CONTROL_COMMAND_REQUEST_SIZE ((uint16_t)2U)

/* Define the fixed successful CONTROL_COMMAND response payload size. */
#define GUARDIAN_CONTROL_COMMAND_RESPONSE_SIZE ((uint16_t)4U)

/* Mark an unready M8 model as a control fault. */
#define GUARDIAN_CONTROL_FAULT_HEALTH_NOT_READY ((uint16_t)0x0001U)

/* Mark a persistent M8 ALARM as a control fault. */
#define GUARDIAN_CONTROL_FAULT_HEALTH_ALARM ((uint16_t)0x0002U)

/* Mark an open local safety interlock as a control fault. */
#define GUARDIAN_CONTROL_FAULT_INTERLOCK_OPEN ((uint16_t)0x0004U)

/* Mark a control-output callback failure as a latched control fault. */
#define GUARDIAN_CONTROL_FAULT_OUTPUT_FAILURE ((uint16_t)0x0008U)

/* Mark a missing safe-output implementation as a live control fault. */
#define GUARDIAN_CONTROL_FAULT_OUTPUT_UNAVAILABLE ((uint16_t)0x0010U)

/* Define deterministic supervisory-control states. */
typedef enum
{
    /* Keep supervision disabled and force run permit low. */
    GUARDIAN_CONTROL_STATE_DISABLED = 0,

    /* Permit local run-request evaluation without energizing the output yet. */
    GUARDIAN_CONTROL_STATE_ARMED = 1,

    /* Publish an active local run permit under normal M8 health. */
    GUARDIAN_CONTROL_STATE_ACTIVE = 2,

    /* Keep the active permit while M8 reports a warning-level condition. */
    GUARDIAN_CONTROL_STATE_DEGRADED = 3,

    /* Force safe output and require explicit reset after a latched fault. */
    GUARDIAN_CONTROL_STATE_FAULT_LATCHED = 4
} guardian_control_state_t;

/* Define host supervisory-control actions that cannot directly request machine motion. */
typedef enum
{
    /* Arm supervisory logic only when all safe-entry conditions are satisfied. */
    GUARDIAN_CONTROL_ACTION_ARM = 1,

    /* Disable supervisory logic and force the safe output immediately. */
    GUARDIAN_CONTROL_ACTION_DISARM = 2,

    /* Clear latched faults only after safe recovery conditions are satisfied. */
    GUARDIAN_CONTROL_ACTION_CLEAR_FAULT = 3
} guardian_control_action_t;

/* Define internal deterministic supervisory-control outcomes. */
typedef enum
{
    /* Indicate successful policy execution. */
    GUARDIAN_CONTROL_OK = 0,

    /* Indicate a missing required pointer. */
    GUARDIAN_CONTROL_ERROR_NULL_ARGUMENT,

    /* Indicate a request denied by current safety policy. */
    GUARDIAN_CONTROL_ERROR_DENIED,

    /* Indicate that the configured output callback failed. */
    GUARDIAN_CONTROL_ERROR_OUTPUT
} guardian_control_result_t;

/* Apply one logical run-permit value to the board-specific safe-output boundary. */
typedef int (*guardian_control_output_fn)(
    void *context,
    uint8_t run_permit);

/* Store one immutable board-specific output adapter. */
typedef struct
{
    /* Store the callback that applies the safe output. */
    guardian_control_output_fn apply;

    /* Store opaque board/application state passed to the callback. */
    void *context;
} guardian_control_output_t;

/* Store the complete host-visible supervisory-control snapshot. */
typedef struct
{
    /* Store the current M9 supervisory state. */
    guardian_control_state_t state;

    /* Store whether supervision is currently armed. */
    uint8_t supervision_enabled;

    /* Store the local-only machine run request. */
    uint8_t local_run_request;

    /* Store the currently commanded logical run permit. */
    uint8_t run_permit;

    /* Store whether the local safety interlock is closed. */
    uint8_t interlock_closed;

    /* Store the most recent M8 health state. */
    guardian_health_state_t health_state;

    /* Store whether a board-specific output callback is installed. */
    uint8_t output_available;

    /* Store all control faults that remain latched until explicit reset. */
    uint16_t latched_faults;

    /* Store control faults active in the current input conditions. */
    uint16_t active_faults;

    /* Store the most recent M8 health score. */
    uint16_t health_score;

    /* Store the most recent M8 anomaly score. */
    uint16_t anomaly_score;

    /* Count supervisory state transitions monotonically. */
    uint32_t transition_count;

    /* Count newly latched fault episodes monotonically. */
    uint32_t fault_latch_count;

    /* Store the action or automatic reason that caused the last transition. */
    uint8_t last_transition_reason;

    /* Reserve one byte for future compatible state metadata. */
    uint8_t reserved;

    /* Reserve two bytes for future compatible status fields. */
    uint16_t reserved2;
} guardian_control_status_t;

/* Store the complete runtime supervisory-control policy state. */
typedef struct
{
    /* Store the host-visible current control snapshot. */
    guardian_control_status_t status;

    /* Store the board-specific safe-output adapter. */
    guardian_control_output_t output;
} guardian_control_t;

/* Initialize supervision disabled with safe output and interlock open. */
void guardian_control_init(
    guardian_control_t *control);

/* Install or replace the board-specific run-permit output callback. */
guardian_control_result_t guardian_control_configure_output(
    guardian_control_t *control,
    const guardian_control_output_t *output);

/* Update the local-only machine run request. */
void guardian_control_set_local_run_request(
    guardian_control_t *control,
    uint8_t requested);

/* Update the local physical/safety interlock state. */
void guardian_control_set_interlock(
    guardian_control_t *control,
    uint8_t closed);

/* Consume one current M8 health snapshot and enforce automatic safety policy. */
void guardian_control_update_health(
    guardian_control_t *control,
    const guardian_health_status_t *health);

/* Execute one host action that never directly asserts local run request. */
guardian_control_result_t guardian_control_action(
    guardian_control_t *control,
    guardian_control_action_t action);

/* Return one immutable current control snapshot by value. */
guardian_control_status_t guardian_control_status(
    const guardian_control_t *control);

/* Return whether the logical safe-output permit is currently asserted. */
uint8_t guardian_control_run_permit(
    const guardian_control_t *control);

/* Encode one fixed GET_CONTROL_STATUS payload. */
guardian_protocol_result_t guardian_control_encode_status_payload(
    const guardian_control_status_t *status,
    uint8_t *payload,
    uint16_t payload_capacity,
    uint16_t *payload_length);

/* Process GET_CONTROL_STATUS or CONTROL_COMMAND. */
guardian_protocol_result_t guardian_control_handle_request(
    guardian_control_t *control,
    const guardian_frame_t *request,
    guardian_frame_t *response);

#endif
