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

    (void)printf("C5_R3A_FRESHNESS_STATE_HOST_TEST=PASS\n");

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