#ifndef GUARDIAN_NODE_LINK_FRESHNESS_PERSISTENCE_BACKEND_H
#define GUARDIAN_NODE_LINK_FRESHNESS_PERSISTENCE_BACKEND_H

#include "guardian_node_link_freshness_persistence_codec.h"

#include <stddef.h>
#include <stdint.h>

/*
 * C5-R3C-E portable physical-persistence backend contract.
 *
 * This layer materializes the already-governed R3C-C provider transaction
 * over two independent erase domains.
 *
 * It does NOT establish:
 *
 * - STM32 linker reservation;
 * - STM32 HAL integration;
 * - hardware power-loss behavior;
 * - authenticated storage;
 * - rollback-resistant anchoring;
 * - persistent anti-replay;
 * - freshness;
 * - authority;
 * - actuation authority.
 *
 * PHYSICAL_INTEGRITY != AUTHENTICATED_STORAGE
 * DUAL_SLOT_RECOVERY != ROLLBACK_RESISTANCE
 * RECORD_GENERATION != STRONG_ROLLBACK_ANCHOR
 * HOST_FAULT_INJECTION != HARDWARE_POWER_LOSS_EVIDENCE
 */

#define GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_FORMAT_VERSION \
    ((uint16_t)1U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_HEADER_SIZE \
    ((size_t)80U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_PAYLOAD_OFFSET \
    ((size_t)80U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_COMMIT_OFFSET \
    ((size_t)444U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_COMMIT_SIZE \
    ((size_t)4U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_IMAGE_SIZE \
    ((size_t)448U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A_CAPACITY \
    ((size_t)16384U)

#define GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_B_CAPACITY \
    ((size_t)65536U)

typedef enum
{
    GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A = 0,
    GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_B = 1
} guardian_node_link_persistence_slot_t;

typedef enum
{
    GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_INVALID = 0,

    GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_OK,

    GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_UNAVAILABLE,

    GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE
} guardian_node_link_persistence_media_result_t;

typedef guardian_node_link_persistence_media_result_t
(*guardian_node_link_persistence_media_read_fn)(
    void *context,
    guardian_node_link_persistence_slot_t slot,
    size_t offset,
    uint8_t *output,
    size_t length);

typedef guardian_node_link_persistence_media_result_t
(*guardian_node_link_persistence_media_erase_fn)(
    void *context,
    guardian_node_link_persistence_slot_t slot);

typedef guardian_node_link_persistence_media_result_t
(*guardian_node_link_persistence_media_program_fn)(
    void *context,
    guardian_node_link_persistence_slot_t slot,
    size_t offset,
    const uint8_t *data,
    size_t length);

typedef struct
{
    void *context;

    guardian_node_link_persistence_media_read_fn read;
    guardian_node_link_persistence_media_erase_fn erase;
    guardian_node_link_persistence_media_program_fn program;
} guardian_node_link_persistence_media_t;

/*
 * Diagnostic classification of the two physical erase domains.
 *
 * This diagnostic is intentionally richer than the R3C-A provider-status
 * vocabulary. It is evidence/diagnostic information only.
 */
typedef enum
{
    GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_INVALID = 0,

    GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_NO_STATE,

    GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_VALID,

    GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_VALID_WITH_TORN_PEER,

    GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_TORN_ONLY,

    GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_CORRUPTED,

    GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_STATE_UNCERTAIN,

    GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_IO_FAILURE
} guardian_node_link_persistence_recovery_diagnostic_t;

typedef struct
{
    guardian_node_link_persistence_media_t media;

    guardian_node_link_persistence_slot_t pending_slot;

    uint8_t pending_valid;
} guardian_node_link_freshness_physical_backend_t;

/*
 * Initialize one backend over an already-provided physical media adapter.
 *
 * Initialization does not inspect, trust, erase, write, or promote storage.
 */
guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_freshness_physical_backend_init(
    guardian_node_link_freshness_physical_backend_t *backend,
    const guardian_node_link_persistence_media_t *media);

/*
 * Produce the existing R3C-C provider surface backed by this physical layer.
 */
guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_freshness_physical_backend_make_provider(
    guardian_node_link_freshness_physical_backend_t *backend,
    guardian_node_link_freshness_persistence_provider_t *provider);

/*
 * Read-only physical recovery/classification.
 *
 * VALID means a uniquely admissible committed slot was selected.
 *
 * A torn inactive peer may coexist with the prior committed valid slot.
 * A committed-looking corrupted peer does NOT permit automatic fallback,
 * because doing so could silently restore an older accepted state.
 *
 * RECOVERY_VALID != FRESH
 * RECOVERY_VALID != ROLLBACK_SAFE
 */
guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_freshness_physical_backend_recover(
    guardian_node_link_freshness_physical_backend_t *backend,
    guardian_node_link_freshness_persistence_status_t *provider_status,
    guardian_node_link_freshness_persisted_record_t *record,
    guardian_node_link_persistence_recovery_diagnostic_t *diagnostic);

/*
 * R3C-C compatible callback implementations.
 */
guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_freshness_physical_backend_load(
    void *context,
    guardian_node_link_freshness_persistence_status_t *provider_status,
    guardian_node_link_freshness_persisted_record_t *record);

guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_freshness_physical_backend_write_candidate(
    void *context,
    const guardian_node_link_freshness_persisted_record_t *candidate);

guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_freshness_physical_backend_verify_candidate(
    void *context,
    const guardian_node_link_freshness_persisted_record_t *candidate);

guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_freshness_physical_backend_commit_candidate(
    void *context,
    const guardian_node_link_freshness_persisted_record_t *candidate);

#endif