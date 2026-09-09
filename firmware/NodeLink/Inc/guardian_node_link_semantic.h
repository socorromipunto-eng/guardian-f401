#ifndef GUARDIAN_NODE_LINK_SEMANTIC_H
#define GUARDIAN_NODE_LINK_SEMANTIC_H

#include "guardian_node_link_auth.h"

#include <stddef.h>
#include <stdint.h>

#define GUARDIAN_NODE_LINK_SEMANTIC_MAX_BINDINGS ((size_t)8U)
#define GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY ((size_t)96U)

typedef enum
{
    GUARDIAN_NODE_LINK_COMPAT_PROVIDER_COMPATIBLE = 0,
    GUARDIAN_NODE_LINK_COMPAT_PROVIDER_INCOMPATIBLE,
    GUARDIAN_NODE_LINK_COMPAT_PROVIDER_UNKNOWN,
    GUARDIAN_NODE_LINK_COMPAT_PROVIDER_UNSUPPORTED,
    GUARDIAN_NODE_LINK_COMPAT_PROVIDER_HISTORICALLY_IMPOSSIBLE,
    GUARDIAN_NODE_LINK_COMPAT_PROVIDER_CONTEXT_INCOMPLETE,
    GUARDIAN_NODE_LINK_COMPAT_PROVIDER_UNAVAILABLE,
    GUARDIAN_NODE_LINK_COMPAT_PROVIDER_FAILURE
} guardian_node_link_compat_provider_result_t;

typedef enum
{
    GUARDIAN_NODE_LINK_SEMANTIC_OK = 0,
    GUARDIAN_NODE_LINK_SEMANTIC_ERROR_NULL_ARGUMENT,
    GUARDIAN_NODE_LINK_SEMANTIC_ERROR_UNCONFIGURED,
    GUARDIAN_NODE_LINK_SEMANTIC_ERROR_BINDING_MISSING,
    GUARDIAN_NODE_LINK_SEMANTIC_ERROR_BINDING_AMBIGUOUS,
    GUARDIAN_NODE_LINK_SEMANTIC_ERROR_CONTEXT_INCOMPLETE,
    GUARDIAN_NODE_LINK_SEMANTIC_ERROR_INCOMPATIBLE,
    GUARDIAN_NODE_LINK_SEMANTIC_ERROR_UNKNOWN,
    GUARDIAN_NODE_LINK_SEMANTIC_ERROR_UNSUPPORTED,
    GUARDIAN_NODE_LINK_SEMANTIC_ERROR_HISTORICALLY_IMPOSSIBLE,
    GUARDIAN_NODE_LINK_SEMANTIC_ERROR_PROVIDER_UNAVAILABLE,
    GUARDIAN_NODE_LINK_SEMANTIC_ERROR_PROVIDER_FAILURE
} guardian_node_link_semantic_result_t;

typedef struct
{
    uint32_t sender_node_id;
    uint32_t producer_id;
    uint32_t key_id;
    uint8_t signature_algorithm;
    const char *producer_semantic_profile_id;
} guardian_node_link_semantic_binding_config_t;

typedef struct
{
    uint32_t sender_node_id;
    uint32_t producer_id;
    uint32_t key_id;
    uint8_t signature_algorithm;

    char producer_semantic_profile_id[
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY];
} guardian_node_link_semantic_binding_t;

typedef guardian_node_link_compat_provider_result_t
(*guardian_node_link_pre_freshness_compatibility_resolve_fn)(
    void *context,
    const char *producer_semantic_profile_id,
    const char *consumer_semantic_profile_id,
    uint8_t protocol_version,
    char *compatibility_contract_id,
    size_t compatibility_contract_id_capacity);

typedef struct
{
    void *context;

    guardian_node_link_pre_freshness_compatibility_resolve_fn
        resolve_pre_freshness_compatibility;

    const char *consumer_semantic_profile_id;

    const guardian_node_link_semantic_binding_config_t *bindings;
    size_t binding_count;
} guardian_node_link_semantic_config_t;

typedef struct
{
    void *context;

    guardian_node_link_pre_freshness_compatibility_resolve_fn
        resolve_pre_freshness_compatibility;

    char consumer_semantic_profile_id[
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY];

    guardian_node_link_semantic_binding_t
        bindings[GUARDIAN_NODE_LINK_SEMANTIC_MAX_BINDINGS];

    size_t binding_count;
    uint8_t configured;
} guardian_node_link_semantic_resolver_t;

typedef struct
{
    guardian_node_link_authenticated_message_t authenticated_message;

    char producer_semantic_profile_id[
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY];

    char consumer_semantic_profile_id[
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY];

    char compatibility_contract_id[
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY];
} guardian_node_link_pre_freshness_compatible_message_t;

void guardian_node_link_semantic_resolver_init(
    guardian_node_link_semantic_resolver_t *resolver);

guardian_node_link_semantic_result_t
guardian_node_link_semantic_resolver_configure(
    guardian_node_link_semantic_resolver_t *resolver,
    const guardian_node_link_semantic_config_t *config);

guardian_node_link_semantic_result_t
guardian_node_link_resolve_pre_freshness_compatibility(
    const guardian_node_link_semantic_resolver_t *resolver,
    const guardian_node_link_authenticated_message_t *authenticated_message,
    guardian_node_link_pre_freshness_compatible_message_t *compatible_message);

#endif