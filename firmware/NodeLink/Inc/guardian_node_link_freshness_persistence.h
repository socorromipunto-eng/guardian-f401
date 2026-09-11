#ifndef GUARDIAN_NODE_LINK_FRESHNESS_PERSISTENCE_H
#define GUARDIAN_NODE_LINK_FRESHNESS_PERSISTENCE_H

#include "guardian_node_link_freshness.h"

#include <stdint.h>

/*
 * C5-R3C-A persistent freshness record and provider-status contract.
 *
 * This header defines only the platform-independent persisted-state schema
 * and bounded provider classification vocabulary.
 *
 * It does not implement persistence I/O, flash storage, secure storage,
 * transactional commit, integrity protection, rollback detection, rejoin,
 * epoch establishment, authority, or actuation.
 *
 * Mandatory separations:
 *
 * API_CALL_SUCCESS != SECURITY_STATUS
 * RESULT_OK != VALID_PERSISTED_STATE
 * RECORD_GENERATION != STRONG_ROLLBACK_ANCHOR
 * CRC != ROLLBACK_PROTECTION
 * HASH != ROLLBACK_PROTECTION
 * PERSISTED_RECORD != FRESHNESS
 * FRESHNESS != AUTHORIZED
 * AUTHORIZED != ACTUATION_AUTHORIZED
 */

/*
 * Initial governed schema version.
 *
 * Unknown schema versions must fail closed when record validation is
 * implemented in C5-R3C-B.
 */
#define GUARDIAN_NODE_LINK_FRESHNESS_PERSISTENCE_SCHEMA_VERSION 1U

/*
 * Provider/security classification.
 *
 * INVALID is zero deliberately so zero-initialized status cannot be
 * interpreted as trusted persisted state.
 *
 * Only VALID_PERSISTED_STATE may become eligible for normal continuation
 * after the independent record-validation rules of C5-R3C-B succeed.
 */
typedef enum
{
    GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_INVALID = 0,

    GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE,

    GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_NO_PERSISTED_STATE,

    GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_CORRUPTED_STATE,

    GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_TORN_OR_INCOMPLETE_UPDATE,

    GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_ROLLBACK_SUSPECTED,

    GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_UNAVAILABLE_STATE
} guardian_node_link_freshness_persistence_status_t;

/*
 * Persisted identity and accepted freshness generation.
 *
 * The identity is the exact authenticated + pre-freshness-compatible identity
 * already modeled by R3A/R3B:
 *
 * - sender_node_id;
 * - producer_id;
 * - key_id;
 * - signature_algorithm;
 * - producer semantic profile;
 * - consumer semantic profile;
 * - compatibility contract.
 *
 * observed_epoch and observed_sequence are intentionally absent.
 * Peer observations are not persisted as accepted authority.
 *
 * accepted_epoch and accepted_sequence must be non-zero for a record to be
 * classified as structurally valid by the future C5-R3C-B validator.
 *
 * record_generation is ordering metadata only. It is not a strong monotonic
 * counter and is not proof of rollback resistance.
 *
 * Integrity-metadata representation is intentionally not selected in R3C-A.
 * The physical/provider-specific mechanism must be separately governed before
 * VALID_PERSISTED_STATE can imply an integrity-validated durable record.
 *
 * This type is a logical in-memory persistence record model.
 *
 * Its native C object representation is NOT a canonical serialized storage
 * format and MUST NOT be persisted, hashed, authenticated, compared, or
 * restored by copying sizeof(struct) raw bytes.
 *
 * Canonical serialization, field encoding, byte order, padding exclusion,
 * integrity metadata, and durable-storage representation are deferred to a
 * separately governed R3C boundary.
 *
 * C_STRUCT_LAYOUT != CANONICAL_PERSISTENCE_FORMAT
 * IN_MEMORY_RECORD != SERIALIZED_RECORD
 * sizeof(struct) != GOVERNED_STORAGE_SCHEMA
 */
typedef struct
{
    uint32_t schema_version;

    guardian_node_link_freshness_identity_t identity;

    uint32_t accepted_epoch;
    uint32_t accepted_sequence;

    uint32_t record_generation;
} guardian_node_link_freshness_persisted_record_t;

/*
 * R3C-A does not declare storage callbacks.
 *
 * Transactional provider operations belong to C5-R3C-C after the record
 * validation/restoration classification work of C5-R3C-B.
 *
 * This prevents a generic read/write callback from being mistaken for
 * demonstrated atomicity, durability, integrity, or rollback protection.
 */

#endif
