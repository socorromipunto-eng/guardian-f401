#ifndef GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_H
#define GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_H

#include "guardian_node_link_freshness_persistence_codec.h"

#include <stddef.h>
#include <stdint.h>

/*
 * C5-R3D-B abstract rollback-anchor software contract implementation.
 *
 * This module materializes only the platform-independent software semantics
 * already governed by the R3D-A rollback-anchor contract.
 *
 * It does not select or implement a physical rollback-anchor backend.
 * It does not claim rollback resistance or persistent anti-replay.
 * It does not grant NodeSupervisor authority, actuation authority, or AI
 * authority.
 *
 * PERSISTENCE != PERSISTENT_ANTI_REPLAY
 * PROVIDER_OK != ANCHOR_VALID
 * ANCHOR_VALID != FRESH
 * FRESH != AUTHORIZED
 * AUTHORIZED != ACTUATION_AUTHORIZED
 * AI_ADVISORY != AUTHORITY
 * AI_ADVISORY != ACTUATION
 */

#define GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMMITMENT_SIZE \
    ((size_t)32U)

#define GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_DOMAIN_SIZE \
    ((size_t)33U)

#define GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_MATERIAL_LENGTH \
    ((uint32_t)332U)

#define GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PREIMAGE_SIZE \
    ((size_t)369U)

/*
 * Commitment bytes for GUARDIAN_R3D_ANCHOR_COMMITMENT_V1.
 *
 * The commitment is not an authority token.
 */
typedef struct
{
    uint8_t bytes[
        GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMMITMENT_SIZE];
} guardian_node_link_rollback_anchor_commitment_t;

/*
 * Security classification of independently obtained anchor evidence.
 *
 * INVALID is deliberately zero so zero-initialized state fails closed.
 */
typedef enum
{
    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_INVALID = 0,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_UNPROVISIONED,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_INDETERMINATE,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_VALID,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_UNAVAILABLE,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_CORRUPT,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_EXHAUSTED,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_IDENTITY_MISMATCH,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_STATE_MISMATCH,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_ROLLBACK_SUSPECTED
} guardian_node_link_rollback_anchor_status_t;

/*
 * Provider execution result.
 *
 * Provider execution and security classification remain distinct.
 *
 * PROVIDER_OK != ANCHOR_VALID
 * PROVIDER_FAILURE != ANCHOR_UNPROVISIONED
 * PROVIDER_CONFLICT != RETRY_PERMISSION
 */
typedef enum
{
    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID = 0,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_UNAVAILABLE,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_IO_ERROR,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INTEGRITY_ERROR,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_CONFLICT,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_EXHAUSTED,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_UNSUPPORTED
} guardian_node_link_rollback_anchor_provider_result_t;

/*
 * One independently established rollback-anchor record.
 *
 * generation occupies the same governed logical uint32 generation domain as
 * the accepted R3C record_generation.
 */
typedef struct
{
    guardian_node_link_freshness_identity_t identity;

    uint32_t generation;

    guardian_node_link_rollback_anchor_commitment_t commitment;
} guardian_node_link_rollback_anchor_record_t;

/*
 * Candidate-versus-anchor comparison result.
 *
 * This classification is evidence only. It grants no authority.
 */
typedef enum
{
    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_INVALID = 0,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_MATCH,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_BEHIND,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_AHEAD,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_STATE_MISMATCH,

    GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_COMPARE_IDENTITY_MISMATCH
} guardian_node_link_rollback_anchor_compare_t;

typedef guardian_node_link_rollback_anchor_provider_result_t
(*guardian_node_link_rollback_anchor_status_fn)(
    void *context,
    const guardian_node_link_freshness_identity_t *identity,
    guardian_node_link_rollback_anchor_status_t *status);

typedef guardian_node_link_rollback_anchor_provider_result_t
(*guardian_node_link_rollback_anchor_read_fn)(
    void *context,
    const guardian_node_link_freshness_identity_t *identity,
    guardian_node_link_rollback_anchor_record_t *record,
    guardian_node_link_rollback_anchor_status_t *status);

typedef guardian_node_link_rollback_anchor_provider_result_t
(*guardian_node_link_rollback_anchor_advance_fn)(
    void *context,
    const guardian_node_link_freshness_identity_t *identity,
    uint32_t expected_generation,
    uint32_t next_generation,
    const guardian_node_link_rollback_anchor_commitment_t
        *next_commitment);

/*
 * Abstract provider surface.
 *
 * No concrete physical backend is selected here.
 */
typedef struct
{
    void *context;

    guardian_node_link_rollback_anchor_status_fn status;
    guardian_node_link_rollback_anchor_read_fn read;
    guardian_node_link_rollback_anchor_advance_fn advance;
} guardian_node_link_rollback_anchor_provider_t;

/*
 * Derive GUARDIAN_R3D_ANCHOR_COMMITMENT_V1 from one governed R3C-D record.
 *
 * Exact preimage:
 *
 * ASCII("GUARDIAN-R3D-ANCHOR-COMMITMENT-V1")
 * || 00 00 01 4C
 * || R3C-D canonical bytes 0..331
 *
 * R3C-D integrity digest bytes 332..363 are excluded.
 */
guardian_node_link_rollback_anchor_provider_result_t
guardian_node_link_rollback_anchor_commitment_from_record(
    const guardian_node_link_freshness_persisted_record_t *record,
    guardian_node_link_rollback_anchor_commitment_t *commitment);

/*
 * Compare one canonical persisted-state candidate with independently obtained
 * anchor evidence.
 *
 * This function is read-only and does not advance an anchor.
 */
guardian_node_link_rollback_anchor_provider_result_t
guardian_node_link_rollback_anchor_compare(
    const guardian_node_link_rollback_anchor_record_t *anchor,
    const guardian_node_link_freshness_persisted_record_t *candidate,
    guardian_node_link_rollback_anchor_compare_t *comparison);

/*
 * Derive the next ordinary automatic R3D generation.
 *
 * UINT32_MAX - 1 is the last ordinarily accepted generation.
 *
 * Therefore both UINT32_MAX - 1 and UINT32_MAX report EXHAUSTED rather than
 * consuming the terminal generation or wrapping.
 */
guardian_node_link_rollback_anchor_provider_result_t
guardian_node_link_rollback_anchor_next_generation(
    uint32_t current_generation,
    uint32_t *next_generation);

/*
 * Provider wrappers.
 *
 * status and read are read-only.
 *
 * advance enforces exact N -> N + 1 ordinary progression and the governed
 * pre-exhaustion boundary before delegating to the provider.
 */
guardian_node_link_rollback_anchor_provider_result_t
guardian_node_link_rollback_anchor_status(
    const guardian_node_link_rollback_anchor_provider_t *provider,
    const guardian_node_link_freshness_identity_t *identity,
    guardian_node_link_rollback_anchor_status_t *status);

guardian_node_link_rollback_anchor_provider_result_t
guardian_node_link_rollback_anchor_read(
    const guardian_node_link_rollback_anchor_provider_t *provider,
    const guardian_node_link_freshness_identity_t *identity,
    guardian_node_link_rollback_anchor_record_t *record,
    guardian_node_link_rollback_anchor_status_t *status);

guardian_node_link_rollback_anchor_provider_result_t
guardian_node_link_rollback_anchor_advance(
    const guardian_node_link_rollback_anchor_provider_t *provider,
    const guardian_node_link_freshness_identity_t *identity,
    uint32_t expected_generation,
    uint32_t next_generation,
    const guardian_node_link_rollback_anchor_commitment_t
        *next_commitment);

#endif
