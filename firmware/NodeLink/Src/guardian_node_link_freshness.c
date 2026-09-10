#include "guardian_node_link_freshness.h"

#include <string.h>

static int guardian_node_link_freshness_ref_valid(
    const char *reference,
    size_t capacity)
{
    size_t length = 0U;

    if ((reference == NULL) || (capacity == 0U))
    {
        return 0;
    }

    while ((length < capacity) &&
           (reference[length] != '\0'))
    {
        length += 1U;
    }

    if ((length == 0U) ||
        (length >= capacity))
    {
        return 0;
    }

    return 1;
}

static int guardian_node_link_freshness_condition_valid(
    guardian_node_link_freshness_condition_t condition)
{
    switch (condition)
    {
        case GUARDIAN_NODE_LINK_FRESHNESS_UNINITIALIZED:
        case GUARDIAN_NODE_LINK_FRESHNESS_ACTIVE:
        case GUARDIAN_NODE_LINK_FRESHNESS_UNKNOWN:
        case GUARDIAN_NODE_LINK_FRESHNESS_REJOIN_REQUIRED:
        case GUARDIAN_NODE_LINK_FRESHNESS_SESSION_REPLACEMENT_REQUIRED:
            return 1;

        default:
            return 0;
    }
}

void guardian_node_link_freshness_runtime_init(
    guardian_node_link_freshness_runtime_t *runtime)
{
    if (runtime == NULL)
    {
        return;
    }

    /*
     * Zero initialization is deliberately fail closed.
     *
     * No identity, observation, accepted epoch, or accepted sequence survives
     * initialization.
     */
    (void)memset(runtime, 0, sizeof(*runtime));

    runtime->condition =
        GUARDIAN_NODE_LINK_FRESHNESS_UNINITIALIZED;
}

guardian_node_link_freshness_result_t
guardian_node_link_freshness_runtime_validate(
    const guardian_node_link_freshness_runtime_t *runtime)
{
    if (runtime == NULL)
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT;
    }

    if (guardian_node_link_freshness_condition_valid(
            runtime->condition) == 0)
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_CONDITION;
    }

    /*
     * Validity indicators are bounded booleans.
     *
     * Values outside {0,1} are malformed state rather than aliases for true.
     */
    if ((runtime->identity_valid > 1U) ||
        (runtime->observation_valid > 1U) ||
        (runtime->accepted_epoch_valid > 1U) ||
        (runtime->accepted_sequence_valid > 1U))
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
    }

    /*
     * Observed or accepted freshness state has meaning only when it remains
     * bound to the authenticated + pre-freshness-compatible peer identity.
     *
     * Freshness state without identity is structurally invalid and must fail
     * closed.
     */
    if (((runtime->observation_valid != 0U) ||
         (runtime->accepted_epoch_valid != 0U) ||
         (runtime->accepted_sequence_valid != 0U)) &&
        (runtime->identity_valid == 0U))
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
    }

    /*
     * identity_valid means the runtime object contains a structurally valid
     * snapshot of the already authenticated and pre-freshness-compatible
     * identity context.
     *
     * This is structural validation only.
     *
     * It does not re-authenticate the peer and does not establish freshness.
     */
    if (runtime->identity_valid != 0U)
    {
        if ((runtime->identity.sender_node_id == 0U) ||
            (runtime->identity.producer_id == 0U) ||
            (runtime->identity.key_id == 0U) ||
            (runtime->identity.signature_algorithm !=
                GUARDIAN_NODE_LINK_SIGNATURE_ED25519))
        {
            return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
        }

        if (guardian_node_link_freshness_ref_valid(
                runtime->identity.producer_semantic_profile_id,
                sizeof(runtime->identity.producer_semantic_profile_id)) == 0)
        {
            return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
        }

        if (guardian_node_link_freshness_ref_valid(
                runtime->identity.consumer_semantic_profile_id,
                sizeof(runtime->identity.consumer_semantic_profile_id)) == 0)
        {
            return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
        }

        if (guardian_node_link_freshness_ref_valid(
                runtime->identity.compatibility_contract_id,
                sizeof(runtime->identity.compatibility_contract_id)) == 0)
        {
            return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
        }
    }

    /*
     * A valid observation must represent a structurally valid NodeLink
     * sequence. NodeLink sequence zero is already invalid at the wire layer
     * and remains invalid here.
     */
    if ((runtime->observation_valid != 0U) &&
        (runtime->observed_sequence == 0U))
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
    }

    /*
     * If accepted sequence state is declared valid, zero may not be treated
     * as an accepted sequence.
     */
    if ((runtime->accepted_sequence_valid != 0U) &&
        (runtime->accepted_sequence == 0U))
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
    }
    /*
     * UNINITIALIZED is a fail-closed baseline.
     *
     * It must not simultaneously claim accepted freshness state.
     */
    if (runtime->condition ==
        GUARDIAN_NODE_LINK_FRESHNESS_UNINITIALIZED)
    {
        if ((runtime->accepted_epoch_valid != 0U) ||
            (runtime->accepted_sequence_valid != 0U))
        {
            return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
        }

        return GUARDIAN_NODE_LINK_FRESHNESS_OK;
    }

    /*
     * An accepted sequence is meaningless without an accepted epoch.
     */
    if ((runtime->accepted_sequence_valid != 0U) &&
        (runtime->accepted_epoch_valid == 0U))
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
    }

    /*
     * ACTIVE is structurally meaningful only when the future governed R3
     * evaluator has established an identity, an accepted epoch, and an
     * accepted non-zero sequence.
     *
     * C5-R3A intentionally contains no API that performs that promotion.
     */
    if (runtime->condition ==
        GUARDIAN_NODE_LINK_FRESHNESS_ACTIVE)
    {
        if ((runtime->identity_valid == 0U) ||
            (runtime->accepted_epoch_valid == 0U) ||
            (runtime->accepted_sequence_valid == 0U) ||
            (runtime->accepted_sequence == 0U))
        {
            return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
        }
    }



    return GUARDIAN_NODE_LINK_FRESHNESS_OK;
}