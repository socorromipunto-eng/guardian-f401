#include "guardian_node_link_rollback_anchor.h"

#include "guardian_crypto.h"

#include <stdint.h>
#include <string.h>

static const uint8_t guardian_r3d_anchor_domain[] =
{
    (uint8_t)'G',
    (uint8_t)'U',
    (uint8_t)'A',
    (uint8_t)'R',
    (uint8_t)'D',
    (uint8_t)'I',
    (uint8_t)'A',
    (uint8_t)'N',
    (uint8_t)'-',
    (uint8_t)'R',
    (uint8_t)'3',
    (uint8_t)'D',
    (uint8_t)'-',
    (uint8_t)'A',
    (uint8_t)'N',
    (uint8_t)'C',
    (uint8_t)'H',
    (uint8_t)'O',
    (uint8_t)'R',
    (uint8_t)'-',
    (uint8_t)'C',
    (uint8_t)'O',
    (uint8_t)'M',
    (uint8_t)'M',
    (uint8_t)'I',
    (uint8_t)'T',
    (uint8_t)'M',
    (uint8_t)'E',
    (uint8_t)'N',
    (uint8_t)'T',
    (uint8_t)'-',
    (uint8_t)'V',
    (uint8_t)'1'
};

typedef char guardian_r3d_anchor_domain_size_must_be_33[
    (sizeof(guardian_r3d_anchor_domain) ==
     GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_DOMAIN_SIZE)
        ? 1
        : -1];

typedef char guardian_r3d_anchor_material_size_must_match_r3c_d[
    (GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_MATERIAL_LENGTH ==
     GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INTEGRITY_MATERIAL_LENGTH)
        ? 1
        : -1];

typedef char guardian_r3d_anchor_integrity_offset_must_be_332[
    (GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INTEGRITY_OFFSET ==
     (size_t)332U)
        ? 1
        : -1];

static int guardian_node_link_rollback_anchor_identity_equal(
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

guardian_node_link_rollback_anchor_provider_result_t
guardian_node_link_rollback_anchor_commitment_from_record(
    const guardian_node_link_freshness_persisted_record_t *record,
    guardian_node_link_rollback_anchor_commitment_t *commitment)
{
    guardian_node_link_freshness_persistence_codec_result_t codec_result;
    uint8_t serialized[
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZED_SIZE];
    uint8_t preimage[
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PREIMAGE_SIZE];
    size_t serialized_written;

    if (commitment != NULL)
    {
        (void)memset(
            commitment,
            0,
            sizeof(*commitment));
    }

    if ((record == NULL) || (commitment == NULL))
    {
        return
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT;
    }

    (void)memset(
        serialized,
        0,
        sizeof(serialized));

    (void)memset(
        preimage,
        0,
        sizeof(preimage));

    serialized_written = 0U;

    codec_result =
        guardian_node_link_freshness_persistence_encode(
            record,
            serialized,
            sizeof(serialized),
            &serialized_written);

    if ((codec_result !=
         GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_OK) ||
        (serialized_written !=
         GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZED_SIZE))
    {
        return
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT;
    }

    (void)memcpy(
        &preimage[0],
        guardian_r3d_anchor_domain,
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_DOMAIN_SIZE);

    preimage[
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_DOMAIN_SIZE + 0U] =
        (uint8_t)0x00U;

    preimage[
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_DOMAIN_SIZE + 1U] =
        (uint8_t)0x00U;

    preimage[
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_DOMAIN_SIZE + 2U] =
        (uint8_t)0x01U;

    preimage[
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_DOMAIN_SIZE + 3U] =
        (uint8_t)0x4CU;

    (void)memcpy(
        &preimage[
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_DOMAIN_SIZE + 4U],
        serialized,
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_MATERIAL_LENGTH);

    guardian_sha256(
        preimage,
        sizeof(preimage),
        commitment->bytes);

    return GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK;
}

guardian_node_link_rollback_anchor_provider_result_t
guardian_node_link_rollback_anchor_compare(
    const guardian_node_link_rollback_anchor_record_t *anchor,
    const guardian_node_link_freshness_persisted_record_t *candidate,
    guardian_node_link_rollback_anchor_compare_t *comparison)
{
    guardian_node_link_rollback_anchor_commitment_t candidate_commitment;
    guardian_node_link_rollback_anchor_provider_result_t result;

    if (comparison != NULL)
    {
        *comparison =
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_INVALID;
    }

    if ((anchor == NULL) ||
        (candidate == NULL) ||
        (comparison == NULL))
    {
        return
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT;
    }

    result =
        guardian_node_link_rollback_anchor_commitment_from_record(
            candidate,
            &candidate_commitment);

    if (result !=
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK)
    {
        return result;
    }

    if (!guardian_node_link_rollback_anchor_identity_equal(
            &anchor->identity,
            &candidate->identity))
    {
        *comparison =
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_IDENTITY_MISMATCH;

        return GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK;
    }

    if (candidate->record_generation < anchor->generation)
    {
        *comparison =
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_BEHIND;

        return GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK;
    }

    if (candidate->record_generation > anchor->generation)
    {
        *comparison =
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_AHEAD;

        return GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK;
    }

    if (!guardian_crypto_constant_time_equal(
            candidate_commitment.bytes,
            anchor->commitment.bytes,
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMMITMENT_SIZE))
    {
        *comparison =
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_STATE_MISMATCH;

        return GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK;
    }

    *comparison =
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_MATCH;

    return GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK;
}

guardian_node_link_rollback_anchor_provider_result_t
guardian_node_link_rollback_anchor_next_generation(
    uint32_t current_generation,
    uint32_t *next_generation)
{
    if (next_generation != NULL)
    {
        *next_generation = 0U;
    }

    if (next_generation == NULL)
    {
        return
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT;
    }

    if (current_generation >= (UINT32_MAX - 1U))
    {
        return
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_EXHAUSTED;
    }

    *next_generation =
        current_generation + 1U;

    return GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK;
}

guardian_node_link_rollback_anchor_provider_result_t
guardian_node_link_rollback_anchor_status(
    const guardian_node_link_rollback_anchor_provider_t *provider,
    const guardian_node_link_freshness_identity_t *identity,
    guardian_node_link_rollback_anchor_status_t *status)
{
    if (status != NULL)
    {
        *status =
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_INVALID;
    }

    if ((provider == NULL) ||
        (identity == NULL) ||
        (status == NULL) ||
        (provider->status == NULL))
    {
        return
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT;
    }

    return
        provider->status(
            provider->context,
            identity,
            status);
}

guardian_node_link_rollback_anchor_provider_result_t
guardian_node_link_rollback_anchor_read(
    const guardian_node_link_rollback_anchor_provider_t *provider,
    const guardian_node_link_freshness_identity_t *identity,
    guardian_node_link_rollback_anchor_record_t *record,
    guardian_node_link_rollback_anchor_status_t *status)
{
    if (record != NULL)
    {
        (void)memset(
            record,
            0,
            sizeof(*record));
    }

    if (status != NULL)
    {
        *status =
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_INVALID;
    }

    if ((provider == NULL) ||
        (identity == NULL) ||
        (record == NULL) ||
        (status == NULL) ||
        (provider->read == NULL))
    {
        return
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT;
    }

    return
        provider->read(
            provider->context,
            identity,
            record,
            status);
}

guardian_node_link_rollback_anchor_provider_result_t
guardian_node_link_rollback_anchor_advance(
    const guardian_node_link_rollback_anchor_provider_t *provider,
    const guardian_node_link_freshness_identity_t *identity,
    uint32_t expected_generation,
    uint32_t next_generation,
    const guardian_node_link_rollback_anchor_commitment_t
        *next_commitment)
{
    guardian_node_link_rollback_anchor_provider_result_t result;
    uint32_t governed_next_generation;

    if ((provider == NULL) ||
        (identity == NULL) ||
        (next_commitment == NULL) ||
        (provider->advance == NULL))
    {
        return
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT;
    }

    governed_next_generation = 0U;

    result =
        guardian_node_link_rollback_anchor_next_generation(
            expected_generation,
            &governed_next_generation);

    if (result !=
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK)
    {
        return result;
    }

    if (next_generation != governed_next_generation)
    {
        return
            GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT;
    }

    return
        provider->advance(
            provider->context,
            identity,
            expected_generation,
            next_generation,
            next_commitment);
}
