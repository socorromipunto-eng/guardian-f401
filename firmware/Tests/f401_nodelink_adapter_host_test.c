#include "guardian_f401_nodelink_adapter.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            ++failures; \
            (void)printf("FAIL:%s:%d:%s\n", __FILE__, __LINE__, #expr); \
        } \
    } while (0)

typedef struct
{
    uint8_t frame[GUARDIAN_NODE_LINK_MAX_FRAME];
    size_t frame_size;
    unsigned send_count;
    int accept;
} capture_t;

static int capture_send(
    void *context,
    const uint8_t *frame,
    size_t frame_size)
{
    capture_t *capture = (capture_t *)context;

    if ((capture == NULL) || (frame == NULL))
    {
        return 0;
    }

    if ((frame_size == 0U) ||
        (frame_size > sizeof(capture->frame)))
    {
        return 0;
    }

    if (capture->accept == 0)
    {
        return 0;
    }

    (void)memcpy(capture->frame, frame, frame_size);
    capture->frame_size = frame_size;
    capture->send_count += 1U;
    return 1;
}

static void test_init_and_hello(void)
{
    guardian_f401_nodelink_adapter_t adapter;
    guardian_f401_nodelink_transport_t transport;
    guardian_node_link_frame_t decoded;
    capture_t capture;

    (void)memset(&adapter, 0, sizeof(adapter));
    (void)memset(&capture, 0, sizeof(capture));
    capture.accept = 1;

    CHECK(
        guardian_f401_nodelink_adapter_init(
            &adapter,
            0x0000F401UL,
            1U) == GUARDIAN_F401_NODELINK_OK);

    transport.send_frame = capture_send;
    transport.context = &capture;

    CHECK(
        guardian_f401_nodelink_adapter_set_transport(
            &adapter,
            &transport) == GUARDIAN_F401_NODELINK_OK);

    CHECK(
        guardian_f401_nodelink_adapter_send_hello(
            &adapter) == GUARDIAN_F401_NODELINK_OK);

    CHECK(capture.send_count == 1U);

    CHECK(
        guardian_node_link_decode(
            capture.frame,
            capture.frame_size,
            &decoded) == GUARDIAN_NODE_LINK_OK);

    CHECK(decoded.message_type == GUARDIAN_NODE_LINK_MESSAGE_HELLO);
    CHECK(decoded.node_state == GUARDIAN_NODE_STATE_BOOT);
    CHECK(decoded.sequence == 1U);
    CHECK(decoded.sender_epoch == 1U);
    CHECK(decoded.sender_node_id == 0x0000F401UL);
    CHECK(adapter.next_sequence == 2U);
}

static void test_heartbeat_timing(void)
{
    guardian_f401_nodelink_adapter_t adapter;
    guardian_f401_nodelink_transport_t transport;
    guardian_node_link_frame_t decoded;
    capture_t capture;
    uint32_t i;

    (void)memset(&adapter, 0, sizeof(adapter));
    (void)memset(&capture, 0, sizeof(capture));
    capture.accept = 1;

    CHECK(
        guardian_f401_nodelink_adapter_init(
            &adapter,
            0x0000F401UL,
            7U) == GUARDIAN_F401_NODELINK_OK);

    transport.send_frame = capture_send;
    transport.context = &capture;
    CHECK(
        guardian_f401_nodelink_adapter_set_transport(
            &adapter,
            &transport) == GUARDIAN_F401_NODELINK_OK);

    CHECK(
        guardian_f401_nodelink_adapter_set_state(
            &adapter,
            GUARDIAN_NODE_STATE_ACTIVE) == GUARDIAN_F401_NODELINK_OK);

    for (i = 0U; i < 999U; ++i)
    {
        CHECK(
            guardian_f401_nodelink_adapter_tick_1ms(
                &adapter) == GUARDIAN_F401_NODELINK_OK);
    }

    CHECK(
        guardian_f401_nodelink_adapter_poll(
            &adapter) == GUARDIAN_F401_NODELINK_OK);
    CHECK(capture.send_count == 0U);

    CHECK(
        guardian_f401_nodelink_adapter_tick_1ms(
            &adapter) == GUARDIAN_F401_NODELINK_OK);

    CHECK(
        guardian_f401_nodelink_adapter_poll(
            &adapter) == GUARDIAN_F401_NODELINK_OK);

    CHECK(capture.send_count == 1U);

    CHECK(
        guardian_node_link_decode(
            capture.frame,
            capture.frame_size,
            &decoded) == GUARDIAN_NODE_LINK_OK);

    CHECK(decoded.message_type == GUARDIAN_NODE_LINK_MESSAGE_HEARTBEAT);
    CHECK(decoded.node_state == GUARDIAN_NODE_STATE_ACTIVE);
    CHECK(decoded.sequence == 1U);
    CHECK(decoded.sender_epoch == 7U);
}

static void test_transport_failure_does_not_advance_sequence(void)
{
    guardian_f401_nodelink_adapter_t adapter;
    guardian_f401_nodelink_transport_t transport;
    capture_t capture;
    uint32_t i;

    (void)memset(&adapter, 0, sizeof(adapter));
    (void)memset(&capture, 0, sizeof(capture));

    CHECK(
        guardian_f401_nodelink_adapter_init(
            &adapter,
            0x0000F401UL,
            3U) == GUARDIAN_F401_NODELINK_OK);

    transport.send_frame = capture_send;
    transport.context = &capture;
    CHECK(
        guardian_f401_nodelink_adapter_set_transport(
            &adapter,
            &transport) == GUARDIAN_F401_NODELINK_OK);

    for (i = 0U; i < 1000U; ++i)
    {
        CHECK(
            guardian_f401_nodelink_adapter_tick_1ms(
                &adapter) == GUARDIAN_F401_NODELINK_OK);
    }

    capture.accept = 0;

    CHECK(
        guardian_f401_nodelink_adapter_poll(
            &adapter) == GUARDIAN_F401_NODELINK_ERROR_TRANSPORT);

    CHECK(adapter.next_sequence == 1U);
    CHECK(adapter.elapsed_ms == 1000U);

    capture.accept = 1;

    CHECK(
        guardian_f401_nodelink_adapter_poll(
            &adapter) == GUARDIAN_F401_NODELINK_OK);

    CHECK(adapter.next_sequence == 2U);
    CHECK(adapter.elapsed_ms == 0U);
}

static void test_rejections(void)
{
    guardian_f401_nodelink_adapter_t adapter;
    guardian_f401_nodelink_transport_t transport;

    (void)memset(&adapter, 0, sizeof(adapter));
    (void)memset(&transport, 0, sizeof(transport));

    CHECK(
        guardian_f401_nodelink_adapter_init(
            NULL,
            1U,
            1U) == GUARDIAN_F401_NODELINK_ERROR_NULL_ARGUMENT);

    CHECK(
        guardian_f401_nodelink_adapter_init(
            &adapter,
            0U,
            1U) == GUARDIAN_F401_NODELINK_ERROR_NODE_ID_ZERO);

    CHECK(
        guardian_f401_nodelink_adapter_init(
            &adapter,
            1U,
            0U) == GUARDIAN_F401_NODELINK_ERROR_EPOCH_ZERO);

    (void)memset(&adapter, 0, sizeof(adapter));

    CHECK(
        guardian_f401_nodelink_adapter_poll(
            &adapter) == GUARDIAN_F401_NODELINK_ERROR_NOT_INITIALIZED);

    CHECK(
        guardian_f401_nodelink_adapter_init(
            &adapter,
            1U,
            1U) == GUARDIAN_F401_NODELINK_OK);

    CHECK(
        guardian_f401_nodelink_adapter_set_transport(
            &adapter,
            &transport) ==
        GUARDIAN_F401_NODELINK_ERROR_TRANSPORT_NOT_CONFIGURED);

    CHECK(
        guardian_f401_nodelink_adapter_set_state(
            &adapter,
            (guardian_node_state_t)0x7FU) ==
        GUARDIAN_F401_NODELINK_ERROR_INVALID_STATE);
}

int main(void)
{
    test_init_and_hello();
    test_heartbeat_timing();
    test_transport_failure_does_not_advance_sequence();
    test_rejections();

    if (failures != 0)
    {
        (void)printf(
            "F401_NODELINK_ADAPTER_HOST_TEST=FAIL failures=%d\n",
            failures);
        return 1;
    }

    (void)printf("F401_NODELINK_ADAPTER_HOST_TEST=PASS\n");
    return 0;
}
