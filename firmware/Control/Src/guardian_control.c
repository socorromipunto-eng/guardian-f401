/* Include the public M9 supervisory-control declarations. */
#include "guardian_control.h"

/* Include memory initialization support for deterministic startup. */
#include <string.h>

/* Identify an automatic transition caused by health becoming unready. */
#define GUARDIAN_CONTROL_REASON_HEALTH_NOT_READY ((uint8_t)0x80U)

/* Identify an automatic transition caused by M8 ALARM. */
#define GUARDIAN_CONTROL_REASON_HEALTH_ALARM ((uint8_t)0x81U)

/* Identify an automatic transition caused by local interlock opening. */
#define GUARDIAN_CONTROL_REASON_INTERLOCK ((uint8_t)0x82U)

/* Identify an automatic transition caused by output-application failure. */
#define GUARDIAN_CONTROL_REASON_OUTPUT_FAILURE ((uint8_t)0x83U)

/* Identify an automatic transition caused by local run-request assertion. */
#define GUARDIAN_CONTROL_REASON_LOCAL_RUN ((uint8_t)0x84U)

/* Identify an automatic transition caused by local run-request removal. */
#define GUARDIAN_CONTROL_REASON_LOCAL_STOP ((uint8_t)0x85U)

/* Identify an automatic transition caused by warning-level health degradation. */
#define GUARDIAN_CONTROL_REASON_HEALTH_WARNING ((uint8_t)0x86U)

/* Identify an automatic transition caused by recovery from warning-level health. */
#define GUARDIAN_CONTROL_REASON_HEALTH_RECOVERED ((uint8_t)0x87U)

/* Saturating-increment one unsigned 32-bit diagnostic counter. */
static void guardian_control_increment_u32(
    uint32_t *value)
{
    /* Ignore missing storage defensively. */
    if (value == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Avoid unsigned diagnostic wrap. */
    if (*value != 0xFFFFFFFFUL)
    {
        /* Increment only while representable. */
        *value += 1U;
    }
}

/* Publish one state transition only when the state actually changes. */
static void guardian_control_transition(
    guardian_control_t *control,
    guardian_control_state_t state,
    uint8_t reason)
{
    /* Record transition diagnostics only when the state changes. */
    if (control->status.state != state)
    {
        /* Publish the new state. */
        control->status.state =
            state;

        /* Count the state transition monotonically. */
        guardian_control_increment_u32(
            &control->status.transition_count);

        /* Preserve the action or automatic reason for the transition. */
        control->status.last_transition_reason =
            reason;
    }
}

/* Return live control faults derived from current input conditions. */
static uint16_t guardian_control_active_faults(
    const guardian_control_t *control)
{
    /* Start without active faults. */
    uint16_t faults = 0U;

    /* Require a trained M8 model before supervision can be armed. */
    if ((control->status.health_state ==
         GUARDIAN_HEALTH_STATE_UNTRAINED) ||
        (control->status.health_state ==
         GUARDIAN_HEALTH_STATE_LEARNING))
    {
        /* Mark health as not ready for supervisory permission. */
        faults |=
            GUARDIAN_CONTROL_FAULT_HEALTH_NOT_READY;
    }

    /* Treat an M8 ALARM as an active shutdown fault. */
    if (control->status.health_state ==
        GUARDIAN_HEALTH_STATE_ALARM)
    {
        /* Mark the severe health condition. */
        faults |=
            GUARDIAN_CONTROL_FAULT_HEALTH_ALARM;
    }

    /* Require the local safety interlock to remain closed. */
    if (control->status.interlock_closed == 0U)
    {
        /* Mark the open local interlock. */
        faults |=
            GUARDIAN_CONTROL_FAULT_INTERLOCK_OPEN;
    }

    /* Require a board/application output adapter before supervision can be armed. */
    if (control->status.output_available == 0U)
    {
        /* Mark the missing output boundary. */
        faults |=
            GUARDIAN_CONTROL_FAULT_OUTPUT_UNAVAILABLE;
    }

    /* Preserve output failures while they remain latched. */
    if ((control->status.latched_faults &
         GUARDIAN_CONTROL_FAULT_OUTPUT_FAILURE) != 0U)
    {
        /* Report the output fault as active until explicit safe reset. */
        faults |=
            GUARDIAN_CONTROL_FAULT_OUTPUT_FAILURE;
    }

    /* Return the current live fault mask. */
    return faults;
}

/* Latch one or more control faults and force the logical output safe. */
static void guardian_control_latch_fault(
    guardian_control_t *control,
    uint16_t faults,
    uint8_t reason)
{
    /* Preserve which faults were newly latched by this episode. */
    uint16_t new_faults =
        (uint16_t)(
            faults &
            (uint16_t)~control->status.latched_faults
        );

    /* Latch every supplied fault until explicit safe reset. */
    control->status.latched_faults |=
        faults;

    /* Count only episodes that introduce at least one new latched fault bit. */
    if (new_faults != 0U)
    {
        /* Advance the fault-latch diagnostic counter. */
        guardian_control_increment_u32(
            &control->status.fault_latch_count);
    }

    /* Remove logical supervision immediately. */
    control->status.supervision_enabled = 0U;

    /* Publish the safe logical run permit immediately. */
    control->status.run_permit = 0U;

    /* Attempt to drive the board-specific output safe when available. */
    if (control->output.apply != NULL)
    {
        /* Apply the de-energized logical permit. */
        if (control->output.apply(
                control->output.context,
                0U) == 0)
        {
            /* Preserve an output failure when safe-off application itself failed. */
            control->status.latched_faults |=
                GUARDIAN_CONTROL_FAULT_OUTPUT_FAILURE;
        }
    }

    /* Publish the latched-fault supervisory state. */
    guardian_control_transition(
        control,
        GUARDIAN_CONTROL_STATE_FAULT_LATCHED,
        reason);

    /* Refresh live faults after the state change. */
    control->status.active_faults =
        guardian_control_active_faults(
            control);
}

/* Apply one logical run-permit value through the configured safe-output boundary. */
static guardian_control_result_t guardian_control_apply_output(
    guardian_control_t *control,
    uint8_t run_permit)
{
    /* Require a board/application output adapter for any control operation. */
    if (control->output.apply == NULL)
    {
        /* Publish unavailable output state. */
        control->status.output_available = 0U;

        /* Refresh live fault state. */
        control->status.active_faults =
            guardian_control_active_faults(
                control);

        /* Deny the operation without asserting the logical permit. */
        return GUARDIAN_CONTROL_ERROR_DENIED;
    }

    /* Normalize the requested permit into zero or one. */
    run_permit =
        (run_permit != 0U)
        ? 1U
        : 0U;

    /* Ask the board/application boundary to apply the requested logical output. */
    if (control->output.apply(
            control->output.context,
            run_permit) == 0)
    {
        /* Latch output failure and force safe logical state. */
        guardian_control_latch_fault(
            control,
            GUARDIAN_CONTROL_FAULT_OUTPUT_FAILURE,
            GUARDIAN_CONTROL_REASON_OUTPUT_FAILURE);

        /* Report the hardware/application output failure. */
        return GUARDIAN_CONTROL_ERROR_OUTPUT;
    }

    /* Publish the successfully applied logical permit. */
    control->status.run_permit =
        run_permit;

    /* Report successful output application. */
    return GUARDIAN_CONTROL_OK;
}

/* Evaluate local run-request and current health after any safe input update. */
static void guardian_control_evaluate(
    guardian_control_t *control)
{
    /* Refresh current live faults first. */
    control->status.active_faults =
        guardian_control_active_faults(
            control);

    /* Do not clear or bypass an existing latched fault automatically. */
    if (control->status.latched_faults != 0U)
    {
        /* Keep the safe logical output. */
        (void)guardian_control_apply_output(
            control,
            0U);

        /* Keep supervision disabled. */
        control->status.supervision_enabled = 0U;

        /* Preserve fault-latched state. */
        guardian_control_transition(
            control,
            GUARDIAN_CONTROL_STATE_FAULT_LATCHED,
            control->status.last_transition_reason);

        /* Return without automatic fault recovery. */
        return;
    }

    /* Keep output safe while supervision is disabled. */
    if (control->status.supervision_enabled == 0U)
    {
        /* Force the safe output without turning a missing output adapter into a latch. */
        if (control->output.apply != NULL)
        {
            /* Best-effort safe-off while disabled. */
            if (control->output.apply(
                    control->output.context,
                    0U) == 0)
            {
                /* Latch an actual output failure. */
                guardian_control_latch_fault(
                    control,
                    GUARDIAN_CONTROL_FAULT_OUTPUT_FAILURE,
                    GUARDIAN_CONTROL_REASON_OUTPUT_FAILURE);

                /* Return after fault handling. */
                return;
            }
        }

        /* Publish the safe logical permit. */
        control->status.run_permit = 0U;

        /* Preserve disabled state. */
        guardian_control_transition(
            control,
            GUARDIAN_CONTROL_STATE_DISABLED,
            control->status.last_transition_reason);

        /* Return because disabled supervision never energizes output. */
        return;
    }

    /* Latch an open interlock when supervision is armed or active. */
    if ((control->status.active_faults &
         GUARDIAN_CONTROL_FAULT_INTERLOCK_OPEN) != 0U)
    {
        /* Force safe-off and require explicit reset. */
        guardian_control_latch_fault(
            control,
            GUARDIAN_CONTROL_FAULT_INTERLOCK_OPEN,
            GUARDIAN_CONTROL_REASON_INTERLOCK);

        /* Return after fault handling. */
        return;
    }

    /* Latch an unready health model when supervision was already armed. */
    if ((control->status.active_faults &
         GUARDIAN_CONTROL_FAULT_HEALTH_NOT_READY) != 0U)
    {
        /* Force safe-off and require a new trained baseline plus explicit reset. */
        guardian_control_latch_fault(
            control,
            GUARDIAN_CONTROL_FAULT_HEALTH_NOT_READY,
            GUARDIAN_CONTROL_REASON_HEALTH_NOT_READY);

        /* Return after fault handling. */
        return;
    }

    /* Latch M8 ALARM immediately while supervision is armed or active. */
    if ((control->status.active_faults &
         GUARDIAN_CONTROL_FAULT_HEALTH_ALARM) != 0U)
    {
        /* Force the safe output and preserve the severe health fault. */
        guardian_control_latch_fault(
            control,
            GUARDIAN_CONTROL_FAULT_HEALTH_ALARM,
            GUARDIAN_CONTROL_REASON_HEALTH_ALARM);

        /* Return after fault handling. */
        return;
    }

    /* Keep output safe while no local machine run request exists. */
    if (control->status.local_run_request == 0U)
    {
        /* Apply the safe output through the configured boundary. */
        if (guardian_control_apply_output(
                control,
                0U) != GUARDIAN_CONTROL_OK)
        {
            /* Return because output handling already published fault state. */
            return;
        }

        /* Return to the armed waiting state. */
        guardian_control_transition(
            control,
            GUARDIAN_CONTROL_STATE_ARMED,
            GUARDIAN_CONTROL_REASON_LOCAL_STOP);

        /* Return with supervision armed but output safe. */
        return;
    }

    /* Apply the local-only run permit after every safety condition passed. */
    if (guardian_control_apply_output(
            control,
            1U) != GUARDIAN_CONTROL_OK)
    {
        /* Return because output handling already published fault state. */
        return;
    }

    /* Mark active operation as degraded while M8 reports WARNING. */
    if (control->status.health_state ==
        GUARDIAN_HEALTH_STATE_WARNING)
    {
        /* Publish degraded active operation while keeping the permit under M9 policy. */
        guardian_control_transition(
            control,
            GUARDIAN_CONTROL_STATE_DEGRADED,
            GUARDIAN_CONTROL_REASON_HEALTH_WARNING);
    }
    else
    {
        /* Publish normal active operation. */
        guardian_control_transition(
            control,
            GUARDIAN_CONTROL_STATE_ACTIVE,
            (control->status.state ==
             GUARDIAN_CONTROL_STATE_DEGRADED)
            ? GUARDIAN_CONTROL_REASON_HEALTH_RECOVERED
            : GUARDIAN_CONTROL_REASON_LOCAL_RUN);
    }
}

/* Initialize supervision disabled with safe output and interlock open. */
void guardian_control_init(
    guardian_control_t *control)
{
    /* Ignore missing caller storage defensively. */
    if (control == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Clear every runtime control field. */
    (void)memset(
        control,
        0,
        sizeof(*control));

    /* Publish the explicit safe initial state. */
    control->status.state =
        GUARDIAN_CONTROL_STATE_DISABLED;

    /* Require a trained health baseline before arming. */
    control->status.health_state =
        GUARDIAN_HEALTH_STATE_UNTRAINED;

    /* Start with neutral health until M8 provides a snapshot. */
    control->status.health_score = 1000U;

    /* Start with no board-specific output adapter. */
    control->status.output_available = 0U;

    /* Start with local interlock open until the application proves otherwise. */
    control->status.interlock_closed = 0U;

    /* Publish initial live safe-entry faults. */
    control->status.active_faults =
        GUARDIAN_CONTROL_FAULT_HEALTH_NOT_READY |
        GUARDIAN_CONTROL_FAULT_INTERLOCK_OPEN |
        GUARDIAN_CONTROL_FAULT_OUTPUT_UNAVAILABLE;
}

/* Install or replace the board-specific run-permit output callback. */
guardian_control_result_t guardian_control_configure_output(
    guardian_control_t *control,
    const guardian_control_output_t *output)
{
    /* Reject missing required storage or callback. */
    if ((control == NULL) ||
        (output == NULL) ||
        (output->apply == NULL))
    {
        /* Report invalid configuration. */
        return GUARDIAN_CONTROL_ERROR_NULL_ARGUMENT;
    }

    /* Copy the immutable output adapter by value. */
    control->output = *output;

    /* Mark the output boundary available before applying safe-off. */
    control->status.output_available = 1U;

    /* Require every new output adapter to accept the safe state immediately. */
    if (control->output.apply(
            control->output.context,
            0U) == 0)
    {
        /* Latch the failed safe-off operation. */
        guardian_control_latch_fault(
            control,
            GUARDIAN_CONTROL_FAULT_OUTPUT_FAILURE,
            GUARDIAN_CONTROL_REASON_OUTPUT_FAILURE);

        /* Report output configuration failure. */
        return GUARDIAN_CONTROL_ERROR_OUTPUT;
    }

    /* Publish the safe logical permit. */
    control->status.run_permit = 0U;

    /* Refresh live faults after installing the adapter. */
    control->status.active_faults =
        guardian_control_active_faults(
            control);

    /* Report successful safe-output configuration. */
    return GUARDIAN_CONTROL_OK;
}

/* Update the local-only machine run request. */
void guardian_control_set_local_run_request(
    guardian_control_t *control,
    uint8_t requested)
{
    /* Ignore missing runtime storage defensively. */
    if (control == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Normalize and publish the local-only run request. */
    control->status.local_run_request =
        (requested != 0U)
        ? 1U
        : 0U;

    /* Re-evaluate supervisory policy immediately. */
    guardian_control_evaluate(
        control);
}

/* Update the local physical/safety interlock state. */
void guardian_control_set_interlock(
    guardian_control_t *control,
    uint8_t closed)
{
    /* Ignore missing runtime storage defensively. */
    if (control == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Normalize and publish the local interlock state. */
    control->status.interlock_closed =
        (closed != 0U)
        ? 1U
        : 0U;

    /* Re-evaluate supervisory policy immediately. */
    guardian_control_evaluate(
        control);
}

/* Consume one current M8 health snapshot and enforce automatic safety policy. */
void guardian_control_update_health(
    guardian_control_t *control,
    const guardian_health_status_t *health)
{
    /* Ignore missing runtime or health storage defensively. */
    if ((control == NULL) ||
        (health == NULL))
    {
        /* Return without touching policy state. */
        return;
    }

    /* Preserve current M8 lifecycle/anomaly state. */
    control->status.health_state =
        health->state;

    /* Preserve current bounded health score. */
    control->status.health_score =
        health->health_score;

    /* Preserve current bounded anomaly severity. */
    control->status.anomaly_score =
        health->anomaly_score;

    /* Re-evaluate supervisory policy immediately. */
    guardian_control_evaluate(
        control);
}

/* Execute one host action that never directly asserts local run request. */
guardian_control_result_t guardian_control_action(
    guardian_control_t *control,
    guardian_control_action_t action)
{
    /* Reject missing runtime storage. */
    if (control == NULL)
    {
        /* Report invalid caller state. */
        return GUARDIAN_CONTROL_ERROR_NULL_ARGUMENT;
    }

    /* Handle safe DISARM first because it is always permitted. */
    if (action ==
        GUARDIAN_CONTROL_ACTION_DISARM)
    {
        /* Remove logical supervision immediately. */
        control->status.supervision_enabled = 0U;

        /* Apply the safe output when an adapter is installed. */
        if (control->output.apply != NULL)
        {
            /* Require safe-off to succeed. */
            if (control->output.apply(
                    control->output.context,
                    0U) == 0)
            {
                /* Latch output failure if safe-off could not be guaranteed. */
                guardian_control_latch_fault(
                    control,
                    GUARDIAN_CONTROL_FAULT_OUTPUT_FAILURE,
                    GUARDIAN_CONTROL_REASON_OUTPUT_FAILURE);

                /* Report output failure. */
                return GUARDIAN_CONTROL_ERROR_OUTPUT;
            }
        }

        /* Publish the safe logical permit. */
        control->status.run_permit = 0U;

        /* Preserve fault-latched state when faults still require explicit reset. */
        if (control->status.latched_faults != 0U)
        {
            /* Publish fault-latched state. */
            guardian_control_transition(
                control,
                GUARDIAN_CONTROL_STATE_FAULT_LATCHED,
                (uint8_t)action);
        }
        else
        {
            /* Publish normal disabled state. */
            guardian_control_transition(
                control,
                GUARDIAN_CONTROL_STATE_DISABLED,
                (uint8_t)action);
        }

        /* Refresh live faults. */
        control->status.active_faults =
            guardian_control_active_faults(
                control);

        /* Report successful safe disarm. */
        return GUARDIAN_CONTROL_OK;
    }

    /* Handle explicit safe fault reset. */
    if (action ==
        GUARDIAN_CONTROL_ACTION_CLEAR_FAULT)
    {
        /* Require supervision to remain disabled during fault reset. */
        if (control->status.supervision_enabled != 0U)
        {
            /* Deny reset while supervisory permission remains armed. */
            return GUARDIAN_CONTROL_ERROR_DENIED;
        }

        /* Require local run request removed before clearing a latch. */
        if (control->status.local_run_request != 0U)
        {
            /* Deny reset while local machine motion is still requested. */
            return GUARDIAN_CONTROL_ERROR_DENIED;
        }

        /* Require a fully READY M8 health model before fault reset. */
        if (control->status.health_state !=
            GUARDIAN_HEALTH_STATE_READY)
        {
            /* Deny reset during WARNING, ALARM or untrained health states. */
            return GUARDIAN_CONTROL_ERROR_DENIED;
        }

        /* Require the local safety interlock to be closed. */
        if (control->status.interlock_closed == 0U)
        {
            /* Deny reset until the local interlock is restored. */
            return GUARDIAN_CONTROL_ERROR_DENIED;
        }

        /* Require a configured safe-output boundary. */
        if (control->output.apply == NULL)
        {
            /* Deny reset without a controllable safe output. */
            return GUARDIAN_CONTROL_ERROR_DENIED;
        }

        /* Require the output boundary to accept safe-off before clearing faults. */
        if (control->output.apply(
                control->output.context,
                0U) == 0)
        {
            /* Preserve or re-latch the output failure. */
            guardian_control_latch_fault(
                control,
                GUARDIAN_CONTROL_FAULT_OUTPUT_FAILURE,
                GUARDIAN_CONTROL_REASON_OUTPUT_FAILURE);

            /* Report output failure. */
            return GUARDIAN_CONTROL_ERROR_OUTPUT;
        }

        /* Clear every latched control fault only after all safe reset conditions passed. */
        control->status.latched_faults = 0U;

        /* Publish the safe logical permit. */
        control->status.run_permit = 0U;

        /* Return to disabled supervision after reset. */
        guardian_control_transition(
            control,
            GUARDIAN_CONTROL_STATE_DISABLED,
            (uint8_t)action);

        /* Refresh live faults after clearing the latch. */
        control->status.active_faults =
            guardian_control_active_faults(
                control);

        /* Report successful explicit fault reset. */
        return GUARDIAN_CONTROL_OK;
    }

    /* Handle ARM only after all safe-entry conditions are proven. */
    if (action ==
        GUARDIAN_CONTROL_ACTION_ARM)
    {
        /* Deny arming when any previous control fault remains latched. */
        if (control->status.latched_faults != 0U)
        {
            /* Require explicit safe fault reset first. */
            return GUARDIAN_CONTROL_ERROR_DENIED;
        }

        /* Require the local machine run request to be absent at the arm boundary. */
        if (control->status.local_run_request != 0U)
        {
            /* Prevent ARM from immediately causing a run permit. */
            return GUARDIAN_CONTROL_ERROR_DENIED;
        }

        /* Require the fully READY trained health state for initial arm. */
        if (control->status.health_state !=
            GUARDIAN_HEALTH_STATE_READY)
        {
            /* Deny initial arm during WARNING, ALARM or baseline learning. */
            return GUARDIAN_CONTROL_ERROR_DENIED;
        }

        /* Require the local safety interlock to be closed. */
        if (control->status.interlock_closed == 0U)
        {
            /* Deny supervision while local safe-entry condition is absent. */
            return GUARDIAN_CONTROL_ERROR_DENIED;
        }

        /* Require a board/application output adapter. */
        if (control->output.apply == NULL)
        {
            /* Deny supervision without an enforceable safe output boundary. */
            return GUARDIAN_CONTROL_ERROR_DENIED;
        }

        /* Require safe-off to succeed before supervision becomes armed. */
        if (guardian_control_apply_output(
                control,
                0U) != GUARDIAN_CONTROL_OK)
        {
            /* Report the output failure generated by the safe-entry check. */
            return GUARDIAN_CONTROL_ERROR_OUTPUT;
        }

        /* Arm supervision without asserting run permit. */
        control->status.supervision_enabled = 1U;

        /* Publish the waiting ARMED state. */
        guardian_control_transition(
            control,
            GUARDIAN_CONTROL_STATE_ARMED,
            (uint8_t)action);

        /* Refresh live faults. */
        control->status.active_faults =
            guardian_control_active_faults(
                control);

        /* Report successful arm. */
        return GUARDIAN_CONTROL_OK;
    }

    /* Reject undefined control actions. */
    return GUARDIAN_CONTROL_ERROR_DENIED;
}

/* Return one immutable current control snapshot by value. */
guardian_control_status_t guardian_control_status(
    const guardian_control_t *control)
{
    /* Create deterministic safe status for a missing pointer. */
    guardian_control_status_t safe = {0};

    /* Handle missing runtime storage safely. */
    if (control == NULL)
    {
        /* Publish explicit disabled state. */
        safe.state =
            GUARDIAN_CONTROL_STATE_DISABLED;

        /* Publish untrained health state. */
        safe.health_state =
            GUARDIAN_HEALTH_STATE_UNTRAINED;

        /* Publish neutral health score. */
        safe.health_score = 1000U;

        /* Publish unavailable output fault. */
        safe.active_faults =
            GUARDIAN_CONTROL_FAULT_OUTPUT_UNAVAILABLE;

        /* Return deterministic safe status. */
        return safe;
    }

    /* Return the bounded current snapshot by value. */
    return control->status;
}

/* Return whether the logical safe-output permit is currently asserted. */
uint8_t guardian_control_run_permit(
    const guardian_control_t *control)
{
    /* Return safe-off for a missing runtime pointer. */
    if (control == NULL)
    {
        /* Publish safe logical output. */
        return 0U;
    }

    /* Return the successfully applied logical permit. */
    return control->status.run_permit;
}

/* Write one unsigned 16-bit integer using Guardian big-endian wire order. */
static void guardian_control_write_u16_be(
    uint8_t *output,
    uint16_t value)
{
    /* Write most-significant byte first. */
    output[0] =
        (uint8_t)((value >> 8U) & 0xFFU);

    /* Write least-significant byte second. */
    output[1] =
        (uint8_t)(value & 0xFFU);
}

/* Write one unsigned 32-bit integer using Guardian big-endian wire order. */
static void guardian_control_write_u32_be(
    uint8_t *output,
    uint32_t value)
{
    /* Write byte 3 first. */
    output[0] =
        (uint8_t)((value >> 24U) & 0xFFU);

    /* Write byte 2. */
    output[1] =
        (uint8_t)((value >> 16U) & 0xFFU);

    /* Write byte 1. */
    output[2] =
        (uint8_t)((value >> 8U) & 0xFFU);

    /* Write byte 0 last. */
    output[3] =
        (uint8_t)(value & 0xFFU);
}

/* Build one request-correlated Guardian ERROR frame. */
static void guardian_control_make_error(
    const guardian_frame_t *request,
    guardian_frame_t *response,
    guardian_error_code_t error)
{
    /* Publish ERROR message semantics. */
    response->message_type =
        GUARDIAN_MESSAGE_ERROR;

    /* Preserve the original command identifier. */
    response->command =
        request->command;

    /* Use protocol v0.1 flags. */
    response->flags =
        GUARDIAN_SUPPORTED_FLAGS;

    /* Preserve request/response correlation. */
    response->sequence =
        request->sequence;

    /* Publish the frozen one-byte error payload. */
    response->payload_length = 1U;

    /* Store the selected Guardian error code. */
    response->payload[0] =
        (uint8_t)error;
}

/* Encode one fixed GET_CONTROL_STATUS payload. */
guardian_protocol_result_t guardian_control_encode_status_payload(
    const guardian_control_status_t *status,
    uint8_t *payload,
    uint16_t payload_capacity,
    uint16_t *payload_length)
{
    /* Reject missing required storage. */
    if ((status == NULL) ||
        (payload == NULL) ||
        (payload_length == NULL))
    {
        /* Report canonical missing-argument failure. */
        return GUARDIAN_PROTOCOL_ERROR_NULL_ARGUMENT;
    }

    /* Require the complete fixed schema output capacity. */
    if (payload_capacity <
        GUARDIAN_CONTROL_STATUS_PAYLOAD_SIZE)
    {
        /* Report bounded output failure. */
        return GUARDIAN_PROTOCOL_ERROR_OUTPUT_TOO_SMALL;
    }

    /* Publish schema revision one. */
    payload[0] =
        GUARDIAN_CONTROL_SCHEMA_VERSION;

    /* Publish current supervisory state. */
    payload[1] =
        (uint8_t)status->state;

    /* Publish whether supervision is armed. */
    payload[2] =
        status->supervision_enabled;

    /* Publish the local-only run request. */
    payload[3] =
        status->local_run_request;

    /* Publish the currently applied logical run permit. */
    payload[4] =
        status->run_permit;

    /* Publish the local interlock state. */
    payload[5] =
        status->interlock_closed;

    /* Publish current M8 health state. */
    payload[6] =
        (uint8_t)status->health_state;

    /* Publish whether a safe-output adapter is installed. */
    payload[7] =
        status->output_available;

    /* Encode latched faults. */
    guardian_control_write_u16_be(
        &payload[8],
        status->latched_faults);

    /* Encode current live faults. */
    guardian_control_write_u16_be(
        &payload[10],
        status->active_faults);

    /* Encode current M8 health score. */
    guardian_control_write_u16_be(
        &payload[12],
        status->health_score);

    /* Encode current M8 anomaly score. */
    guardian_control_write_u16_be(
        &payload[14],
        status->anomaly_score);

    /* Encode monotonic transition count. */
    guardian_control_write_u32_be(
        &payload[16],
        status->transition_count);

    /* Encode monotonic newly latched fault episode count. */
    guardian_control_write_u32_be(
        &payload[20],
        status->fault_latch_count);

    /* Publish last transition action/reason. */
    payload[24] =
        status->last_transition_reason;

    /* Reserve one byte for future compatible semantics. */
    payload[25] = 0U;

    /* Reserve the final two bytes for future compatible semantics. */
    guardian_control_write_u16_be(
        &payload[26],
        0U);

    /* Publish the exact fixed schema length. */
    *payload_length =
        GUARDIAN_CONTROL_STATUS_PAYLOAD_SIZE;

    /* Report successful serialization. */
    return GUARDIAN_PROTOCOL_OK;
}

/* Process GET_CONTROL_STATUS or CONTROL_COMMAND. */
guardian_protocol_result_t guardian_control_handle_request(
    guardian_control_t *control,
    const guardian_frame_t *request,
    guardian_frame_t *response)
{
    /* Reject missing required storage. */
    if ((control == NULL) ||
        (request == NULL) ||
        (response == NULL))
    {
        /* Report canonical missing-argument failure. */
        return GUARDIAN_PROTOCOL_ERROR_NULL_ARGUMENT;
    }

    /* Require host-to-device REQUEST semantics. */
    if (request->message_type !=
        GUARDIAN_MESSAGE_REQUEST)
    {
        /* Reject contradictory message class. */
        guardian_control_make_error(
            request,
            response,
            GUARDIAN_ERROR_MALFORMED_FRAME);

        /* Report successful ERROR frame construction. */
        return GUARDIAN_PROTOCOL_OK;
    }

    /* Handle GET_CONTROL_STATUS. */
    if (request->command ==
        (uint8_t)GUARDIAN_COMMAND_GET_CONTROL_STATUS)
    {
        /* Require the frozen empty request payload. */
        if (request->payload_length != 0U)
        {
            /* Reject undefined status-query bytes. */
            guardian_control_make_error(
                request,
                response,
                GUARDIAN_ERROR_INVALID_PAYLOAD);

            /* Report successful ERROR frame construction. */
            return GUARDIAN_PROTOCOL_OK;
        }

        /* Publish successful response semantics. */
        response->message_type =
            GUARDIAN_MESSAGE_RESPONSE;

        /* Preserve the command identifier. */
        response->command =
            request->command;

        /* Use protocol v0.1 flags. */
        response->flags =
            GUARDIAN_SUPPORTED_FLAGS;

        /* Preserve request/response correlation. */
        response->sequence =
            request->sequence;

        /* Encode the immutable current control snapshot. */
        return guardian_control_encode_status_payload(
            &control->status,
            response->payload,
            GUARDIAN_MAX_PAYLOAD_SIZE,
            &response->payload_length);
    }

    /* Handle host supervisory-control action. */
    if (request->command ==
        (uint8_t)GUARDIAN_COMMAND_CONTROL_COMMAND)
    {
        /* Require the exact schema-and-action request payload. */
        if (request->payload_length !=
            GUARDIAN_CONTROL_COMMAND_REQUEST_SIZE)
        {
            /* Reject truncated or trailing bytes. */
            guardian_control_make_error(
                request,
                response,
                GUARDIAN_ERROR_INVALID_PAYLOAD);

            /* Report successful ERROR frame construction. */
            return GUARDIAN_PROTOCOL_OK;
        }

        /* Require schema revision one. */
        if (request->payload[0] !=
            GUARDIAN_CONTROL_SCHEMA_VERSION)
        {
            /* Reject unsupported control semantics. */
            guardian_control_make_error(
                request,
                response,
                GUARDIAN_ERROR_INVALID_PAYLOAD);

            /* Report successful ERROR frame construction. */
            return GUARDIAN_PROTOCOL_OK;
        }

        /* Decode the explicit host supervisory action. */
        guardian_control_action_t action =
            (guardian_control_action_t)request->payload[1];

        /* Reject undefined action identifiers before evaluating current safety state. */
        if ((action !=
             GUARDIAN_CONTROL_ACTION_ARM) &&
            (action !=
             GUARDIAN_CONTROL_ACTION_DISARM) &&
            (action !=
             GUARDIAN_CONTROL_ACTION_CLEAR_FAULT))
        {
            /* Reject undefined command semantics. */
            guardian_control_make_error(
                request,
                response,
                GUARDIAN_ERROR_INVALID_PAYLOAD);

            /* Report successful ERROR frame construction. */
            return GUARDIAN_PROTOCOL_OK;
        }

        /* Execute the safety-gated action. */
        guardian_control_result_t result =
            guardian_control_action(
                control,
                action);

        /* Map safety-policy denial to the existing BUSY command error. */
        if (result ==
            GUARDIAN_CONTROL_ERROR_DENIED)
        {
            /* Tell the host that current conditions do not permit the action. */
            guardian_control_make_error(
                request,
                response,
                GUARDIAN_ERROR_BUSY);

            /* Report successful ERROR frame construction. */
            return GUARDIAN_PROTOCOL_OK;
        }

        /* Map physical/application output failure to an internal command failure. */
        if (result ==
            GUARDIAN_CONTROL_ERROR_OUTPUT)
        {
            /* Publish an internal failure while fault status remains queryable. */
            guardian_control_make_error(
                request,
                response,
                GUARDIAN_ERROR_INTERNAL);

            /* Report successful ERROR frame construction. */
            return GUARDIAN_PROTOCOL_OK;
        }

        /* Reject undefined actions and impossible internal results. */
        if (result !=
            GUARDIAN_CONTROL_OK)
        {
            /* Reject malformed command semantics. */
            guardian_control_make_error(
                request,
                response,
                GUARDIAN_ERROR_INVALID_PAYLOAD);

            /* Report successful ERROR frame construction. */
            return GUARDIAN_PROTOCOL_OK;
        }

        /* Publish successful response semantics. */
        response->message_type =
            GUARDIAN_MESSAGE_RESPONSE;

        /* Preserve the command identifier. */
        response->command =
            request->command;

        /* Use protocol v0.1 flags. */
        response->flags =
            GUARDIAN_SUPPORTED_FLAGS;

        /* Preserve request/response correlation. */
        response->sequence =
            request->sequence;

        /* Echo the supported schema revision. */
        response->payload[0] =
            GUARDIAN_CONTROL_SCHEMA_VERSION;

        /* Echo the normalized action. */
        response->payload[1] =
            (uint8_t)action;

        /* Publish the resulting supervisory state. */
        response->payload[2] =
            (uint8_t)control->status.state;

        /* Publish the resulting logical run permit. */
        response->payload[3] =
            control->status.run_permit;

        /* Publish the fixed successful response payload length. */
        response->payload_length =
            GUARDIAN_CONTROL_COMMAND_RESPONSE_SIZE;

        /* Report successful response construction. */
        return GUARDIAN_PROTOCOL_OK;
    }

    /* Reject accidental dispatch of another command. */
    guardian_control_make_error(
        request,
        response,
        GUARDIAN_ERROR_UNKNOWN_COMMAND);

    /* Report successful ERROR frame construction. */
    return GUARDIAN_PROTOCOL_OK;
}
