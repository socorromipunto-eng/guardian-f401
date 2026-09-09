#include "guardian_node_link_semantic.h"

#include <stdio.h>
#include <string.h>

#define TEST_SENDER_ID ((uint32_t)0x00001001U)
#define TEST_PRODUCER_ID ((uint32_t)0x00002001U)
#define TEST_KEY_ID ((uint32_t)0x00003001U)

#define CHECK(condition, message) \
    do \
    { \
        if (!(condition)) \
        { \
            (void)fprintf(stderr, "FAIL: %s\n", (message)); \
            return 1; \
        } \
    } while (0)

typedef struct
{
    guardian_node_link_compat_provider_result_t result;
    const char *expected_producer_profile;
    const char *expected_consumer_profile;
    const char *contract_id;
    size_t call_count;
} test_compat_provider_context_t;

static guardian_node_link_verify_result_t
test_verify_signature(
    void *context,
    uint8_t signature_algorithm,
    uint32_t producer_id,
    uint32_t key_id,
    const uint8_t *transcript,
    size_t transcript_length,
    const uint8_t *signature,
    size_t signature_length)
{
    (void)context;
    (void)producer_id;
    (void)key_id;

    if ((signature_algorithm !=
            GUARDIAN_NODE_LINK_SIGNATURE_ED25519) ||
        (transcript == NULL) ||
        (transcript_length == 0U) ||
        (signature == NULL) ||
        (signature_length !=
            GUARDIAN_NODE_LINK_ED25519_SIGNATURE_SIZE))
    {
        return GUARDIAN_NODE_LINK_VERIFY_PROVIDER_FAILURE;
    }

    return GUARDIAN_NODE_LINK_VERIFY_VERIFIED;
}

static guardian_node_link_compat_provider_result_t
test_resolve_pre_freshness_compatibility(
    void *context,
    const char *producer_semantic_profile_id,
    const char *consumer_semantic_profile_id,
    uint8_t protocol_version,
    char *compatibility_contract_id,
    size_t compatibility_contract_id_capacity)
{
    test_compat_provider_context_t *provider =
        (test_compat_provider_context_t *)context;

    size_t contract_length;

    if ((provider == NULL) ||
        (producer_semantic_profile_id == NULL) ||
        (consumer_semantic_profile_id == NULL) ||
        (compatibility_contract_id == NULL) ||
        (compatibility_contract_id_capacity == 0U))
    {
        return GUARDIAN_NODE_LINK_COMPAT_PROVIDER_FAILURE;
    }

    provider->call_count += 1U;

    if (protocol_version != GUARDIAN_NODE_LINK_VERSION)
    {
        return GUARDIAN_NODE_LINK_COMPAT_PROVIDER_FAILURE;
    }

    if ((provider->expected_producer_profile == NULL) ||
        (strcmp(
            producer_semantic_profile_id,
            provider->expected_producer_profile) != 0))
    {
        return GUARDIAN_NODE_LINK_COMPAT_PROVIDER_FAILURE;
    }

    if ((provider->expected_consumer_profile == NULL) ||
        (strcmp(
            consumer_semantic_profile_id,
            provider->expected_consumer_profile) != 0))
    {
        return GUARDIAN_NODE_LINK_COMPAT_PROVIDER_FAILURE;
    }

    if (provider->result !=
        GUARDIAN_NODE_LINK_COMPAT_PROVIDER_COMPATIBLE)
    {
        return provider->result;
    }

    if (provider->contract_id == NULL)
    {
        return GUARDIAN_NODE_LINK_COMPAT_PROVIDER_COMPATIBLE;
    }

    contract_length = strlen(provider->contract_id);

    if ((contract_length + 1U) >
        compatibility_contract_id_capacity)
    {
        return GUARDIAN_NODE_LINK_COMPAT_PROVIDER_FAILURE;
    }

    (void)memcpy(
        compatibility_contract_id,
        provider->contract_id,
        contract_length + 1U);

    return GUARDIAN_NODE_LINK_COMPAT_PROVIDER_COMPATIBLE;
}

static int buffer_is_zero(
    const void *buffer,
    size_t size)
{
    const unsigned char *bytes =
        (const unsigned char *)buffer;

    size_t index;

    for (index = 0U; index < size; index += 1U)
    {
        if (bytes[index] != 0U)
        {
            return 0;
        }
    }

    return 1;
}

static void fill_ascii_string(
    char *buffer,
    size_t length,
    char value)
{
    size_t index;

    for (index = 0U; index < length; index += 1U)
    {
        buffer[index] = value;
    }

    buffer[length] = '\0';
}

static int build_authenticated_message(
    guardian_node_link_authenticated_message_t *message)
{
    guardian_node_link_authenticator_t authenticator;
    guardian_node_link_peer_identity_t peer;
    guardian_node_link_auth_config_t config;
    guardian_node_link_frame_t frame;

    uint8_t signature[
        GUARDIAN_NODE_LINK_ED25519_SIGNATURE_SIZE];

    guardian_node_link_auth_result_t result;

    if (message == NULL)
    {
        return 0;
    }

    (void)memset(&authenticator, 0, sizeof(authenticator));
    (void)memset(&peer, 0, sizeof(peer));
    (void)memset(&config, 0, sizeof(config));
    (void)memset(&frame, 0, sizeof(frame));
    (void)memset(signature, 0xA5, sizeof(signature));
    (void)memset(message, 0, sizeof(*message));

    peer.sender_node_id = TEST_SENDER_ID;
    peer.producer_id = TEST_PRODUCER_ID;
    peer.key_id = TEST_KEY_ID;
    peer.signature_algorithm =
        GUARDIAN_NODE_LINK_SIGNATURE_ED25519;

    config.context = NULL;
    config.verify_signature = test_verify_signature;
    config.peers = &peer;
    config.peer_count = 1U;
    config.required_signature_algorithm =
        GUARDIAN_NODE_LINK_SIGNATURE_ED25519;

    guardian_node_link_authenticator_init(&authenticator);

    result = guardian_node_link_authenticator_configure(
        &authenticator,
        &config);

    if (result != GUARDIAN_NODE_LINK_AUTH_OK)
    {
        return 0;
    }

    frame.message_type =
        GUARDIAN_NODE_LINK_MESSAGE_HEARTBEAT;

    frame.flags =
        GUARDIAN_NODE_LINK_SUPPORTED_FLAGS;

    frame.node_state =
        GUARDIAN_NODE_STATE_ACTIVE;

    frame.payload_length = 1U;
    frame.sequence = 7U;
    frame.sender_epoch = 11U;
    frame.sender_node_id = TEST_SENDER_ID;
    frame.payload[0] = 0x42U;

    result = guardian_node_link_authenticate(
        &authenticator,
        &frame,
        signature,
        sizeof(signature),
        message);

    return result == GUARDIAN_NODE_LINK_AUTH_OK;
}

int main(void)
{
    char producer_profile[] =
        "guardian:test:producer:v1";

    char consumer_profile[] =
        "guardian:test:consumer:v1";

    char max_profile[
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY];

    char too_long_profile[
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY + 1U];

    char max_contract[
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY];

    char too_long_contract[
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY + 1U];

    guardian_node_link_authenticated_message_t authenticated;
    guardian_node_link_authenticated_message_t changed;

    guardian_node_link_semantic_binding_config_t binding;

    guardian_node_link_semantic_binding_config_t
        duplicate_bindings[2];

    guardian_node_link_semantic_binding_config_t
        distinct_credential_bindings[2];

    guardian_node_link_semantic_binding_config_t
        max_binding;

    guardian_node_link_semantic_config_t config;
    guardian_node_link_semantic_config_t bad_config;
    guardian_node_link_semantic_config_t zero_config;
    guardian_node_link_semantic_config_t max_config;

    guardian_node_link_semantic_resolver_t resolver;
    guardian_node_link_semantic_resolver_t bad_resolver;
    guardian_node_link_semantic_resolver_t zero_resolver;
    guardian_node_link_semantic_resolver_t max_resolver;

    guardian_node_link_pre_freshness_compatible_message_t output;

    test_compat_provider_context_t provider;
    test_compat_provider_context_t max_provider;

    guardian_node_link_semantic_result_t result;

    CHECK(
        build_authenticated_message(&authenticated) != 0,
        "R1 authenticated input created through verifier API");

    (void)memset(&binding, 0, sizeof(binding));

    binding.sender_node_id = TEST_SENDER_ID;
    binding.producer_id = TEST_PRODUCER_ID;
    binding.key_id = TEST_KEY_ID;

    binding.signature_algorithm =
        GUARDIAN_NODE_LINK_SIGNATURE_ED25519;

    binding.producer_semantic_profile_id =
        producer_profile;

    (void)memset(&provider, 0, sizeof(provider));

    provider.result =
        GUARDIAN_NODE_LINK_COMPAT_PROVIDER_COMPATIBLE;

    provider.expected_producer_profile =
        "guardian:test:producer:v1";

    provider.expected_consumer_profile =
        "guardian:test:consumer:v1";

    provider.contract_id =
        "guardian:test:compatibility-contract:v1";

    (void)memset(&config, 0, sizeof(config));

    config.context = &provider;

    config.resolve_pre_freshness_compatibility =
        test_resolve_pre_freshness_compatibility;

    config.consumer_semantic_profile_id =
        consumer_profile;

    config.bindings = &binding;
    config.binding_count = 1U;

    guardian_node_link_semantic_resolver_init(&resolver);

    result = guardian_node_link_semantic_resolver_configure(
        &resolver,
        &config);

    CHECK(
        result == GUARDIAN_NODE_LINK_SEMANTIC_OK,
        "semantic resolver configuration accepted");

    producer_profile[0] = 'X';
    consumer_profile[0] = 'Y';

    binding.sender_node_id = 0xFFFFFFFFU;
    binding.producer_id = 0xFFFFFFFFU;
    binding.key_id = 0xFFFFFFFFU;

    (void)memset(&output, 0, sizeof(output));

    result =
        guardian_node_link_resolve_pre_freshness_compatibility(
            &resolver,
            &authenticated,
            &output);

    CHECK(
        result == GUARDIAN_NODE_LINK_SEMANTIC_OK,
        "valid pre-freshness compatibility");

    CHECK(
        strcmp(
            output.producer_semantic_profile_id,
            "guardian:test:producer:v1") == 0,
        "producer semantic profile is snapshotted");

    CHECK(
        strcmp(
            output.consumer_semantic_profile_id,
            "guardian:test:consumer:v1") == 0,
        "consumer semantic profile is snapshotted");

    CHECK(
        strcmp(
            output.compatibility_contract_id,
            "guardian:test:compatibility-contract:v1") == 0,
        "compatible output carries exact contract reference");

    CHECK(
        provider.call_count == 1U,
        "compatibility provider called exactly once");

    changed = authenticated;
    changed.frame.sender_node_id += 1U;

    (void)memset(&output, 0xA5, sizeof(output));

    result =
        guardian_node_link_resolve_pre_freshness_compatibility(
            &resolver,
            &changed,
            &output);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_BINDING_MISSING,
        "wrong sender identity fails closed");

    CHECK(
        buffer_is_zero(
            &output,
            sizeof(output)) != 0,
        "binding failure clears output");

    changed = authenticated;
    changed.producer_id += 1U;

    result =
        guardian_node_link_resolve_pre_freshness_compatibility(
            &resolver,
            &changed,
            &output);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_BINDING_MISSING,
        "wrong producer identity fails closed");

    changed = authenticated;
    changed.key_id += 1U;

    result =
        guardian_node_link_resolve_pre_freshness_compatibility(
            &resolver,
            &changed,
            &output);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_BINDING_MISSING,
        "wrong key identity fails closed");

    changed = authenticated;
    changed.signature_algorithm = 0x7FU;

    result =
        guardian_node_link_resolve_pre_freshness_compatibility(
            &resolver,
            &changed,
            &output);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_BINDING_MISSING,
        "wrong algorithm identity fails closed");

    provider.result =
        GUARDIAN_NODE_LINK_COMPAT_PROVIDER_INCOMPATIBLE;

    result =
        guardian_node_link_resolve_pre_freshness_compatibility(
            &resolver,
            &authenticated,
            &output);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_INCOMPATIBLE,
        "incompatible result fails closed");

    provider.result =
        GUARDIAN_NODE_LINK_COMPAT_PROVIDER_UNKNOWN;

    result =
        guardian_node_link_resolve_pre_freshness_compatibility(
            &resolver,
            &authenticated,
            &output);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_UNKNOWN,
        "unknown compatibility fails closed");

    provider.result =
        GUARDIAN_NODE_LINK_COMPAT_PROVIDER_UNSUPPORTED;

    result =
        guardian_node_link_resolve_pre_freshness_compatibility(
            &resolver,
            &authenticated,
            &output);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_UNSUPPORTED,
        "unsupported compatibility fails closed");

    provider.result =
        GUARDIAN_NODE_LINK_COMPAT_PROVIDER_HISTORICALLY_IMPOSSIBLE;

    result =
        guardian_node_link_resolve_pre_freshness_compatibility(
            &resolver,
            &authenticated,
            &output);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_HISTORICALLY_IMPOSSIBLE,
        "historically impossible compatibility fails closed");

    provider.result =
        GUARDIAN_NODE_LINK_COMPAT_PROVIDER_CONTEXT_INCOMPLETE;

    result =
        guardian_node_link_resolve_pre_freshness_compatibility(
            &resolver,
            &authenticated,
            &output);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_CONTEXT_INCOMPLETE,
        "incomplete context fails closed");

    provider.result =
        GUARDIAN_NODE_LINK_COMPAT_PROVIDER_UNAVAILABLE;

    result =
        guardian_node_link_resolve_pre_freshness_compatibility(
            &resolver,
            &authenticated,
            &output);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_PROVIDER_UNAVAILABLE,
        "unavailable provider fails closed");

    provider.result =
        GUARDIAN_NODE_LINK_COMPAT_PROVIDER_FAILURE;

    result =
        guardian_node_link_resolve_pre_freshness_compatibility(
            &resolver,
            &authenticated,
            &output);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_PROVIDER_FAILURE,
        "provider failure fails closed");

    provider.result =
        (guardian_node_link_compat_provider_result_t)99;

    result =
        guardian_node_link_resolve_pre_freshness_compatibility(
            &resolver,
            &authenticated,
            &output);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_PROVIDER_FAILURE,
        "unknown provider result fails closed");

    provider.result =
        GUARDIAN_NODE_LINK_COMPAT_PROVIDER_COMPATIBLE;

    provider.contract_id = NULL;

    result =
        guardian_node_link_resolve_pre_freshness_compatibility(
            &resolver,
            &authenticated,
            &output);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_PROVIDER_FAILURE,
        "compatible without contract reference fails closed");

    duplicate_bindings[0].sender_node_id =
        TEST_SENDER_ID;

    duplicate_bindings[0].producer_id =
        TEST_PRODUCER_ID;

    duplicate_bindings[0].key_id =
        TEST_KEY_ID;

    duplicate_bindings[0].signature_algorithm =
        GUARDIAN_NODE_LINK_SIGNATURE_ED25519;

    duplicate_bindings[0].producer_semantic_profile_id =
        "guardian:test:producer:v1";

    duplicate_bindings[1] =
        duplicate_bindings[0];

    bad_config = config;
    bad_config.bindings = duplicate_bindings;
    bad_config.binding_count = 2U;

    guardian_node_link_semantic_resolver_init(
        &bad_resolver);

    result =
        guardian_node_link_semantic_resolver_configure(
            &bad_resolver,
            &bad_config);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_BINDING_AMBIGUOUS,
        "duplicate exact semantic binding rejected");

    distinct_credential_bindings[0] =
        duplicate_bindings[0];

    distinct_credential_bindings[1] =
        duplicate_bindings[0];

    distinct_credential_bindings[1].producer_id =
        TEST_PRODUCER_ID + 1U;

    distinct_credential_bindings[1].key_id =
        TEST_KEY_ID + 1U;

    bad_config = config;
    bad_config.bindings =
        distinct_credential_bindings;
    bad_config.binding_count = 2U;

    result =
        guardian_node_link_semantic_resolver_configure(
            &bad_resolver,
            &bad_config);

    CHECK(
        result == GUARDIAN_NODE_LINK_SEMANTIC_OK,
        "same sender different producer and key is distinct credential binding");

    bad_config = config;

    bad_config.resolve_pre_freshness_compatibility =
        NULL;

    result =
        guardian_node_link_semantic_resolver_configure(
            &bad_resolver,
            &bad_config);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_UNCONFIGURED,
        "missing compatibility provider rejected");

    bad_config = config;
    bad_config.consumer_semantic_profile_id = "";

    result =
        guardian_node_link_semantic_resolver_configure(
            &bad_resolver,
            &bad_config);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_CONTEXT_INCOMPLETE,
        "empty consumer semantic profile rejected");

    bad_config = config;

    bad_config.binding_count =
        GUARDIAN_NODE_LINK_SEMANTIC_MAX_BINDINGS + 1U;

    result =
        guardian_node_link_semantic_resolver_configure(
            &bad_resolver,
            &bad_config);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_UNCONFIGURED,
        "binding table maximum enforced");

    /*
     * Zero bindings is a deliberate deny-all configuration.
     */
    (void)memset(&zero_config, 0, sizeof(zero_config));

    zero_config.context = &provider;

    zero_config.resolve_pre_freshness_compatibility =
        test_resolve_pre_freshness_compatibility;

    zero_config.consumer_semantic_profile_id =
        "guardian:test:consumer:v1";

    zero_config.bindings = NULL;
    zero_config.binding_count = 0U;

    guardian_node_link_semantic_resolver_init(
        &zero_resolver);

    result =
        guardian_node_link_semantic_resolver_configure(
            &zero_resolver,
            &zero_config);

    CHECK(
        result == GUARDIAN_NODE_LINK_SEMANTIC_OK,
        "zero binding deny-all resolver configures");

    (void)memset(&output, 0xA5, sizeof(output));

    result =
        guardian_node_link_resolve_pre_freshness_compatibility(
            &zero_resolver,
            &authenticated,
            &output);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_BINDING_MISSING,
        "zero binding deny-all returns binding missing");

    CHECK(
        buffer_is_zero(
            &output,
            sizeof(output)) != 0,
        "zero binding failure clears output");

    /*
     * Semantic references:
     * capacity is 96 bytes, therefore max valid string length is 95.
     */
    fill_ascii_string(
        max_profile,
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY - 1U,
        'P');

    fill_ascii_string(
        too_long_profile,
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY,
        'Q');

    max_binding.sender_node_id = TEST_SENDER_ID;
    max_binding.producer_id = TEST_PRODUCER_ID;
    max_binding.key_id = TEST_KEY_ID;
    max_binding.signature_algorithm =
        GUARDIAN_NODE_LINK_SIGNATURE_ED25519;
    max_binding.producer_semantic_profile_id =
        max_profile;

    (void)memset(&max_provider, 0, sizeof(max_provider));

    max_provider.result =
        GUARDIAN_NODE_LINK_COMPAT_PROVIDER_COMPATIBLE;

    max_provider.expected_producer_profile =
        max_profile;

    max_provider.expected_consumer_profile =
        max_profile;

    fill_ascii_string(
        max_contract,
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY - 1U,
        'C');

    max_provider.contract_id = max_contract;

    (void)memset(&max_config, 0, sizeof(max_config));

    max_config.context = &max_provider;

    max_config.resolve_pre_freshness_compatibility =
        test_resolve_pre_freshness_compatibility;

    max_config.consumer_semantic_profile_id =
        max_profile;

    max_config.bindings = &max_binding;
    max_config.binding_count = 1U;

    guardian_node_link_semantic_resolver_init(
        &max_resolver);

    result =
        guardian_node_link_semantic_resolver_configure(
            &max_resolver,
            &max_config);

    CHECK(
        result == GUARDIAN_NODE_LINK_SEMANTIC_OK,
        "95 byte semantic references accepted");

    result =
        guardian_node_link_resolve_pre_freshness_compatibility(
            &max_resolver,
            &authenticated,
            &output);

    CHECK(
        result == GUARDIAN_NODE_LINK_SEMANTIC_OK,
        "95 byte contract reference accepted");

    CHECK(
        strlen(output.producer_semantic_profile_id) ==
            GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY - 1U,
        "maximum producer semantic reference preserved");

    CHECK(
        strlen(output.compatibility_contract_id) ==
            GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY - 1U,
        "maximum contract reference preserved");

    max_binding.producer_semantic_profile_id =
        too_long_profile;

    max_config.bindings = &max_binding;

    result =
        guardian_node_link_semantic_resolver_configure(
            &max_resolver,
            &max_config);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_CONTEXT_INCOMPLETE,
        "96 byte semantic reference rejected");

    /*
     * Restore valid profile and attack contract output length.
     */
    max_binding.producer_semantic_profile_id =
        max_profile;

    result =
        guardian_node_link_semantic_resolver_configure(
            &max_resolver,
            &max_config);

    CHECK(
        result == GUARDIAN_NODE_LINK_SEMANTIC_OK,
        "max resolver reconfigured with valid profile");

    fill_ascii_string(
        too_long_contract,
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY,
        'D');

    max_provider.contract_id =
        too_long_contract;

    result =
        guardian_node_link_resolve_pre_freshness_compatibility(
            &max_resolver,
            &authenticated,
            &output);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_PROVIDER_FAILURE,
        "96 byte contract reference fails closed");

    /*
     * Explicit public API NULL hostile cases.
     */
    result =
        guardian_node_link_semantic_resolver_configure(
            NULL,
            &config);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_NULL_ARGUMENT,
        "NULL resolver configure rejected");

    result =
        guardian_node_link_semantic_resolver_configure(
            &bad_resolver,
            NULL);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_NULL_ARGUMENT,
        "NULL config rejected");

    result =
        guardian_node_link_resolve_pre_freshness_compatibility(
            NULL,
            &authenticated,
            &output);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_NULL_ARGUMENT,
        "NULL resolver resolution rejected");

    result =
        guardian_node_link_resolve_pre_freshness_compatibility(
            &resolver,
            NULL,
            &output);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_NULL_ARGUMENT,
        "NULL authenticated input rejected");

    result =
        guardian_node_link_resolve_pre_freshness_compatibility(
            &resolver,
            &authenticated,
            NULL);

    CHECK(
        result ==
            GUARDIAN_NODE_LINK_SEMANTIC_ERROR_NULL_ARGUMENT,
        "NULL compatible output rejected");

    (void)printf(
        "C5_R2_PRE_FRESHNESS_SEMANTIC_HOST_TEST=PASS\n");

    (void)printf(
        "SAME_SENDER_DISTINCT_CREDENTIAL_TEST=PASS\n");

    (void)printf(
        "SEMANTIC_REFERENCE_MAX_LENGTH_TEST=PASS\n");

    (void)printf(
        "SEMANTIC_REFERENCE_OVERFLOW_TEST=PASS\n");

    (void)printf(
        "ZERO_BINDING_DENY_ALL_RUNTIME_TEST=PASS\n");

    (void)printf(
        "CONTRACT_REFERENCE_MAX_LENGTH_TEST=PASS\n");

    (void)printf(
        "CONTRACT_REFERENCE_OVERFLOW_TEST=PASS\n");

    (void)printf(
        "PUBLIC_API_NULL_ARGUMENT_TESTS=PASS\n");

    (void)printf(
        "TEST_FIXTURE_COMPATIBILITY_NOT_PRODUCTION_CONTRACT=YES\n");

    (void)printf(
        "REAL_F401_MCXN947_COMPATIBILITY_DECLARED=NO\n");

    (void)printf(
        "FULL_MESSAGE_COMPATIBILITY_IMPLEMENTED=NO\n");

    (void)printf(
        "FRESHNESS_IMPLEMENTED=NO\n");

    (void)printf(
        "NODE_SUPERVISOR_ADDED=NO\n");

    (void)printf(
        "ACTUATION_CHANGED=NO\n");

    return 0;
}