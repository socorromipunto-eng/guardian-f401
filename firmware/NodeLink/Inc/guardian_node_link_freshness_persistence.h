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
 * C5-R3C-C logical persistence provider-operation contract.
 *
 * These declarations define a platform-independent transactional operation
 * surface only.
 *
 * They do not define canonical serialization, physical storage encoding,
 * flash behavior, durability proof, integrity protection, rollback-resistant
 * anchoring, rejoin, freshness establishment, authority, or actuation.
 *
 * WRITE_SUCCESS != COMMIT_SUCCESS
 * COMMIT_SUCCESS != FRESHNESS
 * PROVIDER_RESULT != SECURITY_TRUTH
 * CANDIDATE_RECORD != COMMITTED_RECORD
 * RECORD_GENERATION != STRONG_ROLLBACK_ANCHOR
 * TRANSACTIONAL_ORDERING != DURABILITY_PROOF
 * PERSISTENCE_PROVIDER != AUTHORITY_PROVIDER
 */

/*
 * Bounded result vocabulary for one provider operation.
 *
 * INVALID is zero deliberately so zero-initialized and unknown results fail
 * closed.
 */
typedef enum
{
    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID = 0,

    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK,

    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_UNAVAILABLE,

    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_IO_FAILURE,

    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_VERIFY_FAILURE,

    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_COMMIT_FAILURE,

    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN,

    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_GENERATION_EXHAUSTED
} guardian_node_link_freshness_persistence_operation_result_t;

/*
 * Bounded logical transaction outcome.
 *
 * COMMITTED means only that the provider contract completed the logical
 * write -> verify -> commit operation sequence.
 *
 * It does not prove physical atomicity or durability and does not establish
 * freshness, authority, or actuation authority.
 */
typedef enum
{
    GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_INVALID = 0,

    GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_COMMITTED,

    GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_ABORTED,

    GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_STATE_UNCERTAIN,

    GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_GENERATION_EXHAUSTED
} guardian_node_link_freshness_persistence_transaction_outcome_t;

/*
 * Provider callbacks.
 *
 * context is opaque and may be NULL for a stateless provider.
 *
 * The callback surface does not receive Guardian freshness runtime state and
 * therefore cannot directly promote freshness, authority, or actuation.
 *
 * load is declared as part of the complete R3C-C provider contract even
 * though the bounded commit transaction below does not invoke it.
 */
typedef guardian_node_link_freshness_persistence_operation_result_t
(*guardian_node_link_freshness_persistence_load_fn)(
    void *context,
    guardian_node_link_freshness_persistence_status_t *provider_status,
    guardian_node_link_freshness_persisted_record_t *record);

typedef guardian_node_link_freshness_persistence_operation_result_t
(*guardian_node_link_freshness_persistence_write_candidate_fn)(
    void *context,
    const guardian_node_link_freshness_persisted_record_t *candidate);

typedef guardian_node_link_freshness_persistence_operation_result_t
(*guardian_node_link_freshness_persistence_verify_candidate_fn)(
    void *context,
    const guardian_node_link_freshness_persisted_record_t *candidate);

typedef guardian_node_link_freshness_persistence_operation_result_t
(*guardian_node_link_freshness_persistence_commit_candidate_fn)(
    void *context,
    const guardian_node_link_freshness_persisted_record_t *candidate);

typedef struct
{
    void *context;

    guardian_node_link_freshness_persistence_load_fn load;

    guardian_node_link_freshness_persistence_write_candidate_fn
        write_candidate;

    guardian_node_link_freshness_persistence_verify_candidate_fn
        verify_candidate;

    guardian_node_link_freshness_persistence_commit_candidate_fn
        commit_candidate;
} guardian_node_link_freshness_persistence_provider_t;

/*
 * Execute one bounded logical persistence commit transaction.
 *
 * previous_record may be NULL only for the legitimate no-prior-state
 * bootstrap case.
 *
 * Generation semantics:
 *
 * - no previous record -> candidate generation must be 0;
 * - previous generation N -> candidate generation must be N + 1;
 * - UINT32_MAX never wraps and fails closed.
 *
 * Both previous_record, when present, and candidate must independently pass
 * the existing C5-R3C-B logical record validation against expected_identity.
 *
 * The sequence is:
 *
 * validate previous/candidate
 * -> write candidate
 * -> provider verify candidate
 * -> commit candidate
 *
 * Any failure before successful commit prevents this function from reporting
 * COMMITTED.
 *
 * A provider result outside the bounded vocabulary fails closed as
 * OPERATION_INVALID with transaction STATE_UNCERTAIN.
 *
 * This function does not mutate Guardian freshness runtime state.
 */
guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_freshness_persistence_transact_commit(
    const guardian_node_link_freshness_persistence_provider_t *provider,
    const guardian_node_link_freshness_persisted_record_t *previous_record,
    const guardian_node_link_freshness_identity_t *expected_identity,
    const guardian_node_link_freshness_persisted_record_t *candidate,
    guardian_node_link_freshness_persistence_transaction_outcome_t *outcome);

#endif
