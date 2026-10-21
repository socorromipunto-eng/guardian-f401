#ifndef GUARDIAN_CORROBORATION_WIRE_H
#define GUARDIAN_CORROBORATION_WIRE_H
#include <stddef.h>
#include <stdint.h>
#define GC_WIRE_SIZE ((size_t)52u)
#define GC_WIRE_VERSION ((uint8_t)1u)
#define GC_WIRE_COMMITMENT_SIZE ((size_t)32u)
/* Research payload only. No authentication/freshness/ACK/authority proof. */
typedef struct {
    uint32_t generation, policy, profile, round;
    uint8_t claims_actuation;
    uint8_t commitment[GC_WIRE_COMMITMENT_SIZE];
} gc_wire_observation;
typedef enum { GC_WIRE_OK=0, GC_WIRE_INVALID, GC_WIRE_CAPACITY } gc_wire_result;
/* Valid accessible buffers are required. output_length must not alias input/output.
 * No output bytes on failure; non-NULL output_length is set to zero first. */
gc_wire_result gc_wire_encode(const gc_wire_observation *observation,
                             uint8_t *output, size_t capacity,
                             size_t *output_length);
/* Exact length; output observation remains unchanged on failure. */
gc_wire_result gc_wire_decode(const uint8_t *input, size_t length,
                             gc_wire_observation *observation);
#endif
