#include "guardian_node_link_freshness.h"

#include <stdio.h>
#include <string.h>

#define TEST_ASSERT(condition)                                      \
    do                                                              \
    {                                                               \
        if (!(condition))                                           \
        {                                                           \
            (void)fprintf(                                          \
                stderr,                                             \
                "ASSERT FAIL line %d: %s\n",                        \
                __LINE__,                                           \
                #condition);                                        \
            return 1;                                               \
        }                                                           \
    } while (0)

static void set_ref(
    char *destination,
    size_t destination_capacity,
    const char *value)
{
    size_t length;

    if ((destination == NULL) ||
        (destination_capacity == 0U) ||
        (value == NULL))
    {
        return;
    }

    length = strlen(value);

    if (length >= destination_capacity)
    {
        return;
    }

    (void)memset(destination, 0, destination_capacity);
    (void)memcpy(destination, value, length);
}

static void make_valid_identity(
    guardian_node_link_freshness_runtime_t *runtime)
{
    runtime->identity.sender_node_id = 0x1001U;
    runtime->identity.producer_id = 0x2001U;
    runtime->identity.key_id = 0x3001U;
    runtime->identity.signature_algorithm =
        GUARDIAN_NODE_LINK_SIGNATURE_ED25519;

    set_ref(
        runtime->identity.producer_semantic_profile_id,
        sizeof(runtime->identity.producer_semantic_profile_id),
        "guardian:test:producer:v1");

    set_ref(
        runtime->identity.consumer_semantic_profile_id,
        sizeof(runtime->identity.consumer_semantic_profile_id),
        "guardian:test:consumer:v1");

    set_ref(
        runtime->identity.compatibility_contract_id,
        sizeof(runtime->identity.compatibility_contract_id),
        "guardian:test:compat:v1");

    runtime->identity_valid = 1U;
}

static guardian_node_link_freshness_runtime_t
make_valid_active(void)
{
    guardian_node_link_freshness_runtime_t runtime;

    guardian_node_link_freshness_runtime_init(&runtime);

    make_valid_identity(&runtime);

    runtime.condition =
        GUARDIAN_NODE_LINK_FRESHNESS_ACTIVE;

    runtime.observed_epoch = 7U;
    runtime.observed_sequence = 11U;
    runtime.observation_valid = 1U;

    runtime.accepted_epoch = 7U;
    runtime.accepted_sequence = 11U;
    runtime.accepted_epoch_valid = 1U;
    runtime.accepted_sequence_valid = 1U;

    return runtime;
}

static int test_init_zeroization(void)
{
    guardian_node_link_freshness_runtime_t runtime;
    size_t index;
    const unsigned char *bytes;

    (void)memset(&runtime, 0xA5, sizeof(runtime));

    guardian_node_link_freshness_runtime_init(&runtime);

    TEST_ASSERT(
        runtime.condition ==
        GUARDIAN_NODE_LINK_FRESHNESS_UNINITIALIZED);

    TEST_ASSERT(runtime.identity_valid == 0U);
    TEST_ASSERT(runtime.observation_valid == 0U);
    TEST_ASSERT(runtime.accepted_epoch_valid == 0U);
    TEST_ASSERT(runtime.accepted_sequence_valid == 0U);

    bytes = (const unsigned char *)&runtime;

    for (index = sizeof(runtime.condition);
         index < sizeof(runtime);
         index += 1U)
    {
        TEST_ASSERT(bytes[index] == 0U);
    }

    return 0;
}

static int test_null_and_invalid_condition(void)
{
    guardian_node_link_freshness_runtime_t runtime;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(NULL) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT);

    guardian_node_link_freshness_runtime_init(&runtime);

    runtime.condition =
        (guardian_node_link_freshness_condition_t)0x7FU;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_CONDITION);

    return 0;
}

static int test_boolean_flag_bounds(void)
{
    guardian_node_link_freshness_runtime_t runtime;

    guardian_node_link_freshness_runtime_init(&runtime);
    runtime.identity_valid = 2U;
    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    guardian_node_link_freshness_runtime_init(&runtime);
    runtime.observation_valid = 2U;
    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    guardian_node_link_freshness_runtime_init(&runtime);
    runtime.accepted_epoch_valid = 2U;
    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    guardian_node_link_freshness_runtime_init(&runtime);
    runtime.accepted_sequence_valid = 2U;
    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    return 0;
}

static int test_identity_dependency(void)
{
    guardian_node_link_freshness_runtime_t runtime;

    guardian_node_link_freshness_runtime_init(&runtime);
    runtime.observation_valid = 1U;
    runtime.observed_sequence = 1U;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    guardian_node_link_freshness_runtime_init(&runtime);
    runtime.accepted_epoch_valid = 1U;
    runtime.accepted_epoch = 1U;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    guardian_node_link_freshness_runtime_init(&runtime);
    runtime.accepted_epoch_valid = 1U;
    runtime.accepted_sequence_valid = 1U;
    runtime.accepted_epoch = 1U;
    runtime.accepted_sequence = 1U;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    return 0;
}

static int test_identity_scalar_fields(void)
{
    guardian_node_link_freshness_runtime_t runtime;

    runtime = make_valid_active();
    runtime.identity.sender_node_id = 0U;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    runtime = make_valid_active();
    runtime.identity.producer_id = 0U;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    runtime = make_valid_active();
    runtime.identity.key_id = 0U;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    runtime = make_valid_active();
    runtime.identity.signature_algorithm = 0x7FU;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    return 0;
}

static int test_identity_reference_fields(void)
{
    guardian_node_link_freshness_runtime_t runtime;

    runtime = make_valid_active();
    runtime.identity.producer_semantic_profile_id[0] = '\0';

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    runtime = make_valid_active();

    (void)memset(
        runtime.identity.producer_semantic_profile_id,
        'P',
        sizeof(runtime.identity.producer_semantic_profile_id));

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    runtime = make_valid_active();
    runtime.identity.consumer_semantic_profile_id[0] = '\0';

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    runtime = make_valid_active();

    (void)memset(
        runtime.identity.consumer_semantic_profile_id,
        'C',
        sizeof(runtime.identity.consumer_semantic_profile_id));

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    runtime = make_valid_active();
    runtime.identity.compatibility_contract_id[0] = '\0';

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    runtime = make_valid_active();

    (void)memset(
        runtime.identity.compatibility_contract_id,
        'K',
        sizeof(runtime.identity.compatibility_contract_id));

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    return 0;
}

static void fill_max_valid_ref(
    char *destination,
    size_t destination_capacity,
    char fill)
{
    size_t index;

    if ((destination == NULL) ||
        (destination_capacity < 2U))
    {
        return;
    }

    for (index = 0U;
         index < (destination_capacity - 1U);
         index += 1U)
    {
        destination[index] = fill;
    }

    destination[destination_capacity - 1U] = '\0';
}

static int test_reference_length_boundaries(void)
{
    guardian_node_link_freshness_runtime_t runtime;

    /*
     * Minimum valid non-empty references: one character plus NUL.
     */
    runtime = make_valid_active();

    set_ref(
        runtime.identity.producer_semantic_profile_id,
        sizeof(runtime.identity.producer_semantic_profile_id),
        "P");

    set_ref(
        runtime.identity.consumer_semantic_profile_id,
        sizeof(runtime.identity.consumer_semantic_profile_id),
        "C");

    set_ref(
        runtime.identity.compatibility_contract_id,
        sizeof(runtime.identity.compatibility_contract_id),
        "K");

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    /*
     * Maximum valid references:
     *
     * bytes [0 .. capacity-2] are non-zero
     * byte  [capacity-1]      is NUL
     */
    runtime = make_valid_active();

    fill_max_valid_ref(
        runtime.identity.producer_semantic_profile_id,
        sizeof(runtime.identity.producer_semantic_profile_id),
        'P');

    fill_max_valid_ref(
        runtime.identity.consumer_semantic_profile_id,
        sizeof(runtime.identity.consumer_semantic_profile_id),
        'C');

    fill_max_valid_ref(
        runtime.identity.compatibility_contract_id,
        sizeof(runtime.identity.compatibility_contract_id),
        'K');

    TEST_ASSERT(
        runtime.identity.producer_semantic_profile_id[
            sizeof(runtime.identity.producer_semantic_profile_id) - 1U]
        == '\0');

    TEST_ASSERT(
        runtime.identity.consumer_semantic_profile_id[
            sizeof(runtime.identity.consumer_semantic_profile_id) - 1U]
        == '\0');

    TEST_ASSERT(
        runtime.identity.compatibility_contract_id[
            sizeof(runtime.identity.compatibility_contract_id) - 1U]
        == '\0');

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    return 0;
}

static int test_sequence_epoch_dependencies(void)
{
    guardian_node_link_freshness_runtime_t runtime;

    guardian_node_link_freshness_runtime_init(&runtime);
    make_valid_identity(&runtime);

    runtime.accepted_sequence_valid = 1U;
    runtime.accepted_sequence = 1U;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    guardian_node_link_freshness_runtime_init(&runtime);
    make_valid_identity(&runtime);

    runtime.observation_valid = 1U;
    runtime.observed_epoch = 9U;
    runtime.observed_sequence = 0U;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    guardian_node_link_freshness_runtime_init(&runtime);
    make_valid_identity(&runtime);

    runtime.accepted_epoch_valid = 1U;
    runtime.accepted_sequence_valid = 1U;
    runtime.accepted_epoch = 9U;
    runtime.accepted_sequence = 0U;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    return 0;
}

static int test_active_requirements(void)
{
    guardian_node_link_freshness_runtime_t runtime;

    guardian_node_link_freshness_runtime_init(&runtime);
    runtime.condition = GUARDIAN_NODE_LINK_FRESHNESS_ACTIVE;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    guardian_node_link_freshness_runtime_init(&runtime);
    make_valid_identity(&runtime);
    runtime.condition = GUARDIAN_NODE_LINK_FRESHNESS_ACTIVE;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    guardian_node_link_freshness_runtime_init(&runtime);
    make_valid_identity(&runtime);
    runtime.condition = GUARDIAN_NODE_LINK_FRESHNESS_ACTIVE;
    runtime.accepted_epoch = 3U;
    runtime.accepted_epoch_valid = 1U;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    return 0;
}

static int test_valid_structural_states(void)
{
    guardian_node_link_freshness_runtime_t runtime;

    guardian_node_link_freshness_runtime_init(&runtime);

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    runtime = make_valid_active();

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    runtime = make_valid_active();
    runtime.condition =
        GUARDIAN_NODE_LINK_FRESHNESS_UNKNOWN;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    runtime = make_valid_active();
    runtime.condition =
        GUARDIAN_NODE_LINK_FRESHNESS_REJOIN_REQUIRED;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    runtime = make_valid_active();
    runtime.condition =
        GUARDIAN_NODE_LINK_FRESHNESS_SESSION_REPLACEMENT_REQUIRED;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    return 0;
}

static int test_validate_is_read_only(void)
{
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_runtime_t before;

    runtime = make_valid_active();
    before = runtime;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    runtime.identity.key_id = 0U;
    before = runtime;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    return 0;
}

static int test_observed_and_accepted_are_independent(void)
{
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_runtime_t before;

    runtime = make_valid_active();

    runtime.observed_epoch = 99U;
    runtime.observed_sequence = 700U;

    runtime.accepted_epoch = 7U;
    runtime.accepted_sequence = 11U;

    before = runtime;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(runtime.observed_epoch == 99U);
    TEST_ASSERT(runtime.observed_sequence == 700U);
    TEST_ASSERT(runtime.accepted_epoch == 7U);
    TEST_ASSERT(runtime.accepted_sequence == 11U);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    return 0;
}

static guardian_node_link_pre_freshness_compatible_message_t
make_valid_r3b_message(
    uint32_t epoch,
    uint32_t sequence)
{
    guardian_node_link_pre_freshness_compatible_message_t message;

    (void)memset(&message, 0, sizeof(message));

    message.authenticated_message.frame.sender_node_id = 0x1001U;
    message.authenticated_message.frame.sender_epoch = epoch;
    message.authenticated_message.frame.sequence = sequence;

    message.authenticated_message.producer_id = 0x2001U;
    message.authenticated_message.key_id = 0x3001U;
    message.authenticated_message.signature_algorithm =
        GUARDIAN_NODE_LINK_SIGNATURE_ED25519;

    set_ref(
        message.producer_semantic_profile_id,
        sizeof(message.producer_semantic_profile_id),
        "guardian:test:producer:v1");

    set_ref(
        message.consumer_semantic_profile_id,
        sizeof(message.consumer_semantic_profile_id),
        "guardian:test:consumer:v1");

    set_ref(
        message.compatibility_contract_id,
        sizeof(message.compatibility_contract_id),
        "guardian:test:compat:v1");

    return message;
}

static guardian_node_link_freshness_policy_t
make_valid_r3b_policy(
    uint32_t max_forward_gap)
{
    guardian_node_link_freshness_policy_t policy;

    (void)memset(&policy, 0, sizeof(policy));

    policy.configured = 1U;
    policy.max_forward_gap = max_forward_gap;

    return policy;
}

static int test_r3b_error_output_invalidation(void)
{
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_runtime_t before;
    guardian_node_link_pre_freshness_compatible_message_t message;
    guardian_node_link_freshness_policy_t policy;
    guardian_node_link_freshness_evaluation_t evaluation;

    runtime = make_valid_active();
    message = make_valid_r3b_message(7U, 12U);
    policy = make_valid_r3b_policy(4U);

    (void)memset(&evaluation, 0xA5, sizeof(evaluation));

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            NULL,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_INVALID);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    policy.configured = 0U;

    (void)memset(&evaluation, 0xA5, sizeof(evaluation));

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_POLICY);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_INVALID);

    policy = make_valid_r3b_policy(4U);

    message.authenticated_message.frame.sequence = 0U;

    (void)memset(&evaluation, 0xA5, sizeof(evaluation));

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_MESSAGE);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_INVALID);

    before = runtime;

    (void)memset(&evaluation, 0xA5, sizeof(evaluation));

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate_and_apply(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_MESSAGE);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_INVALID);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    return 0;
}

static int test_r3b_uninitialized_observe_only(void)
{
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_runtime_t before;
    guardian_node_link_pre_freshness_compatible_message_t message;
    guardian_node_link_freshness_policy_t policy;
    guardian_node_link_freshness_evaluation_t evaluation;

    guardian_node_link_freshness_runtime_init(&runtime);

    before = runtime;

    message = make_valid_r3b_message(7U, 1U);
    policy = make_valid_r3b_policy(4U);

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_OBSERVE_ONLY);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    TEST_ASSERT(runtime.accepted_epoch_valid == 0U);
    TEST_ASSERT(runtime.accepted_sequence_valid == 0U);

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate_and_apply(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_OBSERVE_ONLY);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    return 0;
}

static int test_r3b_same_epoch_replay_and_gap_policy(void)
{
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_runtime_t before;
    guardian_node_link_pre_freshness_compatible_message_t message;
    guardian_node_link_freshness_policy_t policy;
    guardian_node_link_freshness_evaluation_t evaluation;

    runtime = make_valid_active();
    policy = make_valid_r3b_policy(4U);

    before = runtime;

    message = make_valid_r3b_message(7U, 11U);

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_REPLAY_DUPLICATE);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    message = make_valid_r3b_message(7U, 10U);

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_REPLAY_REGRESSION);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    message = make_valid_r3b_message(7U, 12U);

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_ACCEPT);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    message = make_valid_r3b_message(7U, 15U);

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_ACCEPT);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    message = make_valid_r3b_message(7U, 16U);

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_FORWARD_GAP_EXCEEDED);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    return 0;
}

static int test_r3b_identity_and_semantic_binding(void)
{
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_pre_freshness_compatible_message_t message;
    guardian_node_link_freshness_policy_t policy;
    guardian_node_link_freshness_evaluation_t evaluation;

    runtime = make_valid_active();
    policy = make_valid_r3b_policy(4U);

    message = make_valid_r3b_message(7U, 12U);
    message.authenticated_message.frame.sender_node_id = 0x1002U;

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_IDENTITY_MISMATCH);

    message = make_valid_r3b_message(7U, 12U);
    message.authenticated_message.producer_id = 0x2002U;

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_IDENTITY_MISMATCH);

    message = make_valid_r3b_message(7U, 12U);
    message.authenticated_message.key_id = 0x3002U;

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_IDENTITY_MISMATCH);

    message = make_valid_r3b_message(7U, 12U);

    set_ref(
        message.producer_semantic_profile_id,
        sizeof(message.producer_semantic_profile_id),
        "guardian:test:producer:v2");

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_IDENTITY_MISMATCH);

    message = make_valid_r3b_message(7U, 12U);

    set_ref(
        message.consumer_semantic_profile_id,
        sizeof(message.consumer_semantic_profile_id),
        "guardian:test:consumer:v2");

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_IDENTITY_MISMATCH);

    message = make_valid_r3b_message(7U, 12U);

    set_ref(
        message.compatibility_contract_id,
        sizeof(message.compatibility_contract_id),
        "guardian:test:compat:v2");

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_IDENTITY_MISMATCH);

    message = make_valid_r3b_message(7U, 12U);
    message.authenticated_message.signature_algorithm = 0x7FU;

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_MESSAGE);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_INVALID);

    return 0;
}

static int test_r3b_epoch_and_exhaustion_precedence(void)
{
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_runtime_t before;
    guardian_node_link_pre_freshness_compatible_message_t message;
    guardian_node_link_freshness_policy_t policy;
    guardian_node_link_freshness_evaluation_t evaluation;

    runtime = make_valid_active();
    policy = make_valid_r3b_policy(4U);

    before = runtime;

    message = make_valid_r3b_message(8U, 12U);

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_EPOCH_TRANSITION_REQUIRED);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    runtime = make_valid_active();
    runtime.accepted_sequence = UINT32_MAX;
    runtime.observed_sequence = UINT32_MAX;

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    before = runtime;

    /*
     * Different epoch is intentional here.
     *
     * Sequence exhaustion must take precedence over ordinary epoch-transition
     * classification once the accepted generation is exhausted.
     */
    message = make_valid_r3b_message(8U, 1U);

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_SESSION_REPLACEMENT_REQUIRED);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    return 0;
}

static int test_r3b_blocked_conditions(void)
{
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_runtime_t before;
    guardian_node_link_pre_freshness_compatible_message_t message;
    guardian_node_link_freshness_policy_t policy;
    guardian_node_link_freshness_evaluation_t evaluation;

    policy = make_valid_r3b_policy(4U);
    message = make_valid_r3b_message(7U, 12U);

    runtime = make_valid_active();
    runtime.condition = GUARDIAN_NODE_LINK_FRESHNESS_UNKNOWN;
    before = runtime;

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate_and_apply(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_STATE_BLOCKED);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    runtime = make_valid_active();
    runtime.condition = GUARDIAN_NODE_LINK_FRESHNESS_REJOIN_REQUIRED;
    before = runtime;

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate_and_apply(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_STATE_BLOCKED);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    runtime = make_valid_active();
    runtime.condition =
        GUARDIAN_NODE_LINK_FRESHNESS_SESSION_REPLACEMENT_REQUIRED;

    before = runtime;

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate_and_apply(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_STATE_BLOCKED);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    return 0;
}

static int test_r3b_evaluate_and_apply_transaction(void)
{
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_runtime_t before;
    guardian_node_link_pre_freshness_compatible_message_t message;
    guardian_node_link_freshness_policy_t policy;
    guardian_node_link_freshness_policy_t wide_policy;
    guardian_node_link_freshness_evaluation_t evaluation;

    runtime = make_valid_active();

    policy = make_valid_r3b_policy(4U);
    message = make_valid_r3b_message(7U, 12U);

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate_and_apply(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_ACCEPT);

    TEST_ASSERT(evaluation.transition_applied == 1U);

    TEST_ASSERT(runtime.condition == GUARDIAN_NODE_LINK_FRESHNESS_ACTIVE);
    TEST_ASSERT(runtime.accepted_epoch == 7U);
    TEST_ASSERT(runtime.accepted_sequence == 12U);
    TEST_ASSERT(runtime.observed_epoch == 7U);
    TEST_ASSERT(runtime.observed_sequence == 12U);

    before = runtime;

    /*
     * The same message after successful application is now a duplicate.
     * A second call cannot advance state again.
     */
    TEST_ASSERT(
        guardian_node_link_freshness_evaluate_and_apply(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_REPLAY_DUPLICATE);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    /*
     * Demonstrate that the mutation boundary uses the CURRENT policy.
     *
     * A wide read-only evaluation cannot later be supplied as mutation
     * authorization because no public apply(evaluation) API exists.
     */
    runtime = make_valid_active();
    message = make_valid_r3b_message(7U, 20U);

    wide_policy = make_valid_r3b_policy(100U);

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &wide_policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_ACCEPT);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    before = runtime;

    policy = make_valid_r3b_policy(4U);

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate_and_apply(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_FORWARD_GAP_EXCEEDED);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    /*
     * A different authenticated epoch remains an observed classification.
     * It cannot replace accepted epoch through normal R3B processing.
     */
    message = make_valid_r3b_message(8U, 12U);
    before = runtime;

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate_and_apply(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_EPOCH_TRANSITION_REQUIRED);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    return 0;
}
static int test_r3b_zero_gap_policy_rejected(void)
{
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_runtime_t before;
    guardian_node_link_pre_freshness_compatible_message_t message;
    guardian_node_link_freshness_policy_t policy;
    guardian_node_link_freshness_evaluation_t evaluation;

    runtime = make_valid_active();
    before = runtime;

    message = make_valid_r3b_message(7U, 12U);

    policy.configured = 1U;
    policy.max_forward_gap = 0U;

    (void)memset(&evaluation, 0xA5, sizeof(evaluation));

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_POLICY);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_INVALID);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    return 0;
}

static int test_r3b_lower_numeric_epoch_requires_transition(void)
{
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_runtime_t before;
    guardian_node_link_pre_freshness_compatible_message_t message;
    guardian_node_link_freshness_policy_t policy;
    guardian_node_link_freshness_evaluation_t evaluation;

    runtime = make_valid_active();
    before = runtime;

    policy = make_valid_r3b_policy(4U);

    message = make_valid_r3b_message(6U, 12U);

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_EPOCH_TRANSITION_REQUIRED);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    TEST_ASSERT(runtime.accepted_epoch == 7U);
    TEST_ASSERT(runtime.accepted_sequence == 11U);

    return 0;
}

static int test_r3b_malformed_runtime_invalidates_output(void)
{
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_runtime_t before;
    guardian_node_link_pre_freshness_compatible_message_t message;
    guardian_node_link_freshness_policy_t policy;
    guardian_node_link_freshness_evaluation_t evaluation;

    runtime = make_valid_active();

    /*
     * Deliberately violate the R3A structural contract.
     */
    runtime.identity.key_id = 0U;

    before = runtime;

    policy = make_valid_r3b_policy(4U);
    message = make_valid_r3b_message(7U, 12U);

    (void)memset(&evaluation, 0xA5, sizeof(evaluation));

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_INVALID);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    (void)memset(&evaluation, 0xA5, sizeof(evaluation));

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate_and_apply(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_INVALID);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    return 0;
}
static int test_r3b_zero_epoch_rejected(void)
{
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_runtime_t before;
    guardian_node_link_pre_freshness_compatible_message_t message;
    guardian_node_link_freshness_policy_t policy;
    guardian_node_link_freshness_evaluation_t evaluation;

    runtime = make_valid_active();
    before = runtime;
    policy = make_valid_r3b_policy(4U);

    message = make_valid_r3b_message(0U, 12U);

    (void)memset(&evaluation, 0xA5, sizeof(evaluation));

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_MESSAGE);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_INVALID);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    (void)memset(&evaluation, 0xA5, sizeof(evaluation));

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate_and_apply(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_MESSAGE);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_INVALID);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    runtime = make_valid_active();
    runtime.accepted_epoch = 0U;
    before = runtime;

    message = make_valid_r3b_message(7U, 12U);

    (void)memset(&evaluation, 0xA5, sizeof(evaluation));

    TEST_ASSERT(
        guardian_node_link_freshness_runtime_validate(&runtime) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_INVALID);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    return 0;
}

static int test_r3b_null_argument_contract(void)
{
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_runtime_t before;
    guardian_node_link_pre_freshness_compatible_message_t message;
    guardian_node_link_freshness_policy_t policy;
    guardian_node_link_freshness_evaluation_t evaluation;

    runtime = make_valid_active();
    before = runtime;
    policy = make_valid_r3b_policy(4U);
    message = make_valid_r3b_message(7U, 12U);

    (void)memset(&evaluation, 0xA5, sizeof(evaluation));

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            NULL,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_INVALID);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    (void)memset(&evaluation, 0xA5, sizeof(evaluation));

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            NULL,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_INVALID);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate(
            &policy,
            &runtime,
            &message,
            NULL) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT);

    (void)memset(&evaluation, 0xA5, sizeof(evaluation));

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate_and_apply(
            &policy,
            NULL,
            &message,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_INVALID);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    (void)memset(&evaluation, 0xA5, sizeof(evaluation));

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate_and_apply(
            &policy,
            &runtime,
            NULL,
            &evaluation) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT);

    TEST_ASSERT(
        evaluation.decision ==
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_INVALID);

    TEST_ASSERT(evaluation.transition_applied == 0U);

    TEST_ASSERT(
        guardian_node_link_freshness_evaluate_and_apply(
            &policy,
            &runtime,
            &message,
            NULL) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT);

    TEST_ASSERT(
        memcmp(&runtime, &before, sizeof(runtime)) == 0);

    return 0;
}
int main(void)
{
    TEST_ASSERT(test_init_zeroization() == 0);
    TEST_ASSERT(test_null_and_invalid_condition() == 0);
    TEST_ASSERT(test_boolean_flag_bounds() == 0);
    TEST_ASSERT(test_identity_dependency() == 0);
    TEST_ASSERT(test_identity_scalar_fields() == 0);
    TEST_ASSERT(test_identity_reference_fields() == 0);
    TEST_ASSERT(test_reference_length_boundaries() == 0);
    TEST_ASSERT(test_sequence_epoch_dependencies() == 0);
    TEST_ASSERT(test_active_requirements() == 0);
    TEST_ASSERT(test_valid_structural_states() == 0);
    TEST_ASSERT(test_validate_is_read_only() == 0);
    TEST_ASSERT(test_observed_and_accepted_are_independent() == 0);

    TEST_ASSERT(test_r3b_error_output_invalidation() == 0);
    TEST_ASSERT(test_r3b_uninitialized_observe_only() == 0);
    TEST_ASSERT(test_r3b_same_epoch_replay_and_gap_policy() == 0);
    TEST_ASSERT(test_r3b_identity_and_semantic_binding() == 0);
    TEST_ASSERT(test_r3b_epoch_and_exhaustion_precedence() == 0);
    TEST_ASSERT(test_r3b_blocked_conditions() == 0);
    TEST_ASSERT(test_r3b_evaluate_and_apply_transaction() == 0);
    TEST_ASSERT(test_r3b_zero_gap_policy_rejected() == 0);
    TEST_ASSERT(test_r3b_lower_numeric_epoch_requires_transition() == 0);
    TEST_ASSERT(test_r3b_malformed_runtime_invalidates_output() == 0);
    TEST_ASSERT(test_r3b_zero_epoch_rejected() == 0);
    TEST_ASSERT(test_r3b_null_argument_contract() == 0);

    (void)printf("C5_R3A_FRESHNESS_STATE_HOST_TEST=PASS\n");
    (void)printf("C5_R3B_FRESHNESS_EVALUATOR_HOST_TEST=PASS\n");

    (void)printf("R3B_UNINITIALIZED_AUTO_ACCEPT=NO\n");
    (void)printf("R3B_NEW_EPOCH_AUTO_ACCEPT=NO\n");
    (void)printf("R3B_PUBLIC_APPLY_EVALUATION_API=NO\n");
    (void)printf("R3B_CURRENT_POLICY_AT_MUTATION_BOUNDARY=YES\n");
    (void)printf("R3B_SEQUENCE_EXHAUSTION_PRECEDENCE=PASS\n");

    (void)printf(
        "STRUCTURAL_VALIDITY_REAUTHENTICATES=NO\n");

    (void)printf(
        "STRUCTURAL_VALIDITY_ESTABLISHES_FRESHNESS=NO\n");

    (void)printf(
        "PERSISTENT_ANTI_REPLAY_DEMONSTRATED=NO\n");

    (void)printf(
        "REJOIN_IMPLEMENTED=NO\n");

    (void)printf(
        "NODE_SUPERVISOR_AUTHORITY=NO\n");

    (void)printf(
        "ACTUATION_AUTHORITY=NO\n");

    return 0;
}