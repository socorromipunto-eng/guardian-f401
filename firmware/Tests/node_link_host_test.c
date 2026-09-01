#include "guardian_node_link.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK_TRUE(expr) \
    do { \
        if (!(expr)) { \
            ++failures; \
            (void)printf("FAIL:%s:%d:%s\n", __FILE__, __LINE__, #expr); \
        } \
    } while (0)

static guardian_node_link_frame_t make_frame(void)
{
    guardian_node_link_frame_t frame;
    (void)memset(&frame, 0, sizeof(frame));

    frame.message_type = GUARDIAN_NODE_LINK_MESSAGE_HEARTBEAT;
    frame.flags = GUARDIAN_NODE_LINK_SUPPORTED_FLAGS;
    frame.node_state = GUARDIAN_NODE_STATE_ACTIVE;
    frame.payload_length = 4U;
    frame.sequence = 1U;
    frame.sender_epoch = 7U;
    frame.sender_node_id = 0x0000F401UL;
    frame.payload[0] = 0xDEU;
    frame.payload[1] = 0xADU;
    frame.payload[2] = 0xBEU;
    frame.payload[3] = 0xEFU;
    return frame;
}

static void test_round_trip(void)
{
    guardian_node_link_frame_t input = make_frame();
    guardian_node_link_frame_t decoded;
    uint8_t bytes[GUARDIAN_NODE_LINK_MAX_FRAME];
    size_t size = 0U;

    CHECK_TRUE(
        guardian_node_link_encode(&input, bytes, sizeof(bytes), &size) ==
        GUARDIAN_NODE_LINK_OK);

    CHECK_TRUE(size == 28U);

    (void)memset(&decoded, 0, sizeof(decoded));
    CHECK_TRUE(
        guardian_node_link_decode(bytes, size, &decoded) ==
        GUARDIAN_NODE_LINK_OK);

    CHECK_TRUE(decoded.message_type == input.message_type);
    CHECK_TRUE(decoded.node_state == input.node_state);
    CHECK_TRUE(decoded.sequence == input.sequence);
    CHECK_TRUE(decoded.sender_epoch == input.sender_epoch);
    CHECK_TRUE(decoded.sender_node_id == input.sender_node_id);
    CHECK_TRUE(decoded.payload_length == input.payload_length);
    CHECK_TRUE(memcmp(decoded.payload, input.payload, input.payload_length) == 0);
}

static void test_crc_corruption(void)
{
    guardian_node_link_frame_t frame = make_frame();
    guardian_node_link_frame_t decoded;
    uint8_t bytes[GUARDIAN_NODE_LINK_MAX_FRAME];
    size_t size = 0U;

    CHECK_TRUE(
        guardian_node_link_encode(&frame, bytes, sizeof(bytes), &size) ==
        GUARDIAN_NODE_LINK_OK);

    bytes[20] ^= 0x01U;

    CHECK_TRUE(
        guardian_node_link_decode(bytes, size, &decoded) ==
        GUARDIAN_NODE_LINK_ERROR_CRC);
}

static void test_invalid_wire_fields(void)
{
    guardian_node_link_frame_t frame = make_frame();
    guardian_node_link_frame_t decoded;
    uint8_t bytes[GUARDIAN_NODE_LINK_MAX_FRAME];
    uint8_t copy[GUARDIAN_NODE_LINK_MAX_FRAME];
    size_t size = 0U;

    CHECK_TRUE(
        guardian_node_link_encode(&frame, bytes, sizeof(bytes), &size) ==
        GUARDIAN_NODE_LINK_OK);

    (void)memcpy(copy, bytes, size);
    copy[0] = 0U;
    CHECK_TRUE(
        guardian_node_link_decode(copy, size, &decoded) ==
        GUARDIAN_NODE_LINK_ERROR_MAGIC);

    (void)memcpy(copy, bytes, size);
    copy[2] = 2U;
    CHECK_TRUE(
        guardian_node_link_decode(copy, size, &decoded) ==
        GUARDIAN_NODE_LINK_ERROR_VERSION);

    (void)memcpy(copy, bytes, size);
    copy[3] = 0x55U;
    CHECK_TRUE(
        guardian_node_link_decode(copy, size, &decoded) ==
        GUARDIAN_NODE_LINK_ERROR_MESSAGE_TYPE);

    (void)memcpy(copy, bytes, size);
    copy[4] = 1U;
    CHECK_TRUE(
        guardian_node_link_decode(copy, size, &decoded) ==
        GUARDIAN_NODE_LINK_ERROR_FLAGS);

    (void)memcpy(copy, bytes, size);
    copy[5] = 0x40U;
    CHECK_TRUE(
        guardian_node_link_decode(copy, size, &decoded) ==
        GUARDIAN_NODE_LINK_ERROR_STATE);

    CHECK_TRUE(
        guardian_node_link_decode(bytes, size - 1U, &decoded) ==
        GUARDIAN_NODE_LINK_ERROR_LENGTH_MISMATCH);
}

static void test_sequence_guard(void)
{
    guardian_node_link_sequence_guard_t guard;

    guard.last_sequence = 0U;
    guard.initialized = 0U;

    CHECK_TRUE(
        guardian_node_link_accept_sequence(&guard, 1U) ==
        GUARDIAN_NODE_LINK_OK);

    CHECK_TRUE(
        guardian_node_link_accept_sequence(&guard, 1U) ==
        GUARDIAN_NODE_LINK_ERROR_REPLAY_OR_REGRESSION);

    CHECK_TRUE(
        guardian_node_link_accept_sequence(&guard, 0U) ==
        GUARDIAN_NODE_LINK_ERROR_SEQUENCE_ZERO);

    CHECK_TRUE(
        guardian_node_link_accept_sequence(&guard, 2U) ==
        GUARDIAN_NODE_LINK_OK);

    CHECK_TRUE(
        guardian_node_link_accept_sequence(&guard, 1U) ==
        GUARDIAN_NODE_LINK_ERROR_REPLAY_OR_REGRESSION);
}

static void test_encode_rejections(void)
{
    guardian_node_link_frame_t frame = make_frame();
    uint8_t bytes[GUARDIAN_NODE_LINK_MAX_FRAME];
    size_t size = 0U;

    frame.sequence = 0U;
    CHECK_TRUE(
        guardian_node_link_encode(&frame, bytes, sizeof(bytes), &size) ==
        GUARDIAN_NODE_LINK_ERROR_SEQUENCE_ZERO);

    frame = make_frame();
    frame.flags = 1U;
    CHECK_TRUE(
        guardian_node_link_encode(&frame, bytes, sizeof(bytes), &size) ==
        GUARDIAN_NODE_LINK_ERROR_FLAGS);

    frame = make_frame();
    frame.payload_length = (uint16_t)(GUARDIAN_NODE_LINK_MAX_PAYLOAD + 1U);
    CHECK_TRUE(
        guardian_node_link_encode(&frame, bytes, sizeof(bytes), &size) ==
        GUARDIAN_NODE_LINK_ERROR_PAYLOAD_TOO_LARGE);
}

int main(void)
{
    test_round_trip();
    test_crc_corruption();
    test_invalid_wire_fields();
    test_sequence_guard();
    test_encode_rejections();

    if (failures != 0)
    {
        (void)printf("NODE_LINK_HOST_TEST=FAIL failures=%d\n", failures);
        return 1;
    }

    (void)printf("NODE_LINK_HOST_TEST=PASS\n");
    return 0;
}
