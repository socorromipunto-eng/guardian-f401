#include "guardian_node_link_freshness_persistence.h"

#include <stdint.h>
#include <string.h>

typedef enum
{
    GUARDIAN_NODE_LINK_PERSISTENCE_STAGE_WRITE = 0,
    GUARDIAN_NODE_LINK_PERSISTENCE_STAGE_VERIFY,
    GUARDIAN_NODE_LINK_PERSISTENCE_STAGE_COMMIT
} guardian_node_link_freshness_persistence_stage_t;

static int
guardian_node_link_freshness_persistence_identity_equal(
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

static int
guardian_node_link_freshness_persistence_record_equal(
    const guardian_node_link_freshness_persisted_record_t *left,
    const guardian_node_link_freshness_persisted_record_t *right)
{
    if ((left == NULL) || (right == NULL))
    {
        return 0;
    }

    if ((left->schema_version != right->schema_version) ||
        (left->accepted_epoch != right->accepted_epoch) ||
        (left->accepted_sequence != right->accepted_sequence) ||
        (left->record_generation != right->record_generation))
    {
        return 0;
    }

    return guardian_node_link_freshness_persistence_identity_equal(
        &left->identity,
        &right->identity);
}

static int
guardian_node_link_freshness_persistence_transition_valid(
    const guardian_node_link_freshness_persisted_record_t *previous_record,
    const guardian_node_link_freshness_persisted_record_t *candidate)
{
    if (candidate == NULL)
    {
        return 0;
    }

    if (previous_record == NULL)
    {
        return 1;
    }

    /*
     * R3C-C is not the governed epoch-transition authorization boundary.
     *
     * A different epoch therefore cannot be converted into normal
     * persistence continuation merely because its numeric value is larger.
     *
     * AUTHENTICATED_NEW_EPOCH != AUTHORIZED_EPOCH_TRANSITION
     */
    if (candidate->accepted_epoch != previous_record->accepted_epoch)
    {
        return 0;
    }

    /*
     * Within one accepted epoch, persistence continuation is strictly
     * monotonic in accepted sequence.
     *
     * A newer persistence record_generation cannot carry a replay or
     * rollback of the accepted freshness sequence.
     */
    if (candidate->accepted_sequence <=
        previous_record->accepted_sequence)
    {
        return 0;
    }

    return 1;
}

static int
guardian_node_link_freshness_persistence_result_valid(
    guardian_node_link_freshness_persistence_operation_result_t result)
{
    switch (result)
    {
        case GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK:
        case GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_UNAVAILABLE:
        case GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_IO_FAILURE:
        case GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_VERIFY_FAILURE:
        case GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_COMMIT_FAILURE:
        case GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN:
        case GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_GENERATION_EXHAUSTED:
            return 1;

        case GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID:
        default:
            return 0;
    }
}

static int
guardian_node_link_freshness_persistence_result_valid_for_stage(
    guardian_node_link_freshness_persistence_stage_t stage,
    guardian_node_link_freshness_persistence_operation_result_t result)
{
    if (guardian_node_link_freshness_persistence_result_valid(
            result) == 0)
    {
        return 0;
    }

    switch (stage)
    {
        case GUARDIAN_NODE_LINK_PERSISTENCE_STAGE_WRITE:
            return
                (result ==
                    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK) ||
                (result ==
                    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_UNAVAILABLE) ||
                (result ==
                    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_IO_FAILURE) ||
                (result ==
                    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN);

        case GUARDIAN_NODE_LINK_PERSISTENCE_STAGE_VERIFY:
            return
                (result ==
                    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK) ||
                (result ==
                    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_UNAVAILABLE) ||
                (result ==
                    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_IO_FAILURE) ||
                (result ==
                    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_VERIFY_FAILURE) ||
                (result ==
                    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN);

        case GUARDIAN_NODE_LINK_PERSISTENCE_STAGE_COMMIT:
            return
                (result ==
                    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK) ||
                (result ==
                    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_UNAVAILABLE) ||
                (result ==
                    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_IO_FAILURE) ||
                (result ==
                    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_COMMIT_FAILURE) ||
                (result ==
                    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN);

        default:
            return 0;
    }
}

static guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_freshness_persistence_validate_record(
    const guardian_node_link_freshness_persisted_record_t *record,
    const guardian_node_link_freshness_identity_t *expected_identity)
{
    guardian_node_link_freshness_persistence_classification_t
        classification;

    classification =
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID;

    if (record == NULL)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    if (guardian_node_link_freshness_persistence_classify(
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE,
            record,
            expected_identity,
            &classification) !=
        GUARDIAN_NODE_LINK_FRESHNESS_OK)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    if (classification !=
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_VALIDATED_RECORD_CANDIDATE)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
}

static guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_freshness_persistence_next_generation(
    const guardian_node_link_freshness_persisted_record_t *previous_record,
    uint32_t *next_generation)
{
    if (next_generation == NULL)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    *next_generation = 0U;

    if (previous_record == NULL)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
    }

    if (previous_record->record_generation == UINT32_MAX)
    {
        return
            GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_GENERATION_EXHAUSTED;
    }

    *next_generation =
        previous_record->record_generation + 1U;

    return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
}

static guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_freshness_persistence_handle_stage_result(
    guardian_node_link_freshness_persistence_stage_t stage,
    guardian_node_link_freshness_persistence_operation_result_t result,
    guardian_node_link_freshness_persistence_transaction_outcome_t *outcome)
{
    if (outcome == NULL)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    if (guardian_node_link_freshness_persistence_result_valid_for_stage(
            stage,
            result) == 0)
    {
        *outcome =
            GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_STATE_UNCERTAIN;

        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    if (result ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN)
    {
        *outcome =
            GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_STATE_UNCERTAIN;

        return result;
    }

    if (result !=
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK)
    {
        *outcome =
            GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_ABORTED;

        return result;
    }

    return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
}

guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_freshness_persistence_transact_commit(
    const guardian_node_link_freshness_persistence_provider_t *provider,
    const guardian_node_link_freshness_persisted_record_t *previous_record,
    const guardian_node_link_freshness_identity_t *expected_identity,
    const guardian_node_link_freshness_persisted_record_t *candidate,
    guardian_node_link_freshness_persistence_transaction_outcome_t *outcome)
{
    guardian_node_link_freshness_persistence_operation_result_t result;
    guardian_node_link_freshness_persisted_record_t validated_candidate;
    guardian_node_link_freshness_persisted_record_t transaction_candidate;
    uint32_t expected_generation;

    if (outcome != NULL)
    {
        *outcome =
            GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_INVALID;
    }

    if ((provider == NULL) ||
        (expected_identity == NULL) ||
        (candidate == NULL) ||
        (outcome == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    if ((provider->load == NULL) ||
        (provider->write_candidate == NULL) ||
        (provider->verify_candidate == NULL) ||
        (provider->commit_candidate == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    if (previous_record != NULL)
    {
        result =
            guardian_node_link_freshness_persistence_validate_record(
                previous_record,
                expected_identity);

        if (result !=
            GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK)
        {
            *outcome =
                GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_ABORTED;

            return result;
        }
    }

    result =
        guardian_node_link_freshness_persistence_validate_record(
            candidate,
            expected_identity);

    if (result !=
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK)
    {
        *outcome =
            GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_ABORTED;

        return result;
    }

    result =
        guardian_node_link_freshness_persistence_next_generation(
            previous_record,
            &expected_generation);

    if (result ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_GENERATION_EXHAUSTED)
    {
        *outcome =
            GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_GENERATION_EXHAUSTED;

        return result;
    }

    if (result !=
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK)
    {
        *outcome =
            GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_ABORTED;

        return result;
    }

    if (candidate->record_generation != expected_generation)
    {
        *outcome =
            GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_ABORTED;

        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    if (guardian_node_link_freshness_persistence_transition_valid(
            previous_record,
            candidate) == 0)
    {
        *outcome =
            GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_ABORTED;

        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    /*
     * Freeze the caller-validated logical candidate into an internal
     * transaction snapshot.
     *
     * Provider callbacks receive only the internal copy. A provider that
     * violates the const contract and mutates that copy is detected before
     * the transaction may advance.
     *
     * This is an in-memory logical record copy only. It is not persistence
     * serialization and does not define any canonical byte representation.
     */
    validated_candidate = *candidate;
    transaction_candidate = validated_candidate;

    result =
        provider->write_candidate(
            provider->context,
            &transaction_candidate);

    result =
        guardian_node_link_freshness_persistence_handle_stage_result(
            GUARDIAN_NODE_LINK_PERSISTENCE_STAGE_WRITE,
            result,
            outcome);

    if (result !=
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK)
    {
        return result;
    }

    if (guardian_node_link_freshness_persistence_record_equal(
            &transaction_candidate,
            &validated_candidate) == 0)
    {
        *outcome =
            GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_STATE_UNCERTAIN;

        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    if (guardian_node_link_freshness_persistence_validate_record(
            &transaction_candidate,
            expected_identity) !=
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK)
    {
        *outcome =
            GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_STATE_UNCERTAIN;

        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    result =
        provider->verify_candidate(
            provider->context,
            &transaction_candidate);

    result =
        guardian_node_link_freshness_persistence_handle_stage_result(
            GUARDIAN_NODE_LINK_PERSISTENCE_STAGE_VERIFY,
            result,
            outcome);

    if (result !=
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK)
    {
        return result;
    }

    if (guardian_node_link_freshness_persistence_record_equal(
            &transaction_candidate,
            &validated_candidate) == 0)
    {
        *outcome =
            GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_STATE_UNCERTAIN;

        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    if (guardian_node_link_freshness_persistence_validate_record(
            &transaction_candidate,
            expected_identity) !=
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK)
    {
        *outcome =
            GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_STATE_UNCERTAIN;

        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    result =
        provider->commit_candidate(
            provider->context,
            &transaction_candidate);

    result =
        guardian_node_link_freshness_persistence_handle_stage_result(
            GUARDIAN_NODE_LINK_PERSISTENCE_STAGE_COMMIT,
            result,
            outcome);

    if (result !=
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK)
    {
        return result;
    }

    /*
     * A provider that altered the transaction object during commit has
     * violated the logical provider contract. At this point external storage
     * outcome may be unknowable, so fail closed as STATE_UNCERTAIN rather
     * than reporting COMMITTED.
     */
    if (guardian_node_link_freshness_persistence_record_equal(
            &transaction_candidate,
            &validated_candidate) == 0)
    {
        *outcome =
            GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_STATE_UNCERTAIN;

        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    if (guardian_node_link_freshness_persistence_validate_record(
            &transaction_candidate,
            expected_identity) !=
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK)
    {
        *outcome =
            GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_STATE_UNCERTAIN;

        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    *outcome =
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_COMMITTED;

    return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
}