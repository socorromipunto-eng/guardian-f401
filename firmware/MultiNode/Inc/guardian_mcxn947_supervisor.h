#ifndef GUARDIAN_MCXN947_SUPERVISOR_H
#define GUARDIAN_MCXN947_SUPERVISOR_H

#include "guardian_multinode.h"

#include <stdint.h>

#define GUARDIAN_MCXN947_DEFAULT_MISSED_HEARTBEAT_LIMIT \
    ((uint32_t)3U)

typedef enum
{
    GUARDIAN_MCXN947_STATE_BOOTSTRAP = 0,

    GUARDIAN_MCXN947_STATE_MONITORING,

    GUARDIAN_MCXN947_STATE_DEGRADED,

    GUARDIAN_MCXN947_STATE_SAFE_HOLD

} guardian_mcxn947_state_t;

typedef enum
{
    GUARDIAN_MCXN947_OK = 0,

    GUARDIAN_MCXN947_ERROR_NULL_ARGUMENT,

    GUARDIAN_MCXN947_ERROR_INVALID_CONFIG,

    GUARDIAN_MCXN947_ERROR_INVALID_NODE,

    GUARDIAN_MCXN947_ERROR_NOT_AUTHENTICATED,

    GUARDIAN_MCXN947_ERROR_NOT_FRESH,

    GUARDIAN_MCXN947_ERROR_SEMANTICALLY_INCOMPATIBLE,

    GUARDIAN_MCXN947_ERROR_REPLAY_OR_REGRESSION,

    GUARDIAN_MCXN947_ERROR_ACTUATION_PROHIBITED,

    GUARDIAN_MCXN947_ERROR_INVALID_HEARTBEAT

} guardian_mcxn947_result_t;

/*
 * Portable supervisor heartbeat input.
 *
 * authentication/freshness/semantic compatibility are deliberately
 * represented separately.
 *
 * AUTHENTICATED != FRESH
 * FRESH != TRUTHFUL
 * SEMANTICALLY_COMPATIBLE != AUTHORIZED
 * SUPERVISION != ACTUATION_AUTHORITY
 *
 * Health and disagreement flags are canonical booleans: only 0 and 1.
 * Malformed flags are rejected without changing supervisor state.
 * Rejected heartbeats do not count as received; the scheduler must continue
 * its independent missed-heartbeat accounting.
 *
 * A later NodeLink adapter shall populate these values only after the
 * corresponding authenticated/freshness/semantic processing stages.
 */
typedef struct
{
    uint32_t sequence;

    uint32_t epoch;

    uint8_t authenticated;

    uint8_t fresh;

    uint8_t semantically_compatible;

    uint8_t peer_healthy;

    uint8_t disagreement;

} guardian_mcxn947_heartbeat_t;

/*
 * Recovery requests are observations/recommendations only.
 *
 * They are never authorization tokens.
 */
typedef struct
{
    uint32_t requesting_node_id;

    uint32_t observed_epoch;

    uint32_t observed_sequence;

    uint8_t requested;

    uint8_t actuation_authorized;

} guardian_mcxn947_recovery_request_t;

typedef struct
{
    guardian_multinode_node_t self;

    guardian_mcxn947_state_t state;

    uint32_t accepted_epoch;

    uint32_t accepted_sequence;

    uint32_t missed_heartbeats;

    uint32_t missed_heartbeat_limit;

    uint8_t heartbeat_initialized;

    uint8_t recovery_requested;

} guardian_mcxn947_supervisor_t;

void guardian_mcxn947_supervisor_init(
    guardian_mcxn947_supervisor_t *supervisor);

guardian_mcxn947_result_t
guardian_mcxn947_supervisor_configure(
    guardian_mcxn947_supervisor_t *supervisor,
    const guardian_multinode_node_t *self,
    uint32_t missed_heartbeat_limit);

guardian_mcxn947_result_t
guardian_mcxn947_supervisor_accept_heartbeat(
    guardian_mcxn947_supervisor_t *supervisor,
    const guardian_mcxn947_heartbeat_t *heartbeat);

guardian_mcxn947_result_t
guardian_mcxn947_supervisor_note_missed_heartbeat(
    guardian_mcxn947_supervisor_t *supervisor);

guardian_mcxn947_result_t
guardian_mcxn947_supervisor_build_recovery_request(
    const guardian_mcxn947_supervisor_t *supervisor,
    guardian_mcxn947_recovery_request_t *request);

guardian_mcxn947_result_t
guardian_mcxn947_supervisor_can_actuate(
    const guardian_mcxn947_supervisor_t *supervisor,
    uint8_t *allowed);

#endif
