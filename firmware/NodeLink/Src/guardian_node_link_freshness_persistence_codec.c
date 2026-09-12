#include "guardian_node_link_freshness_persistence_codec.h"

#include "guardian_crypto.h"

#include <string.h>

#define R3C_D_MAGIC_0 ((uint8_t)0x47U)
#define R3C_D_MAGIC_1 ((uint8_t)0x46U)
#define R3C_D_MAGIC_2 ((uint8_t)0x52U)
#define R3C_D_MAGIC_3 ((uint8_t)0x31U)

static void r3c_d_put_u16_be(
    uint8_t *output,
    uint16_t value)
{
    output[0] = (uint8_t)((value >> 8U) & 0xFFU);
    output[1] = (uint8_t)(value & 0xFFU);
}

static uint16_t r3c_d_get_u16_be(
    const uint8_t *input)
{
    return (uint16_t)(
        ((uint16_t)input[0] << 8U) |
        (uint16_t)input[1]);
}

static void r3c_d_put_u32_be(
    uint8_t *output,
    uint32_t value)
{
    output[0] = (uint8_t)((value >> 24U) & 0xFFU);
    output[1] = (uint8_t)((value >> 16U) & 0xFFU);
    output[2] = (uint8_t)((value >> 8U) & 0xFFU);
    output[3] = (uint8_t)(value & 0xFFU);
}

static uint32_t r3c_d_get_u32_be(
    const uint8_t *input)
{
    return
        ((uint32_t)input[0] << 24U) |
        ((uint32_t)input[1] << 16U) |
        ((uint32_t)input[2] << 8U) |
        (uint32_t)input[3];
}

static size_t r3c_d_ref_length(
    const char *reference,
    size_t capacity)
{
    size_t index;

    if ((reference == NULL) || (capacity == 0U))
    {
        return capacity;
    }

    for (index = 0U; index < capacity; index += 1U)
    {
        if (reference[index] == '\0')
        {
            return index;
        }
    }

    return capacity;
}

static int r3c_d_encode_ref(
    const char *reference,
    uint8_t *output)
{
    size_t length;

    length =
        r3c_d_ref_length(
            reference,
            GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY);

    if ((length == 0U) ||
        (length >= GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY))
    {
        return 0;
    }

    (void)memset(
        output,
        0,
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY);

    (void)memcpy(
        output,
        reference,
        length);

    return 1;
}

static int r3c_d_decode_ref(
    const uint8_t *input,
    char *reference)
{
    size_t terminator;
    size_t index;

    if ((input == NULL) || (reference == NULL))
    {
        return 0;
    }

    terminator =
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY;

    for (index = 0U;
         index < GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY;
         index += 1U)
    {
        if (input[index] == 0U)
        {
            terminator = index;
            break;
        }
    }

    if ((terminator == 0U) ||
        (terminator >= GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY))
    {
        return 0;
    }

    for (index = terminator + 1U;
         index < GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY;
         index += 1U)
    {
        if (input[index] != 0U)
        {
            return 0;
        }
    }

    (void)memset(
        reference,
        0,
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY);

    (void)memcpy(
        reference,
        input,
        terminator);

    return 1;
}

static int r3c_d_record_is_logically_valid(
    const guardian_node_link_freshness_persisted_record_t *record)
{
    guardian_node_link_freshness_persistence_classification_t
        classification;

    if (record == NULL)
    {
        return 0;
    }

    classification =
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID;

    if (guardian_node_link_freshness_persistence_classify(
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE,
            record,
            &record->identity,
            &classification) !=
        GUARDIAN_NODE_LINK_FRESHNESS_OK)
    {
        return 0;
    }

    return
        classification ==
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_VALIDATED_RECORD_CANDIDATE;
}

guardian_node_link_freshness_persistence_codec_result_t
guardian_node_link_freshness_persistence_encode(
    const guardian_node_link_freshness_persisted_record_t *record,
    uint8_t *output,
    size_t output_capacity,
    size_t *output_written)
{
    uint8_t digest[GUARDIAN_SHA256_SIZE];

    if (output_written != NULL)
    {
        *output_written = 0U;
    }

    if ((record == NULL) ||
        (output == NULL) ||
        (output_written == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_BAD_ARGUMENT;
    }

    if (output_capacity <
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZED_SIZE)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_BUFFER_TOO_SMALL;
    }

    if (!r3c_d_record_is_logically_valid(record))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INVALID_FIELD;
    }

    (void)memset(
        output,
        0,
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZED_SIZE);

    output[
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_MAGIC_OFFSET + 0U] =
        R3C_D_MAGIC_0;

    output[
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_MAGIC_OFFSET + 1U] =
        R3C_D_MAGIC_1;

    output[
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_MAGIC_OFFSET + 2U] =
        R3C_D_MAGIC_2;

    output[
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_MAGIC_OFFSET + 3U] =
        R3C_D_MAGIC_3;

    r3c_d_put_u16_be(
        &output[
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZATION_VERSION_OFFSET],
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZATION_VERSION);

    r3c_d_put_u16_be(
        &output[
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_HEADER_LENGTH_OFFSET],
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_HEADER_LENGTH);

    r3c_d_put_u32_be(
        &output[
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_LOGICAL_SCHEMA_OFFSET],
        record->schema_version);

    r3c_d_put_u32_be(
        &output[
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SENDER_NODE_ID_OFFSET],
        record->identity.sender_node_id);

    r3c_d_put_u32_be(
        &output[
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_PRODUCER_ID_OFFSET],
        record->identity.producer_id);

    r3c_d_put_u32_be(
        &output[
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_KEY_ID_OFFSET],
        record->identity.key_id);

    output[
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SIGNATURE_ALGORITHM_OFFSET] =
        record->identity.signature_algorithm;

    /*
     * Bytes 25..27 remain canonical zero reserved bytes because the complete
     * output buffer was zero-initialized.
     */

    if (!r3c_d_encode_ref(
            record->identity.producer_semantic_profile_id,
            &output[
                GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_PRODUCER_PROFILE_OFFSET]))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INVALID_FIELD;
    }

    if (!r3c_d_encode_ref(
            record->identity.consumer_semantic_profile_id,
            &output[
                GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_CONSUMER_PROFILE_OFFSET]))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INVALID_FIELD;
    }

    if (!r3c_d_encode_ref(
            record->identity.compatibility_contract_id,
            &output[
                GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_COMPATIBILITY_CONTRACT_OFFSET]))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INVALID_FIELD;
    }

    r3c_d_put_u32_be(
        &output[
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_ACCEPTED_EPOCH_OFFSET],
        record->accepted_epoch);

    r3c_d_put_u32_be(
        &output[
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_ACCEPTED_SEQUENCE_OFFSET],
        record->accepted_sequence);

    r3c_d_put_u32_be(
        &output[
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_RECORD_GENERATION_OFFSET],
        record->record_generation);

    r3c_d_put_u32_be(
        &output[
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_MATERIAL_LENGTH_OFFSET],
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INTEGRITY_MATERIAL_LENGTH);

    guardian_sha256(
        output,
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INTEGRITY_OFFSET,
        digest);

    (void)memcpy(
        &output[
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INTEGRITY_OFFSET],
        digest,
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INTEGRITY_SIZE);

    *output_written =
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZED_SIZE;

    return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_OK;
}

guardian_node_link_freshness_persistence_codec_result_t
guardian_node_link_freshness_persistence_decode(
    const uint8_t *input,
    size_t input_length,
    guardian_node_link_freshness_persisted_record_t *record)
{
    guardian_node_link_freshness_persisted_record_t candidate;
    uint8_t expected_digest[GUARDIAN_SHA256_SIZE];

    if (record != NULL)
    {
        (void)memset(
            record,
            0,
            sizeof(*record));
    }

    if ((input == NULL) || (record == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_BAD_ARGUMENT;
    }

    if (input_length <
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZED_SIZE)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_BAD_LENGTH;
    }

    if (input_length >
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZED_SIZE)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_TRAILING_DATA;
    }

    if ((input[
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_MAGIC_OFFSET + 0U] !=
         R3C_D_MAGIC_0) ||
        (input[
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_MAGIC_OFFSET + 1U] !=
         R3C_D_MAGIC_1) ||
        (input[
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_MAGIC_OFFSET + 2U] !=
         R3C_D_MAGIC_2) ||
        (input[
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_MAGIC_OFFSET + 3U] !=
         R3C_D_MAGIC_3))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_BAD_MAGIC;
    }

    if (r3c_d_get_u16_be(
            &input[
                GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZATION_VERSION_OFFSET]) !=
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZATION_VERSION)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_UNSUPPORTED_VERSION;
    }

    if (r3c_d_get_u16_be(
            &input[
                GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_HEADER_LENGTH_OFFSET]) !=
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_HEADER_LENGTH)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_BAD_LENGTH;
    }

    if ((input[
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_RESERVED_OFFSET + 0U] != 0U) ||
        (input[
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_RESERVED_OFFSET + 1U] != 0U) ||
        (input[
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_RESERVED_OFFSET + 2U] != 0U))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_RESERVED_NONZERO;
    }

    if (r3c_d_get_u32_be(
            &input[
                GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_MATERIAL_LENGTH_OFFSET]) !=
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INTEGRITY_MATERIAL_LENGTH)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_BAD_LENGTH;
    }

    guardian_sha256(
        input,
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INTEGRITY_OFFSET,
        expected_digest);

    if (!guardian_crypto_constant_time_equal(
            expected_digest,
            &input[
                GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INTEGRITY_OFFSET],
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INTEGRITY_SIZE))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INTEGRITY_FAILURE;
    }

    (void)memset(
        &candidate,
        0,
        sizeof(candidate));

    candidate.schema_version =
        r3c_d_get_u32_be(
            &input[
                GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_LOGICAL_SCHEMA_OFFSET]);

    candidate.identity.sender_node_id =
        r3c_d_get_u32_be(
            &input[
                GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SENDER_NODE_ID_OFFSET]);

    candidate.identity.producer_id =
        r3c_d_get_u32_be(
            &input[
                GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_PRODUCER_ID_OFFSET]);

    candidate.identity.key_id =
        r3c_d_get_u32_be(
            &input[
                GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_KEY_ID_OFFSET]);

    candidate.identity.signature_algorithm =
        input[
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SIGNATURE_ALGORITHM_OFFSET];

    if (!r3c_d_decode_ref(
            &input[
                GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_PRODUCER_PROFILE_OFFSET],
            candidate.identity.producer_semantic_profile_id))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INVALID_FIELD;
    }

    if (!r3c_d_decode_ref(
            &input[
                GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_CONSUMER_PROFILE_OFFSET],
            candidate.identity.consumer_semantic_profile_id))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INVALID_FIELD;
    }

    if (!r3c_d_decode_ref(
            &input[
                GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_COMPATIBILITY_CONTRACT_OFFSET],
            candidate.identity.compatibility_contract_id))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INVALID_FIELD;
    }

    candidate.accepted_epoch =
        r3c_d_get_u32_be(
            &input[
                GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_ACCEPTED_EPOCH_OFFSET]);

    candidate.accepted_sequence =
        r3c_d_get_u32_be(
            &input[
                GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_ACCEPTED_SEQUENCE_OFFSET]);

    candidate.record_generation =
        r3c_d_get_u32_be(
            &input[
                GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_RECORD_GENERATION_OFFSET]);

    if (!r3c_d_record_is_logically_valid(&candidate))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INVALID_FIELD;
    }

    *record = candidate;

    return GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_OK;
}