#include "guardian_mcxn947_supervisor.h"

#include <stdio.h>
#include <string.h>

#define TEST_CHECK(condition) \
    do \
    { \
        if (!(condition)) \
        { \
            return __LINE__; \
        } \
    } \
    while (0)

static guardian_multinode_node_t
make_mcxn947_node(void)
{
    guardian_multinode_node_t node;

    node.node_id =
        2U;

    node.producer_id =
        102U;

    node.key_id =
        1002U;

    node.role =
        GUARDIAN_NODE_ROLE_MCXN947_SUPERVISORY_PEER;

    node.authority_class =
        GUARDIAN_AUTHORITY_CLASS_OBSERVE;

    node.capabilities =
        GUARDIAN_CAP_OBSERVATION |
        GUARDIAN_CAP_SUPERVISION |
        GUARDIAN_CAP_EVIDENCE_WITNESS |
        GUARDIAN_CAP_RECOVERY_ASSIST |
        GUARDIAN_CAP_SIGNATURE_VERIFY;

    return node;
}

static guardian_mcxn947_heartbeat_t
make_heartbeat(
    uint32_t sequence)
{
    guardian_mcxn947_heartbeat_t heartbeat;

    heartbeat.sequence =
        sequence;

    heartbeat.epoch =
        1U;

    heartbeat.authenticated =
        1U;

    heartbeat.fresh =
        1U;

    heartbeat.semantically_compatible =
        1U;

    heartbeat.peer_healthy =
        1U;

    heartbeat.disagreement =
        0U;

    return heartbeat;
}

static int test_canonical_health_flags(void)
{
    guardian_mcxn947_supervisor_t supervisor;
    guardian_mcxn947_heartbeat_t heartbeat;
    guardian_multinode_node_t node = make_mcxn947_node();
    unsigned char state_before[sizeof(supervisor)];
    unsigned char input_before[sizeof(heartbeat)];
    unsigned int context;
    unsigned int field;
    unsigned int value;
    unsigned int missed;
    unsigned int healthy;
    unsigned int disagreement;
    unsigned int rejection_count = 0U;
    uint8_t allowed;

    /* Every noncanonical byte, both fields, all four reachable states. */
    for (context = 0U; context < 4U; context++)
    {
        TEST_CHECK(guardian_mcxn947_supervisor_configure(
            &supervisor, &node, 3U) == GUARDIAN_MCXN947_OK);
        if (context != 0U)
        {
            heartbeat = make_heartbeat(1U);
            TEST_CHECK(guardian_mcxn947_supervisor_accept_heartbeat(
                &supervisor, &heartbeat) == GUARDIAN_MCXN947_OK);
            for (missed = 1U; missed < context; missed++)
            {
                TEST_CHECK(guardian_mcxn947_supervisor_note_missed_heartbeat(
                    &supervisor) == GUARDIAN_MCXN947_OK);
            }
            if (context == 3U)
            {
                TEST_CHECK(guardian_mcxn947_supervisor_note_missed_heartbeat(
                    &supervisor) == GUARDIAN_MCXN947_OK);
            }
        }
        TEST_CHECK(supervisor.state == (guardian_mcxn947_state_t)context);
        for (field = 0U; field < 2U; field++)
        {
            for (value = 2U; value <= 255U; value++)
            {
                heartbeat = make_heartbeat(10U);
                heartbeat.epoch = 2U;
                if (field == 0U) { heartbeat.peer_healthy = (uint8_t)value; }
                else { heartbeat.disagreement = (uint8_t)value; }
                (void)memcpy(state_before, &supervisor, sizeof(supervisor));
                (void)memcpy(input_before, &heartbeat, sizeof(heartbeat));
                TEST_CHECK(guardian_mcxn947_supervisor_accept_heartbeat(
                    &supervisor, &heartbeat) ==
                    GUARDIAN_MCXN947_ERROR_INVALID_HEARTBEAT);
                TEST_CHECK(memcmp(state_before, &supervisor,
                    sizeof(supervisor)) == 0);
                TEST_CHECK(memcmp(input_before, &heartbeat,
                    sizeof(heartbeat)) == 0);
                allowed = 1U;
                TEST_CHECK(guardian_mcxn947_supervisor_can_actuate(
                    &supervisor, &allowed) ==
                    GUARDIAN_MCXN947_ERROR_ACTUATION_PROHIBITED);
                TEST_CHECK(allowed == 0U);
                rejection_count++;
            }
        }
        /* Rejected future epoch/sequence must not consume freshness. */
        heartbeat = make_heartbeat(2U);
        TEST_CHECK(guardian_mcxn947_supervisor_accept_heartbeat(
            &supervisor, &heartbeat) == GUARDIAN_MCXN947_OK);
        TEST_CHECK(supervisor.accepted_epoch == 1U);
        TEST_CHECK(supervisor.accepted_sequence == 2U);
    }
    TEST_CHECK(rejection_count == 2032U);

    /* All canonical health/disagreement combinations keep their semantics. */
    for (healthy = 0U; healthy <= 1U; healthy++)
    {
        for (disagreement = 0U; disagreement <= 1U; disagreement++)
        {
            TEST_CHECK(guardian_mcxn947_supervisor_configure(
                &supervisor, &node, 3U) == GUARDIAN_MCXN947_OK);
            heartbeat = make_heartbeat(1U);
            heartbeat.peer_healthy = (uint8_t)healthy;
            heartbeat.disagreement = (uint8_t)disagreement;
            TEST_CHECK(guardian_mcxn947_supervisor_accept_heartbeat(
                &supervisor, &heartbeat) == GUARDIAN_MCXN947_OK);
            TEST_CHECK(supervisor.state ==
                ((healthy == 1U && disagreement == 0U) ?
                 GUARDIAN_MCXN947_STATE_MONITORING :
                 GUARDIAN_MCXN947_STATE_DEGRADED));
        }
    }
    puts("M17_CANONICAL_HEARTBEAT_REJECTIONS=2032");
    return 0;
}

int main(void)
{
    guardian_mcxn947_supervisor_t supervisor;

    guardian_mcxn947_recovery_request_t
        recovery;

    guardian_multinode_node_t node =
        make_mcxn947_node();

    guardian_mcxn947_heartbeat_t heartbeat;

    uint8_t allowed =
        1U;

    TEST_CHECK(test_canonical_health_flags() == 0);

    /*
     * PT01 - deterministic bootstrap.
     */
    guardian_mcxn947_supervisor_init(
        &supervisor);

    TEST_CHECK(
        supervisor.state ==
        GUARDIAN_MCXN947_STATE_BOOTSTRAP);

    /*
     * PT02 - valid MCXN947 configuration.
     */
    TEST_CHECK(
        guardian_mcxn947_supervisor_configure(
            &supervisor,
            &node,
            GUARDIAN_MCXN947_DEFAULT_MISSED_HEARTBEAT_LIMIT) ==
        GUARDIAN_MCXN947_OK);

    /*
     * PT03 - authenticated + fresh + semantically compatible heartbeat.
     */
    heartbeat =
        make_heartbeat(
            1U);

    TEST_CHECK(
        guardian_mcxn947_supervisor_accept_heartbeat(
            &supervisor,
            &heartbeat) ==
        GUARDIAN_MCXN947_OK);

    TEST_CHECK(
        supervisor.state ==
        GUARDIAN_MCXN947_STATE_MONITORING);

    TEST_CHECK(
        supervisor.accepted_sequence ==
        1U);

    /*
     * PT04 - authenticated disagreement degrades supervision.
     */
    heartbeat =
        make_heartbeat(
            2U);

    heartbeat.disagreement =
        1U;

    TEST_CHECK(
        guardian_mcxn947_supervisor_accept_heartbeat(
            &supervisor,
            &heartbeat) ==
        GUARDIAN_MCXN947_OK);

    TEST_CHECK(
        supervisor.state ==
        GUARDIAN_MCXN947_STATE_DEGRADED);

    /*
     * NT01 - unauthenticated heartbeat rejected.
     */
    heartbeat =
        make_heartbeat(
            3U);

    heartbeat.authenticated =
        0U;

    TEST_CHECK(
        guardian_mcxn947_supervisor_accept_heartbeat(
            &supervisor,
            &heartbeat) ==
        GUARDIAN_MCXN947_ERROR_NOT_AUTHENTICATED);

    /*
     * NT02 - stale/not-fresh heartbeat rejected.
     */
    heartbeat =
        make_heartbeat(
            3U);

    heartbeat.fresh =
        0U;

    TEST_CHECK(
        guardian_mcxn947_supervisor_accept_heartbeat(
            &supervisor,
            &heartbeat) ==
        GUARDIAN_MCXN947_ERROR_NOT_FRESH);

    /*
     * NT03 - semantic incompatibility fails closed.
     */
    heartbeat =
        make_heartbeat(
            3U);

    heartbeat.semantically_compatible =
        0U;

    TEST_CHECK(
        guardian_mcxn947_supervisor_accept_heartbeat(
            &supervisor,
            &heartbeat) ==
        GUARDIAN_MCXN947_ERROR_SEMANTICALLY_INCOMPATIBLE);

    /*
     * NT04 - duplicate/regressed sequence rejected.
     */
    heartbeat =
        make_heartbeat(
            2U);

    TEST_CHECK(
        guardian_mcxn947_supervisor_accept_heartbeat(
            &supervisor,
            &heartbeat) ==
        GUARDIAN_MCXN947_ERROR_REPLAY_OR_REGRESSION);

    /*
     * PT05 / NT05 -
     * heartbeat loss moves to SAFE_HOLD.
     *
     * It does not authorize takeover.
     */
    TEST_CHECK(
        guardian_mcxn947_supervisor_note_missed_heartbeat(
            &supervisor) ==
        GUARDIAN_MCXN947_OK);

    TEST_CHECK(
        guardian_mcxn947_supervisor_note_missed_heartbeat(
            &supervisor) ==
        GUARDIAN_MCXN947_OK);

    TEST_CHECK(
        guardian_mcxn947_supervisor_note_missed_heartbeat(
            &supervisor) ==
        GUARDIAN_MCXN947_OK);

    TEST_CHECK(
        supervisor.state ==
        GUARDIAN_MCXN947_STATE_SAFE_HOLD);

    TEST_CHECK(
        supervisor.recovery_requested ==
        1U);

    /*
     * PT06 -
     * recovery request can be emitted.
     */
    TEST_CHECK(
        guardian_mcxn947_supervisor_build_recovery_request(
            &supervisor,
            &recovery) ==
        GUARDIAN_MCXN947_OK);

    TEST_CHECK(
        recovery.requested ==
        1U);

    /*
     * NT06 -
     * recovery request is never an actuation authorization.
     */
    TEST_CHECK(
        recovery.actuation_authorized ==
        0U);

    /*
     * NT07 -
     * supervisor has no direct actuation authority.
     */
    allowed =
        1U;

    TEST_CHECK(
        guardian_mcxn947_supervisor_can_actuate(
            &supervisor,
            &allowed) ==
        GUARDIAN_MCXN947_ERROR_ACTUATION_PROHIBITED);

    TEST_CHECK(
        allowed ==
        0U);

    /*
     * NT08 -
     * configuration fails closed if MCXN947 tries to acquire the
     * F401-only actuation capability.
     */
    node.capabilities |=
        GUARDIAN_CAP_ACTUATION_GATE;

    TEST_CHECK(
        guardian_mcxn947_supervisor_configure(
            &supervisor,
            &node,
            GUARDIAN_MCXN947_DEFAULT_MISSED_HEARTBEAT_LIMIT) ==
        GUARDIAN_MCXN947_ERROR_INVALID_NODE);

    puts(
        "M17_MCXN947_SUPERVISOR_HOST_TEST=PASS");

    return 0;
}
