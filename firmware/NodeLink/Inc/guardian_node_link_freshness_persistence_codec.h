#ifndef GUARDIAN_NODE_LINK_FRESHNESS_PERSISTENCE_CODEC_H
#define GUARDIAN_NODE_LINK_FRESHNESS_PERSISTENCE_CODEC_H

#include "guardian_node_link_freshness_persistence.h"

#include <stddef.h>
#include <stdint.h>

/*
 * C5-R3C-D canonical persistence serialization contract.
 *
 * This codec converts the bounded logical R3C persistence record to and from
 * one deterministic byte representation.
 *
 * It does not implement physical storage, flash allocation, durable commit,
 * power-loss recovery, authenticated storage, rollback-resistant anchoring,
 * rejoin, freshness establishment, authority, or actuation.
 *
 * IN_MEMORY_RECORD != SERIALIZED_RECORD
 * sizeof(struct) != GOVERNED_STORAGE_SCHEMA
 * INTEGRITY_OK != TRUSTED_STORAGE
 * INTEGRITY_OK != ROLLBACK_RESISTANCE
 * DECODE_OK != FRESH
 * DECODE_OK != AUTHORIZED
 * DECODE_OK != ACTUATION_AUTHORIZED
 */

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZATION_VERSION \
    ((uint16_t)1U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_HEADER_LENGTH \
    ((uint16_t)28U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INTEGRITY_MATERIAL_LENGTH \
    ((uint32_t)332U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZED_SIZE \
    ((size_t)364U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INTEGRITY_SIZE \
    ((size_t)32U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_MAGIC_OFFSET \
    ((size_t)0U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZATION_VERSION_OFFSET \
    ((size_t)4U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_HEADER_LENGTH_OFFSET \
    ((size_t)6U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_LOGICAL_SCHEMA_OFFSET \
    ((size_t)8U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SENDER_NODE_ID_OFFSET \
    ((size_t)12U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_PRODUCER_ID_OFFSET \
    ((size_t)16U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_KEY_ID_OFFSET \
    ((size_t)20U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SIGNATURE_ALGORITHM_OFFSET \
    ((size_t)24U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_RESERVED_OFFSET \
    ((size_t)25U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_PRODUCER_PROFILE_OFFSET \
    ((size_t)28U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_CONSUMER_PROFILE_OFFSET \
    ((size_t)124U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_COMPATIBILITY_CONTRACT_OFFSET \
    ((size_t)220U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_ACCEPTED_EPOCH_OFFSET \
    ((size_t)316U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_ACCEPTED_SEQUENCE_OFFSET \
    ((size_t)320U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_RECORD_GENERATION_OFFSET \
    ((size_t)324U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_MATERIAL_LENGTH_OFFSET \
    ((size_t)328U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INTEGRITY_OFFSET \
    ((size_t)332U)

/*
 * Codec outcome vocabulary.
 *
 * INVALID is deliberately zero so unknown/zero-initialized results fail
 * closed.
 */
typedef enum
{
    GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INVALID = 0,

    GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_OK,

    GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_BAD_ARGUMENT,

    GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_BUFFER_TOO_SMALL,

    GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_BAD_MAGIC,

    GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_UNSUPPORTED_VERSION,

    GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_BAD_LENGTH,

    GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_RESERVED_NONZERO,

    GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INVALID_FIELD,

    GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_INTEGRITY_FAILURE,

    GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_TRAILING_DATA
} guardian_node_link_freshness_persistence_codec_result_t;

/*
 * Encode one logical persisted record into the exact R3C-D v1 canonical
 * representation.
 *
 * Integer fields use big-endian byte order.
 *
 * Semantic references remain the already governed bounded C-string values.
 * The bytes before the first NUL are preserved exactly; the remainder of each
 * fixed 96-byte serialized field is canonical zero fill.
 *
 * No character-set normalization, case folding, or semantic rewriting occurs.
 *
 * output_written is set to zero before fail-closed return whenever it exists.
 */
guardian_node_link_freshness_persistence_codec_result_t
guardian_node_link_freshness_persistence_encode(
    const guardian_node_link_freshness_persisted_record_t *record,
    uint8_t *output,
    size_t output_capacity,
    size_t *output_written);

/*
 * Decode and integrity-check one exact R3C-D v1 canonical representation.
 *
 * The destination is zeroed before fail-closed return whenever it exists.
 * Parsed fields are first held in a local candidate and are copied to output
 * only after:
 *
 * - fixed framing validation;
 * - exact-length validation;
 * - reserved-byte validation;
 * - SHA-256 integrity validation;
 * - canonical semantic-reference validation; and
 * - existing R3C-B logical-record validation.
 *
 * CODEC_OK establishes only canonical structural/integrity validity.
 * It does not establish provider trust, physical durability, rollback safety,
 * freshness, authority, or actuation authority.
 */
guardian_node_link_freshness_persistence_codec_result_t
guardian_node_link_freshness_persistence_decode(
    const uint8_t *input,
    size_t input_length,
    guardian_node_link_freshness_persisted_record_t *record);

#endif