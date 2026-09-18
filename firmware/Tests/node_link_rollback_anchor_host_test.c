#include "guardian_node_link_rollback_anchor.h"

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

/*
 * C5-R3D-B positive host tests.
 *
 * These tests exercise only the abstract rollback-anchor software contract.
 *
 * They do not demonstrate:
 *
 * - physical rollback resistance;
 * - persistent anti-replay;
 * - durable anchor storage;
 * - power-loss recovery;
 * - recovery orchestration;
 * - NodeSupervisor authority;
 * - actuation authority; or
 * - AI authority.
 */

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

    (void)memset(
        destination,
        0,
        destination_capacity);

    (void)memcpy(
        destination,
        value,
        length);
}

static guardian_node_link_freshness_identity_t
make_valid_identity(void)
{
    guardian_node_link_freshness_identity_t identity;

    (void)memset(
        &identity,
        0,
        sizeof(identity));

    identity.sender_node_id = 0x1001U;
    identity.producer_id = 0x2001U;
    identity.key_id = 0x3001U;

    identity.signature_algorithm =
        GUARDIAN_NODE_LINK_SIGNATURE_ED25519;

    set_ref(
        identity.producer_semantic_profile_id,
        sizeof(identity.producer_semantic_profile_id),
        "guardian:test:r3d:producer:v1");

    set_ref(
        identity.consumer_semantic_profile_id,
        sizeof(identity.consumer_semantic_profile_id),
        "guardian:test:r3d:consumer:v1");

    set_ref(
        identity.compatibility_contract_id,
        sizeof(identity.compatibility_contract_id),
        "guardian:test:r3d:compat:v1");

    return identity;
}

static guardian_node_link_freshness_persisted_record_t
make_valid_record(
    uint32_t generation,
    uint32_t accepted_sequence)
{
    guardian_node_link_freshness_persisted_record_t record;

    (void)memset(
        &record,
        0,
        sizeof(record));

    record.schema_version =
        GUARDIAN_NODE_LINK_FRESHNESS_PERSISTENCE_SCHEMA_VERSION;

    record.identity =
        make_valid_identity();

    record.accepted_epoch = 7U;
    record.accepted_sequence = accepted_sequence;
    record.record_generation = generation;

    return record;
}

static int identity_equal(
    const guardian_node_link_freshness_identity_t *left,
    const guardian_node_link_freshness_identity_t *right)
{
    if ((left == NULL) || (right == NULL))
    {
        return 0;
    }

    if ((left->sender_node_id != right->sender_node_id) ||
        (left->producer_id != right->producer_id) ||
        (left->key_id != right->key_id) ||
        (left->signature_algorithm != right->signature_algorithm))
    {
        return 0;
    }

    if (strncmp(
            left->producer_semantic_profile_id,
            right->producer_semantic_profile_id,
            GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY) != 0)
    {
        return 0;
    }

    if (strncmp(
            left->consumer_semantic_profile_id,
            right->consumer_semantic_profile_id,
            GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY) != 0)
    {
        return 0;
    }

    if (strncmp(
            left->compatibility_contract_id,
            right->compatibility_contract_id,
            GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY) != 0)
    {
        return 0;
    }

    return 1;
}

static int commitment_is_zero(
    const guardian_node_link_rollback_anchor_commitment_t *commitment)
{
    size_t index;

    if (commitment == NULL)
    {
        return 1;
    }

    for (index = 0U;
         index <
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMMITMENT_SIZE;
         index += 1U)
    {
        if (commitment->bytes[index] != 0U)
        {
            return 0;
        }
    }

    return 1;
}

/*
 * PT01 - A valid canonical persisted record produces one commitment.
 */
static int test_pt01_commitment_valid_record_returns_ok(void)
{
    guardian_node_link_freshness_persisted_record_t record;
    guardian_node_link_rollback_anchor_commitment_t commitment;

    record = make_valid_record(10U, 100U);

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_commitment_from_record(
            &record,
            &commitment) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(!commitment_is_zero(&commitment));

    return 0;
}

/*
 * PT02 - Equal canonical records deterministically produce equal commitments.
 */
static int test_pt02_commitment_is_deterministic(void)
{
    guardian_node_link_freshness_persisted_record_t record;
    guardian_node_link_rollback_anchor_commitment_t first;
    guardian_node_link_rollback_anchor_commitment_t second;

    record = make_valid_record(10U, 100U);

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_commitment_from_record(
            &record,
            &first) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_commitment_from_record(
            &record,
            &second) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(
        memcmp(
            first.bytes,
            second.bytes,
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMMITMENT_SIZE) == 0);

    return 0;
}

/*
 * PT03 - A governed canonical-state change participates in the commitment.
 */
static int test_pt03_commitment_changes_with_canonical_state(void)
{
    guardian_node_link_freshness_persisted_record_t first_record;
    guardian_node_link_freshness_persisted_record_t second_record;
    guardian_node_link_rollback_anchor_commitment_t first;
    guardian_node_link_rollback_anchor_commitment_t second;

    first_record = make_valid_record(10U, 100U);
    second_record = first_record;
    second_record.accepted_sequence = 101U;

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_commitment_from_record(
            &first_record,
            &first) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_commitment_from_record(
            &second_record,
            &second) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(
        memcmp(
            first.bytes,
            second.bytes,
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMMITMENT_SIZE) != 0);

    return 0;
}

/*
 * PT04 - Exact identity, generation, and commitment classify as MATCH.
 */
static int test_pt04_compare_match(void)
{
    guardian_node_link_freshness_persisted_record_t candidate;
    guardian_node_link_rollback_anchor_record_t anchor;
    guardian_node_link_rollback_anchor_compare_t comparison;

    candidate = make_valid_record(10U, 100U);

    (void)memset(&anchor, 0, sizeof(anchor));

    anchor.identity = candidate.identity;
    anchor.generation = candidate.record_generation;

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_commitment_from_record(
            &candidate,
            &anchor.commitment) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    comparison =
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_INVALID;

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_compare(
            &anchor,
            &candidate,
            &comparison) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(
        comparison ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_MATCH);

    return 0;
}

/*
 * PT05 - A candidate generation below the independent anchor is BEHIND.
 */
static int test_pt05_compare_behind(void)
{
    guardian_node_link_freshness_persisted_record_t candidate;
    guardian_node_link_rollback_anchor_record_t anchor;
    guardian_node_link_rollback_anchor_compare_t comparison;

    candidate = make_valid_record(10U, 100U);

    (void)memset(&anchor, 0, sizeof(anchor));

    anchor.identity = candidate.identity;
    anchor.generation = 11U;

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_compare(
            &anchor,
            &candidate,
            &comparison) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(
        comparison ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_BEHIND);

    return 0;
}

/*
 * PT06 - A candidate generation above the independent anchor is AHEAD.
 */
static int test_pt06_compare_ahead(void)
{
    guardian_node_link_freshness_persisted_record_t candidate;
    guardian_node_link_rollback_anchor_record_t anchor;
    guardian_node_link_rollback_anchor_compare_t comparison;

    candidate = make_valid_record(11U, 100U);

    (void)memset(&anchor, 0, sizeof(anchor));

    anchor.identity = candidate.identity;
    anchor.generation = 10U;

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_compare(
            &anchor,
            &candidate,
            &comparison) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(
        comparison ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_AHEAD);

    return 0;
}

/*
 * PT07 - Generation equality does not erase exact identity separation.
 */
static int test_pt07_compare_identity_mismatch(void)
{
    guardian_node_link_freshness_persisted_record_t candidate;
    guardian_node_link_rollback_anchor_record_t anchor;
    guardian_node_link_rollback_anchor_compare_t comparison;

    candidate = make_valid_record(10U, 100U);

    (void)memset(&anchor, 0, sizeof(anchor));

    anchor.identity = candidate.identity;
    anchor.identity.key_id += 1U;
    anchor.generation = candidate.record_generation;

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_compare(
            &anchor,
            &candidate,
            &comparison) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(
        comparison ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_IDENTITY_MISMATCH);

    return 0;
}

/*
 * PT08 - Same generation with a different canonical commitment fails MATCH.
 */
static int test_pt08_compare_same_generation_state_mismatch(void)
{
    guardian_node_link_freshness_persisted_record_t anchored_record;
    guardian_node_link_freshness_persisted_record_t candidate;
    guardian_node_link_rollback_anchor_record_t anchor;
    guardian_node_link_rollback_anchor_compare_t comparison;

    anchored_record = make_valid_record(10U, 100U);
    candidate = anchored_record;
    candidate.accepted_sequence = 101U;

    (void)memset(&anchor, 0, sizeof(anchor));

    anchor.identity = anchored_record.identity;
    anchor.generation = anchored_record.record_generation;

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_commitment_from_record(
            &anchored_record,
            &anchor.commitment) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_compare(
            &anchor,
            &candidate,
            &comparison) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(
        comparison ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_STATE_MISMATCH);

    return 0;
}

/*
 * PT09 - The ordinary generation domain starts with 0 -> 1.
 */
static int test_pt09_next_generation_zero_to_one(void)
{
    uint32_t next_generation;

    next_generation = 99U;

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_next_generation(
            0U,
            &next_generation) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(next_generation == 1U);

    return 0;
}

/*
 * PT10 - Ordinary non-boundary generation progression is N -> N + 1.
 */
static int test_pt10_next_generation_ordinary_progress(void)
{
    uint32_t next_generation;

    next_generation = 0U;

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_next_generation(
            41U,
            &next_generation) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(next_generation == 42U);

    return 0;
}

typedef struct
{
    guardian_node_link_freshness_identity_t expected_identity;

    guardian_node_link_rollback_anchor_record_t read_record;

    guardian_node_link_rollback_anchor_commitment_t
        expected_next_commitment;

    uint32_t expected_generation;
    uint32_t expected_next_generation;

    unsigned int status_calls;
    unsigned int read_calls;
    unsigned int advance_calls;

    int callback_argument_failure;
} fake_anchor_provider_context_t;

static guardian_node_link_rollback_anchor_provider_result_t
fake_status(
    void *context,
    const guardian_node_link_freshness_identity_t *identity,
    guardian_node_link_rollback_anchor_status_t *status)
{
    fake_anchor_provider_context_t *fake;

    fake = (fake_anchor_provider_context_t *)context;

    if ((fake == NULL) ||
        (identity == NULL) ||
        (status == NULL))
    {
        return
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT;
    }

    fake->status_calls += 1U;

    if (!identity_equal(
            identity,
            &fake->expected_identity))
    {
        fake->callback_argument_failure = 1;
    }

    *status =
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_VALID;

    return
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK;
}

static guardian_node_link_rollback_anchor_provider_result_t
fake_read(
    void *context,
    const guardian_node_link_freshness_identity_t *identity,
    guardian_node_link_rollback_anchor_record_t *record,
    guardian_node_link_rollback_anchor_status_t *status)
{
    fake_anchor_provider_context_t *fake;

    fake = (fake_anchor_provider_context_t *)context;

    if ((fake == NULL) ||
        (identity == NULL) ||
        (record == NULL) ||
        (status == NULL))
    {
        return
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT;
    }

    fake->read_calls += 1U;

    if (!identity_equal(
            identity,
            &fake->expected_identity))
    {
        fake->callback_argument_failure = 1;
    }

    *record = fake->read_record;

    *status =
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_VALID;

    return
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK;
}

static guardian_node_link_rollback_anchor_provider_result_t
fake_advance(
    void *context,
    const guardian_node_link_freshness_identity_t *identity,
    uint32_t expected_generation,
    uint32_t next_generation,
    const guardian_node_link_rollback_anchor_commitment_t
        *next_commitment)
{
    fake_anchor_provider_context_t *fake;

    fake = (fake_anchor_provider_context_t *)context;

    if ((fake == NULL) ||
        (identity == NULL) ||
        (next_commitment == NULL))
    {
        return
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT;
    }

    fake->advance_calls += 1U;

    if (!identity_equal(
            identity,
            &fake->expected_identity))
    {
        fake->callback_argument_failure = 1;
    }

    if (expected_generation != fake->expected_generation)
    {
        fake->callback_argument_failure = 1;
    }

    if (next_generation != fake->expected_next_generation)
    {
        fake->callback_argument_failure = 1;
    }

    if (memcmp(
            next_commitment->bytes,
            fake->expected_next_commitment.bytes,
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMMITMENT_SIZE) != 0)
    {
        fake->callback_argument_failure = 1;
    }

    return
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK;
}

static void fake_provider_init(
    fake_anchor_provider_context_t *context,
    guardian_node_link_rollback_anchor_provider_t *provider)
{
    guardian_node_link_freshness_persisted_record_t record;

    (void)memset(
        context,
        0,
        sizeof(*context));

    (void)memset(
        provider,
        0,
        sizeof(*provider));

    context->expected_identity =
        make_valid_identity();

    record =
        make_valid_record(
            10U,
            100U);

    context->read_record.identity =
        record.identity;

    context->read_record.generation =
        record.record_generation;

    (void)guardian_node_link_rollback_anchor_commitment_from_record(
        &record,
        &context->read_record.commitment);

    context->expected_generation = 10U;
    context->expected_next_generation = 11U;

    record.record_generation = 11U;
    record.accepted_sequence = 101U;

    (void)guardian_node_link_rollback_anchor_commitment_from_record(
        &record,
        &context->expected_next_commitment);

    provider->context = context;
    provider->status = fake_status;
    provider->read = fake_read;
    provider->advance = fake_advance;
}

/*
 * PT11 - status delegates exactly once to the abstract provider.
 */
static int test_pt11_provider_status_delegates(void)
{
    fake_anchor_provider_context_t context;
    guardian_node_link_rollback_anchor_provider_t provider;
    guardian_node_link_rollback_anchor_status_t status;

    fake_provider_init(
        &context,
        &provider);

    status =
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_INVALID;

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_status(
            &provider,
            &context.expected_identity,
            &status) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(context.status_calls == 1U);
    TEST_ASSERT(context.read_calls == 0U);
    TEST_ASSERT(context.advance_calls == 0U);
    TEST_ASSERT(context.callback_argument_failure == 0);

    TEST_ASSERT(
        status ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_VALID);

    return 0;
}

/*
 * PT12 - read and ordinary advance delegate exact governed arguments.
 */
static int test_pt12_provider_read_and_advance_delegate(void)
{
    fake_anchor_provider_context_t context;
    guardian_node_link_rollback_anchor_provider_t provider;
    guardian_node_link_rollback_anchor_record_t observed_record;
    guardian_node_link_rollback_anchor_status_t status;

    fake_provider_init(
        &context,
        &provider);

    (void)memset(
        &observed_record,
        0,
        sizeof(observed_record));

    status =
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_INVALID;

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_read(
            &provider,
            &context.expected_identity,
            &observed_record,
            &status) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(context.read_calls == 1U);
    TEST_ASSERT(context.advance_calls == 0U);
    TEST_ASSERT(context.callback_argument_failure == 0);

    TEST_ASSERT(
        status ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_VALID);

    TEST_ASSERT(
        identity_equal(
            &observed_record.identity,
            &context.read_record.identity));

    TEST_ASSERT(
        observed_record.generation ==
        context.read_record.generation);

    TEST_ASSERT(
        memcmp(
            observed_record.commitment.bytes,
            context.read_record.commitment.bytes,
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMMITMENT_SIZE) == 0);

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_advance(
            &provider,
            &context.expected_identity,
            context.expected_generation,
            context.expected_next_generation,
            &context.expected_next_commitment) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(context.advance_calls == 1U);
    TEST_ASSERT(context.callback_argument_failure == 0);

    return 0;
}

/*
 * R3D-B negative / hostile host tests.
 *
 * This slice demonstrates only abstract software fail-closed behavior.
 *
 * It does not demonstrate:
 * - physical rollback-resistant storage;
 * - persistent anti-replay across resets;
 * - recovery / rejoin orchestration;
 * - automatic authority;
 * - actuator authority;
 * - AI authority.
 */

static int test_nt01_commitment_null_record_rejected(void)
{
    fake_anchor_provider_context_t context;
    guardian_node_link_rollback_anchor_provider_t provider;

    fake_provider_init(
        &context,
        &provider);

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_commitment_from_record(
            NULL,
            &context.expected_next_commitment) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT);

    return 0;
}

static int test_nt02_commitment_null_output_rejected(void)
{
    guardian_node_link_freshness_persisted_record_t candidate;

    candidate = make_valid_record(10U, 100U);

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_commitment_from_record(
            &candidate,
            NULL) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT);

    return 0;
}

static int test_nt03_compare_null_argument_rejected(void)
{
    {
        guardian_node_link_freshness_persisted_record_t candidate;
        guardian_node_link_rollback_anchor_compare_t comparison;

        candidate = make_valid_record(10U, 100U);

        comparison =
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_MATCH;

        TEST_ASSERT(
            guardian_node_link_rollback_anchor_compare(
                NULL,
                &candidate,
                &comparison) ==
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT);

        TEST_ASSERT(
            comparison ==
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_INVALID);

    }

    {
        guardian_node_link_rollback_anchor_record_t anchor;
        guardian_node_link_rollback_anchor_compare_t comparison;

        (void)memset(
            &anchor,
            0,
            sizeof(anchor));

        comparison =
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_MATCH;

        TEST_ASSERT(
            guardian_node_link_rollback_anchor_compare(
                &anchor,
                NULL,
                &comparison) ==
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT);

        TEST_ASSERT(
            comparison ==
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_INVALID);

    }

    {
        guardian_node_link_freshness_persisted_record_t candidate;
        guardian_node_link_rollback_anchor_record_t anchor;

        candidate = make_valid_record(10U, 100U);

        (void)memset(
            &anchor,
            0,
            sizeof(anchor));

        anchor.identity = candidate.identity;
        anchor.generation = candidate.record_generation;

        TEST_ASSERT(
            guardian_node_link_rollback_anchor_compare(
                &anchor,
                &candidate,
                NULL) ==
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT);

    }

    return 0;
}

static int test_nt04_compare_identity_mismatch_reported(void)
{
    guardian_node_link_freshness_persisted_record_t candidate;
    guardian_node_link_rollback_anchor_record_t anchor;
    guardian_node_link_rollback_anchor_compare_t comparison;

    candidate = make_valid_record(10U, 100U);

    (void)memset(&anchor, 0, sizeof(anchor));

    anchor.identity = candidate.identity;
    anchor.identity.key_id += 1U;
    anchor.generation = candidate.record_generation;

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_compare(
            &anchor,
            &candidate,
            &comparison) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(
        comparison ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_IDENTITY_MISMATCH);

    return 0;
}

static int test_nt05_compare_behind_reported(void)
{
    guardian_node_link_freshness_persisted_record_t candidate;
    guardian_node_link_rollback_anchor_record_t anchor;
    guardian_node_link_rollback_anchor_compare_t comparison;

    candidate = make_valid_record(10U, 100U);

    (void)memset(&anchor, 0, sizeof(anchor));

    anchor.identity = candidate.identity;
    anchor.generation = 11U;

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_compare(
            &anchor,
            &candidate,
            &comparison) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(
        comparison ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_BEHIND);

    return 0;
}

static int test_nt06_compare_ahead_reported(void)
{
    guardian_node_link_freshness_persisted_record_t candidate;
    guardian_node_link_rollback_anchor_record_t anchor;
    guardian_node_link_rollback_anchor_compare_t comparison;

    candidate = make_valid_record(11U, 100U);

    (void)memset(&anchor, 0, sizeof(anchor));

    anchor.identity = candidate.identity;
    anchor.generation = 10U;

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_compare(
            &anchor,
            &candidate,
            &comparison) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(
        comparison ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_AHEAD);

    return 0;
}

static int test_nt07_compare_same_generation_state_mismatch_reported(void)
{
    guardian_node_link_freshness_persisted_record_t anchored_record;
    guardian_node_link_freshness_persisted_record_t candidate;
    guardian_node_link_rollback_anchor_record_t anchor;
    guardian_node_link_rollback_anchor_compare_t comparison;

    anchored_record = make_valid_record(10U, 100U);
    candidate = anchored_record;
    candidate.accepted_sequence = 101U;

    (void)memset(&anchor, 0, sizeof(anchor));

    anchor.identity = anchored_record.identity;
    anchor.generation = anchored_record.record_generation;

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_commitment_from_record(
            &anchored_record,
            &anchor.commitment) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_compare(
            &anchor,
            &candidate,
            &comparison) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);

    TEST_ASSERT(
        comparison ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_STATE_MISMATCH);

    return 0;
}

static int test_nt08_next_generation_null_output_rejected(void)
{
    TEST_ASSERT(
        guardian_node_link_rollback_anchor_next_generation(
            0U,
            NULL) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT);

    return 0;
}

static int test_nt09_next_generation_uint32_max_minus_one_exhausted_no_wrap(void)
{
    uint32_t next_generation;

    next_generation = UINT32_MAX;

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_next_generation(
            UINT32_MAX - 1U,
            &next_generation) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_EXHAUSTED);

    TEST_ASSERT(next_generation == 0U);

    return 0;
}

static int test_nt10_next_generation_uint32_max_exhausted_no_wrap(void)
{
    uint32_t next_generation;

    next_generation = UINT32_MAX;

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_next_generation(
            UINT32_MAX,
            &next_generation) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_EXHAUSTED);

    TEST_ASSERT(next_generation == 0U);

    return 0;
}

static int test_nt11_provider_wrappers_missing_callback_fail_closed(void)
{
    fake_anchor_provider_context_t context;
    guardian_node_link_rollback_anchor_provider_t provider;
    guardian_node_link_rollback_anchor_record_t observed_record;
    guardian_node_link_rollback_anchor_status_t status;

    fake_provider_init(&context, &provider);

    provider.status = NULL;
    status = GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_VALID;

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_status(
            &provider,
            &context.expected_identity,
            &status) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT);

    TEST_ASSERT(
        status ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_INVALID);

    TEST_ASSERT(context.status_calls == 0U);

    fake_provider_init(&context, &provider);
    provider.read = NULL;

    (void)memset(
        &observed_record,
        0,
        sizeof(observed_record));

    status = GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_VALID;

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_read(
            &provider,
            &context.expected_identity,
            &observed_record,
            &status) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT);

    TEST_ASSERT(
        status ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_INVALID);

    TEST_ASSERT(context.read_calls == 0U);

    fake_provider_init(&context, &provider);
    provider.advance = NULL;

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_advance(
            &provider,
            &context.expected_identity,
            context.expected_generation,
            context.expected_next_generation,
            &context.expected_next_commitment) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT);

    TEST_ASSERT(context.advance_calls == 0U);
    TEST_ASSERT(context.callback_argument_failure == 0);

    return 0;
}

static int test_nt12_advance_wrong_next_generation_rejected_before_provider_call(void)
{
    fake_anchor_provider_context_t context;
    guardian_node_link_rollback_anchor_provider_t provider;

    fake_provider_init(
        &context,
        &provider);

    TEST_ASSERT(
        guardian_node_link_rollback_anchor_advance(
            &provider,
            &context.expected_identity,
            5U,
            7U,
            &context.expected_next_commitment) ==
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT);

    TEST_ASSERT(context.advance_calls == 0U);
    TEST_ASSERT(context.callback_argument_failure == 0);

    return 0;
}

typedef int (*negative_test_fn)(void);

typedef struct
{
    const char *name;
    negative_test_fn function;
} negative_test_case_t;

static int run_r3d_b_negative_hostile_tests(void)
{
    static const negative_test_case_t tests[] =
    {
        {
            "NT01_COMMITMENT_NULL_RECORD_REJECTED",
            test_nt01_commitment_null_record_rejected
        },
        {
            "NT02_COMMITMENT_NULL_OUTPUT_REJECTED",
            test_nt02_commitment_null_output_rejected
        },
        {
            "NT03_COMPARE_NULL_ARGUMENT_REJECTED",
            test_nt03_compare_null_argument_rejected
        },
        {
            "NT04_COMPARE_IDENTITY_MISMATCH_REPORTED",
            test_nt04_compare_identity_mismatch_reported
        },
        {
            "NT05_COMPARE_BEHIND_REPORTED",
            test_nt05_compare_behind_reported
        },
        {
            "NT06_COMPARE_AHEAD_REPORTED",
            test_nt06_compare_ahead_reported
        },
        {
            "NT07_COMPARE_SAME_GENERATION_STATE_MISMATCH_REPORTED",
            test_nt07_compare_same_generation_state_mismatch_reported
        },
        {
            "NT08_NEXT_GENERATION_NULL_OUTPUT_REJECTED",
            test_nt08_next_generation_null_output_rejected
        },
        {
            "NT09_NEXT_GENERATION_UINT32_MAX_MINUS_ONE_EXHAUSTED_NO_WRAP",
            test_nt09_next_generation_uint32_max_minus_one_exhausted_no_wrap
        },
        {
            "NT10_NEXT_GENERATION_UINT32_MAX_EXHAUSTED_NO_WRAP",
            test_nt10_next_generation_uint32_max_exhausted_no_wrap
        },
        {
            "NT11_PROVIDER_WRAPPERS_MISSING_CALLBACK_FAIL_CLOSED",
            test_nt11_provider_wrappers_missing_callback_fail_closed
        },
        {
            "NT12_ADVANCE_WRONG_NEXT_GENERATION_REJECTED_BEFORE_PROVIDER_CALL",
            test_nt12_advance_wrong_next_generation_rejected_before_provider_call
        }
    };

    size_t index;

    for (
        index = 0U;
        index < (sizeof(tests) / sizeof(tests[0]));
        index += 1U)
    {
        if (tests[index].function() != 0)
        {
            (void)fprintf(
                stderr,
                "FAIL: %s\n",
                tests[index].name);

            return 1;
        }

        (void)printf(
            "PASS: %s\n",
            tests[index].name);
    }

    (void)printf(
        "R3D_B_NEGATIVE_HOSTILE_TESTS_PASS_COUNT=%u\n",
        (unsigned int)(
            sizeof(tests) /
            sizeof(tests[0])));

    return 0;
}
typedef int (*positive_test_fn)(void);

typedef struct
{
    const char *name;
    positive_test_fn function;
} positive_test_case_t;

int main(void)
{
    static const positive_test_case_t tests[] =
    {
        {
            "PT01_COMMITMENT_VALID_RECORD_RETURNS_OK",
            test_pt01_commitment_valid_record_returns_ok
        },
        {
            "PT02_COMMITMENT_IS_DETERMINISTIC",
            test_pt02_commitment_is_deterministic
        },
        {
            "PT03_COMMITMENT_CHANGES_WHEN_CANONICAL_STATE_CHANGES",
            test_pt03_commitment_changes_with_canonical_state
        },
        {
            "PT04_COMPARE_MATCH",
            test_pt04_compare_match
        },
        {
            "PT05_COMPARE_BEHIND",
            test_pt05_compare_behind
        },
        {
            "PT06_COMPARE_AHEAD",
            test_pt06_compare_ahead
        },
        {
            "PT07_COMPARE_IDENTITY_MISMATCH",
            test_pt07_compare_identity_mismatch
        },
        {
            "PT08_COMPARE_SAME_GENERATION_STATE_MISMATCH",
            test_pt08_compare_same_generation_state_mismatch
        },
        {
            "PT09_NEXT_GENERATION_ZERO_TO_ONE",
            test_pt09_next_generation_zero_to_one
        },
        {
            "PT10_NEXT_GENERATION_ORDINARY_PROGRESS",
            test_pt10_next_generation_ordinary_progress
        },
        {
            "PT11_PROVIDER_STATUS_DELEGATES",
            test_pt11_provider_status_delegates
        },
        {
            "PT12_PROVIDER_READ_AND_ADVANCE_DELEGATE",
            test_pt12_provider_read_and_advance_delegate
        }
    };

    size_t index;

    for (index = 0U;
         index < (sizeof(tests) / sizeof(tests[0]));
         index += 1U)
    {
        if (tests[index].function() != 0)
        {
            (void)fprintf(
                stderr,
                "FAIL: %s\n",
                tests[index].name);

            return 1;
        }

        (void)printf(
            "PASS: %s\n",
            tests[index].name);
    }

    (void)printf(
        "R3D_B_POSITIVE_TESTS_PASS_COUNT=%u\n",
        (unsigned int)(
            sizeof(tests) /
            sizeof(tests[0])));

    if (run_r3d_b_negative_hostile_tests() != 0)
    {
        return 1;
    }

    return 0;
}
