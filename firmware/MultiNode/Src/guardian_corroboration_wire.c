#include "guardian_corroboration_wire.h"
#include <string.h>
static void put32(uint8_t *output, uint32_t value)
{
    output[0]=(uint8_t)value;
    output[1]=(uint8_t)(value >> 8u);
    output[2]=(uint8_t)(value >> 16u);
    output[3]=(uint8_t)(value >> 24u);
}
static uint32_t get32(const uint8_t *input)
{
    return (uint32_t)input[0] | ((uint32_t)input[1] << 8u) |
           ((uint32_t)input[2] << 16u) | ((uint32_t)input[3] << 24u);
}
static int valid(const gc_wire_observation *value)
{
    return value->generation != 0u && value->policy != 0u &&
           value->profile != 0u && value->round != 0u &&
           value->claims_actuation <= 1u;
}
gc_wire_result gc_wire_encode(const gc_wire_observation *observation,
                             uint8_t *output, size_t capacity,
                             size_t *output_length)
{
    uint8_t wire[GC_WIRE_SIZE];
    if (output_length == NULL) { return GC_WIRE_INVALID; }
    *output_length=0u;
    if (observation == NULL || output == NULL || !valid(observation)) {
        return GC_WIRE_INVALID;
    }
    if (capacity < GC_WIRE_SIZE) { return GC_WIRE_CAPACITY; }
    memset(wire,0,sizeof(wire));
    wire[0]=GC_WIRE_VERSION;
    wire[1]=observation->claims_actuation;
    put32(wire+4u,observation->generation);
    put32(wire+8u,observation->policy);
    put32(wire+12u,observation->profile);
    put32(wire+16u,observation->round);
    memcpy(wire+20u,observation->commitment,GC_WIRE_COMMITMENT_SIZE);
    memcpy(output,wire,GC_WIRE_SIZE);
    *output_length=GC_WIRE_SIZE;
    return GC_WIRE_OK;
}
gc_wire_result gc_wire_decode(const uint8_t *input, size_t length,
                             gc_wire_observation *observation)
{
    gc_wire_observation candidate;
    if (input == NULL || observation == NULL || length != GC_WIRE_SIZE) {
        return GC_WIRE_INVALID;
    }
    if (input[0] != GC_WIRE_VERSION || input[1] > 1u ||
        input[2] != 0u || input[3] != 0u) { return GC_WIRE_INVALID; }
    memset(&candidate,0,sizeof(candidate));
    candidate.generation=get32(input+4u);
    candidate.policy=get32(input+8u);
    candidate.profile=get32(input+12u);
    candidate.round=get32(input+16u);
    candidate.claims_actuation=input[1];
    memcpy(candidate.commitment,input+20u,GC_WIRE_COMMITMENT_SIZE);
    if (!valid(&candidate)) { return GC_WIRE_INVALID; }
    *observation=candidate;
    return GC_WIRE_OK;
}
