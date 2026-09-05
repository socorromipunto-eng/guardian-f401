#include "guardian_node_link.h"
#include <stddef.h>
#include <stdint.h>
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    guardian_node_link_frame_t frame;
    guardian_node_link_result_t result=guardian_node_link_decode(data,size,&frame);
    if (result==GUARDIAN_NODE_LINK_OK)
    {
        guardian_node_link_sequence_guard_t guard={0U,0U};
        (void)guardian_node_link_accept_sequence(&guard,frame.sequence);
        (void)guardian_node_link_accept_sequence(&guard,frame.sequence);
    }
    return 0;
}
