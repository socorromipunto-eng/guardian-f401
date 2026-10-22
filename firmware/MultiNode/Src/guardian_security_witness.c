#include "guardian_security_witness.h"

#include <limits.h>
#include <string.h>

static const uint8_t
guardian_security_witness_domain[] =
    GUARDIAN_SECURITY_WITNESS_DOMAIN;

static int
guardian_security_witness_evidence_type_valid(
    guardian_security_witness_evidence_type_t type)
{
    return
        (type >
            GUARDIAN_SECURITY_WITNESS_EVIDENCE_INVALID) &&
        (type <=
            GUARDIAN_SECURITY_WITNESS_EVIDENCE_EXECUTION_RESULT);
}

static void
guardian_security_witness_put_u32_be(
    uint8_t *output,
    uint32_t value)
{
    output[0] =
        (uint8_t)((value >> 24) & 0xFFU);

    output[1] =
        (uint8_t)((value >> 16) & 0xFFU);

    output[2] =
        (uint8_t)((value >> 8) & 0xFFU);

    output[3] =
        (uint8_t)(value & 0xFFU);
}

static guardian_security_witness_result_t
guardian_security_witness_map_provider(
    guardian_security_witness_provider_result_t result)
{
    switch (result)
    {
        case GUARDIAN_SECURITY_WITNESS_PROVIDER_OK:

            return
                GUARDIAN_SECURITY_WITNESS_OK;

        case GUARDIAN_SECURITY_WITNESS_PROVIDER_SIGNATURE_INVALID:

            return
                GUARDIAN_SECURITY_WITNESS_ERROR_SIGNATURE_INVALID;

        case GUARDIAN_SECURITY_WITNESS_PROVIDER_KEY_UNKNOWN:

            return
                GUARDIAN_SECURITY_WITNESS_ERROR_KEY_UNKNOWN;

        case GUARDIAN_SECURITY_WITNESS_PROVIDER_KEY_REVOKED:

            return
                GUARDIAN_SECURITY_WITNESS_ERROR_KEY_REVOKED;

        case GUARDIAN_SECURITY_WITNESS_PROVIDER_UNAVAILABLE:

            return
                GUARDIAN_SECURITY_WITNESS_ERROR_PROVIDER_UNAVAILABLE;

        case GUARDIAN_SECURITY_WITNESS_PROVIDER_FAILURE:
        default:

            return
                GUARDIAN_SECURITY_WITNESS_ERROR_PROVIDER_FAILURE;
    }
}

static int
guardian_security_witness_record_structurally_valid(
    const guardian_security_witness_record_t *record)
{
    if (record == NULL)
    {
        return 0;
    }

    if (record->schema_version !=
        GUARDIAN_SECURITY_WITNESS_SCHEMA_VERSION)
    {
        return 0;
    }

    if (guardian_security_witness_evidence_type_valid(
            record->evidence_type) == 0)
    {
        return 0;
    }

    if (record->signature_algorithm !=
        GUARDIAN_SECURITY_WITNESS_SIGNATURE_ED25519)
    {
        return 0;
    }

    if (record->authority_flags !=
        GUARDIAN_SECURITY_WITNESS_AUTHORITY_NONE)
    {
        return 0;
    }

    if ((record->witness_node_id == 0U) ||
        (record->witness_producer_id == 0U) ||
        (record->witness_key_id == 0U) ||
        (record->subject_node_id == 0U) ||
        (record->subject_producer_id == 0U) ||
        (record->subject_key_id == 0U) ||
        (record->epoch == 0U) ||
        (record->sequence == 0U))
    {
        return 0;
    }

    if ((record->payload_length == 0U) ||
        (record->payload_length >
            (uint32_t)
            GUARDIAN_SECURITY_WITNESS_MAX_PAYLOAD))
    {
        return 0;
    }

    return 1;
}

static int
guardian_security_witness_subject_equal(
    const guardian_security_witness_subject_state_t *state,
    const guardian_security_witness_input_t *input)
{
    if ((state == NULL) ||
        (input == NULL))
    {
        return 0;
    }

    return
        (state->active == 1U) &&
        (state->subject_node_id ==
            input->subject_node_id) &&
        (state->subject_producer_id ==
            input->subject_producer_id) &&
        (state->subject_key_id ==
            input->subject_key_id);
}

void guardian_security_witness_init(
    guardian_security_witness_node_t *node)
{
    if (node == NULL)
    {
        return;
    }

    (void)memset(
        node,
        0,
        sizeof(*node));
}

guardian_security_witness_result_t
guardian_security_witness_configure(
    guardian_security_witness_node_t *node,
    const guardian_multinode_node_t *self,
    const guardian_security_witness_provider_t *provider)
{
    guardian_multinode_result_t node_result;

    if ((node == NULL) ||
        (self == NULL) ||
        (provider == NULL))
    {
        return
            GUARDIAN_SECURITY_WITNESS_ERROR_NULL_ARGUMENT;
    }

    node_result =
        guardian_multinode_validate_node(
            self);

    if (node_result !=
        GUARDIAN_MULTINODE_OK)
    {
        return
            GUARDIAN_SECURITY_WITNESS_ERROR_INVALID_NODE;
    }

    if ((self->role !=
            GUARDIAN_NODE_ROLE_SECURITY_WITNESS) ||
        (self->authority_class !=
            GUARDIAN_AUTHORITY_CLASS_WITNESS))
    {
        return
            GUARDIAN_SECURITY_WITNESS_ERROR_INVALID_NODE;
    }

    if ((self->capabilities &
         GUARDIAN_CAP_EVIDENCE_WITNESS) == 0U)
    {
        return
            GUARDIAN_SECURITY_WITNESS_ERROR_INVALID_NODE;
    }

    if ((self->capabilities &
         GUARDIAN_CAP_SIGNATURE_VERIFY) == 0U)
    {
        return
            GUARDIAN_SECURITY_WITNESS_ERROR_INVALID_NODE;
    }

    if ((self->capabilities &
         GUARDIAN_CAP_ACTUATION_GATE) != 0U)
    {
        return
            GUARDIAN_SECURITY_WITNESS_ERROR_INVALID_NODE;
    }

    if ((provider->sign == NULL) ||
        (provider->verify == NULL) ||
        (provider->signature_algorithm !=
            GUARDIAN_SECURITY_WITNESS_SIGNATURE_ED25519))
    {
        return
            GUARDIAN_SECURITY_WITNESS_ERROR_INVALID_CONFIG;
    }

    guardian_security_witness_init(
        node);

    node->self =
        *self;

    node->provider =
        *provider;

    node->initialized =
        1U;

    return
        GUARDIAN_SECURITY_WITNESS_OK;
}

guardian_security_witness_result_t
guardian_security_witness_build_transcript(
    const guardian_security_witness_record_t *record,
    uint8_t *output,
    size_t output_capacity,
    size_t *output_length)
{
    size_t offset = 0U;

    size_t domain_length =
        sizeof(
            guardian_security_witness_domain
        ) - 1U;

    if (output_length != NULL)
    {
        *output_length =
            0U;
    }

    if ((record == NULL) ||
        (output == NULL) ||
        (output_length == NULL))
    {
        return
            GUARDIAN_SECURITY_WITNESS_ERROR_NULL_ARGUMENT;
    }

    if (guardian_security_witness_record_structurally_valid(
            record) == 0)
    {
        return
            GUARDIAN_SECURITY_WITNESS_ERROR_INVALID_EVIDENCE;
    }

    if (output_capacity <
        GUARDIAN_SECURITY_WITNESS_TRANSCRIPT_MAX_SIZE)
    {
        return
            GUARDIAN_SECURITY_WITNESS_ERROR_TRANSCRIPT;
    }

    (void)memset(
        output,
        0,
        output_capacity);

    (void)memcpy(
        &output[offset],
        guardian_security_witness_domain,
        domain_length);

    offset +=
        domain_length;

    output[offset] =
        record->schema_version;

    offset += 1U;

    output[offset] =
        (uint8_t)record->evidence_type;

    offset += 1U;

    output[offset] =
        record->signature_algorithm;

    offset += 1U;

    output[offset] =
        record->authority_flags;

    offset += 1U;

    guardian_security_witness_put_u32_be(
        &output[offset],
        record->witness_node_id);

    offset += 4U;

    guardian_security_witness_put_u32_be(
        &output[offset],
        record->witness_producer_id);

    offset += 4U;

    guardian_security_witness_put_u32_be(
        &output[offset],
        record->witness_key_id);

    offset += 4U;

    guardian_security_witness_put_u32_be(
        &output[offset],
        record->subject_node_id);

    offset += 4U;

    guardian_security_witness_put_u32_be(
        &output[offset],
        record->subject_producer_id);

    offset += 4U;

    guardian_security_witness_put_u32_be(
        &output[offset],
        record->subject_key_id);

    offset += 4U;

    guardian_security_witness_put_u32_be(
        &output[offset],
        record->epoch);

    offset += 4U;

    guardian_security_witness_put_u32_be(
        &output[offset],
        record->sequence);

    offset += 4U;

    guardian_security_witness_put_u32_be(
        &output[offset],
        record->payload_length);

    offset += 4U;

    (void)memcpy(
        &output[offset],
        record->payload_digest,
        GUARDIAN_SHA256_SIZE);

    offset +=
        GUARDIAN_SHA256_SIZE;

    if (offset >
        GUARDIAN_SECURITY_WITNESS_TRANSCRIPT_MAX_SIZE)
    {
        (void)memset(
            output,
            0,
            output_capacity);

        return
            GUARDIAN_SECURITY_WITNESS_ERROR_TRANSCRIPT;
    }

    *output_length =
        offset;

    return
        GUARDIAN_SECURITY_WITNESS_OK;
}

guardian_security_witness_result_t
guardian_security_witness_create(
    guardian_security_witness_node_t *node,
    const guardian_security_witness_input_t *input,
    guardian_security_witness_record_t *record)
{
    guardian_security_witness_subject_state_t
        *state = NULL;

    guardian_security_witness_provider_result_t
        provider_result;

    guardian_security_witness_result_t result;

    uint8_t transcript[
        GUARDIAN_SECURITY_WITNESS_TRANSCRIPT_MAX_SIZE];

    size_t transcript_length = 0U;

    size_t index;
    size_t free_index =
        GUARDIAN_SECURITY_WITNESS_MAX_SUBJECTS;

    uint8_t existing =
        0U;

    if ((node == NULL) ||
        (input == NULL) ||
        (record == NULL))
    {
        return
            GUARDIAN_SECURITY_WITNESS_ERROR_NULL_ARGUMENT;
    }

    (void)memset(
        record,
        0,
        sizeof(*record));

    if (node->initialized != 1U)
    {
        return
            GUARDIAN_SECURITY_WITNESS_ERROR_INVALID_CONFIG;
    }

    if ((input->subject_node_id == 0U) ||
        (input->subject_producer_id == 0U) ||
        (input->subject_key_id == 0U) ||
        (input->epoch == 0U) ||
        (input->sequence == 0U) ||
        (input->payload == NULL) ||
        (input->payload_length == 0U) ||
        (input->payload_length >
            GUARDIAN_SECURITY_WITNESS_MAX_PAYLOAD) ||
        (guardian_security_witness_evidence_type_valid(
            input->evidence_type) == 0))
    {
        return
            GUARDIAN_SECURITY_WITNESS_ERROR_INVALID_EVIDENCE;
    }

    for (index = 0U;
         index <
            GUARDIAN_SECURITY_WITNESS_MAX_SUBJECTS;
         index += 1U)
    {
        if (guardian_security_witness_subject_equal(
                &node->subjects[index],
                input) != 0)
        {
            state =
                &node->subjects[index];

            existing =
                1U;

            break;
        }

        if ((free_index ==
                GUARDIAN_SECURITY_WITNESS_MAX_SUBJECTS) &&
            (node->subjects[index].active == 0U))
        {
            free_index =
                index;
        }
    }

    if (existing != 0U)
    {
        if (input->epoch <
            state->accepted_epoch)
        {
            return
                GUARDIAN_SECURITY_WITNESS_ERROR_REPLAY_OR_REGRESSION;
        }

        if ((input->epoch ==
                state->accepted_epoch) &&
            (input->sequence <=
                state->accepted_sequence))
        {
            return
                GUARDIAN_SECURITY_WITNESS_ERROR_REPLAY_OR_REGRESSION;
        }
    }
    else
    {
        if (free_index >=
            GUARDIAN_SECURITY_WITNESS_MAX_SUBJECTS)
        {
            return
                GUARDIAN_SECURITY_WITNESS_ERROR_SUBJECT_TABLE_FULL;
        }

        state =
            &node->subjects[free_index];
    }

    record->schema_version =
        GUARDIAN_SECURITY_WITNESS_SCHEMA_VERSION;

    record->evidence_type =
        input->evidence_type;

    record->signature_algorithm =
        node->provider.signature_algorithm;

    record->authority_flags =
        GUARDIAN_SECURITY_WITNESS_AUTHORITY_NONE;

    record->witness_node_id =
        node->self.node_id;

    record->witness_producer_id =
        node->self.producer_id;

    record->witness_key_id =
        node->self.key_id;

    record->subject_node_id =
        input->subject_node_id;

    record->subject_producer_id =
        input->subject_producer_id;

    record->subject_key_id =
        input->subject_key_id;

    record->epoch =
        input->epoch;

    record->sequence =
        input->sequence;

    record->payload_length =
        (uint32_t)input->payload_length;

    guardian_sha256(
        input->payload,
        input->payload_length,
        record->payload_digest);

    result =
        guardian_security_witness_build_transcript(
            record,
            transcript,
            sizeof(transcript),
            &transcript_length);

    if (result !=
        GUARDIAN_SECURITY_WITNESS_OK)
    {
        (void)memset(
            transcript,
            0,
            sizeof(transcript));

        (void)memset(
            record,
            0,
            sizeof(*record));

        return result;
    }

    provider_result =
        node->provider.sign(
            node->provider.context,
            node->provider.signature_algorithm,
            node->self.key_id,
            transcript,
            transcript_length,
            record->signature,
            sizeof(record->signature));

    (void)memset(
        transcript,
        0,
        sizeof(transcript));

    result =
        guardian_security_witness_map_provider(
            provider_result);

    if (result !=
        GUARDIAN_SECURITY_WITNESS_OK)
    {
        (void)memset(
            record,
            0,
            sizeof(*record));

        return result;
    }

    if (existing == 0U)
    {
        (void)memset(
            state,
            0,
            sizeof(*state));

        state->subject_node_id =
            input->subject_node_id;

        state->subject_producer_id =
            input->subject_producer_id;

        state->subject_key_id =
            input->subject_key_id;

        state->active =
            1U;

        if (node->subject_count <
            GUARDIAN_SECURITY_WITNESS_MAX_SUBJECTS)
        {
            node->subject_count +=
                1U;
        }
    }

    state->accepted_epoch =
        input->epoch;

    state->accepted_sequence =
        input->sequence;

    return
        GUARDIAN_SECURITY_WITNESS_OK;
}

guardian_security_witness_result_t
guardian_security_witness_verify(
    const guardian_security_witness_node_t *node,
    const guardian_security_witness_record_t *record)
{
    guardian_security_witness_provider_result_t
        provider_result;

    guardian_security_witness_result_t result;

    uint8_t transcript[
        GUARDIAN_SECURITY_WITNESS_TRANSCRIPT_MAX_SIZE];

    size_t transcript_length =
        0U;

    if ((node == NULL) ||
        (record == NULL))
    {
        return
            GUARDIAN_SECURITY_WITNESS_ERROR_NULL_ARGUMENT;
    }

    if (node->initialized != 1U)
    {
        return
            GUARDIAN_SECURITY_WITNESS_ERROR_INVALID_CONFIG;
    }

    if (guardian_security_witness_record_structurally_valid(
            record) == 0)
    {
        return
            GUARDIAN_SECURITY_WITNESS_ERROR_INVALID_EVIDENCE;
    }

    if ((record->witness_node_id !=
            node->self.node_id) ||
        (record->witness_producer_id !=
            node->self.producer_id) ||
        (record->witness_key_id !=
            node->self.key_id))
    {
        return
            GUARDIAN_SECURITY_WITNESS_ERROR_INVALID_EVIDENCE;
    }

    result =
        guardian_security_witness_build_transcript(
            record,
            transcript,
            sizeof(transcript),
            &transcript_length);

    if (result !=
        GUARDIAN_SECURITY_WITNESS_OK)
    {
        return result;
    }

    provider_result =
        node->provider.verify(
            node->provider.context,
            record->signature_algorithm,
            record->witness_key_id,
            transcript,
            transcript_length,
            record->signature,
            sizeof(record->signature));

    (void)memset(
        transcript,
        0,
        sizeof(transcript));

    return
        guardian_security_witness_map_provider(
            provider_result);
}

guardian_security_witness_result_t
guardian_security_witness_can_actuate(
    const guardian_security_witness_node_t *node,
    uint8_t *allowed)
{
    if ((node == NULL) ||
        (allowed == NULL))
    {
        return
            GUARDIAN_SECURITY_WITNESS_ERROR_NULL_ARGUMENT;
    }

    *allowed =
        0U;

    return
        GUARDIAN_SECURITY_WITNESS_ERROR_ACTUATION_PROHIBITED;
}
