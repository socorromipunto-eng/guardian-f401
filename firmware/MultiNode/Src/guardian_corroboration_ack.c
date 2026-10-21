#include "guardian_corroboration_ack.h"
#include <string.h>
static int valid(const gc_ack *a) {
    return a != NULL && a->generation != 0u && a->policy != 0u &&
        a->profile != 0u && a->round != 0u && a->observation_epoch != 0u &&
        a->observation_sequence != 0u;
}
static void put32(uint8_t *p, uint32_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8u);
    p[2]=(uint8_t)(v>>16u); p[3]=(uint8_t)(v>>24u);
}
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8u) |
        ((uint32_t)p[2]<<16u) | ((uint32_t)p[3]<<24u);
}
gc_ack_result gc_ack_encode(const gc_ack *a, uint8_t *output,
                           size_t capacity, size_t *length) {
    uint8_t bytes[GC_ACK_SIZE];
    if (length == NULL) { return GC_ACK_INVALID; }
    *length=0u;
    if (!valid(a) || output == NULL) { return GC_ACK_INVALID; }
    if (capacity < GC_ACK_SIZE) { return GC_ACK_CAPACITY; }
    memset(bytes,0,sizeof(bytes)); bytes[0]=GC_ACK_VERSION; bytes[1]=GC_ACK_KIND;
    put32(bytes+4u,a->generation); put32(bytes+8u,a->policy);
    put32(bytes+12u,a->profile); put32(bytes+16u,a->round);
    put32(bytes+20u,a->observation_epoch); put32(bytes+24u,a->observation_sequence);
    memcpy(bytes+28u,a->commitment,32u); memcpy(output,bytes,sizeof(bytes));
    *length=sizeof(bytes); return GC_ACK_OK;
}
gc_ack_result gc_ack_decode(const uint8_t *input, size_t length, gc_ack *a) {
    gc_ack candidate;
    if (input == NULL || a == NULL || length != GC_ACK_SIZE) { return GC_ACK_INVALID; }
    if (input[0] != GC_ACK_VERSION || input[1] != GC_ACK_KIND ||
        input[2] != 0u || input[3] != 0u) { return GC_ACK_INVALID; }
    memset(&candidate,0,sizeof(candidate));
    candidate.generation=get32(input+4u); candidate.policy=get32(input+8u);
    candidate.profile=get32(input+12u); candidate.round=get32(input+16u);
    candidate.observation_epoch=get32(input+20u);
    candidate.observation_sequence=get32(input+24u);
    memcpy(candidate.commitment,input+28u,32u);
    if (!valid(&candidate)) { return GC_ACK_INVALID; }
    *a=candidate; return GC_ACK_OK;
}
gc_ack_result gc_ack_match(const gc_ack *a, const gc_ack *pending) {
    uint8_t diff=0u; size_t i;
    if (!valid(a) || !valid(pending)) { return GC_ACK_INVALID; }
    if (a->generation != pending->generation || a->policy != pending->policy ||
        a->profile != pending->profile || a->round != pending->round ||
        a->observation_epoch != pending->observation_epoch ||
        a->observation_sequence != pending->observation_sequence) { return GC_ACK_MISMATCH; }
    for (i=0u;i<32u;++i) { diff=(uint8_t)(diff | (uint8_t)(a->commitment[i]^pending->commitment[i])); }
    return diff == 0u ? GC_ACK_OK : GC_ACK_MISMATCH;
}
