#include "guardian_multinode.h"

#include <string.h>

static uint32_t
guardian_multinode_required_capabilities(
    guardian_multinode_role_t role)
{
    switch (role)
    {
        case GUARDIAN_NODE_ROLE_F401_REFERENCE_CONTROL:

            return
                GUARDIAN_CAP_OBSERVATION |
                GUARDIAN_CAP_POLICY_REFERENCE |
                GUARDIAN_CAP_ACTUATION_GATE;

        case GUARDIAN_NODE_ROLE_MCXN947_SUPERVISORY_PEER:

            return
                GUARDIAN_CAP_OBSERVATION |
                GUARDIAN_CAP_SUPERVISION |
                GUARDIAN_CAP_EVIDENCE_WITNESS |
                GUARDIAN_CAP_RECOVERY_ASSIST |
                GUARDIAN_CAP_SIGNATURE_VERIFY;

        case GUARDIAN_NODE_ROLE_ST_MEASUREMENT_REFERENCE:

            return
                GUARDIAN_CAP_OBSERVATION |
                GUARDIAN_CAP_SENSOR_ACQUISITION;

        case GUARDIAN_NODE_ROLE_SECURITY_WITNESS:

            return
                GUARDIAN_CAP_OBSERVATION |
                GUARDIAN_CAP_EVIDENCE_WITNESS |
                GUARDIAN_CAP_SIGNATURE_VERIFY;

        case GUARDIAN_NODE_ROLE_HIGH_COMPUTE_PROCESSING:

            return
                GUARDIAN_CAP_OBSERVATION |
                GUARDIAN_CAP_SIGNAL_PROCESSING;

        case GUARDIAN_NODE_ROLE_INVALID:
        default:

            return 0U;
    }
}

static guardian_multinode_authority_class_t
guardian_multinode_expected_authority(
    guardian_multinode_role_t role)
{
    switch (role)
    {
        case GUARDIAN_NODE_ROLE_F401_REFERENCE_CONTROL:

            return
                GUARDIAN_AUTHORITY_CLASS_REFERENCE_CONTROL;

        case GUARDIAN_NODE_ROLE_MCXN947_SUPERVISORY_PEER:

        case GUARDIAN_NODE_ROLE_ST_MEASUREMENT_REFERENCE:

        case GUARDIAN_NODE_ROLE_HIGH_COMPUTE_PROCESSING:

            return
                GUARDIAN_AUTHORITY_CLASS_OBSERVE;

        case GUARDIAN_NODE_ROLE_SECURITY_WITNESS:

            return
                GUARDIAN_AUTHORITY_CLASS_WITNESS;

        case GUARDIAN_NODE_ROLE_INVALID:
        default:

            return
                GUARDIAN_AUTHORITY_CLASS_NONE;
    }
}

void guardian_multinode_registry_init(
    guardian_multinode_registry_t *registry)
{
    if (registry == NULL)
    {
        return;
    }

    (void)memset(
        registry,
        0,
        sizeof(*registry));
}

guardian_multinode_result_t
guardian_multinode_validate_node(
    const guardian_multinode_node_t *node)
{
    uint32_t required;

    guardian_multinode_authority_class_t
        expected_authority;

    if (node == NULL)
    {
        return
            GUARDIAN_MULTINODE_ERROR_NULL_ARGUMENT;
    }

    if ((node->node_id == 0U) ||
        (node->producer_id == 0U) ||
        (node->key_id == 0U))
    {
        return
            GUARDIAN_MULTINODE_ERROR_INVALID_IDENTITY;
    }

    if ((node->role <=
            GUARDIAN_NODE_ROLE_INVALID) ||
        (node->role >
            GUARDIAN_NODE_ROLE_HIGH_COMPUTE_PROCESSING))
    {
        return
            GUARDIAN_MULTINODE_ERROR_INVALID_ROLE;
    }

    if ((node->capabilities == 0U) ||
        ((node->capabilities &
          ~GUARDIAN_CAP_ALL) != 0U))
    {
        return
            GUARDIAN_MULTINODE_ERROR_INVALID_CAPABILITIES;
    }

    required =
        guardian_multinode_required_capabilities(
            node->role);

    expected_authority =
        guardian_multinode_expected_authority(
            node->role);

    if (required == 0U)
    {
        return
            GUARDIAN_MULTINODE_ERROR_INVALID_ROLE;
    }

    if (node->authority_class !=
        expected_authority)
    {
        return
            GUARDIAN_MULTINODE_ERROR_INVALID_AUTHORITY_CLASS;
    }

    if ((node->capabilities & required) !=
        required)
    {
        return
            GUARDIAN_MULTINODE_ERROR_INVALID_CAPABILITIES;
    }

    /*
     * Hard authority boundary.
     *
     * No supervisory, witness, measurement or
     * compute node may self-assign the physical
     * actuation gate capability.
     */
    if ((node->capabilities &
         GUARDIAN_CAP_ACTUATION_GATE) != 0U)
    {
        if ((node->role !=
                GUARDIAN_NODE_ROLE_F401_REFERENCE_CONTROL) ||
            (node->authority_class !=
                GUARDIAN_AUTHORITY_CLASS_REFERENCE_CONTROL))
        {
            return
                GUARDIAN_MULTINODE_ERROR_AUTHORITY_ESCALATION;
        }
    }

    if ((node->role !=
            GUARDIAN_NODE_ROLE_F401_REFERENCE_CONTROL) &&
        (node->authority_class ==
            GUARDIAN_AUTHORITY_CLASS_REFERENCE_CONTROL))
    {
        return
            GUARDIAN_MULTINODE_ERROR_AUTHORITY_ESCALATION;
    }

    return GUARDIAN_MULTINODE_OK;
}

guardian_multinode_result_t
guardian_multinode_register_node(
    guardian_multinode_registry_t *registry,
    const guardian_multinode_node_t *node)
{
    guardian_multinode_result_t result;

    size_t index;

    if ((registry == NULL) ||
        (node == NULL))
    {
        return
            GUARDIAN_MULTINODE_ERROR_NULL_ARGUMENT;
    }

    result =
        guardian_multinode_validate_node(
            node);

    if (result != GUARDIAN_MULTINODE_OK)
    {
        return result;
    }

    if (registry->node_count >=
        GUARDIAN_MULTINODE_MAX_NODES)
    {
        return
            GUARDIAN_MULTINODE_ERROR_REGISTRY_FULL;
    }

    for (index = 0U;
         index < registry->node_count;
         index += 1U)
    {
        if (registry->nodes[index].node_id ==
            node->node_id)
        {
            return
                GUARDIAN_MULTINODE_ERROR_DUPLICATE_NODE_ID;
        }

        if (registry->nodes[index].producer_id ==
            node->producer_id)
        {
            return
                GUARDIAN_MULTINODE_ERROR_DUPLICATE_PRODUCER_ID;
        }

        if (registry->nodes[index].role ==
            node->role)
        {
            return
                GUARDIAN_MULTINODE_ERROR_DUPLICATE_ROLE;
        }
    }

    registry->nodes[
        registry->node_count
    ] = *node;

    registry->node_count += 1U;

    return GUARDIAN_MULTINODE_OK;
}

guardian_multinode_result_t
guardian_multinode_validate_complete_topology(
    const guardian_multinode_registry_t *registry)
{
    uint32_t roles_seen = 0U;

    uint32_t expected_roles;

    size_t index;

    if (registry == NULL)
    {
        return
            GUARDIAN_MULTINODE_ERROR_NULL_ARGUMENT;
    }

    if (registry->node_count !=
        GUARDIAN_MULTINODE_MAX_NODES)
    {
        return
            GUARDIAN_MULTINODE_ERROR_TOPOLOGY_INCOMPLETE;
    }

    for (index = 0U;
         index < registry->node_count;
         index += 1U)
    {
        guardian_multinode_result_t result =
            guardian_multinode_validate_node(
                &registry->nodes[index]);

        if (result != GUARDIAN_MULTINODE_OK)
        {
            return result;
        }

        roles_seen |=
            (uint32_t)1U <<
            (uint32_t)registry->nodes[index].role;
    }

    expected_roles =
        ((uint32_t)1U <<
            (uint32_t)
            GUARDIAN_NODE_ROLE_F401_REFERENCE_CONTROL) |

        ((uint32_t)1U <<
            (uint32_t)
            GUARDIAN_NODE_ROLE_MCXN947_SUPERVISORY_PEER) |

        ((uint32_t)1U <<
            (uint32_t)
            GUARDIAN_NODE_ROLE_ST_MEASUREMENT_REFERENCE) |

        ((uint32_t)1U <<
            (uint32_t)
            GUARDIAN_NODE_ROLE_SECURITY_WITNESS) |

        ((uint32_t)1U <<
            (uint32_t)
            GUARDIAN_NODE_ROLE_HIGH_COMPUTE_PROCESSING);

    if (roles_seen != expected_roles)
    {
        return
            GUARDIAN_MULTINODE_ERROR_TOPOLOGY_INCOMPLETE;
    }

    return GUARDIAN_MULTINODE_OK;
}

guardian_multinode_result_t
guardian_multinode_can_request_actuation(
    const guardian_multinode_registry_t *registry,
    uint32_t requesting_node_id,
    uint8_t *allowed)
{
    size_t index;

    if (allowed != NULL)
    {
        *allowed = 0U;
    }

    if ((registry == NULL) ||
        (allowed == NULL))
    {
        return
            GUARDIAN_MULTINODE_ERROR_NULL_ARGUMENT;
    }

    if (requesting_node_id == 0U)
    {
        return
            GUARDIAN_MULTINODE_ERROR_INVALID_IDENTITY;
    }

    for (index = 0U;
         index < registry->node_count;
         index += 1U)
    {
        const guardian_multinode_node_t *node =
            &registry->nodes[index];

        if (node->node_id ==
            requesting_node_id)
        {
            guardian_multinode_result_t result =
                guardian_multinode_validate_node(
                    node);

            if (result != GUARDIAN_MULTINODE_OK)
            {
                return result;
            }

            if ((node->role !=
                    GUARDIAN_NODE_ROLE_F401_REFERENCE_CONTROL) ||
                (node->authority_class !=
                    GUARDIAN_AUTHORITY_CLASS_REFERENCE_CONTROL) ||
                ((node->capabilities &
                  GUARDIAN_CAP_ACTUATION_GATE) == 0U))
            {
                return
                    GUARDIAN_MULTINODE_ERROR_NOT_REFERENCE_CONTROL;
            }

            *allowed = 1U;

            return GUARDIAN_MULTINODE_OK;
        }
    }

    return
        GUARDIAN_MULTINODE_ERROR_INVALID_IDENTITY;
}
