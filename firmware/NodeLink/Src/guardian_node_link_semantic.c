#include "guardian_node_link_semantic.h"

#include <string.h>

static int guardian_node_link_semantic_snapshot_ref(
    char *destination,
    size_t destination_capacity,
    const char *source)
{
    size_t length = 0U;

    if ((destination == NULL) ||
        (source == NULL) ||
        (destination_capacity == 0U))
    {
        return 0;
    }

    while ((length < destination_capacity) &&
           (source[length] != '\0'))
    {
        length += 1U;
    }

    if ((length == 0U) ||
        (length >= destination_capacity))
    {
        return 0;
    }

    (void)memcpy(destination, source, length);
    destination[length] = '\0';

    return 1;
}

static int guardian_node_link_semantic_identity_equal(
    const guardian_node_link_semantic_binding_t *binding,
    const guardian_node_link_authenticated_message_t *message)
{
    if ((binding == NULL) || (message == NULL))
    {
        return 0;
    }

    return (
        (binding->sender_node_id ==
            message->frame.sender_node_id) &&
        (binding->producer_id ==
            message->producer_id) &&
        (binding->key_id ==
            message->key_id) &&
        (binding->signature_algorithm ==
            message->signature_algorithm));
}

static guardian_node_link_semantic_result_t
guardian_node_link_semantic_map_provider_result(
    guardian_node_link_compat_provider_result_t result)
{
    switch (result)
    {
        case GUARDIAN_NODE_LINK_COMPAT_PROVIDER_COMPATIBLE:
            return GUARDIAN_NODE_LINK_SEMANTIC_OK;

        case GUARDIAN_NODE_LINK_COMPAT_PROVIDER_INCOMPATIBLE:
            return GUARDIAN_NODE_LINK_SEMANTIC_ERROR_INCOMPATIBLE;

        case GUARDIAN_NODE_LINK_COMPAT_PROVIDER_UNKNOWN:
            return GUARDIAN_NODE_LINK_SEMANTIC_ERROR_UNKNOWN;

        case GUARDIAN_NODE_LINK_COMPAT_PROVIDER_UNSUPPORTED:
            return GUARDIAN_NODE_LINK_SEMANTIC_ERROR_UNSUPPORTED;

        case GUARDIAN_NODE_LINK_COMPAT_PROVIDER_HISTORICALLY_IMPOSSIBLE:
            return
                GUARDIAN_NODE_LINK_SEMANTIC_ERROR_HISTORICALLY_IMPOSSIBLE;

        case GUARDIAN_NODE_LINK_COMPAT_PROVIDER_CONTEXT_INCOMPLETE:
            return
                GUARDIAN_NODE_LINK_SEMANTIC_ERROR_CONTEXT_INCOMPLETE;

        case GUARDIAN_NODE_LINK_COMPAT_PROVIDER_UNAVAILABLE:
            return
                GUARDIAN_NODE_LINK_SEMANTIC_ERROR_PROVIDER_UNAVAILABLE;

        case GUARDIAN_NODE_LINK_COMPAT_PROVIDER_FAILURE:
        default:
            return
                GUARDIAN_NODE_LINK_SEMANTIC_ERROR_PROVIDER_FAILURE;
    }
}

void guardian_node_link_semantic_resolver_init(
    guardian_node_link_semantic_resolver_t *resolver)
{
    if (resolver == NULL)
    {
        return;
    }

    (void)memset(resolver, 0, sizeof(*resolver));
}

guardian_node_link_semantic_result_t
guardian_node_link_semantic_resolver_configure(
    guardian_node_link_semantic_resolver_t *resolver,
    const guardian_node_link_semantic_config_t *config)
{
    size_t first;
    size_t second;

    if ((resolver == NULL) || (config == NULL))
    {
        return GUARDIAN_NODE_LINK_SEMANTIC_ERROR_NULL_ARGUMENT;
    }

    (void)memset(resolver, 0, sizeof(*resolver));

    if (config->resolve_pre_freshness_compatibility == NULL)
    {
        return GUARDIAN_NODE_LINK_SEMANTIC_ERROR_UNCONFIGURED;
    }

    if (config->binding_count >
        GUARDIAN_NODE_LINK_SEMANTIC_MAX_BINDINGS)
    {
        return GUARDIAN_NODE_LINK_SEMANTIC_ERROR_UNCONFIGURED;
    }

    if ((config->binding_count > 0U) &&
        (config->bindings == NULL))
    {
        return GUARDIAN_NODE_LINK_SEMANTIC_ERROR_UNCONFIGURED;
    }

    if (guardian_node_link_semantic_snapshot_ref(
            resolver->consumer_semantic_profile_id,
            sizeof(resolver->consumer_semantic_profile_id),
            config->consumer_semantic_profile_id) == 0)
    {
        return GUARDIAN_NODE_LINK_SEMANTIC_ERROR_CONTEXT_INCOMPLETE;
    }

    for (first = 0U;
         first < config->binding_count;
         first += 1U)
    {
        const guardian_node_link_semantic_binding_config_t *source =
            &config->bindings[first];

        guardian_node_link_semantic_binding_t *destination =
            &resolver->bindings[first];

        if ((source->sender_node_id == 0U) ||
            (source->producer_id == 0U) ||
            (source->key_id == 0U))
        {
            return GUARDIAN_NODE_LINK_SEMANTIC_ERROR_CONTEXT_INCOMPLETE;
        }

        if (source->signature_algorithm !=
            GUARDIAN_NODE_LINK_SIGNATURE_ED25519)
        {
            return GUARDIAN_NODE_LINK_SEMANTIC_ERROR_UNSUPPORTED;
        }

        if (guardian_node_link_semantic_snapshot_ref(
                destination->producer_semantic_profile_id,
                sizeof(destination->producer_semantic_profile_id),
                source->producer_semantic_profile_id) == 0)
        {
            return GUARDIAN_NODE_LINK_SEMANTIC_ERROR_CONTEXT_INCOMPLETE;
        }

        destination->sender_node_id =
            source->sender_node_id;

        destination->producer_id =
            source->producer_id;

        destination->key_id =
            source->key_id;

        destination->signature_algorithm =
            source->signature_algorithm;

        for (second = 0U;
             second < first;
             second += 1U)
        {
            const guardian_node_link_semantic_binding_t *existing =
                &resolver->bindings[second];

            if ((existing->sender_node_id ==
                    destination->sender_node_id) &&
                (existing->producer_id ==
                    destination->producer_id) &&
                (existing->key_id ==
                    destination->key_id) &&
                (existing->signature_algorithm ==
                    destination->signature_algorithm))
            {
                return
                    GUARDIAN_NODE_LINK_SEMANTIC_ERROR_BINDING_AMBIGUOUS;
            }
        }
    }

    resolver->context = config->context;

    resolver->resolve_pre_freshness_compatibility =
        config->resolve_pre_freshness_compatibility;

    resolver->binding_count =
        config->binding_count;

    resolver->configured = 1U;

    return GUARDIAN_NODE_LINK_SEMANTIC_OK;
}

guardian_node_link_semantic_result_t
guardian_node_link_resolve_pre_freshness_compatibility(
    const guardian_node_link_semantic_resolver_t *resolver,
    const guardian_node_link_authenticated_message_t *authenticated_message,
    guardian_node_link_pre_freshness_compatible_message_t *compatible_message)
{
    const guardian_node_link_semantic_binding_t *binding = NULL;

    guardian_node_link_compat_provider_result_t provider_result;
    guardian_node_link_semantic_result_t mapped_result;

    char contract_id[
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY];

    size_t match_count = 0U;
    size_t index;

    if (compatible_message != NULL)
    {
        (void)memset(
            compatible_message,
            0,
            sizeof(*compatible_message));
    }

    if ((resolver == NULL) ||
        (authenticated_message == NULL) ||
        (compatible_message == NULL))
    {
        return GUARDIAN_NODE_LINK_SEMANTIC_ERROR_NULL_ARGUMENT;
    }

    if ((resolver->configured == 0U) ||
        (resolver->resolve_pre_freshness_compatibility == NULL))
    {
        return GUARDIAN_NODE_LINK_SEMANTIC_ERROR_UNCONFIGURED;
    }

    for (index = 0U;
         index < resolver->binding_count;
         index += 1U)
    {
        if (guardian_node_link_semantic_identity_equal(
                &resolver->bindings[index],
                authenticated_message) != 0)
        {
            binding = &resolver->bindings[index];
            match_count += 1U;
        }
    }

    if (match_count == 0U)
    {
        return GUARDIAN_NODE_LINK_SEMANTIC_ERROR_BINDING_MISSING;
    }

    if (match_count != 1U)
    {
        return GUARDIAN_NODE_LINK_SEMANTIC_ERROR_BINDING_AMBIGUOUS;
    }

    (void)memset(contract_id, 0, sizeof(contract_id));

    provider_result =
        resolver->resolve_pre_freshness_compatibility(
            resolver->context,
            binding->producer_semantic_profile_id,
            resolver->consumer_semantic_profile_id,
            GUARDIAN_NODE_LINK_VERSION,
            contract_id,
            sizeof(contract_id));

    mapped_result =
        guardian_node_link_semantic_map_provider_result(
            provider_result);

    if (mapped_result != GUARDIAN_NODE_LINK_SEMANTIC_OK)
    {
        (void)memset(contract_id, 0, sizeof(contract_id));
        return mapped_result;
    }

    if (guardian_node_link_semantic_snapshot_ref(
            compatible_message->compatibility_contract_id,
            sizeof(compatible_message->compatibility_contract_id),
            contract_id) == 0)
    {
        (void)memset(contract_id, 0, sizeof(contract_id));

        (void)memset(
            compatible_message,
            0,
            sizeof(*compatible_message));

        return GUARDIAN_NODE_LINK_SEMANTIC_ERROR_PROVIDER_FAILURE;
    }

    compatible_message->authenticated_message =
        *authenticated_message;

    if (guardian_node_link_semantic_snapshot_ref(
            compatible_message->producer_semantic_profile_id,
            sizeof(compatible_message->producer_semantic_profile_id),
            binding->producer_semantic_profile_id) == 0)
    {
        (void)memset(contract_id, 0, sizeof(contract_id));

        (void)memset(
            compatible_message,
            0,
            sizeof(*compatible_message));

        return GUARDIAN_NODE_LINK_SEMANTIC_ERROR_PROVIDER_FAILURE;
    }

    if (guardian_node_link_semantic_snapshot_ref(
            compatible_message->consumer_semantic_profile_id,
            sizeof(compatible_message->consumer_semantic_profile_id),
            resolver->consumer_semantic_profile_id) == 0)
    {
        (void)memset(contract_id, 0, sizeof(contract_id));

        (void)memset(
            compatible_message,
            0,
            sizeof(*compatible_message));

        return GUARDIAN_NODE_LINK_SEMANTIC_ERROR_PROVIDER_FAILURE;
    }

    (void)memset(contract_id, 0, sizeof(contract_id));

    return GUARDIAN_NODE_LINK_SEMANTIC_OK;
}