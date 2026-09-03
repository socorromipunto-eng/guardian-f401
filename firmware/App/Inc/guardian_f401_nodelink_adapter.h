#ifndef GUARDIAN_F401_NODELINK_ADAPTER_H
#define GUARDIAN_F401_NODELINK_ADAPTER_H

#include <stddef.h>
#include <stdint.h>

#include "guardian_node_link.h"

#define GUARDIAN_F401_NODELINK_DEFAULT_HEARTBEAT_MS ((uint32_t)1000U)

typedef int (*guardian_f401_nodelink_send_frame_fn)(
    void *context,
    const uint8_t *frame,
    size_t frame_size);

typedef struct
{
    guardian_f401_nodelink_send_frame_fn send_frame;
    void *context;
} guardian_f401_nodelink_transport_t;

typedef struct
{
    guardian_f401_nodelink_transport_t transport;
    guardian_node_state_t local_state;
    uint32_t sender_node_id;
    uint32_t sender_epoch;
    uint32_t next_sequence;
    uint32_t heartbeat_interval_ms;
    uint32_t elapsed_ms;
    uint8_t initialized;
    uint8_t sequence_exhausted;
} guardian_f401_nodelink_adapter_t;

typedef enum
{
    GUARDIAN_F401_NODELINK_OK = 0,
    GUARDIAN_F401_NODELINK_ERROR_NULL_ARGUMENT,
    GUARDIAN_F401_NODELINK_ERROR_NODE_ID_ZERO,
    GUARDIAN_F401_NODELINK_ERROR_EPOCH_ZERO,
    GUARDIAN_F401_NODELINK_ERROR_NOT_INITIALIZED,
    GUARDIAN_F401_NODELINK_ERROR_INVALID_STATE,
    GUARDIAN_F401_NODELINK_ERROR_TRANSPORT_NOT_CONFIGURED,
    GUARDIAN_F401_NODELINK_ERROR_ENCODE,
    GUARDIAN_F401_NODELINK_ERROR_TRANSPORT,
    GUARDIAN_F401_NODELINK_ERROR_SEQUENCE_EXHAUSTED
} guardian_f401_nodelink_adapter_result_t;

guardian_f401_nodelink_adapter_result_t guardian_f401_nodelink_adapter_init(
    guardian_f401_nodelink_adapter_t *adapter,
    uint32_t sender_node_id,
    uint32_t sender_epoch);

guardian_f401_nodelink_adapter_result_t guardian_f401_nodelink_adapter_set_transport(
    guardian_f401_nodelink_adapter_t *adapter,
    const guardian_f401_nodelink_transport_t *transport);

guardian_f401_nodelink_adapter_result_t guardian_f401_nodelink_adapter_set_state(
    guardian_f401_nodelink_adapter_t *adapter,
    guardian_node_state_t state);

guardian_f401_nodelink_adapter_result_t guardian_f401_nodelink_adapter_tick_1ms(
    guardian_f401_nodelink_adapter_t *adapter);

guardian_f401_nodelink_adapter_result_t guardian_f401_nodelink_adapter_send_hello(
    guardian_f401_nodelink_adapter_t *adapter);

guardian_f401_nodelink_adapter_result_t guardian_f401_nodelink_adapter_poll(
    guardian_f401_nodelink_adapter_t *adapter);

#endif
