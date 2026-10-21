#ifndef GUARDIAN_SIGNED_ENVELOPE_H
#define GUARDIAN_SIGNED_ENVELOPE_H
#include <stddef.h>
#include <stdint.h>
#define GSE_HEADER ((size_t)32U)
#define GSE_SIGNATURE ((size_t)64U)
#define GSE_MAX_PAYLOAD ((size_t)64U)
#define GSE_MAX_WIRE ((size_t)160U)
#define GSE_DOMAIN "GUARDIAN-F401:NODELINK:SIGNED-ENVELOPE:V2"
#define GSE_DOMAIN_SIZE (sizeof(GSE_DOMAIN)-1U)
#define GSE_MAX_TRANSCRIPT (GSE_DOMAIN_SIZE+GSE_HEADER+GSE_MAX_PAYLOAD)
/* Structurally parsed UNVERIFIED input, never an authenticated capability. */
typedef struct {
    uint8_t type, state;
    uint16_t payload_length;
    uint32_t sender, producer, key, epoch, sequence;
    uint8_t payload[GSE_MAX_PAYLOAD], signature[GSE_SIGNATURE];
} gse_unverified;
typedef enum { GSE_OK=0, GSE_INVALID, GSE_CAPACITY } gse_result;
/* Failure leaves bytes/output struct unchanged; reported length is zero.
 * Input/output data may overlap. Length pointer must not alias input/output.
 * No authentication, freshness, truth or actuator authority is established. */
gse_result gse_encode(const gse_unverified *input,uint8_t *wire,size_t capacity,size_t *length);
gse_result gse_decode(const uint8_t *wire,size_t length,gse_unverified *output);
gse_result gse_transcript(const gse_unverified *input,uint8_t *bytes,size_t capacity,size_t *length);
#endif
