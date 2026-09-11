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
 * C5-R3C-B bounded persistence restoration classification.
 *
 * These values describe only the result of platform-independent validation
 * and classification of a provider-reported persistence state.
 *
 * VALIDATED_RECORD_CANDIDATE means that:
 *
 * - provider status was VALID_PERSISTED_STATE;
 * - the logical record uses the governed schema version;
 * - the record identity is structurally valid;
 * - the record identity exactly matches the expected freshness identity;
 * - accepted_epoch is non-zero;
 * - accepted_sequence is non-zero.
 *
 * It does NOT mean that the provider is trusted, that durable integrity has
 * been demonstrated, that rollback has been detected or excluded, that the
 * peer is fresh, that rejoin has completed, or that authority exists.
 *
 * INVALID is zero deliberately so zero-initialized output fails closed.
 *
 * VALIDATED_RECORD_CANDIDATE != FRESH
 * VALIDATED_RECORD_CANDIDATE != AUTHORIZED
 * VALIDATED_RECORD_CANDIDATE != ACTUATION_AUTHORIZED
 * PROVIDER_STATUS != RECORD_CONTENT_VALIDATION
 * RECORD_GENERATION != STRONG_ROLLBACK_ANCHOR
 */
typedef enum
{
    GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID = 0,

    GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_VALIDATED_RECORD_CANDIDATE,

    GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_NO_STATE_BOOTSTRAP,

    GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_FRESHNESS_UNKNOWN
} guardian_node_link_freshness_persistence_classification_t;

/*
 * Validate and classify one provider-reported persistence result.
 *
 * expected_identity is required for every classification because persistence
 * state is scoped to one exact governed freshness identity.
 *
 * record is required only for VALID_PERSISTED_STATE.
 *
 * For NO_PERSISTED_STATE and all explicit unusable-state classifications,
 * record payload is ignored and may be NULL.
 *
 * NO_PERSISTED_STATE maps only to NO_STATE_BOOTSTRAP. It does not establish
 * an accepted epoch, accepted sequence, freshness, authority, actuation
 * authority, or rejoin requirement.
 *
 * CORRUPTED_STATE, TORN_OR_INCOMPLETE_UPDATE, ROLLBACK_SUSPECTED, and
 * UNAVAILABLE_STATE map to FRESHNESS_UNKNOWN classification.
 *
 * ROLLBACK_SUSPECTED is consumed as provider classification only.
 * This function does not infer rollback from record_generation.
 *
 * Unknown/INVALID provider status, malformed expected identity, malformed
 * VALID_PERSISTED_STATE records, schema mismatch, exact-identity mismatch,
 * zero accepted epoch, and zero accepted sequence fail closed.
 *
 * The function never mutates Guardian freshness runtime state.
 */
guardian_node_link_freshness_result_t
guardian_node_link_freshness_persistence_classify(
    guardian_node_link_freshness_persistence_status_t provider_status,
    const guardian_node_link_freshness_persisted_record_t *record,
    const guardian_node_link_freshness_identity_t *expected_identity,
    guardian_node_link_freshness_persistence_classification_t
        *classification);

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
