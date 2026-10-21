#ifndef GUARDIAN_CORROBORATION_ACK_H
#define GUARDIAN_CORROBORATION_ACK_H
#include <stddef.h>
#include <stdint.h>
#define GC_ACK_SIZE ((size_t)60u)
#define GC_ACK_VERSION ((uint8_t)1u)
#define GC_ACK_KIND ((uint8_t)0xa1u)
/* Canonical acknowledgement reference only. No authentication or authority. */
typedef struct {
    uint32_t generation, policy, profile, round;
    uint32_t observation_epoch, observation_sequence;
    uint8_t commitment[32];
} gc_ack;
typedef enum { GC_ACK_OK=0, GC_ACK_INVALID, GC_ACK_CAPACITY, GC_ACK_MISMATCH } gc_ack_result;
/* Accessible buffers required; output_length must not alias input/output.
 * Encode may overlap input/output; all input is read before output is written.
 * Failed encode leaves output unchanged and sets output_length to zero.
 * Failed decode leaves ack unchanged. No packed structs are serialized. */
gc_ack_result gc_ack_encode(const gc_ack *ack, uint8_t *output,
                           size_t capacity, size_t *output_length);
gc_ack_result gc_ack_decode(const uint8_t *input, size_t length, gc_ack *ack);
/* Compares every reference field. Does not establish sender identity,
 * authentication, freshness, truthful evidence, quorum or actuation rights.
 * Caller must use a locally recorded pending observation, never wire truth. */
gc_ack_result gc_ack_match(const gc_ack *ack, const gc_ack *pending);
#endif
