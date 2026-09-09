#include "guardian_node_link_auth.h"

#include <string.h>

static void guardian_node_link_auth_write_u16(
    uint8_t *output,
    uint16_t value)
{
    output[0] = (uint8_t)((value >> 8U) & 0xFFU);
    output[1] = (uint8_t)(value & 0xFFU);
}

static void guardian_node_link_auth_write_u32(
    uint8_t *output,
    uint32_t value)
{
    output[0] = (uint8_t)((value >> 24U) & 0xFFU);
    output[1] = (uint8_t)((value >> 16U) & 0xFFU);
    output[2] = (uint8_t)((value >> 8U) & 0xFFU);
    output[3] = (uint8_t)(value & 0xFFU);
}

static int guardian_node_link_auth_message_type_valid(
    guardian_node_link_message_type_t message_type)
{
    switch (message_type)
    {
        case GUARDIAN_NODE_LINK_MESSAGE_HELLO:
        case GUARDIAN_NODE_LINK_MESSAGE_CHALLENGE:
        case GUARDIAN_NODE_LINK_MESSAGE_RESPONSE:
        case GUARDIAN_NODE_LINK_MESSAGE_HEARTBEAT:
        case GUARDIAN_NODE_LINK_MESSAGE_HEALTH:
        case GUARDIAN_NODE_LINK_MESSAGE_SUPERVISION_STATE:
        case GUARDIAN_NODE_LINK_MESSAGE_ERROR:
            return 1;

        default:
            return 0;
    }
}

static int guardian_node_link_auth_state_valid(
    guardian_node_state_t node_state)
{
    return (
        (uint32_t)node_state <=
        (uint32_t)GUARDIAN_NODE_STATE_FAULT);
}

static const guardian_node_link_peer_identity_t *
guardian_node_link_auth_resolve_peer(
    const guardian_node_link_authenticator_t *authenticator,
    uint32_t sender_node_id)
{
    size_t index;

    if (authenticator == NULL)
    {
        return NULL;
    }

    for (index = 0U;
         index < authenticator->peer_count;
         index += 1U)
    {
        if (authenticator->peers[index].sender_node_id ==
            sender_node_id)
        {
            return &authenticator->peers[index];
        }
    }

    return NULL;
}

static guardian_node_link_auth_result_t
guardian_node_link_auth_map_verify_result(
    guardian_node_link_verify_result_t result)
{
    switch (result)
    {
        case GUARDIAN_NODE_LINK_VERIFY_VERIFIED:
            return GUARDIAN_NODE_LINK_AUTH_OK;

        case GUARDIAN_NODE_LINK_VERIFY_SIGNATURE_INVALID:
            return GUARDIAN_NODE_LINK_AUTH_ERROR_SIGNATURE_INVALID;

        case GUARDIAN_NODE_LINK_VERIFY_KEY_UNKNOWN:
            return GUARDIAN_NODE_LINK_AUTH_ERROR_KEY_UNKNOWN;

        case GUARDIAN_NODE_LINK_VERIFY_KEY_REVOKED:
            return GUARDIAN_NODE_LINK_AUTH_ERROR_KEY_REVOKED;

        case GUARDIAN_NODE_LINK_VERIFY_ALGORITHM_UNSUPPORTED:
            return GUARDIAN_NODE_LINK_AUTH_ERROR_ALGORITHM_UNSUPPORTED;

        case GUARDIAN_NODE_LINK_VERIFY_PROVIDER_UNAVAILABLE:
            return GUARDIAN_NODE_LINK_AUTH_ERROR_PROVIDER_UNAVAILABLE;

        case GUARDIAN_NODE_LINK_VERIFY_PROVIDER_FAILURE:
        default:
            return GUARDIAN_NODE_LINK_AUTH_ERROR_PROVIDER_FAILURE;
    }
}

void guardian_node_link_authenticator_init(
    guardian_node_link_authenticator_t *authenticator)
{
    if (authenticator == NULL)
    {
        return;
    }

    (void)memset(authenticator, 0, sizeof(*authenticator));
}

guardian_node_link_auth_result_t
guardian_node_link_authenticator_configure(
    guardian_node_link_authenticator_t *authenticator,
    const guardian_node_link_auth_config_t *config)
{
    size_t first;
    size_t second;

    if ((authenticator == NULL) || (config == NULL))
    {
        return GUARDIAN_NODE_LINK_AUTH_ERROR_NULL_ARGUMENT;
    }

    (void)memset(authenticator, 0, sizeof(*authenticator));

    if (config->verify_signature == NULL)
    {
        return GUARDIAN_NODE_LINK_AUTH_ERROR_UNCONFIGURED;
    }

    if (config->peer_count > GUARDIAN_NODE_LINK_AUTH_MAX_PEERS)
    {
        return GUARDIAN_NODE_LINK_AUTH_ERROR_UNCONFIGURED;
    }

    if ((config->peer_count > 0U) &&
        (config->peers == NULL))
    {
        return GUARDIAN_NODE_LINK_AUTH_ERROR_UNCONFIGURED;
    }

    if (config->required_signature_algorithm !=
        GUARDIAN_NODE_LINK_SIGNATURE_ED25519)
    {
        return GUARDIAN_NODE_LINK_AUTH_ERROR_ALGORITHM_UNSUPPORTED;
    }

    for (first = 0U; first < config->peer_count; first += 1U)
    {
        const guardian_node_link_peer_identity_t *peer =
            &config->peers[first];

        if ((peer->sender_node_id == 0U) ||
            (peer->producer_id == 0U) ||
            (peer->key_id == 0U))
        {
            return GUARDIAN_NODE_LINK_AUTH_ERROR_UNCONFIGURED;
        }

        if (peer->signature_algorithm !=
            config->required_signature_algorithm)
        {
            return GUARDIAN_NODE_LINK_AUTH_ERROR_ALGORITHM_UNSUPPORTED;
        }

        for (second = first + 1U;
             second < config->peer_count;
             second += 1U)
        {
            if (peer->sender_node_id ==
                config->peers[second].sender_node_id)
            {
                return GUARDIAN_NODE_LINK_AUTH_ERROR_UNCONFIGURED;
            }
        }
    }

    authenticator->context = config->context;
    authenticator->verify_signature = config->verify_signature;
    authenticator->peer_count = config->peer_count;
    authenticator->required_signature_algorithm =
        config->required_signature_algorithm;

    if (config->peer_count > 0U)
    {
        (void)memcpy(
            authenticator->peers,
            config->peers,
            config->peer_count *
                sizeof(guardian_node_link_peer_identity_t));
    }

    authenticator->configured = 1U;

    return GUARDIAN_NODE_LINK_AUTH_OK;
}

guardian_node_link_auth_result_t
guardian_node_link_build_auth_transcript(
    const guardian_node_link_frame_t *frame,
    uint32_t producer_id,
    uint32_t key_id,
    uint8_t *output,
    size_t output_capacity,
    size_t *output_length)
{
    static const uint8_t domain[] =
        GUARDIAN_NODE_LINK_AUTH_DOMAIN;

    size_t required;
    size_t offset = 0U;

    if ((frame == NULL) ||
        (output == NULL) ||
        (output_length == NULL))
    {
        return GUARDIAN_NODE_LINK_AUTH_ERROR_NULL_ARGUMENT;
    }

    *output_length = 0U;

    if ((frame->flags != GUARDIAN_NODE_LINK_SUPPORTED_FLAGS) ||
        (!guardian_node_link_auth_message_type_valid(
            frame->message_type)) ||
        (!guardian_node_link_auth_state_valid(
            frame->node_state)) ||
        (frame->sequence == 0U) ||
        (frame->sender_epoch == 0U) ||
        (frame->sender_node_id == 0U) ||
        (producer_id == 0U) ||
        (key_id == 0U) ||
        (frame->payload_length >
            GUARDIAN_NODE_LINK_MAX_PAYLOAD))
    {
        return GUARDIAN_NODE_LINK_AUTH_ERROR_TRANSCRIPT;
    }

    required =
        GUARDIAN_NODE_LINK_AUTH_TRANSCRIPT_FIXED_SIZE +
        (size_t)frame->payload_length;

    if (output_capacity < required)
    {
        return GUARDIAN_NODE_LINK_AUTH_ERROR_TRANSCRIPT;
    }

    (void)memcpy(
        &output[offset],
        domain,
        GUARDIAN_NODE_LINK_AUTH_DOMAIN_SIZE);

    offset += GUARDIAN_NODE_LINK_AUTH_DOMAIN_SIZE;

    output[offset] = GUARDIAN_NODE_LINK_VERSION;
    offset += 1U;

    output[offset] = (uint8_t)frame->message_type;
    offset += 1U;

    output[offset] = frame->flags;
    offset += 1U;

    output[offset] = (uint8_t)frame->node_state;
    offset += 1U;

    guardian_node_link_auth_write_u32(
        &output[offset],
        frame->sequence);
    offset += 4U;

    guardian_node_link_auth_write_u32(
        &output[offset],
        frame->sender_epoch);
    offset += 4U;

    guardian_node_link_auth_write_u32(
        &output[offset],
        frame->sender_node_id);
    offset += 4U;

    guardian_node_link_auth_write_u32(
        &output[offset],
        producer_id);
    offset += 4U;

    guardian_node_link_auth_write_u32(
        &output[offset],
        key_id);
    offset += 4U;

    guardian_node_link_auth_write_u16(
        &output[offset],
        frame->payload_length);
    offset += 2U;

    if (frame->payload_length > 0U)
    {
        (void)memcpy(
            &output[offset],
            frame->payload,
            frame->payload_length);

        offset += frame->payload_length;
    }

    if (offset != required)
    {
        (void)memset(output, 0, output_capacity);
        return GUARDIAN_NODE_LINK_AUTH_ERROR_TRANSCRIPT;
    }

    *output_length = offset;

    return GUARDIAN_NODE_LINK_AUTH_OK;
}

guardian_node_link_auth_result_t
guardian_node_link_authenticate(
    const guardian_node_link_authenticator_t *authenticator,
    const guardian_node_link_frame_t *frame,
    const uint8_t *signature,
    size_t signature_length,
    guardian_node_link_authenticated_message_t *authenticated_message)
{
    const guardian_node_link_peer_identity_t *peer;
    uint8_t transcript[
        GUARDIAN_NODE_LINK_AUTH_TRANSCRIPT_MAX_SIZE];
    size_t transcript_length = 0U;
    guardian_node_link_auth_result_t transcript_result;
    guardian_node_link_verify_result_t verify_result;
    guardian_node_link_auth_result_t mapped_result;

    if (authenticated_message != NULL)
    {
        (void)memset(
            authenticated_message,
            0,
            sizeof(*authenticated_message));
    }

    if ((authenticator == NULL) ||
        (frame == NULL) ||
        (signature == NULL) ||
        (authenticated_message == NULL))
    {
        return GUARDIAN_NODE_LINK_AUTH_ERROR_NULL_ARGUMENT;
    }

    if ((authenticator->configured == 0U) ||
        (authenticator->verify_signature == NULL))
    {
        return GUARDIAN_NODE_LINK_AUTH_ERROR_UNCONFIGURED;
    }

    if (signature_length !=
        GUARDIAN_NODE_LINK_ED25519_SIGNATURE_SIZE)
    {
        return GUARDIAN_NODE_LINK_AUTH_ERROR_SIGNATURE_INVALID;
    }

    peer = guardian_node_link_auth_resolve_peer(
        authenticator,
        frame->sender_node_id);

    if (peer == NULL)
    {
        return GUARDIAN_NODE_LINK_AUTH_ERROR_PEER_UNKNOWN;
    }

    if (peer->signature_algorithm !=
        authenticator->required_signature_algorithm)
    {
        return GUARDIAN_NODE_LINK_AUTH_ERROR_ALGORITHM_UNSUPPORTED;
    }

    transcript_result =
        guardian_node_link_build_auth_transcript(
            frame,
            peer->producer_id,
            peer->key_id,
            transcript,
            sizeof(transcript),
            &transcript_length);

    if (transcript_result != GUARDIAN_NODE_LINK_AUTH_OK)
    {
        (void)memset(transcript, 0, sizeof(transcript));
        return transcript_result;
    }

    verify_result = authenticator->verify_signature(
        authenticator->context,
        peer->signature_algorithm,
        peer->producer_id,
        peer->key_id,
        transcript,
        transcript_length,
        signature,
        signature_length);

    (void)memset(transcript, 0, sizeof(transcript));

    mapped_result =
        guardian_node_link_auth_map_verify_result(
            verify_result);

    if (mapped_result != GUARDIAN_NODE_LINK_AUTH_OK)
    {
        return mapped_result;
    }

    authenticated_message->frame = *frame;
    authenticated_message->producer_id =
        peer->producer_id;
    authenticated_message->key_id =
        peer->key_id;
    authenticated_message->signature_algorithm =
        peer->signature_algorithm;

    return GUARDIAN_NODE_LINK_AUTH_OK;
}