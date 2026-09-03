#include "guardian_f401_nodelink_adapter.h"

#include <limits.h>
#include <string.h>

static int guardian_f401_nodelink_state_valid(
    guardian_node_state_t state)
{
    return ((uint32_t)state <= (uint32_t)GUARDIAN_NODE_STATE_FAULT);
}

static guardian_f401_nodelink_adapter_result_t guardian_f401_nodelink_send(
    guardian_f401_nodelink_adapter_t *adapter,
    guardian_node_link_message_type_t message_type)
{
    guardian_node_link_frame_t frame;
    uint8_t encoded[GUARDIAN_NODE_LINK_MAX_FRAME];
    size_t encoded_size = 0U;
    guardian_node_link_result_t encode_result;

    if (adapter == NULL)
    {
        return GUARDIAN_F401_NODELINK_ERROR_NULL_ARGUMENT;
    }

    if (adapter->initialized == 0U)
    {
        return GUARDIAN_F401_NODELINK_ERROR_NOT_INITIALIZED;
    }

    if (adapter->transport.send_frame == NULL)
    {
        return GUARDIAN_F401_NODELINK_ERROR_TRANSPORT_NOT_CONFIGURED;
    }

    if (adapter->sequence_exhausted != 0U)
    {
        return GUARDIAN_F401_NODELINK_ERROR_SEQUENCE_EXHAUSTED;
    }

    (void)memset(&frame, 0, sizeof(frame));
    frame.message_type = message_type;
    frame.flags = GUARDIAN_NODE_LINK_SUPPORTED_FLAGS;
    frame.node_state = adapter->local_state;
    frame.payload_length = 0U;
    frame.sequence = adapter->next_sequence;
    frame.sender_epoch = adapter->sender_epoch;
    frame.sender_node_id = adapter->sender_node_id;

    encode_result = guardian_node_link_encode(
        &frame,
        encoded,
        sizeof(encoded),
        &encoded_size);

    if (encode_result != GUARDIAN_NODE_LINK_OK)
    {
        return GUARDIAN_F401_NODELINK_ERROR_ENCODE;
    }

    if (adapter->transport.send_frame(
            adapter->transport.context,
            encoded,
            encoded_size) == 0)
    {
        return GUARDIAN_F401_NODELINK_ERROR_TRANSPORT;
    }

    if (adapter->next_sequence == UINT32_MAX)
    {
        adapter->sequence_exhausted = 1U;
    }
    else
    {
        adapter->next_sequence += 1U;
    }

    return GUARDIAN_F401_NODELINK_OK;
}

guardian_f401_nodelink_adapter_result_t guardian_f401_nodelink_adapter_init(
    guardian_f401_nodelink_adapter_t *adapter,
    uint32_t sender_node_id,
    uint32_t sender_epoch)
{
    if (adapter == NULL)
    {
        return GUARDIAN_F401_NODELINK_ERROR_NULL_ARGUMENT;
    }

    if (sender_node_id == 0U)
    {
        return GUARDIAN_F401_NODELINK_ERROR_NODE_ID_ZERO;
    }

    if (sender_epoch == 0U)
    {
        return GUARDIAN_F401_NODELINK_ERROR_EPOCH_ZERO;
    }

    (void)memset(adapter, 0, sizeof(*adapter));

    adapter->local_state = GUARDIAN_NODE_STATE_BOOT;
    adapter->sender_node_id = sender_node_id;
    adapter->sender_epoch = sender_epoch;
    adapter->next_sequence = 1U;
    adapter->heartbeat_interval_ms =
        GUARDIAN_F401_NODELINK_DEFAULT_HEARTBEAT_MS;
    adapter->initialized = 1U;

    return GUARDIAN_F401_NODELINK_OK;
}

guardian_f401_nodelink_adapter_result_t guardian_f401_nodelink_adapter_set_transport(
    guardian_f401_nodelink_adapter_t *adapter,
    const guardian_f401_nodelink_transport_t *transport)
{
    if ((adapter == NULL) || (transport == NULL))
    {
        return GUARDIAN_F401_NODELINK_ERROR_NULL_ARGUMENT;
    }

    if (adapter->initialized == 0U)
    {
        return GUARDIAN_F401_NODELINK_ERROR_NOT_INITIALIZED;
    }

    if (transport->send_frame == NULL)
    {
        return GUARDIAN_F401_NODELINK_ERROR_TRANSPORT_NOT_CONFIGURED;
    }

    adapter->transport = *transport;
    return GUARDIAN_F401_NODELINK_OK;
}

guardian_f401_nodelink_adapter_result_t guardian_f401_nodelink_adapter_set_state(
    guardian_f401_nodelink_adapter_t *adapter,
    guardian_node_state_t state)
{
    if (adapter == NULL)
    {
        return GUARDIAN_F401_NODELINK_ERROR_NULL_ARGUMENT;
    }

    if (adapter->initialized == 0U)
    {
        return GUARDIAN_F401_NODELINK_ERROR_NOT_INITIALIZED;
    }

    if (!guardian_f401_nodelink_state_valid(state))
    {
        return GUARDIAN_F401_NODELINK_ERROR_INVALID_STATE;
    }

    adapter->local_state = state;
    return GUARDIAN_F401_NODELINK_OK;
}

guardian_f401_nodelink_adapter_result_t guardian_f401_nodelink_adapter_tick_1ms(
    guardian_f401_nodelink_adapter_t *adapter)
{
    if (adapter == NULL)
    {
        return GUARDIAN_F401_NODELINK_ERROR_NULL_ARGUMENT;
    }

    if (adapter->initialized == 0U)
    {
        return GUARDIAN_F401_NODELINK_ERROR_NOT_INITIALIZED;
    }

    if (adapter->elapsed_ms < adapter->heartbeat_interval_ms)
    {
        adapter->elapsed_ms += 1U;
    }

    return GUARDIAN_F401_NODELINK_OK;
}

guardian_f401_nodelink_adapter_result_t guardian_f401_nodelink_adapter_send_hello(
    guardian_f401_nodelink_adapter_t *adapter)
{
    return guardian_f401_nodelink_send(
        adapter,
        GUARDIAN_NODE_LINK_MESSAGE_HELLO);
}

guardian_f401_nodelink_adapter_result_t guardian_f401_nodelink_adapter_poll(
    guardian_f401_nodelink_adapter_t *adapter)
{
    guardian_f401_nodelink_adapter_result_t result;

    if (adapter == NULL)
    {
        return GUARDIAN_F401_NODELINK_ERROR_NULL_ARGUMENT;
    }

    if (adapter->initialized == 0U)
    {
        return GUARDIAN_F401_NODELINK_ERROR_NOT_INITIALIZED;
    }

    if (adapter->elapsed_ms < adapter->heartbeat_interval_ms)
    {
        return GUARDIAN_F401_NODELINK_OK;
    }

    result = guardian_f401_nodelink_send(
        adapter,
        GUARDIAN_NODE_LINK_MESSAGE_HEARTBEAT);

    if (result == GUARDIAN_F401_NODELINK_OK)
    {
        adapter->elapsed_ms = 0U;
    }

    return result;
}
