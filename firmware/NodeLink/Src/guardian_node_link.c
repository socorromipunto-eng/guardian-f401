#include "guardian_node_link.h"

#include <string.h>

static void write_u16_be(uint8_t *output, uint16_t value)
{
    output[0] = (uint8_t)(value >> 8);
    output[1] = (uint8_t)value;
}

static void write_u32_be(uint8_t *output, uint32_t value)
{
    output[0] = (uint8_t)(value >> 24);
    output[1] = (uint8_t)(value >> 16);
    output[2] = (uint8_t)(value >> 8);
    output[3] = (uint8_t)value;
}

static uint16_t read_u16_be(const uint8_t *input)
{
    return (uint16_t)(((uint16_t)input[0] << 8) | (uint16_t)input[1]);
}

static uint32_t read_u32_be(const uint8_t *input)
{
    return ((uint32_t)input[0] << 24) |
           ((uint32_t)input[1] << 16) |
           ((uint32_t)input[2] << 8) |
           (uint32_t)input[3];
}

static int valid_message_type(uint8_t value)
{
    switch (value)
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

static int valid_state(uint8_t value)
{
    return value <= (uint8_t)GUARDIAN_NODE_STATE_FAULT;
}

uint32_t guardian_node_link_crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;
    size_t i;

    if ((data == NULL) && (length != 0U))
    {
        return 0U;
    }

    for (i = 0U; i < length; ++i)
    {
        uint32_t byte = (uint32_t)data[i];
        uint32_t bit;

        crc ^= byte;
        for (bit = 0U; bit < 8U; ++bit)
        {
            uint32_t mask = (uint32_t)(0U - (crc & 1U));
            crc = (crc >> 1) ^ (0xEDB88320UL & mask);
        }
    }

    return ~crc;
}

guardian_node_link_result_t guardian_node_link_encode(
    const guardian_node_link_frame_t *frame,
    uint8_t *output,
    size_t output_capacity,
    size_t *output_size)
{
    size_t frame_size;
    uint32_t crc;

    if ((frame == NULL) || (output == NULL) || (output_size == NULL))
    {
        return GUARDIAN_NODE_LINK_ERROR_NULL_ARGUMENT;
    }

    if (!valid_message_type((uint8_t)frame->message_type))
    {
        return GUARDIAN_NODE_LINK_ERROR_MESSAGE_TYPE;
    }

    if (frame->flags != GUARDIAN_NODE_LINK_SUPPORTED_FLAGS)
    {
        return GUARDIAN_NODE_LINK_ERROR_FLAGS;
    }

    if (!valid_state((uint8_t)frame->node_state))
    {
        return GUARDIAN_NODE_LINK_ERROR_STATE;
    }

    if (frame->sequence == 0U)
    {
        return GUARDIAN_NODE_LINK_ERROR_SEQUENCE_ZERO;
    }

    if ((size_t)frame->payload_length > GUARDIAN_NODE_LINK_MAX_PAYLOAD)
    {
        return GUARDIAN_NODE_LINK_ERROR_PAYLOAD_TOO_LARGE;
    }

    frame_size = GUARDIAN_NODE_LINK_HEADER_SIZE +
                 (size_t)frame->payload_length +
                 GUARDIAN_NODE_LINK_CRC_SIZE;

    if (output_capacity < frame_size)
    {
        return GUARDIAN_NODE_LINK_ERROR_OUTPUT_TOO_SMALL;
    }

    output[0] = GUARDIAN_NODE_LINK_MAGIC_0;
    output[1] = GUARDIAN_NODE_LINK_MAGIC_1;
    output[2] = GUARDIAN_NODE_LINK_VERSION;
    output[3] = (uint8_t)frame->message_type;
    output[4] = frame->flags;
    output[5] = (uint8_t)frame->node_state;
    write_u16_be(&output[6], frame->payload_length);
    write_u32_be(&output[8], frame->sequence);
    write_u32_be(&output[12], frame->sender_epoch);
    write_u32_be(&output[16], frame->sender_node_id);

    if (frame->payload_length != 0U)
    {
        (void)memcpy(
            &output[GUARDIAN_NODE_LINK_HEADER_SIZE],
            frame->payload,
            frame->payload_length);
    }

    crc = guardian_node_link_crc32(
        output,
        GUARDIAN_NODE_LINK_HEADER_SIZE + (size_t)frame->payload_length);

    write_u32_be(
        &output[GUARDIAN_NODE_LINK_HEADER_SIZE + (size_t)frame->payload_length],
        crc);

    *output_size = frame_size;
    return GUARDIAN_NODE_LINK_OK;
}

guardian_node_link_result_t guardian_node_link_decode(
    const uint8_t *input,
    size_t input_size,
    guardian_node_link_frame_t *frame)
{
    uint16_t payload_length;
    size_t expected_size;
    uint32_t encoded_crc;
    uint32_t calculated_crc;

    if ((input == NULL) || (frame == NULL))
    {
        return GUARDIAN_NODE_LINK_ERROR_NULL_ARGUMENT;
    }

    if (input_size < (GUARDIAN_NODE_LINK_HEADER_SIZE + GUARDIAN_NODE_LINK_CRC_SIZE))
    {
        return GUARDIAN_NODE_LINK_ERROR_FRAME_TOO_SHORT;
    }

    if ((input[0] != GUARDIAN_NODE_LINK_MAGIC_0) ||
        (input[1] != GUARDIAN_NODE_LINK_MAGIC_1))
    {
        return GUARDIAN_NODE_LINK_ERROR_MAGIC;
    }

    if (input[2] != GUARDIAN_NODE_LINK_VERSION)
    {
        return GUARDIAN_NODE_LINK_ERROR_VERSION;
    }

    if (!valid_message_type(input[3]))
    {
        return GUARDIAN_NODE_LINK_ERROR_MESSAGE_TYPE;
    }

    if (input[4] != GUARDIAN_NODE_LINK_SUPPORTED_FLAGS)
    {
        return GUARDIAN_NODE_LINK_ERROR_FLAGS;
    }

    if (!valid_state(input[5]))
    {
        return GUARDIAN_NODE_LINK_ERROR_STATE;
    }

    payload_length = read_u16_be(&input[6]);
    if ((size_t)payload_length > GUARDIAN_NODE_LINK_MAX_PAYLOAD)
    {
        return GUARDIAN_NODE_LINK_ERROR_PAYLOAD_TOO_LARGE;
    }

    expected_size = GUARDIAN_NODE_LINK_HEADER_SIZE +
                    (size_t)payload_length +
                    GUARDIAN_NODE_LINK_CRC_SIZE;

    if (input_size != expected_size)
    {
        return GUARDIAN_NODE_LINK_ERROR_LENGTH_MISMATCH;
    }

    if (read_u32_be(&input[8]) == 0U)
    {
        return GUARDIAN_NODE_LINK_ERROR_SEQUENCE_ZERO;
    }

    encoded_crc = read_u32_be(
        &input[GUARDIAN_NODE_LINK_HEADER_SIZE + (size_t)payload_length]);

    calculated_crc = guardian_node_link_crc32(
        input,
        GUARDIAN_NODE_LINK_HEADER_SIZE + (size_t)payload_length);

    if (encoded_crc != calculated_crc)
    {
        return GUARDIAN_NODE_LINK_ERROR_CRC;
    }

    frame->message_type = (guardian_node_link_message_type_t)input[3];
    frame->flags = input[4];
    frame->node_state = (guardian_node_state_t)input[5];
    frame->payload_length = payload_length;
    frame->sequence = read_u32_be(&input[8]);
    frame->sender_epoch = read_u32_be(&input[12]);
    frame->sender_node_id = read_u32_be(&input[16]);

    if (payload_length != 0U)
    {
        (void)memcpy(
            frame->payload,
            &input[GUARDIAN_NODE_LINK_HEADER_SIZE],
            payload_length);
    }

    if ((size_t)payload_length < GUARDIAN_NODE_LINK_MAX_PAYLOAD)
    {
        (void)memset(
            &frame->payload[payload_length],
            0,
            GUARDIAN_NODE_LINK_MAX_PAYLOAD - (size_t)payload_length);
    }

    return GUARDIAN_NODE_LINK_OK;
}

guardian_node_link_result_t guardian_node_link_accept_sequence(
    guardian_node_link_sequence_guard_t *guard,
    uint32_t sequence)
{
    if (guard == NULL)
    {
        return GUARDIAN_NODE_LINK_ERROR_NULL_ARGUMENT;
    }

    if (sequence == 0U)
    {
        return GUARDIAN_NODE_LINK_ERROR_SEQUENCE_ZERO;
    }

    if (guard->initialized != 0U)
    {
        if (sequence <= guard->last_sequence)
        {
            return GUARDIAN_NODE_LINK_ERROR_REPLAY_OR_REGRESSION;
        }
    }

    guard->last_sequence = sequence;
    guard->initialized = 1U;
    return GUARDIAN_NODE_LINK_OK;
}
