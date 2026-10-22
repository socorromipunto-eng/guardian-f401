#include "guardian_mcxn947_supervisor.h"

#include <limits.h>
#include <string.h>

void guardian_mcxn947_supervisor_init(
    guardian_mcxn947_supervisor_t *supervisor)
{
    if (supervisor == NULL)
    {
        return;
    }

    (void)memset(
        supervisor,
        0,
        sizeof(*supervisor));

    supervisor->state =
        GUARDIAN_MCXN947_STATE_BOOTSTRAP;
}

guardian_mcxn947_result_t
guardian_mcxn947_supervisor_configure(
    guardian_mcxn947_supervisor_t *supervisor,
    const guardian_multinode_node_t *self,
    uint32_t missed_heartbeat_limit)
{
    guardian_multinode_result_t
        node_result;

    if ((supervisor == NULL) ||
        (self == NULL))
    {
        return
            GUARDIAN_MCXN947_ERROR_NULL_ARGUMENT;
    }

    if (missed_heartbeat_limit == 0U)
    {
        return
            GUARDIAN_MCXN947_ERROR_INVALID_CONFIG;
    }

    node_result =
        guardian_multinode_validate_node(
            self);

    if (node_result !=
        GUARDIAN_MULTINODE_OK)
    {
        return
            GUARDIAN_MCXN947_ERROR_INVALID_NODE;
    }

    if (self->role !=
        GUARDIAN_NODE_ROLE_MCXN947_SUPERVISORY_PEER)
    {
        return
            GUARDIAN_MCXN947_ERROR_INVALID_NODE;
    }

    if (self->authority_class !=
        GUARDIAN_AUTHORITY_CLASS_OBSERVE)
    {
        return
            GUARDIAN_MCXN947_ERROR_INVALID_NODE;
    }

    if ((self->capabilities &
         GUARDIAN_CAP_SUPERVISION) == 0U)
    {
        return
            GUARDIAN_MCXN947_ERROR_INVALID_NODE;
    }

    if ((self->capabilities &
         GUARDIAN_CAP_RECOVERY_ASSIST) == 0U)
    {
        return
            GUARDIAN_MCXN947_ERROR_INVALID_NODE;
    }

    if ((self->capabilities &
         GUARDIAN_CAP_ACTUATION_GATE) != 0U)
    {
        return
            GUARDIAN_MCXN947_ERROR_INVALID_NODE;
    }

    guardian_mcxn947_supervisor_init(
        supervisor);

    supervisor->self =
        *self;

    supervisor->missed_heartbeat_limit =
        missed_heartbeat_limit;

    return GUARDIAN_MCXN947_OK;
}

guardian_mcxn947_result_t
guardian_mcxn947_supervisor_accept_heartbeat(
    guardian_mcxn947_supervisor_t *supervisor,
    const guardian_mcxn947_heartbeat_t *heartbeat)
{
    if ((supervisor == NULL) ||
        (heartbeat == NULL))
    {
        return
            GUARDIAN_MCXN947_ERROR_NULL_ARGUMENT;
    }

    if (supervisor->missed_heartbeat_limit == 0U)
    {
        return
            GUARDIAN_MCXN947_ERROR_INVALID_CONFIG;
    }

    /* Reject noncanonical health/disagreement before any state write. */
    if ((heartbeat->peer_healthy > 1U) ||
        (heartbeat->disagreement > 1U))
    {
        return GUARDIAN_MCXN947_ERROR_INVALID_HEARTBEAT;
    }

    /*
     * The supervisor does not create authentication.
     *
     * It may consume only a heartbeat that a lower layer reports
     * authenticated.
     */
    if (heartbeat->authenticated != 1U)
    {
        return
            GUARDIAN_MCXN947_ERROR_NOT_AUTHENTICATED;
    }

    /*
     * Authentication alone is insufficient.
     */
    if (heartbeat->fresh != 1U)
    {
        return
            GUARDIAN_MCXN947_ERROR_NOT_FRESH;
    }

    /*
     * Protocol-valid bytes are insufficient if semantic compatibility has
     * not been established.
     */
    if (heartbeat->semantically_compatible != 1U)
    {
        return
            GUARDIAN_MCXN947_ERROR_SEMANTICALLY_INCOMPATIBLE;
    }

    if ((heartbeat->epoch == 0U) ||
        (heartbeat->sequence == 0U))
    {
        return
            GUARDIAN_MCXN947_ERROR_REPLAY_OR_REGRESSION;
    }

    if (supervisor->heartbeat_initialized != 0U)
    {
        if (heartbeat->epoch <
            supervisor->accepted_epoch)
        {
            return
                GUARDIAN_MCXN947_ERROR_REPLAY_OR_REGRESSION;
        }

        if ((heartbeat->epoch ==
                supervisor->accepted_epoch) &&
            (heartbeat->sequence <=
                supervisor->accepted_sequence))
        {
            return
                GUARDIAN_MCXN947_ERROR_REPLAY_OR_REGRESSION;
        }
    }

    supervisor->accepted_epoch =
        heartbeat->epoch;

    supervisor->accepted_sequence =
        heartbeat->sequence;

    supervisor->heartbeat_initialized =
        1U;

    supervisor->missed_heartbeats =
        0U;

    supervisor->recovery_requested =
        0U;

    /*
     * Authenticated + fresh does not imply truthful.
     *
     * The peer's health assertion and disagreement indicator remain policy
     * inputs. They do not grant authority.
     */
    if ((heartbeat->peer_healthy != 1U) ||
        (heartbeat->disagreement == 1U))
    {
        supervisor->state =
            GUARDIAN_MCXN947_STATE_DEGRADED;
    }
    else
    {
        supervisor->state =
            GUARDIAN_MCXN947_STATE_MONITORING;
    }

    return GUARDIAN_MCXN947_OK;
}

guardian_mcxn947_result_t
guardian_mcxn947_supervisor_note_missed_heartbeat(
    guardian_mcxn947_supervisor_t *supervisor)
{
    if (supervisor == NULL)
    {
        return
            GUARDIAN_MCXN947_ERROR_NULL_ARGUMENT;
    }

    if (supervisor->missed_heartbeat_limit == 0U)
    {
        return
            GUARDIAN_MCXN947_ERROR_INVALID_CONFIG;
    }

    if (supervisor->missed_heartbeats <
        UINT32_MAX)
    {
        supervisor->missed_heartbeats +=
            1U;
    }

    if (supervisor->missed_heartbeats >=
        supervisor->missed_heartbeat_limit)
    {
        /*
         * Silence does not authorize takeover.
         *
         * The only automatic response is SAFE_HOLD plus a bounded recovery
         * request.
         */
        supervisor->state =
            GUARDIAN_MCXN947_STATE_SAFE_HOLD;

        supervisor->recovery_requested =
            1U;
    }
    else if (supervisor->state ==
        GUARDIAN_MCXN947_STATE_MONITORING)
    {
        supervisor->state =
            GUARDIAN_MCXN947_STATE_DEGRADED;
    }

    return GUARDIAN_MCXN947_OK;
}

guardian_mcxn947_result_t
guardian_mcxn947_supervisor_build_recovery_request(
    const guardian_mcxn947_supervisor_t *supervisor,
    guardian_mcxn947_recovery_request_t *request)
{
    if ((supervisor == NULL) ||
        (request == NULL))
    {
        return
            GUARDIAN_MCXN947_ERROR_NULL_ARGUMENT;
    }

    (void)memset(
        request,
        0,
        sizeof(*request));

    request->requesting_node_id =
        supervisor->self.node_id;

    request->observed_epoch =
        supervisor->accepted_epoch;

    request->observed_sequence =
        supervisor->accepted_sequence;

    request->requested =
        supervisor->recovery_requested;

    /*
     * Hard invariant:
     *
     * recovery request != authorization.
     */
    request->actuation_authorized =
        0U;

    return GUARDIAN_MCXN947_OK;
}

guardian_mcxn947_result_t
guardian_mcxn947_supervisor_can_actuate(
    const guardian_mcxn947_supervisor_t *supervisor,
    uint8_t *allowed)
{
    if ((supervisor == NULL) ||
        (allowed == NULL))
    {
        return
            GUARDIAN_MCXN947_ERROR_NULL_ARGUMENT;
    }

    /*
     * MCXN947 supervision never becomes physical actuation authority.
     */
    *allowed =
        0U;

    return
        GUARDIAN_MCXN947_ERROR_ACTUATION_PROHIBITED;
}
