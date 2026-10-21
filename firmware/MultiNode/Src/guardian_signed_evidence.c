#include "guardian_signed_evidence.h"

#include <string.h>

static const uint8_t guardian_signed_evidence_domain[] =
    GUARDIAN_SIGNED_EVIDENCE_DOMAIN;

static const uint8_t guardian_signed_evidence_digest_domain[] =
    GUARDIAN_SIGNED_EVIDENCE_DIGEST_DOMAIN;

static void guardian_signed_evidence_put_u32_be(
    uint8_t *output,
    uint32_t value)
{
    output[0] = (uint8_t)((value >> 24U) & 0xFFU);
    output[1] = (uint8_t)((value >> 16U) & 0xFFU);
    output[2] = (uint8_t)((value >> 8U) & 0xFFU);
    output[3] = (uint8_t)(value & 0xFFU);
}

static int guardian_signed_evidence_all_zero(
    const uint8_t value[GUARDIAN_SHA256_SIZE])
{
    size_t index;
    uint8_t aggregate = 0U;

    for (index = 0U; index < GUARDIAN_SHA256_SIZE; index += 1U)
    {
        aggregate = (uint8_t)(aggregate | value[index]);
    }

    return aggregate == 0U ? 1 : 0;
}

static int guardian_signed_evidence_class_valid(
    guardian_signed_evidence_class_t evidence_class)
{
    return
        (evidence_class > GUARDIAN_SIGNED_EVIDENCE_CLASS_INVALID) &&
        (evidence_class <= GUARDIAN_SIGNED_EVIDENCE_CLASS_RECOVERY);
}

static guardian_security_witness_evidence_type_t
    guardian_signed_evidence_witness_type(
        guardian_signed_evidence_class_t evidence_class)
{
    switch (evidence_class)
    {
        case GUARDIAN_SIGNED_EVIDENCE_CLASS_MEASUREMENT:
            return GUARDIAN_SECURITY_WITNESS_EVIDENCE_OBSERVATION;

        case GUARDIAN_SIGNED_EVIDENCE_CLASS_ANALYSIS:
        case GUARDIAN_SIGNED_EVIDENCE_CLASS_FUSION:
            return GUARDIAN_SECURITY_WITNESS_EVIDENCE_EXECUTION_RESULT;

        case GUARDIAN_SIGNED_EVIDENCE_CLASS_SUPERVISION:
            return GUARDIAN_SECURITY_WITNESS_EVIDENCE_SUPERVISION;

        case GUARDIAN_SIGNED_EVIDENCE_CLASS_RECOVERY:
            return GUARDIAN_SECURITY_WITNESS_EVIDENCE_RECOVERY_REQUEST;

        case GUARDIAN_SIGNED_EVIDENCE_CLASS_INVALID:
        default:
            return GUARDIAN_SECURITY_WITNESS_EVIDENCE_INVALID;
    }
}

static int guardian_signed_evidence_role_authorized(
    guardian_signed_evidence_class_t evidence_class,
    const guardian_multinode_node_t *resolved)
{
    if (resolved == NULL)
    {
        return 0;
    }

    if ((resolved->capabilities & GUARDIAN_CAP_ACTUATION_GATE) != 0U)
    {
        return 0;
    }

    switch (evidence_class)
    {
        case GUARDIAN_SIGNED_EVIDENCE_CLASS_MEASUREMENT:
            return
                (resolved->role == GUARDIAN_NODE_ROLE_ST_MEASUREMENT_REFERENCE) &&
                (resolved->authority_class == GUARDIAN_AUTHORITY_CLASS_OBSERVE) &&
                ((resolved->capabilities & GUARDIAN_CAP_SENSOR_ACQUISITION) != 0U);

        case GUARDIAN_SIGNED_EVIDENCE_CLASS_ANALYSIS:
        case GUARDIAN_SIGNED_EVIDENCE_CLASS_FUSION:
            return
                (resolved->role == GUARDIAN_NODE_ROLE_HIGH_COMPUTE_PROCESSING) &&
                (resolved->authority_class == GUARDIAN_AUTHORITY_CLASS_OBSERVE) &&
                ((resolved->capabilities & GUARDIAN_CAP_SIGNAL_PROCESSING) != 0U);

        case GUARDIAN_SIGNED_EVIDENCE_CLASS_SUPERVISION:
            return
                (resolved->role == GUARDIAN_NODE_ROLE_MCXN947_SUPERVISORY_PEER) &&
                (resolved->authority_class == GUARDIAN_AUTHORITY_CLASS_OBSERVE) &&
                ((resolved->capabilities & GUARDIAN_CAP_SUPERVISION) != 0U);

        case GUARDIAN_SIGNED_EVIDENCE_CLASS_RECOVERY:
            return
                (resolved->role == GUARDIAN_NODE_ROLE_MCXN947_SUPERVISORY_PEER) &&
                (resolved->authority_class == GUARDIAN_AUTHORITY_CLASS_OBSERVE) &&
                ((resolved->capabilities & GUARDIAN_CAP_RECOVERY_ASSIST) != 0U);

        case GUARDIAN_SIGNED_EVIDENCE_CLASS_INVALID:
        default:
            return 0;
    }
}

static guardian_signed_evidence_result_t
    guardian_signed_evidence_map_witness_result(
        guardian_security_witness_result_t result)
{
    switch (result)
    {
        case GUARDIAN_SECURITY_WITNESS_OK:
            return GUARDIAN_SIGNED_EVIDENCE_OK;

        case GUARDIAN_SECURITY_WITNESS_ERROR_SIGNATURE_INVALID:
            return GUARDIAN_SIGNED_EVIDENCE_ERROR_SIGNATURE_INVALID;

        case GUARDIAN_SECURITY_WITNESS_ERROR_KEY_UNKNOWN:
            return GUARDIAN_SIGNED_EVIDENCE_ERROR_KEY_UNKNOWN;

        case GUARDIAN_SECURITY_WITNESS_ERROR_KEY_REVOKED:
            return GUARDIAN_SIGNED_EVIDENCE_ERROR_KEY_REVOKED;

        case GUARDIAN_SECURITY_WITNESS_ERROR_PROVIDER_UNAVAILABLE:
            return GUARDIAN_SIGNED_EVIDENCE_ERROR_PROVIDER_UNAVAILABLE;

        case GUARDIAN_SECURITY_WITNESS_ERROR_PROVIDER_FAILURE:
            return GUARDIAN_SIGNED_EVIDENCE_ERROR_PROVIDER_FAILURE;

        default:
            return GUARDIAN_SIGNED_EVIDENCE_ERROR_INVALID_ENVELOPE;
    }
}

static guardian_signed_evidence_result_t
    guardian_signed_evidence_compute_digest(
        const guardian_signed_evidence_core_t *core,
        const guardian_security_witness_record_t *witness_record,
        uint8_t digest[GUARDIAN_SHA256_SIZE])
{
    uint8_t transcript[GUARDIAN_SIGNED_EVIDENCE_TRANSCRIPT_MAX_SIZE];
    uint8_t digest_input[GUARDIAN_SIGNED_EVIDENCE_DIGEST_INPUT_MAX_SIZE];
    size_t transcript_length = 0U;
    size_t offset = 0U;
    size_t domain_length = sizeof(guardian_signed_evidence_digest_domain) - 1U;
    guardian_signed_evidence_result_t result;

    if ((core == NULL) || (witness_record == NULL) || (digest == NULL))
    {
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_NULL_ARGUMENT;
    }

    result = guardian_signed_evidence_build_transcript(
        core,
        transcript,
        sizeof(transcript),
        &transcript_length);

    if (result != GUARDIAN_SIGNED_EVIDENCE_OK)
    {
        return result;
    }

    if ((domain_length + transcript_length +
         GUARDIAN_SECURITY_WITNESS_SIGNATURE_SIZE) > sizeof(digest_input))
    {
        guardian_crypto_zero(transcript, sizeof(transcript));
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_TRANSCRIPT;
    }

    (void)memcpy(&digest_input[offset], guardian_signed_evidence_digest_domain, domain_length);
    offset += domain_length;

    (void)memcpy(&digest_input[offset], transcript, transcript_length);
    offset += transcript_length;

    (void)memcpy(
        &digest_input[offset],
        witness_record->signature,
        GUARDIAN_SECURITY_WITNESS_SIGNATURE_SIZE);
    offset += GUARDIAN_SECURITY_WITNESS_SIGNATURE_SIZE;

    guardian_sha256(digest_input, offset, digest);

    guardian_crypto_zero(transcript, sizeof(transcript));
    guardian_crypto_zero(digest_input, sizeof(digest_input));

    return GUARDIAN_SIGNED_EVIDENCE_OK;
}

static guardian_signed_evidence_producer_state_t *
    guardian_signed_evidence_find_state(
        guardian_signed_evidence_t *context,
        const guardian_signed_evidence_core_t *core,
        size_t *free_index)
{
    size_t index;

    *free_index = GUARDIAN_SIGNED_EVIDENCE_MAX_TRACKED_PRODUCERS;

    for (index = 0U; index < GUARDIAN_SIGNED_EVIDENCE_MAX_TRACKED_PRODUCERS; index += 1U)
    {
        guardian_signed_evidence_producer_state_t *state = &context->producers[index];

        if ((state->active == 1U) &&
            (state->producer_node_id == core->producer_node_id) &&
            (state->producer_id == core->producer_id) &&
            (state->producer_key_id == core->producer_key_id))
        {
            return state;
        }

        if ((*free_index == GUARDIAN_SIGNED_EVIDENCE_MAX_TRACKED_PRODUCERS) &&
            (state->active == 0U))
        {
            *free_index = index;
        }
    }

    return NULL;
}

void guardian_signed_evidence_init(
    guardian_signed_evidence_t *context)
{
    if (context == NULL)
    {
        return;
    }

    (void)memset(context, 0, sizeof(*context));
}

guardian_signed_evidence_result_t guardian_signed_evidence_configure(
    guardian_signed_evidence_t *context,
    guardian_security_witness_node_t *witness,
    const guardian_signed_evidence_config_t *config)
{
    if ((context == NULL) || (witness == NULL) || (config == NULL))
    {
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_NULL_ARGUMENT;
    }

    if ((witness->initialized != 1U) || (config->lookup == NULL))
    {
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_INVALID_CONFIG;
    }

    guardian_signed_evidence_init(context);
    context->witness = witness;
    context->config = *config;
    context->initialized = 1U;

    return GUARDIAN_SIGNED_EVIDENCE_OK;
}

guardian_signed_evidence_result_t guardian_signed_evidence_build_transcript(
    const guardian_signed_evidence_core_t *core,
    uint8_t *output,
    size_t output_capacity,
    size_t *output_length)
{
    size_t offset = 0U;
    size_t domain_length = sizeof(guardian_signed_evidence_domain) - 1U;

    if (output_length != NULL)
    {
        *output_length = 0U;
    }

    if ((core == NULL) || (output == NULL) || (output_length == NULL))
    {
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_NULL_ARGUMENT;
    }

    if ((core->schema_version != GUARDIAN_SIGNED_EVIDENCE_SCHEMA_VERSION) ||
        (guardian_signed_evidence_class_valid(core->evidence_class) == 0) ||
        (core->actuation_authorized != GUARDIAN_SIGNED_EVIDENCE_AUTHORITY_NONE) ||
        (core->producer_role == GUARDIAN_NODE_ROLE_INVALID) ||
        (core->producer_node_id == 0U) ||
        (core->producer_id == 0U) ||
        (core->producer_key_id == 0U) ||
        (core->epoch == 0U) ||
        (core->sequence == 0U))
    {
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_INVALID_ENVELOPE;
    }

    if (output_capacity < GUARDIAN_SIGNED_EVIDENCE_TRANSCRIPT_MAX_SIZE)
    {
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_TRANSCRIPT;
    }

    (void)memset(output, 0, output_capacity);

    (void)memcpy(&output[offset], guardian_signed_evidence_domain, domain_length);
    offset += domain_length;

    output[offset] = core->schema_version;
    offset += 1U;

    output[offset] = (uint8_t)core->evidence_class;
    offset += 1U;

    output[offset] = (uint8_t)core->producer_role;
    offset += 1U;

    output[offset] = core->actuation_authorized;
    offset += 1U;

    guardian_signed_evidence_put_u32_be(&output[offset], core->producer_node_id);
    offset += 4U;

    guardian_signed_evidence_put_u32_be(&output[offset], core->producer_id);
    offset += 4U;

    guardian_signed_evidence_put_u32_be(&output[offset], core->producer_key_id);
    offset += 4U;

    guardian_signed_evidence_put_u32_be(&output[offset], core->epoch);
    offset += 4U;

    guardian_signed_evidence_put_u32_be(&output[offset], core->sequence);
    offset += 4U;

    (void)memcpy(&output[offset], core->payload_digest, GUARDIAN_SHA256_SIZE);
    offset += GUARDIAN_SHA256_SIZE;

    (void)memcpy(&output[offset], core->analysis_digest, GUARDIAN_SHA256_SIZE);
    offset += GUARDIAN_SHA256_SIZE;

    (void)memcpy(&output[offset], core->previous_evidence_digest, GUARDIAN_SHA256_SIZE);
    offset += GUARDIAN_SHA256_SIZE;

    if (offset > GUARDIAN_SIGNED_EVIDENCE_TRANSCRIPT_MAX_SIZE)
    {
        (void)memset(output, 0, output_capacity);
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_TRANSCRIPT;
    }

    *output_length = offset;
    return GUARDIAN_SIGNED_EVIDENCE_OK;
}

guardian_signed_evidence_result_t guardian_signed_evidence_create(
    guardian_signed_evidence_t *context,
    const guardian_multinode_node_t *producer,
    guardian_signed_evidence_class_t evidence_class,
    uint32_t epoch,
    uint32_t sequence,
    const uint8_t payload_digest[GUARDIAN_SHA256_SIZE],
    const uint8_t analysis_digest[GUARDIAN_SHA256_SIZE],
    const uint8_t previous_evidence_digest[GUARDIAN_SHA256_SIZE],
    guardian_signed_evidence_envelope_t *envelope)
{
    guardian_security_witness_input_t witness_input;
    guardian_security_witness_result_t witness_result;
    guardian_signed_evidence_result_t result;
    uint8_t transcript[GUARDIAN_SIGNED_EVIDENCE_TRANSCRIPT_MAX_SIZE];
    size_t transcript_length = 0U;

    if ((context == NULL) || (producer == NULL) ||
        (payload_digest == NULL) || (analysis_digest == NULL) ||
        (previous_evidence_digest == NULL) || (envelope == NULL))
    {
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_NULL_ARGUMENT;
    }

    (void)memset(envelope, 0, sizeof(*envelope));

    if ((context->initialized != 1U) || (context->witness == NULL))
    {
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_INVALID_CONFIG;
    }

    if ((guardian_multinode_validate_node(producer) != GUARDIAN_MULTINODE_OK) ||
        (guardian_signed_evidence_class_valid(evidence_class) == 0) ||
        (epoch == 0U) || (sequence == 0U))
    {
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_INVALID_PRODUCER;
    }

    envelope->core.schema_version = GUARDIAN_SIGNED_EVIDENCE_SCHEMA_VERSION;
    envelope->core.evidence_class = evidence_class;
    envelope->core.producer_role = producer->role;
    envelope->core.actuation_authorized = GUARDIAN_SIGNED_EVIDENCE_AUTHORITY_NONE;
    envelope->core.producer_node_id = producer->node_id;
    envelope->core.producer_id = producer->producer_id;
    envelope->core.producer_key_id = producer->key_id;
    envelope->core.epoch = epoch;
    envelope->core.sequence = sequence;

    (void)memcpy(envelope->core.payload_digest, payload_digest, GUARDIAN_SHA256_SIZE);
    (void)memcpy(envelope->core.analysis_digest, analysis_digest, GUARDIAN_SHA256_SIZE);
    (void)memcpy(
        envelope->core.previous_evidence_digest,
        previous_evidence_digest,
        GUARDIAN_SHA256_SIZE);

    result = guardian_signed_evidence_build_transcript(
        &envelope->core,
        transcript,
        sizeof(transcript),
        &transcript_length);

    if (result != GUARDIAN_SIGNED_EVIDENCE_OK)
    {
        return result;
    }

    witness_input.subject_node_id = producer->node_id;
    witness_input.subject_producer_id = producer->producer_id;
    witness_input.subject_key_id = producer->key_id;
    witness_input.epoch = epoch;
    witness_input.sequence = sequence;
    witness_input.evidence_type = guardian_signed_evidence_witness_type(evidence_class);
    witness_input.payload = transcript;
    witness_input.payload_length = transcript_length;

    witness_result = guardian_security_witness_create(
        context->witness,
        &witness_input,
        &envelope->witness_record);

    guardian_crypto_zero(transcript, sizeof(transcript));

    if (witness_result != GUARDIAN_SECURITY_WITNESS_OK)
    {
        (void)memset(envelope, 0, sizeof(*envelope));
        return guardian_signed_evidence_map_witness_result(witness_result);
    }

    result = guardian_signed_evidence_compute_digest(
        &envelope->core,
        &envelope->witness_record,
        envelope->evidence_digest);

    if (result != GUARDIAN_SIGNED_EVIDENCE_OK)
    {
        (void)memset(envelope, 0, sizeof(*envelope));
        return result;
    }

    return GUARDIAN_SIGNED_EVIDENCE_OK;
}

guardian_signed_evidence_result_t guardian_signed_evidence_verify_and_accept(
    guardian_signed_evidence_t *context,
    const guardian_signed_evidence_envelope_t *envelope,
    guardian_signed_evidence_verdict_t *verdict)
{
    guardian_multinode_node_t resolved_node;
    guardian_signed_evidence_trust_t trust = GUARDIAN_SIGNED_EVIDENCE_TRUST_UNKNOWN;
    guardian_security_witness_result_t witness_result;
    guardian_signed_evidence_result_t result;
    guardian_signed_evidence_producer_state_t *state;
    size_t free_index;
    uint8_t transcript[GUARDIAN_SIGNED_EVIDENCE_TRANSCRIPT_MAX_SIZE];
    uint8_t transcript_digest[GUARDIAN_SHA256_SIZE];
    uint8_t recomputed_evidence_digest[GUARDIAN_SHA256_SIZE];
    size_t transcript_length = 0U;

    if ((context == NULL) || (envelope == NULL) || (verdict == NULL))
    {
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_NULL_ARGUMENT;
    }

    (void)memset(verdict, 0, sizeof(*verdict));
    (void)memset(&resolved_node, 0, sizeof(resolved_node));

    if ((context->initialized != 1U) ||
        (context->witness == NULL) ||
        (context->config.lookup == NULL))
    {
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_INVALID_CONFIG;
    }

    verdict->actuation_authorized = GUARDIAN_SIGNED_EVIDENCE_AUTHORITY_NONE;

    result = guardian_signed_evidence_build_transcript(
        &envelope->core,
        transcript,
        sizeof(transcript),
        &transcript_length);

    if (result != GUARDIAN_SIGNED_EVIDENCE_OK)
    {
        return result;
    }

    if ((envelope->witness_record.subject_node_id != envelope->core.producer_node_id) ||
        (envelope->witness_record.subject_producer_id != envelope->core.producer_id) ||
        (envelope->witness_record.subject_key_id != envelope->core.producer_key_id) ||
        (envelope->witness_record.epoch != envelope->core.epoch) ||
        (envelope->witness_record.sequence != envelope->core.sequence) ||
        (envelope->witness_record.evidence_type !=
            guardian_signed_evidence_witness_type(envelope->core.evidence_class)))
    {
        guardian_crypto_zero(transcript, sizeof(transcript));
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_INVALID_ENVELOPE;
    }

    guardian_sha256(transcript, transcript_length, transcript_digest);

    if (guardian_crypto_constant_time_equal(
            transcript_digest,
            envelope->witness_record.payload_digest,
            GUARDIAN_SHA256_SIZE) == 0)
    {
        guardian_crypto_zero(transcript, sizeof(transcript));
        guardian_crypto_zero(transcript_digest, sizeof(transcript_digest));
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_SIGNATURE_INVALID;
    }

    guardian_crypto_zero(transcript, sizeof(transcript));
    guardian_crypto_zero(transcript_digest, sizeof(transcript_digest));

    witness_result = guardian_security_witness_verify(
        context->witness,
        &envelope->witness_record);

    if (witness_result != GUARDIAN_SECURITY_WITNESS_OK)
    {
        return guardian_signed_evidence_map_witness_result(witness_result);
    }

    verdict->signature_valid = 1U;

    result = guardian_signed_evidence_compute_digest(
        &envelope->core,
        &envelope->witness_record,
        recomputed_evidence_digest);

    if (result != GUARDIAN_SIGNED_EVIDENCE_OK)
    {
        return result;
    }

    if (guardian_crypto_constant_time_equal(
            recomputed_evidence_digest,
            envelope->evidence_digest,
            GUARDIAN_SHA256_SIZE) == 0)
    {
        guardian_crypto_zero(recomputed_evidence_digest, sizeof(recomputed_evidence_digest));
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_INVALID_ENVELOPE;
    }

    guardian_crypto_zero(recomputed_evidence_digest, sizeof(recomputed_evidence_digest));

    if (context->config.lookup(
            context->config.lookup_context,
            envelope->core.producer_node_id,
            envelope->core.producer_id,
            envelope->core.producer_key_id,
            &resolved_node,
            &trust) == 0)
    {
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_KEY_UNKNOWN;
    }

    if (trust == GUARDIAN_SIGNED_EVIDENCE_TRUST_REVOKED)
    {
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_KEY_REVOKED;
    }

    if (trust != GUARDIAN_SIGNED_EVIDENCE_TRUST_TRUSTED)
    {
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_UNTRUSTED;
    }

    verdict->trusted = 1U;

    if ((resolved_node.node_id != envelope->core.producer_node_id) ||
        (resolved_node.producer_id != envelope->core.producer_id) ||
        (resolved_node.key_id != envelope->core.producer_key_id) ||
        (resolved_node.role != envelope->core.producer_role) ||
        (guardian_multinode_validate_node(&resolved_node) != GUARDIAN_MULTINODE_OK) ||
        (guardian_signed_evidence_role_authorized(
            envelope->core.evidence_class,
            &resolved_node) == 0))
    {
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_UNAUTHORIZED_ROLE;
    }

    verdict->authorized = 1U;

    state = guardian_signed_evidence_find_state(
        context,
        &envelope->core,
        &free_index);

    if (state == NULL)
    {
        if (free_index >= GUARDIAN_SIGNED_EVIDENCE_MAX_TRACKED_PRODUCERS)
        {
            return GUARDIAN_SIGNED_EVIDENCE_ERROR_TRACKING_TABLE_FULL;
        }

        if (guardian_signed_evidence_all_zero(
                envelope->core.previous_evidence_digest) == 0)
        {
            return GUARDIAN_SIGNED_EVIDENCE_ERROR_CHAIN_MISMATCH;
        }

        verdict->chain_valid = 1U;
        verdict->fresh = 1U;
        state = &context->producers[free_index];
    }
    else
    {
        if (guardian_crypto_constant_time_equal(
                envelope->core.previous_evidence_digest,
                state->last_evidence_digest,
                GUARDIAN_SHA256_SIZE) == 0)
        {
            return GUARDIAN_SIGNED_EVIDENCE_ERROR_CHAIN_MISMATCH;
        }

        verdict->chain_valid = 1U;

        if ((envelope->core.epoch < state->accepted_epoch) ||
            ((envelope->core.epoch == state->accepted_epoch) &&
             (envelope->core.sequence <= state->accepted_sequence)))
        {
            return GUARDIAN_SIGNED_EVIDENCE_ERROR_REPLAY_OR_REGRESSION;
        }

        verdict->fresh = 1U;
    }

    if (state->active == 0U)
    {
        (void)memset(state, 0, sizeof(*state));
        state->producer_node_id = envelope->core.producer_node_id;
        state->producer_id = envelope->core.producer_id;
        state->producer_key_id = envelope->core.producer_key_id;
        state->producer_role = envelope->core.producer_role;
        state->active = 1U;
        context->producer_count += 1U;
    }

    state->accepted_epoch = envelope->core.epoch;
    state->accepted_sequence = envelope->core.sequence;
    (void)memcpy(
        state->last_evidence_digest,
        envelope->evidence_digest,
        GUARDIAN_SHA256_SIZE);

    verdict->accepted = 1U;
    verdict->actuation_authorized = GUARDIAN_SIGNED_EVIDENCE_AUTHORITY_NONE;

    return GUARDIAN_SIGNED_EVIDENCE_OK;
}

guardian_signed_evidence_result_t guardian_signed_evidence_can_actuate(
    const guardian_signed_evidence_t *context,
    uint8_t *allowed)
{
    if ((context == NULL) || (allowed == NULL))
    {
        return GUARDIAN_SIGNED_EVIDENCE_ERROR_NULL_ARGUMENT;
    }

    *allowed = 0U;
    return GUARDIAN_SIGNED_EVIDENCE_ERROR_ACTUATION_PROHIBITED;
}
