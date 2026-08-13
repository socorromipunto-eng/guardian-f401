/* Include the portable M9 supervisory-control engine under test. */
#include "guardian_control.h"

/* Include assertion support for deterministic host verification. */
#include <assert.h>

/* Include standard output for one concise CI success line. */
#include <stdio.h>

/* Store one fake board/application output boundary for tests. */
typedef struct
{
    /* Store the last successfully requested logical permit. */
    uint8_t last_permit;

    /* Count output callback calls. */
    uint32_t calls;

    /* Force callback failure when non-zero. */
    uint8_t fail;
} test_output_t;

/* Apply one fake logical output. */
static int test_output_apply(
    void *context,
    uint8_t run_permit)
{
    /* Recover the deterministic fake output state. */
    test_output_t *output =
        (test_output_t *)context;

    /* Reject missing fake output storage. */
    assert(output != NULL);

    /* Count every attempted output application. */
    output->calls += 1U;

    /* Simulate hardware/application failure when requested by the test. */
    if (output->fail != 0U)
    {
        /* Report output application failure. */
        return 0;
    }

    /* Preserve the normalized logical permit. */
    output->last_permit =
        (run_permit != 0U)
        ? 1U
        : 0U;

    /* Report successful application. */
    return 1;
}

/* Return one deterministic M8 health snapshot. */
static guardian_health_status_t test_health(
    guardian_health_state_t state,
    uint16_t health_score,
    uint16_t anomaly_score)
{
    /* Create zero-initialized health storage. */
    guardian_health_status_t health = {0};

    /* Publish requested health state. */
    health.state = state;

    /* Publish bounded health score. */
    health.health_score =
        health_score;

    /* Publish bounded anomaly score. */
    health.anomaly_score =
        anomaly_score;

    /* Return the complete M8 snapshot by value. */
    return health;
}

/* Configure one fake output boundary and require safe-off application. */
static void test_configure_output(
    guardian_control_t *control,
    test_output_t *output)
{
    /* Build the fake output adapter. */
    guardian_control_output_t adapter = {0};

    /* Connect the fake callback. */
    adapter.apply =
        test_output_apply;

    /* Pass fake output storage back to the callback. */
    adapter.context =
        output;

    /* Require successful safe-output configuration. */
    assert(
        guardian_control_configure_output(
            control,
            &adapter) ==
        GUARDIAN_CONTROL_OK);

    /* Require immediate safe-off application. */
    assert(output->last_permit == 0U);
}

/* Verify startup is disabled and cannot arm without proven safe-entry conditions. */
static void test_safe_startup_and_arm_gate(void)
{
    /* Create fresh control state. */
    guardian_control_t control = {0};

    /* Create fake output state. */
    test_output_t output = {0};

    /* Initialize M9 supervision. */
    guardian_control_init(
        &control);

    /* Require explicit disabled startup. */
    assert(
        control.status.state ==
        GUARDIAN_CONTROL_STATE_DISABLED);

    /* Require safe logical output. */
    assert(
        control.status.run_permit ==
        0U);

    /* Require output unavailability before configuration. */
    assert(
        (control.status.active_faults &
         GUARDIAN_CONTROL_FAULT_OUTPUT_UNAVAILABLE) != 0U);

    /* Install the fake safe-output boundary. */
    test_configure_output(
        &control,
        &output);

    /* Require ARM denial before a trained health baseline exists. */
    assert(
        guardian_control_action(
            &control,
            GUARDIAN_CONTROL_ACTION_ARM) ==
        GUARDIAN_CONTROL_ERROR_DENIED);

    /* Publish a fully READY M8 health snapshot. */
    guardian_health_status_t health =
        test_health(
            GUARDIAN_HEALTH_STATE_READY,
            1000U,
            0U);

    /* Update health input. */
    guardian_control_update_health(
        &control,
        &health);

    /* Require ARM denial while local interlock remains open. */
    assert(
        guardian_control_action(
            &control,
            GUARDIAN_CONTROL_ACTION_ARM) ==
        GUARDIAN_CONTROL_ERROR_DENIED);

    /* Close the local safety interlock. */
    guardian_control_set_interlock(
        &control,
        1U);

    /* Require ARM to succeed with every safe-entry condition satisfied. */
    assert(
        guardian_control_action(
            &control,
            GUARDIAN_CONTROL_ACTION_ARM) ==
        GUARDIAN_CONTROL_OK);

    /* Require ARMED state without run permit. */
    assert(
        control.status.state ==
        GUARDIAN_CONTROL_STATE_ARMED);

    /* Require host ARM never to assert machine run permit directly. */
    assert(
        control.status.run_permit ==
        0U);

    /* Require the fake output to remain safe-off. */
    assert(
        output.last_permit ==
        0U);
}

/* Verify local run request drives ACTIVE and WARNING drives DEGRADED without host start command. */
static void test_local_run_and_degraded_policy(void)
{
    /* Create fresh control state. */
    guardian_control_t control = {0};

    /* Create fake output state. */
    test_output_t output = {0};

    /* Initialize supervision. */
    guardian_control_init(
        &control);

    /* Install the fake output boundary. */
    test_configure_output(
        &control,
        &output);

    /* Close local interlock. */
    guardian_control_set_interlock(
        &control,
        1U);

    /* Publish READY health. */
    guardian_health_status_t ready =
        test_health(
            GUARDIAN_HEALTH_STATE_READY,
            1000U,
            0U);

    /* Update health input. */
    guardian_control_update_health(
        &control,
        &ready);

    /* Arm supervision. */
    assert(
        guardian_control_action(
            &control,
            GUARDIAN_CONTROL_ACTION_ARM) ==
        GUARDIAN_CONTROL_OK);

    /* Assert the local-only run request. */
    guardian_control_set_local_run_request(
        &control,
        1U);

    /* Require normal active state. */
    assert(
        control.status.state ==
        GUARDIAN_CONTROL_STATE_ACTIVE);

    /* Require run permit to be applied only after the local request. */
    assert(
        control.status.run_permit ==
        1U);

    /* Require the fake output boundary to observe the permit. */
    assert(
        output.last_permit ==
        1U);

    /* Publish warning-level M8 health. */
    guardian_health_status_t warning =
        test_health(
            GUARDIAN_HEALTH_STATE_WARNING,
            600U,
            400U);

    /* Update health input. */
    guardian_control_update_health(
        &control,
        &warning);

    /* Require explicit DEGRADED state under M9 warning policy. */
    assert(
        control.status.state ==
        GUARDIAN_CONTROL_STATE_DEGRADED);

    /* Require warning policy to preserve the active permit. */
    assert(
        control.status.run_permit ==
        1U);

    /* Remove the local run request. */
    guardian_control_set_local_run_request(
        &control,
        0U);

    /* Require return to ARMED. */
    assert(
        control.status.state ==
        GUARDIAN_CONTROL_STATE_ARMED);

    /* Require safe output after local stop. */
    assert(
        control.status.run_permit ==
        0U);
}

/* Verify M8 ALARM forces safe-off and requires explicit safe fault reset. */
static void test_health_alarm_latches_and_recovers(void)
{
    /* Create fresh control state. */
    guardian_control_t control = {0};

    /* Create fake output state. */
    test_output_t output = {0};

    /* Initialize and configure supervision. */
    guardian_control_init(
        &control);

    /* Install safe output boundary. */
    test_configure_output(
        &control,
        &output);

    /* Close local interlock. */
    guardian_control_set_interlock(
        &control,
        1U);

    /* Publish READY health. */
    guardian_health_status_t ready =
        test_health(
            GUARDIAN_HEALTH_STATE_READY,
            1000U,
            0U);

    /* Update READY health. */
    guardian_control_update_health(
        &control,
        &ready);

    /* Arm supervision. */
    assert(
        guardian_control_action(
            &control,
            GUARDIAN_CONTROL_ACTION_ARM) ==
        GUARDIAN_CONTROL_OK);

    /* Request local machine run. */
    guardian_control_set_local_run_request(
        &control,
        1U);

    /* Publish M8 ALARM. */
    guardian_health_status_t alarm =
        test_health(
            GUARDIAN_HEALTH_STATE_ALARM,
            0U,
            1000U);

    /* Enforce automatic health-driven policy. */
    guardian_control_update_health(
        &control,
        &alarm);

    /* Require latched fault state. */
    assert(
        control.status.state ==
        GUARDIAN_CONTROL_STATE_FAULT_LATCHED);

    /* Require immediate logical safe-off. */
    assert(
        control.status.run_permit ==
        0U);

    /* Require physical/application fake output safe-off. */
    assert(
        output.last_permit ==
        0U);

    /* Require the health-alarm fault bit to latch. */
    assert(
        (control.status.latched_faults &
         GUARDIAN_CONTROL_FAULT_HEALTH_ALARM) != 0U);

    /* Require fault reset denial while local run request remains asserted. */
    assert(
        guardian_control_action(
            &control,
            GUARDIAN_CONTROL_ACTION_CLEAR_FAULT) ==
        GUARDIAN_CONTROL_ERROR_DENIED);

    /* Remove the local run request. */
    guardian_control_set_local_run_request(
        &control,
        0U);

    /* Restore fully READY health. */
    guardian_control_update_health(
        &control,
        &ready);

    /* Require explicit safe fault reset to succeed now. */
    assert(
        guardian_control_action(
            &control,
            GUARDIAN_CONTROL_ACTION_CLEAR_FAULT) ==
        GUARDIAN_CONTROL_OK);

    /* Require reset to return to disabled state. */
    assert(
        control.status.state ==
        GUARDIAN_CONTROL_STATE_DISABLED);

    /* Require every latched fault to clear. */
    assert(
        control.status.latched_faults ==
        0U);

    /* Require run permit to remain safe-off after reset. */
    assert(
        control.status.run_permit ==
        0U);
}

/* Verify an interlock opening while active immediately latches safe-off. */
static void test_interlock_fault_latches(void)
{
    /* Create fresh control state. */
    guardian_control_t control = {0};

    /* Create fake output state. */
    test_output_t output = {0};

    /* Initialize and configure supervision. */
    guardian_control_init(
        &control);

    /* Install safe output boundary. */
    test_configure_output(
        &control,
        &output);

    /* Close local interlock. */
    guardian_control_set_interlock(
        &control,
        1U);

    /* Publish READY health. */
    guardian_health_status_t ready =
        test_health(
            GUARDIAN_HEALTH_STATE_READY,
            1000U,
            0U);

    /* Update health input. */
    guardian_control_update_health(
        &control,
        &ready);

    /* Arm supervision. */
    assert(
        guardian_control_action(
            &control,
            GUARDIAN_CONTROL_ACTION_ARM) ==
        GUARDIAN_CONTROL_OK);

    /* Request local run. */
    guardian_control_set_local_run_request(
        &control,
        1U);

    /* Open the local safety interlock. */
    guardian_control_set_interlock(
        &control,
        0U);

    /* Require immediate latched fault. */
    assert(
        control.status.state ==
        GUARDIAN_CONTROL_STATE_FAULT_LATCHED);

    /* Require safe-off. */
    assert(
        control.status.run_permit ==
        0U);

    /* Require the interlock fault bit to latch. */
    assert(
        (control.status.latched_faults &
         GUARDIAN_CONTROL_FAULT_INTERLOCK_OPEN) != 0U);
}

/* Verify loss of M8 baseline readiness while armed is fail-safe. */
static void test_health_readiness_loss_latches(void)
{
    /* Create fresh control state. */
    guardian_control_t control = {0};

    /* Create fake output state. */
    test_output_t output = {0};

    /* Initialize and configure output. */
    guardian_control_init(
        &control);

    /* Install output boundary. */
    test_configure_output(
        &control,
        &output);

    /* Close local interlock. */
    guardian_control_set_interlock(
        &control,
        1U);

    /* Publish READY health. */
    guardian_health_status_t ready =
        test_health(
            GUARDIAN_HEALTH_STATE_READY,
            1000U,
            0U);

    /* Update health state. */
    guardian_control_update_health(
        &control,
        &ready);

    /* Arm supervision. */
    assert(
        guardian_control_action(
            &control,
            GUARDIAN_CONTROL_ACTION_ARM) ==
        GUARDIAN_CONTROL_OK);

    /* Simulate baseline reset/start making M8 LEARNING. */
    guardian_health_status_t learning =
        test_health(
            GUARDIAN_HEALTH_STATE_LEARNING,
            1000U,
            0U);

    /* Enforce new health input. */
    guardian_control_update_health(
        &control,
        &learning);

    /* Require fail-safe latch. */
    assert(
        control.status.state ==
        GUARDIAN_CONTROL_STATE_FAULT_LATCHED);

    /* Require health-not-ready fault. */
    assert(
        (control.status.latched_faults &
         GUARDIAN_CONTROL_FAULT_HEALTH_NOT_READY) != 0U);
}

/* Verify output failure is latched when active permit application fails. */
static void test_output_failure_latches(void)
{
    /* Create fresh control state. */
    guardian_control_t control = {0};

    /* Create fake output state. */
    test_output_t output = {0};

    /* Initialize and configure output. */
    guardian_control_init(
        &control);

    /* Install output boundary while it is healthy. */
    test_configure_output(
        &control,
        &output);

    /* Close local interlock. */
    guardian_control_set_interlock(
        &control,
        1U);

    /* Publish READY health. */
    guardian_health_status_t ready =
        test_health(
            GUARDIAN_HEALTH_STATE_READY,
            1000U,
            0U);

    /* Update health input. */
    guardian_control_update_health(
        &control,
        &ready);

    /* Arm supervision. */
    assert(
        guardian_control_action(
            &control,
            GUARDIAN_CONTROL_ACTION_ARM) ==
        GUARDIAN_CONTROL_OK);

    /* Force the next output application to fail. */
    output.fail = 1U;

    /* Assert local run request so M9 attempts active permit. */
    guardian_control_set_local_run_request(
        &control,
        1U);

    /* Require latched fault state. */
    assert(
        control.status.state ==
        GUARDIAN_CONTROL_STATE_FAULT_LATCHED);

    /* Require logical safe-off despite the callback failure. */
    assert(
        control.status.run_permit ==
        0U);

    /* Require explicit output-failure latch. */
    assert(
        (control.status.latched_faults &
         GUARDIAN_CONTROL_FAULT_OUTPUT_FAILURE) != 0U);
}

/* Verify M9 protocol status and action response framing. */
static void test_control_protocol_handler(void)
{
    /* Create fresh control state. */
    guardian_control_t control = {0};

    /* Create fake output state. */
    test_output_t output = {0};

    /* Create one host request frame. */
    guardian_frame_t request = {0};

    /* Create response storage. */
    guardian_frame_t response = {0};

    /* Initialize and configure safe conditions. */
    guardian_control_init(
        &control);

    /* Install output boundary. */
    test_configure_output(
        &control,
        &output);

    /* Close interlock. */
    guardian_control_set_interlock(
        &control,
        1U);

    /* Publish READY health. */
    guardian_health_status_t ready =
        test_health(
            GUARDIAN_HEALTH_STATE_READY,
            1000U,
            0U);

    /* Update M9 health input. */
    guardian_control_update_health(
        &control,
        &ready);

    /* Build GET_CONTROL_STATUS. */
    request.message_type =
        GUARDIAN_MESSAGE_REQUEST;

    /* Select M9 status command. */
    request.command =
        (uint8_t)GUARDIAN_COMMAND_GET_CONTROL_STATUS;

    /* Publish correlation sequence. */
    request.sequence = 900U;

    /* Require successful status response. */
    assert(
        guardian_control_handle_request(
            &control,
            &request,
            &response) ==
        GUARDIAN_PROTOCOL_OK);

    /* Require successful response class. */
    assert(
        response.message_type ==
        GUARDIAN_MESSAGE_RESPONSE);

    /* Require exact fixed status payload length. */
    assert(
        response.payload_length ==
        GUARDIAN_CONTROL_STATUS_PAYLOAD_SIZE);

    /* Build CONTROL_COMMAND ARM. */
    request.command =
        (uint8_t)GUARDIAN_COMMAND_CONTROL_COMMAND;

    /* Update correlation sequence. */
    request.sequence = 901U;

    /* Publish exact two-byte request. */
    request.payload_length =
        GUARDIAN_CONTROL_COMMAND_REQUEST_SIZE;

    /* Publish schema revision one. */
    request.payload[0] =
        GUARDIAN_CONTROL_SCHEMA_VERSION;

    /* Select ARM. */
    request.payload[1] =
        (uint8_t)GUARDIAN_CONTROL_ACTION_ARM;

    /* Execute ARM through protocol handler. */
    assert(
        guardian_control_handle_request(
            &control,
            &request,
            &response) ==
        GUARDIAN_PROTOCOL_OK);

    /* Require successful command response. */
    assert(
        response.message_type ==
        GUARDIAN_MESSAGE_RESPONSE);

    /* Require normalized action echo. */
    assert(
        response.payload[1] ==
        (uint8_t)GUARDIAN_CONTROL_ACTION_ARM);

    /* Require resulting ARMED state. */
    assert(
        response.payload[2] ==
        (uint8_t)GUARDIAN_CONTROL_STATE_ARMED);

    /* Require host ARM not to assert run permit. */
    assert(
        response.payload[3] ==
        0U);
}

/* Execute every portable M9 supervisory-control test. */
int main(void)
{
    /* Verify safe startup and arm gating. */
    test_safe_startup_and_arm_gate();

    /* Verify local-only run request and degraded warning policy. */
    test_local_run_and_degraded_policy();

    /* Verify health alarm fault latch and explicit recovery. */
    test_health_alarm_latches_and_recovers();

    /* Verify local interlock fault latch. */
    test_interlock_fault_latches();

    /* Verify baseline readiness loss fails safe. */
    test_health_readiness_loss_latches();

    /* Verify output application failure fails safe. */
    test_output_failure_latches();

    /* Verify M9 wire command handling. */
    test_control_protocol_handler();

    /* Print one concise success line for local and CI logs. */
    (void)printf("Guardian M9 control host tests: PASS\n");

    /* Return conventional successful process status. */
    return 0;
}
