#include "guardian_node_link_persistent_anti_replay.h"

#include <stdint.h>
#include <string.h>

static void guardian_persistent_anti_replay_evidence_init(
    guardian_node_link_persistent_anti_replay_evidence_t *evidence)
{
    if (evidence != NULL)
    {
        (void)memset(evidence, 0, sizeof(*evidence));
        evidence->persistence_status =
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_INVALID;
        evidence->persistence_classification =
            GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID;
        evidence->anchor_status =
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_INVALID;
        evidence->anchor_comparison =
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_INVALID;
    }
}

static guardian_node_link_persistent_anti_replay_result_t
map_persistence_operation(
    guardian_node_link_freshness_persistence_operation_result_t result)
{
    switch (result)
    {
        case GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_UNAVAILABLE:
            return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_PROVIDER_UNAVAILABLE;

        case GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_GENERATION_EXHAUSTED:
            return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_GENERATION_EXHAUSTED;

        case GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN:
        case GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_IO_FAILURE:
        case GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_VERIFY_FAILURE:
        case GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_COMMIT_FAILURE:
            return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_STATE_UNCERTAIN;

        case GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK:
            return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_OK;

        case GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID:
        default:
            return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_INVALID;
    }
}

static guardian_node_link_persistent_anti_replay_result_t
map_anchor_provider_result(
    guardian_node_link_rollback_anchor_provider_result_t result)
{
    switch (result)
    {
        case GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK:
            return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_OK;

        case GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_UNAVAILABLE:
            return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_PROVIDER_UNAVAILABLE;

        case GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_EXHAUSTED:
            return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_GENERATION_EXHAUSTED;

        case GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_IO_ERROR:
        case GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INTEGRITY_ERROR:
        case GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_CONFLICT:
            return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_STATE_UNCERTAIN;

        case GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT:
        case GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_UNSUPPORTED:
        case GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID:
        default:
            return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_INVALID;
    }
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

static guardian_node_link_persistent_anti_replay_result_t
restore_runtime_from_record(
    const guardian_node_link_freshness_persisted_record_t *record,
    guardian_node_link_freshness_runtime_t *runtime)
{
    guardian_node_link_freshness_runtime_t candidate;

    if ((record == NULL) || (runtime == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_INVALID;
    }

    guardian_node_link_freshness_runtime_init(&candidate);

    candidate.condition = GUARDIAN_NODE_LINK_FRESHNESS_ACTIVE;
    candidate.identity = record->identity;
    candidate.accepted_epoch = record->accepted_epoch;
    candidate.accepted_sequence = record->accepted_sequence;
    candidate.identity_valid = 1U;
    candidate.accepted_epoch_valid = 1U;
    candidate.accepted_sequence_valid = 1U;

    if (guardian_node_link_freshness_runtime_validate(&candidate) !=
        GUARDIAN_NODE_LINK_FRESHNESS_OK)
    {
        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_INVALID;
    }

    *runtime = candidate;
    return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_OK;
}

guardian_node_link_persistent_anti_replay_result_t
guardian_node_link_persistent_anti_replay_recover(
    const guardian_node_link_freshness_persistence_provider_t *persistence_provider,
    const guardian_node_link_rollback_anchor_provider_t *anchor_provider,
    const guardian_node_link_freshness_identity_t *expected_identity,
    guardian_node_link_freshness_runtime_t *runtime,
    guardian_node_link_freshness_persisted_record_t *restored_record,
    guardian_node_link_persistent_anti_replay_evidence_t *evidence)
{
    guardian_node_link_freshness_persistence_operation_result_t persistence_result;
    guardian_node_link_freshness_persistence_status_t persistence_status;
    guardian_node_link_freshness_persistence_classification_t classification;
    guardian_node_link_freshness_persisted_record_t loaded;
    guardian_node_link_rollback_anchor_provider_result_t anchor_result;
    guardian_node_link_rollback_anchor_status_t anchor_status;
    guardian_node_link_rollback_anchor_record_t anchor_record;
    guardian_node_link_rollback_anchor_compare_t comparison;
    guardian_node_link_persistent_anti_replay_result_t result;

    if (runtime != NULL)
    {
        guardian_node_link_freshness_runtime_init(runtime);
    }

    if (restored_record != NULL)
    {
        (void)memset(restored_record, 0, sizeof(*restored_record));
    }

    guardian_persistent_anti_replay_evidence_init(evidence);

    if ((persistence_provider == NULL) ||
        (anchor_provider == NULL) ||
        (expected_identity == NULL) ||
        (runtime == NULL) ||
        (restored_record == NULL) ||
        (evidence == NULL) ||
        (persistence_provider->load == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_INVALID;
    }

    (void)memset(&loaded, 0, sizeof(loaded));
    persistence_status = GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_INVALID;

    persistence_result =
        persistence_provider->load(
            persistence_provider->context,
            &persistence_status,
            &loaded);

    evidence->persistence_status = persistence_status;

    if (persistence_result != GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK)
    {
        return map_persistence_operation(persistence_result);
    }

    classification = GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID;

    if (guardian_node_link_freshness_persistence_classify(
            persistence_status,
            &loaded,
            expected_identity,
            &classification) != GUARDIAN_NODE_LINK_FRESHNESS_OK)
    {
        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_INVALID;
    }

    evidence->persistence_classification = classification;

    if (classification ==
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_FRESHNESS_UNKNOWN)
    {
        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_FRESHNESS_UNKNOWN;
    }

    anchor_status = GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_INVALID;

    anchor_result =
        guardian_node_link_rollback_anchor_status(
            anchor_provider,
            expected_identity,
            &anchor_status);

    evidence->anchor_status = anchor_status;

    if (anchor_result != GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK)
    {
        return map_anchor_provider_result(anchor_result);
    }

    if (classification ==
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_NO_STATE_BOOTSTRAP)
    {
        if (anchor_status == GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_UNPROVISIONED)
        {
            return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_BOOTSTRAP_REQUIRED;
        }

        if (anchor_status == GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_VALID)
        {
            return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_ROLLBACK_SUSPECTED;
        }

        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_FRESHNESS_UNKNOWN;
    }

    if (classification !=
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_VALIDATED_RECORD_CANDIDATE)
    {
        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_INVALID;
    }

    if (anchor_status != GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_VALID)
    {
        if (anchor_status == GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_UNPROVISIONED)
        {
            return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_ROLLBACK_SUSPECTED;
        }

        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_FRESHNESS_UNKNOWN;
    }

    (void)memset(&anchor_record, 0, sizeof(anchor_record));
    anchor_status = GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_INVALID;

    anchor_result =
        guardian_node_link_rollback_anchor_read(
            anchor_provider,
            expected_identity,
            &anchor_record,
            &anchor_status);

    evidence->anchor_status = anchor_status;

    if (anchor_result != GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK)
    {
        return map_anchor_provider_result(anchor_result);
    }

    if (anchor_status != GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_VALID)
    {
        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_FRESHNESS_UNKNOWN;
    }

    comparison = GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_INVALID;

    anchor_result =
        guardian_node_link_rollback_anchor_compare(
            &anchor_record,
            &loaded,
            &comparison);

    evidence->anchor_comparison = comparison;

    if (anchor_result != GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK)
    {
        return map_anchor_provider_result(anchor_result);
    }

    switch (comparison)
    {
        case GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_MATCH:
            break;

        case GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_BEHIND:
            return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_ROLLBACK_SUSPECTED;

        case GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_AHEAD:
            return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_STATE_UNCERTAIN;

        case GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_STATE_MISMATCH:
        case GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_IDENTITY_MISMATCH:
            return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_STATE_MISMATCH;

        case GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_INVALID:
        default:
            return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_INVALID;
    }

    result = restore_runtime_from_record(&loaded, runtime);

    if (result != GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_OK)
    {
        guardian_node_link_freshness_runtime_init(runtime);
        return result;
    }

    *restored_record = loaded;
    evidence->accepted_epoch = loaded.accepted_epoch;
    evidence->accepted_sequence = loaded.accepted_sequence;
    evidence->record_generation = loaded.record_generation;
    evidence->runtime_restored = 1U;
    evidence->anchor_readback_verified = 1U;
    evidence->persistent_anti_replay_valid = 1U;
    evidence->actuation_authorized = 0U;

    return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_OK;
}

guardian_node_link_persistent_anti_replay_result_t
guardian_node_link_persistent_anti_replay_commit_continuation(
    const guardian_node_link_freshness_persistence_provider_t *persistence_provider,
    const guardian_node_link_rollback_anchor_provider_t *anchor_provider,
    const guardian_node_link_freshness_identity_t *expected_identity,
    const guardian_node_link_freshness_persisted_record_t *previous_record,
    uint32_t next_epoch,
    uint32_t next_sequence,
    guardian_node_link_freshness_runtime_t *runtime,
    guardian_node_link_freshness_persisted_record_t *committed_record,
    guardian_node_link_persistent_anti_replay_evidence_t *evidence)
{
    guardian_node_link_freshness_persistence_classification_t classification;
    guardian_node_link_freshness_persisted_record_t candidate;
    guardian_node_link_freshness_persistence_transaction_outcome_t outcome;
    guardian_node_link_freshness_persistence_operation_result_t persistence_result;
    guardian_node_link_rollback_anchor_commitment_t commitment;
    guardian_node_link_rollback_anchor_provider_result_t anchor_result;
    guardian_node_link_rollback_anchor_record_t anchor_record;
    guardian_node_link_rollback_anchor_status_t anchor_status;
    guardian_node_link_rollback_anchor_compare_t comparison;
    guardian_node_link_persistent_anti_replay_result_t result;

    if (committed_record != NULL)
    {
        (void)memset(committed_record, 0, sizeof(*committed_record));
    }

    guardian_persistent_anti_replay_evidence_init(evidence);

    if ((persistence_provider == NULL) ||
        (anchor_provider == NULL) ||
        (expected_identity == NULL) ||
        (previous_record == NULL) ||
        (runtime == NULL) ||
        (committed_record == NULL) ||
        (evidence == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_INVALID;
    }

    classification = GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID;

    if (guardian_node_link_freshness_persistence_classify(
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE,
            previous_record,
            expected_identity,
            &classification) != GUARDIAN_NODE_LINK_FRESHNESS_OK)
    {
        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_INVALID;
    }

    if (classification !=
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_VALIDATED_RECORD_CANDIDATE)
    {
        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_INVALID;
    }

    if ((guardian_node_link_freshness_runtime_validate(runtime) !=
         GUARDIAN_NODE_LINK_FRESHNESS_OK) ||
        (runtime->condition != GUARDIAN_NODE_LINK_FRESHNESS_ACTIVE) ||
        (runtime->identity_valid == 0U) ||
        (runtime->accepted_epoch_valid == 0U) ||
        (runtime->accepted_sequence_valid == 0U) ||
        (!identity_equal(&runtime->identity, expected_identity)) ||
        (runtime->accepted_epoch != previous_record->accepted_epoch) ||
        (runtime->accepted_sequence != previous_record->accepted_sequence))
    {
        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_STATE_MISMATCH;
    }

    anchor_status = GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_INVALID;
    (void)memset(&anchor_record, 0, sizeof(anchor_record));

    anchor_result =
        guardian_node_link_rollback_anchor_read(
            anchor_provider,
            expected_identity,
            &anchor_record,
            &anchor_status);

    evidence->anchor_status = anchor_status;

    if (anchor_result != GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK)
    {
        return map_anchor_provider_result(anchor_result);
    }

    if (anchor_status != GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_VALID)
    {
        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_FRESHNESS_UNKNOWN;
    }

    comparison = GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_INVALID;

    anchor_result =
        guardian_node_link_rollback_anchor_compare(
            &anchor_record,
            previous_record,
            &comparison);

    evidence->anchor_comparison = comparison;

    if (anchor_result != GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK)
    {
        return map_anchor_provider_result(anchor_result);
    }

    if (comparison == GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_BEHIND)
    {
        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_ROLLBACK_SUSPECTED;
    }

    if (comparison != GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_MATCH)
    {
        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_STATE_MISMATCH;
    }

    if (next_epoch != previous_record->accepted_epoch)
    {
        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_EPOCH_TRANSITION_REQUIRED;
    }

    if ((next_sequence == 0U) ||
        (next_sequence <= previous_record->accepted_sequence))
    {
        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_REPLAY_OR_REGRESSION;
    }

    if (previous_record->record_generation >= (UINT32_MAX - 1U))
    {
        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_GENERATION_EXHAUSTED;
    }

    candidate = *previous_record;
    candidate.accepted_sequence = next_sequence;
    candidate.record_generation = previous_record->record_generation + 1U;

    outcome = GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_INVALID;

    persistence_result =
        guardian_node_link_freshness_persistence_transact_commit(
            persistence_provider,
            previous_record,
            expected_identity,
            &candidate,
            &outcome);

    if ((persistence_result != GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK) ||
        (outcome != GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_COMMITTED))
    {
        return map_persistence_operation(persistence_result);
    }

    evidence->persistence_committed = 1U;

    (void)memset(&commitment, 0, sizeof(commitment));

    anchor_result =
        guardian_node_link_rollback_anchor_commitment_from_record(
            &candidate,
            &commitment);

    if (anchor_result != GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK)
    {
        return map_anchor_provider_result(anchor_result);
    }

    anchor_result =
        guardian_node_link_rollback_anchor_advance(
            anchor_provider,
            expected_identity,
            previous_record->record_generation,
            candidate.record_generation,
            &commitment);

    if (anchor_result != GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK)
    {
        return map_anchor_provider_result(anchor_result);
    }

    evidence->anchor_advanced = 1U;

    (void)memset(&anchor_record, 0, sizeof(anchor_record));
    anchor_status = GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_INVALID;

    anchor_result =
        guardian_node_link_rollback_anchor_read(
            anchor_provider,
            expected_identity,
            &anchor_record,
            &anchor_status);

    evidence->anchor_status = anchor_status;

    if (anchor_result != GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK)
    {
        return map_anchor_provider_result(anchor_result);
    }

    if (anchor_status != GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_VALID)
    {
        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_STATE_UNCERTAIN;
    }

    comparison = GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_INVALID;

    anchor_result =
        guardian_node_link_rollback_anchor_compare(
            &anchor_record,
            &candidate,
            &comparison);

    evidence->anchor_comparison = comparison;

    if ((anchor_result != GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK) ||
        (comparison != GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_MATCH))
    {
        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_STATE_UNCERTAIN;
    }

    evidence->anchor_readback_verified = 1U;

    result = restore_runtime_from_record(&candidate, runtime);

    if (result != GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_OK)
    {
        return result;
    }

    *committed_record = candidate;
    evidence->accepted_epoch = candidate.accepted_epoch;
    evidence->accepted_sequence = candidate.accepted_sequence;
    evidence->record_generation = candidate.record_generation;
    evidence->runtime_restored = 1U;
    evidence->persistent_anti_replay_valid = 1U;
    evidence->actuation_authorized = 0U;

    return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_OK;
}

guardian_node_link_persistent_anti_replay_result_t
guardian_node_link_persistent_anti_replay_can_actuate(uint8_t *allowed)
{
    if (allowed == NULL)
    {
        return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_INVALID;
    }

    *allowed = 0U;
    return GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_OK;
}