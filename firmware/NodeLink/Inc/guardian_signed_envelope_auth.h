#ifndef GUARDIAN_SIGNED_ENVELOPE_AUTH_H
#define GUARDIAN_SIGNED_ENVELOPE_AUTH_H
#include "guardian_signed_envelope.h"
#include <stddef.h>
#include <stdint.h>
#define GSEA_MAX_PEERS ((size_t)8U)
#define GSEA_PUBLIC_KEY_SIZE ((size_t)32U)
#define GSEA_OBSERVATION ((uint8_t)1U)
#define GSEA_ACK ((uint8_t)2U)
#define GSEA_BOTH ((uint8_t)3U)
typedef struct {
    uint32_t sender, producer, key;
    uint8_t public_key[GSEA_PUBLIC_KEY_SIZE];
    uint8_t revoked, allowed_types;
} gsea_peer;
typedef struct {
    gsea_peer peers[GSEA_MAX_PEERS];
    size_t count;
    uint32_t registry_revision;
    uint8_t configured;
} gsea_context;
/* Local signature evidence, not an unforgeable C capability or actuation grant.
 * Semantics, freshness, physical truth, pending ACK and durability are unchecked. */
typedef struct {
    gse_unverified envelope;
    uint32_t registry_revision;
} gsea_signature_evidence;
typedef enum {
    GSEA_OK=0, GSEA_INVALID, GSEA_UNCONFIGURED, GSEA_PEER_UNKNOWN,
    GSEA_KEY_REVOKED, GSEA_TYPE_DENIED, GSEA_SIGNATURE_INVALID
} gsea_result;
void gsea_init(gsea_context *context);
/* Copies a caller-trusted LOCAL peer registry. Failed configuration invalidates
 * the previous registry. Public keys do not come from the received packet.
 * The registry/revocation state is owned by one serialized caller. */
gsea_result gsea_configure(gsea_context *context, const gsea_peer *peers,
                           size_t count, uint32_t registry_revision);
/* Failure leaves output bytes unchanged and never mutates context.
 * Accessible buffers required. output must not overlap context; input wire may
 * overlap output because the complete input is consumed before publication.
 * Caller must revalidate registry revision/revocation at later commit time.
 * Fixed Ed25519/V2 only. No key lookup from firmware-signing authorization. */
gsea_result gsea_verify(const gsea_context *context, const uint8_t *wire,
                        size_t length, gsea_signature_evidence *output);
#endif
