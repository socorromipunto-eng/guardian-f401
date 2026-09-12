#include "guardian_node_link_freshness.h"
#include "guardian_node_link_freshness_persistence.h"

#include <stdio.h>
#include <string.h>
/*
 * C5-R3C-A compile-time persistence-contract assertions.
 *
 * These assertions validate only the declared schema and classification
 * surface. They do not demonstrate persistent anti-replay, durable storage,
 * integrity protection, rollback resistance, rejoin, authority, or actuation.
 */
_Static_assert(
    GUARDIAN_NODE_LINK_FRESHNESS_PERSISTENCE_SCHEMA_VERSION == 1U,
    "R3C-A persistence schema version must be 1");

_Static_assert(
    GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_INVALID == 0,
    "R3C-A invalid persistence status must be zero");

_Static_assert(
    GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE !=
        GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_NO_PERSISTED_STATE,
    "valid persisted state must differ from no persisted state");

_Static_assert(
    GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_CORRUPTED_STATE !=
        GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_ROLLBACK_SUSPECTED,
    "corruption and rollback suspicion must remain distinct");

_Static_assert(
    sizeof(
        ((guardian_node_link_freshness_persisted_record_t *)0)->
            accepted_epoch) == sizeof(uint32_t),
    "persisted accepted epoch must remain uint32_t");

_Static_assert(
    sizeof(
        ((guardian_node_link_freshness_persisted_record_t *)0)->
            accepted_sequence) == sizeof(uint32_t),
    "persisted accepted sequence must remain uint32_t");

_Static_assert(
    sizeof(
        ((guardian_node_link_freshness_persisted_record_t *)0)->
            record_generation) == sizeof(uint32_t),
    "persisted record generation must remain uint32_t");

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
static guardian_node_link_freshness_persisted_record_t
make_valid_r3c_b_persisted_record(void)
{
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_persisted_record_t record;

    runtime = make_valid_active();

    (void)memset(&record, 0, sizeof(record));

    record.schema_version =
        GUARDIAN_NODE_LINK_FRESHNESS_PERSISTENCE_SCHEMA_VERSION;

    record.identity = runtime.identity;
    record.accepted_epoch = runtime.accepted_epoch;
    record.accepted_sequence = runtime.accepted_sequence;
    record.record_generation = 1U;

    return record;
}

static int expect_r3c_b_invalid_record(
    const guardian_node_link_freshness_persisted_record_t *record,
    const guardian_node_link_freshness_identity_t *expected_identity)
{
    guardian_node_link_freshness_persistence_classification_t classification;

    classification =
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_VALIDATED_RECORD_CANDIDATE;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_classify(
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE,
            record,
            expected_identity,
            &classification) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    TEST_ASSERT(
        classification ==
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID);

    return 0;
}

static int test_r3c_b_persistence_classification(void)
{
    guardian_node_link_freshness_persisted_record_t record;
    guardian_node_link_freshness_persisted_record_t poisoned_record;
    guardian_node_link_freshness_identity_t expected_identity;
    guardian_node_link_freshness_identity_t invalid_expected_identity;
    guardian_node_link_freshness_persistence_classification_t classification;

    record = make_valid_r3c_b_persisted_record();
    expected_identity = record.identity;

    classification =
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_classify(
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE,
            &record,
            &expected_identity,
            &classification) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        classification ==
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_VALIDATED_RECORD_CANDIDATE);

    /*
     * record_generation zero remains legal ordering metadata.
     * It is not a rollback decision and is not a trust anchor.
     */
    record.record_generation = 0U;

    classification =
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_classify(
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE,
            &record,
            &expected_identity,
            &classification) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        classification ==
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_VALIDATED_RECORD_CANDIDATE);

    /*
     * Schema mismatch fails closed without being mislabeled as proven
     * corruption or rollback.
     */
    record = make_valid_r3c_b_persisted_record();
    record.schema_version =
        GUARDIAN_NODE_LINK_FRESHNESS_PERSISTENCE_SCHEMA_VERSION + 1U;

    TEST_ASSERT(
        expect_r3c_b_invalid_record(
            &record,
            &expected_identity) == 0);

    record = make_valid_r3c_b_persisted_record();
    record.accepted_epoch = 0U;

    TEST_ASSERT(
        expect_r3c_b_invalid_record(
            &record,
            &expected_identity) == 0);

    record = make_valid_r3c_b_persisted_record();
    record.accepted_sequence = 0U;

    TEST_ASSERT(
        expect_r3c_b_invalid_record(
            &record,
            &expected_identity) == 0);

    /*
     * Every scalar field in the exact persistence identity is security
     * relevant and mismatch must fail closed.
     */
    record = make_valid_r3c_b_persisted_record();
    record.identity.sender_node_id += 1U;

    TEST_ASSERT(
        expect_r3c_b_invalid_record(
            &record,
            &expected_identity) == 0);

    record = make_valid_r3c_b_persisted_record();
    record.identity.producer_id += 1U;

    TEST_ASSERT(
        expect_r3c_b_invalid_record(
            &record,
            &expected_identity) == 0);

    record = make_valid_r3c_b_persisted_record();
    record.identity.key_id += 1U;

    TEST_ASSERT(
        expect_r3c_b_invalid_record(
            &record,
            &expected_identity) == 0);

    record = make_valid_r3c_b_persisted_record();
    record.identity.signature_algorithm = 0U;

    TEST_ASSERT(
        expect_r3c_b_invalid_record(
            &record,
            &expected_identity) == 0);

    record = make_valid_r3c_b_persisted_record();

    set_ref(
        record.identity.producer_semantic_profile_id,
        sizeof(record.identity.producer_semantic_profile_id),
        "guardian:test:producer:other");

    TEST_ASSERT(
        expect_r3c_b_invalid_record(
            &record,
            &expected_identity) == 0);

    record = make_valid_r3c_b_persisted_record();

    set_ref(
        record.identity.consumer_semantic_profile_id,
        sizeof(record.identity.consumer_semantic_profile_id),
        "guardian:test:consumer:other");

    TEST_ASSERT(
        expect_r3c_b_invalid_record(
            &record,
            &expected_identity) == 0);

    record = make_valid_r3c_b_persisted_record();

    set_ref(
        record.identity.compatibility_contract_id,
        sizeof(record.identity.compatibility_contract_id),
        "guardian:test:compat:other");

    TEST_ASSERT(
        expect_r3c_b_invalid_record(
            &record,
            &expected_identity) == 0);

    /*
     * Unterminated semantic identity references are malformed rather than
     * comparable strings.
     */
    record = make_valid_r3c_b_persisted_record();

    (void)memset(
        record.identity.producer_semantic_profile_id,
        'A',
        sizeof(record.identity.producer_semantic_profile_id));

    TEST_ASSERT(
        expect_r3c_b_invalid_record(
            &record,
            &expected_identity) == 0);

    /*
     * Non-VALID provider classifications do not consume record payload.
     * A deliberately malformed payload cannot alter their bounded mapping.
     */
    poisoned_record = make_valid_r3c_b_persisted_record();
    poisoned_record.schema_version = 0U;
    poisoned_record.accepted_epoch = 0U;
    poisoned_record.accepted_sequence = 0U;

    classification =
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_classify(
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_NO_PERSISTED_STATE,
            &poisoned_record,
            &expected_identity,
            &classification) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        classification ==
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_NO_STATE_BOOTSTRAP);

    classification =
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_classify(
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_NO_PERSISTED_STATE,
            NULL,
            &expected_identity,
            &classification) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        classification ==
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_NO_STATE_BOOTSTRAP);

    classification =
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_classify(
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_CORRUPTED_STATE,
            &poisoned_record,
            &expected_identity,
            &classification) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        classification ==
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_FRESHNESS_UNKNOWN);

    classification =
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_classify(
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_TORN_OR_INCOMPLETE_UPDATE,
            NULL,
            &expected_identity,
            &classification) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        classification ==
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_FRESHNESS_UNKNOWN);

    classification =
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_classify(
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_ROLLBACK_SUSPECTED,
            NULL,
            &expected_identity,
            &classification) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        classification ==
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_FRESHNESS_UNKNOWN);

    classification =
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_classify(
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_UNAVAILABLE_STATE,
            NULL,
            &expected_identity,
            &classification) ==
        GUARDIAN_NODE_LINK_FRESHNESS_OK);

    TEST_ASSERT(
        classification ==
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_FRESHNESS_UNKNOWN);

    /*
     * Unknown and deliberately INVALID provider values fail closed.
     */
    classification =
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_VALIDATED_RECORD_CANDIDATE;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_classify(
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_INVALID,
            &record,
            &expected_identity,
            &classification) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    TEST_ASSERT(
        classification ==
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID);

    classification =
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_VALIDATED_RECORD_CANDIDATE;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_classify(
            (guardian_node_link_freshness_persistence_status_t)255,
            &record,
            &expected_identity,
            &classification) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    TEST_ASSERT(
        classification ==
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID);

    /*
     * VALID_PERSISTED_STATE requires an actual record.
     */
    classification =
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_VALIDATED_RECORD_CANDIDATE;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_classify(
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE,
            NULL,
            &expected_identity,
            &classification) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT);

    TEST_ASSERT(
        classification ==
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID);

    /*
     * Invalid expected identity fails closed even when provider says VALID.
     */
    invalid_expected_identity = expected_identity;
    invalid_expected_identity.sender_node_id = 0U;

    classification =
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_VALIDATED_RECORD_CANDIDATE;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_classify(
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE,
            &record,
            &invalid_expected_identity,
            &classification) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE);

    TEST_ASSERT(
        classification ==
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID);

    /*
     * Null argument behavior also invalidates caller-visible output whenever
     * the output pointer itself exists.
     */
    classification =
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_VALIDATED_RECORD_CANDIDATE;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_classify(
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE,
            &record,
            NULL,
            &classification) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT);

    TEST_ASSERT(
        classification ==
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID);

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_classify(
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE,
            &record,
            &expected_identity,
            NULL) ==
        GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT);

    return 0;
}

typedef struct
{
    guardian_node_link_freshness_persistence_operation_result_t
        load_result;

    guardian_node_link_freshness_persistence_operation_result_t
        write_result;

    guardian_node_link_freshness_persistence_operation_result_t
        verify_result;

    guardian_node_link_freshness_persistence_operation_result_t
        commit_result;

    unsigned int load_calls;
    unsigned int write_calls;
    unsigned int verify_calls;
    unsigned int commit_calls;
} r3c_c_fake_provider_context_t;

static guardian_node_link_freshness_persistence_operation_result_t
r3c_c_fake_load(
    void *context,
    guardian_node_link_freshness_persistence_status_t *provider_status,
    guardian_node_link_freshness_persisted_record_t *record)
{
    r3c_c_fake_provider_context_t *provider_context;

    provider_context =
        (r3c_c_fake_provider_context_t *)context;

    if (provider_context == NULL)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    provider_context->load_calls += 1U;

    if (provider_status != NULL)
    {
        *provider_status =
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_NO_PERSISTED_STATE;
    }

    if (record != NULL)
    {
        (void)memset(record, 0, sizeof(*record));
    }

    return provider_context->load_result;
}

static guardian_node_link_freshness_persistence_operation_result_t
r3c_c_fake_write(
    void *context,
    const guardian_node_link_freshness_persisted_record_t *candidate)
{
    r3c_c_fake_provider_context_t *provider_context;

    provider_context =
        (r3c_c_fake_provider_context_t *)context;

    if ((provider_context == NULL) ||
        (candidate == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    provider_context->write_calls += 1U;

    return provider_context->write_result;
}

static guardian_node_link_freshness_persistence_operation_result_t
r3c_c_fake_verify(
    void *context,
    const guardian_node_link_freshness_persisted_record_t *candidate)
{
    r3c_c_fake_provider_context_t *provider_context;

    provider_context =
        (r3c_c_fake_provider_context_t *)context;

    if ((provider_context == NULL) ||
        (candidate == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    provider_context->verify_calls += 1U;

    return provider_context->verify_result;
}

static guardian_node_link_freshness_persistence_operation_result_t
r3c_c_fake_commit(
    void *context,
    const guardian_node_link_freshness_persisted_record_t *candidate)
{
    r3c_c_fake_provider_context_t *provider_context;

    provider_context =
        (r3c_c_fake_provider_context_t *)context;

    if ((provider_context == NULL) ||
        (candidate == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    provider_context->commit_calls += 1U;

    return provider_context->commit_result;
}

static void r3c_c_fake_provider_init(
    r3c_c_fake_provider_context_t *context,
    guardian_node_link_freshness_persistence_provider_t *provider)
{
    (void)memset(context, 0, sizeof(*context));
    (void)memset(provider, 0, sizeof(*provider));

    context->load_result =
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;

    context->write_result =
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;

    context->verify_result =
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;

    context->commit_result =
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;

    provider->context = context;
    provider->load = r3c_c_fake_load;
    provider->write_candidate = r3c_c_fake_write;
    provider->verify_candidate = r3c_c_fake_verify;
    provider->commit_candidate = r3c_c_fake_commit;
}

static int test_r3c_c_persistence_transaction(void)
{
    r3c_c_fake_provider_context_t context;
    guardian_node_link_freshness_persistence_provider_t provider;
    guardian_node_link_freshness_persisted_record_t previous;
    guardian_node_link_freshness_persisted_record_t candidate;
    guardian_node_link_freshness_identity_t expected_identity;
    guardian_node_link_freshness_persistence_transaction_outcome_t outcome;

    previous = make_valid_r3c_b_persisted_record();
    expected_identity = previous.identity;

    r3c_c_fake_provider_init(&context, &provider);

    candidate = previous;
    candidate.record_generation =
        previous.record_generation + 1U;
    candidate.accepted_sequence += 1U;

    outcome =
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_INVALID;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_transact_commit(
            &provider,
            &previous,
            &expected_identity,
            &candidate,
            &outcome) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK);

    TEST_ASSERT(
        outcome ==
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_COMMITTED);

    TEST_ASSERT(context.write_calls == 1U);
    TEST_ASSERT(context.verify_calls == 1U);
    TEST_ASSERT(context.commit_calls == 1U);
    TEST_ASSERT(context.load_calls == 0U);

    /*
     * Legitimate no-prior-state bootstrap begins at generation zero.
     */
    r3c_c_fake_provider_init(&context, &provider);

    candidate = make_valid_r3c_b_persisted_record();
    candidate.record_generation = 0U;

    outcome =
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_INVALID;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_transact_commit(
            &provider,
            NULL,
            &candidate.identity,
            &candidate,
            &outcome) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK);

    TEST_ASSERT(
        outcome ==
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_COMMITTED);

    TEST_ASSERT(context.write_calls == 1U);
    TEST_ASSERT(context.verify_calls == 1U);
    TEST_ASSERT(context.commit_calls == 1U);

    /*
     * Candidate generation must be exactly previous + 1.
     */
    r3c_c_fake_provider_init(&context, &provider);

    previous = make_valid_r3c_b_persisted_record();
    candidate = previous;
    candidate.record_generation =
        previous.record_generation + 2U;

    outcome =
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_COMMITTED;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_transact_commit(
            &provider,
            &previous,
            &previous.identity,
            &candidate,
            &outcome) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID);

    TEST_ASSERT(
        outcome ==
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_ABORTED);

    TEST_ASSERT(context.write_calls == 0U);
    TEST_ASSERT(context.verify_calls == 0U);
    TEST_ASSERT(context.commit_calls == 0U);

    /*
     * Generation exhaustion is explicit and never wraps.
     */
    r3c_c_fake_provider_init(&context, &provider);

    previous = make_valid_r3c_b_persisted_record();
    previous.record_generation = UINT32_MAX;

    candidate = previous;
    candidate.record_generation = 0U;

    outcome =
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_COMMITTED;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_transact_commit(
            &provider,
            &previous,
            &previous.identity,
            &candidate,
            &outcome) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_GENERATION_EXHAUSTED);

    TEST_ASSERT(
        outcome ==
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_GENERATION_EXHAUSTED);

    TEST_ASSERT(context.write_calls == 0U);
    TEST_ASSERT(context.verify_calls == 0U);
    TEST_ASSERT(context.commit_calls == 0U);

    /*
     * Write failure prevents verify and commit.
     */
    r3c_c_fake_provider_init(&context, &provider);

    previous = make_valid_r3c_b_persisted_record();
    candidate = previous;
    candidate.accepted_sequence += 1U;
    candidate.record_generation += 1U;

    context.write_result =
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_IO_FAILURE;

    outcome =
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_COMMITTED;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_transact_commit(
            &provider,
            &previous,
            &previous.identity,
            &candidate,
            &outcome) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_IO_FAILURE);

    TEST_ASSERT(
        outcome ==
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_ABORTED);

    TEST_ASSERT(context.write_calls == 1U);
    TEST_ASSERT(context.verify_calls == 0U);
    TEST_ASSERT(context.commit_calls == 0U);

    /*
     * Verify failure prevents commit.
     */
    r3c_c_fake_provider_init(&context, &provider);

    context.verify_result =
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_VERIFY_FAILURE;

    outcome =
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_COMMITTED;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_transact_commit(
            &provider,
            &previous,
            &previous.identity,
            &candidate,
            &outcome) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_VERIFY_FAILURE);

    TEST_ASSERT(
        outcome ==
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_ABORTED);

    TEST_ASSERT(context.write_calls == 1U);
    TEST_ASSERT(context.verify_calls == 1U);
    TEST_ASSERT(context.commit_calls == 0U);

    /*
     * Commit failure cannot be reported as committed.
     */
    r3c_c_fake_provider_init(&context, &provider);

    context.commit_result =
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_COMMIT_FAILURE;

    outcome =
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_COMMITTED;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_transact_commit(
            &provider,
            &previous,
            &previous.identity,
            &candidate,
            &outcome) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_COMMIT_FAILURE);

    TEST_ASSERT(
        outcome ==
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_ABORTED);

    TEST_ASSERT(context.write_calls == 1U);
    TEST_ASSERT(context.verify_calls == 1U);
    TEST_ASSERT(context.commit_calls == 1U);

    /*
     * Unknown commit outcome is explicitly uncertain.
     */
    r3c_c_fake_provider_init(&context, &provider);

    context.commit_result =
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN;

    outcome =
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_COMMITTED;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_transact_commit(
            &provider,
            &previous,
            &previous.identity,
            &candidate,
            &outcome) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN);

    TEST_ASSERT(
        outcome ==
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_STATE_UNCERTAIN);

    /*
     * An out-of-vocabulary provider result fails closed.
     */
    r3c_c_fake_provider_init(&context, &provider);

    context.write_result =
        (guardian_node_link_freshness_persistence_operation_result_t)255;

    outcome =
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_COMMITTED;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_transact_commit(
            &provider,
            &previous,
            &previous.identity,
            &candidate,
            &outcome) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID);

    TEST_ASSERT(
        outcome ==
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_STATE_UNCERTAIN);

    TEST_ASSERT(context.write_calls == 1U);
    TEST_ASSERT(context.verify_calls == 0U);
    TEST_ASSERT(context.commit_calls == 0U);

    /*
     * A valid result from the wrong transaction phase also fails closed.
     */
    r3c_c_fake_provider_init(&context, &provider);

    context.write_result =
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_COMMIT_FAILURE;

    outcome =
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_COMMITTED;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_transact_commit(
            &provider,
            &previous,
            &previous.identity,
            &candidate,
            &outcome) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID);

    TEST_ASSERT(
        outcome ==
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_STATE_UNCERTAIN);

    /*
     * Malformed candidate is rejected before provider callbacks.
     */
    r3c_c_fake_provider_init(&context, &provider);

    candidate = previous;
    candidate.record_generation += 1U;
    candidate.accepted_sequence = 0U;

    outcome =
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_COMMITTED;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_transact_commit(
            &provider,
            &previous,
            &previous.identity,
            &candidate,
            &outcome) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID);

    TEST_ASSERT(
        outcome ==
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_ABORTED);

    TEST_ASSERT(context.write_calls == 0U);
    TEST_ASSERT(context.verify_calls == 0U);
    TEST_ASSERT(context.commit_calls == 0U);

    /*
     * Malformed previous committed state also blocks the transaction.
     */
    r3c_c_fake_provider_init(&context, &provider);

    previous = make_valid_r3c_b_persisted_record();
    previous.schema_version = 0U;

    candidate = make_valid_r3c_b_persisted_record();
    candidate.record_generation = 2U;

    outcome =
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_COMMITTED;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_transact_commit(
            &provider,
            &previous,
            &candidate.identity,
            &candidate,
            &outcome) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID);

    TEST_ASSERT(
        outcome ==
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_ABORTED);

    TEST_ASSERT(context.write_calls == 0U);
    TEST_ASSERT(context.verify_calls == 0U);
    TEST_ASSERT(context.commit_calls == 0U);

    /*
     * Missing provider callback is rejected before any storage operation.
     */
    r3c_c_fake_provider_init(&context, &provider);

    previous = make_valid_r3c_b_persisted_record();
    candidate = previous;
    candidate.record_generation += 1U;

    provider.verify_candidate = NULL;

    outcome =
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_COMMITTED;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_transact_commit(
            &provider,
            &previous,
            &previous.identity,
            &candidate,
            &outcome) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID);

    TEST_ASSERT(
        outcome ==
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_INVALID);

    TEST_ASSERT(context.write_calls == 0U);
    TEST_ASSERT(context.verify_calls == 0U);
    TEST_ASSERT(context.commit_calls == 0U);

    /*
     * Null public arguments fail closed.
     */
    r3c_c_fake_provider_init(&context, &provider);

    outcome =
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_COMMITTED;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_transact_commit(
            NULL,
            NULL,
            &candidate.identity,
            &candidate,
            &outcome) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID);

    TEST_ASSERT(
        outcome ==
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_INVALID);

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_transact_commit(
            &provider,
            NULL,
            NULL,
            &candidate,
            &outcome) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID);

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_transact_commit(
            &provider,
            NULL,
            &candidate.identity,
            NULL,
            &outcome) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID);

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_transact_commit(
            &provider,
            NULL,
            &candidate.identity,
            &candidate,
            NULL) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID);

    return 0;
}
static guardian_node_link_freshness_persistence_operation_result_t
r3c_c_hostile_mutating_write(
    void *context,
    const guardian_node_link_freshness_persisted_record_t *candidate)
{
    r3c_c_fake_provider_context_t *provider_context;
    guardian_node_link_freshness_persisted_record_t *mutable_candidate;

    provider_context =
        (r3c_c_fake_provider_context_t *)context;

    if ((provider_context == NULL) ||
        (candidate == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    provider_context->write_calls += 1U;

    mutable_candidate =
        (guardian_node_link_freshness_persisted_record_t *)candidate;

    mutable_candidate->accepted_sequence = 1U;
    mutable_candidate->record_generation = 0U;

    return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
}

static int test_r3c_c_td_014_td_016_regressions(void)
{
    r3c_c_fake_provider_context_t context;
    guardian_node_link_freshness_persistence_provider_t provider;
    guardian_node_link_freshness_persisted_record_t previous;
    guardian_node_link_freshness_persisted_record_t candidate;
    guardian_node_link_freshness_persisted_record_t caller_before;
    guardian_node_link_freshness_persistence_transaction_outcome_t outcome;

    /*
     * TD-014: a newer record_generation must not carry a lower accepted
     * sequence within the same accepted epoch.
     */
    r3c_c_fake_provider_init(&context, &provider);

    previous = make_valid_r3c_b_persisted_record();
    previous.accepted_epoch = 7U;
    previous.accepted_sequence = 100U;
    previous.record_generation = 10U;

    candidate = previous;
    candidate.accepted_sequence = 99U;
    candidate.record_generation = 11U;

    outcome =
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_COMMITTED;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_transact_commit(
            &provider,
            &previous,
            &previous.identity,
            &candidate,
            &outcome) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID);

    TEST_ASSERT(
        outcome ==
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_ABORTED);

    TEST_ASSERT(context.write_calls == 0U);
    TEST_ASSERT(context.verify_calls == 0U);
    TEST_ASSERT(context.commit_calls == 0U);

    /*
     * R3C-C must not silently authorize a different accepted epoch.
     */
    r3c_c_fake_provider_init(&context, &provider);

    candidate = previous;
    candidate.accepted_epoch = previous.accepted_epoch + 1U;
    candidate.accepted_sequence = 1U;
    candidate.record_generation = 11U;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_transact_commit(
            &provider,
            &previous,
            &previous.identity,
            &candidate,
            &outcome) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID);

    TEST_ASSERT(context.write_calls == 0U);
    TEST_ASSERT(context.verify_calls == 0U);
    TEST_ASSERT(context.commit_calls == 0U);

    /*
     * TD-016: a hostile provider that casts away const and mutates the
     * transaction object must be detected before COMMITTED can be reported.
     *
     * The caller-owned candidate must remain unchanged because callbacks
     * operate only on the core-owned transaction copy.
     */
    r3c_c_fake_provider_init(&context, &provider);

    candidate = previous;
    candidate.accepted_sequence = 101U;
    candidate.record_generation = 11U;

    caller_before = candidate;

    provider.write_candidate =
        r3c_c_hostile_mutating_write;

    outcome =
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_COMMITTED;

    TEST_ASSERT(
        guardian_node_link_freshness_persistence_transact_commit(
            &provider,
            &previous,
            &previous.identity,
            &candidate,
            &outcome) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID);

    TEST_ASSERT(
        outcome ==
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_STATE_UNCERTAIN);

    TEST_ASSERT(
        memcmp(
            &candidate,
            &caller_before,
            sizeof(candidate)) == 0);

    TEST_ASSERT(context.write_calls == 1U);
    TEST_ASSERT(context.verify_calls == 0U);
    TEST_ASSERT(context.commit_calls == 0U);

    return 0;
}
int main(void)
{
    TEST_ASSERT(test_r3c_c_td_014_td_016_regressions() == 0);
    TEST_ASSERT(test_r3c_c_persistence_transaction() == 0);
    TEST_ASSERT(test_r3c_b_persistence_classification() == 0);

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

    (void)printf(
        "C5_R3C_B_PERSISTENCE_CLASSIFICATION_HOST_TEST=PASS\n");

    (void)printf(
        "R3C_B_VALIDATED_RECORD_CANDIDATE_ESTABLISHES_FRESHNESS=NO\n");

    (void)printf(
        "R3C_B_RECORD_GENERATION_IS_ROLLBACK_ANCHOR=NO\n");

    (void)printf(
        "R3C_B_RUNTIME_MUTATION=NO\n");

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