#ifndef GUARDIAN_NODE_LINK_H
#define GUARDIAN_NODE_LINK_H

#include <stddef.h>
#include <stdint.h>

#define GUARDIAN_NODE_LINK_MAGIC_0 ((uint8_t)0x47U)
#define GUARDIAN_NODE_LINK_MAGIC_1 ((uint8_t)0x4EU)
#define GUARDIAN_NODE_LINK_VERSION ((uint8_t)0x01U)
#define GUARDIAN_NODE_LINK_HEADER_SIZE ((size_t)20U)
#define GUARDIAN_NODE_LINK_CRC_SIZE ((size_t)4U)
#define GUARDIAN_NODE_LINK_MAX_PAYLOAD ((size_t)64U)
#define GUARDIAN_NODE_LINK_MAX_FRAME \
    (GUARDIAN_NODE_LINK_HEADER_SIZE + GUARDIAN_NODE_LINK_MAX_PAYLOAD + GUARDIAN_NODE_LINK_CRC_SIZE)
#define GUARDIAN_NODE_LINK_SUPPORTED_FLAGS ((uint8_t)0x00U)

typedef enum
{
    GUARDIAN_NODE_LINK_MESSAGE_HELLO = 0x01,
    GUARDIAN_NODE_LINK_MESSAGE_CHALLENGE = 0x02,
    GUARDIAN_NODE_LINK_MESSAGE_RESPONSE = 0x03,
    GUARDIAN_NODE_LINK_MESSAGE_HEARTBEAT = 0x04,
    GUARDIAN_NODE_LINK_MESSAGE_HEALTH = 0x05,
    GUARDIAN_NODE_LINK_MESSAGE_SUPERVISION_STATE = 0x06,
    GUARDIAN_NODE_LINK_MESSAGE_ERROR = 0x7F
} guardian_node_link_message_type_t;

typedef enum
{
    GUARDIAN_NODE_STATE_BOOT = 0x00,
    GUARDIAN_NODE_STATE_DISCOVERING = 0x01,
    GUARDIAN_NODE_STATE_ACTIVE = 0x02,
    GUARDIAN_NODE_STATE_DEGRADED = 0x03,
    GUARDIAN_NODE_STATE_SAFE_HOLD = 0x04,
    GUARDIAN_NODE_STATE_FAULT = 0x05
} guardian_node_state_t;

typedef enum
{
    GUARDIAN_NODE_LINK_OK = 0,
    GUARDIAN_NODE_LINK_ERROR_NULL_ARGUMENT,
    GUARDIAN_NODE_LINK_ERROR_OUTPUT_TOO_SMALL,
    GUARDIAN_NODE_LINK_ERROR_FRAME_TOO_SHORT,
    GUARDIAN_NODE_LINK_ERROR_MAGIC,
    GUARDIAN_NODE_LINK_ERROR_VERSION,
    GUARDIAN_NODE_LINK_ERROR_MESSAGE_TYPE,
    GUARDIAN_NODE_LINK_ERROR_FLAGS,
    GUARDIAN_NODE_LINK_ERROR_STATE,
    GUARDIAN_NODE_LINK_ERROR_SEQUENCE_ZERO,
    GUARDIAN_NODE_LINK_ERROR_PAYLOAD_TOO_LARGE,
    GUARDIAN_NODE_LINK_ERROR_LENGTH_MISMATCH,
    GUARDIAN_NODE_LINK_ERROR_CRC,
    GUARDIAN_NODE_LINK_ERROR_REPLAY_OR_REGRESSION
} guardian_node_link_result_t;

typedef struct
{
    guardian_node_link_message_type_t message_type;
    uint8_t flags;
    guardian_node_state_t node_state;
    uint16_t payload_length;
    uint32_t sequence;
    uint32_t sender_epoch;
    uint32_t sender_node_id;
    uint8_t payload[GUARDIAN_NODE_LINK_MAX_PAYLOAD];
} guardian_node_link_frame_t;

typedef struct
{
    uint32_t last_sequence;
    uint8_t initialized;
} guardian_node_link_sequence_guard_t;

uint32_t guardian_node_link_crc32(const uint8_t *data, size_t length);

guardian_node_link_result_t guardian_node_link_encode(
    const guardian_node_link_frame_t *frame,
    uint8_t *output,
    size_t output_capacity,
    size_t *output_size);

guardian_node_link_result_t guardian_node_link_decode(
    const uint8_t *input,
    size_t input_size,
    guardian_node_link_frame_t *frame);

guardian_node_link_result_t guardian_node_link_accept_sequence(
    guardian_node_link_sequence_guard_t *guard,
    uint32_t sequence);

#endif
