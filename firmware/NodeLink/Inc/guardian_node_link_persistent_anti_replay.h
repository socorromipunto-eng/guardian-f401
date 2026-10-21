#ifndef GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_H
#define GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_H

#include "guardian_node_link_freshness_persistence.h"
#include "guardian_node_link_rollback_anchor.h"

#include <stdint.h>

/*
 * M17 persistent anti-replay continuation coordinator.
 *
 * This layer composes already-governed freshness persistence and rollback
 * anchor contracts. It does not implement a physical rollback anchor,
 * initial anchor provisioning, hardware durability, authority, or actuation.
 *
 * PERSISTED_STATE != FRESH
 * ANCHOR_MATCH != AUTHORIZED
 * FRESH != AUTHORIZED
 * AUTHORIZED != ACTUATION_AUTHORIZED
 */
typedef enum
{
    GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_INVALID = 0,
    GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_OK,
    GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_BOOTSTRAP_REQUIRED,
    GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_FRESHNESS_UNKNOWN,
    GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_ROLLBACK_SUSPECTED,
    GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_STATE_MISMATCH,
    GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_PROVIDER_UNAVAILABLE,
    GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_STATE_UNCERTAIN,
    GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_REPLAY_OR_REGRESSION,
    GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_EPOCH_TRANSITION_REQUIRED,
    GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_GENERATION_EXHAUSTED
} guardian_node_link_persistent_anti_replay_result_t;

typedef struct
{
    guardian_node_link_freshness_persistence_status_t persistence_status;
    guardian_node_link_freshness_persistence_classification_t persistence_classification;
    guardian_node_link_rollback_anchor_status_t anchor_status;
    guardian_node_link_rollback_anchor_compare_t anchor_comparison;
    uint32_t accepted_epoch;
    uint32_t accepted_sequence;
    uint32_t record_generation;
    uint8_t runtime_restored;
    uint8_t persistence_committed;
    uint8_t anchor_advanced;
    uint8_t anchor_readback_verified;
    uint8_t persistent_anti_replay_valid;
    uint8_t actuation_authorized;
} guardian_node_link_persistent_anti_replay_evidence_t;

guardian_node_link_persistent_anti_replay_result_t
guardian_node_link_persistent_anti_replay_recover(
    const guardian_node_link_freshness_persistence_provider_t *persistence_provider,
    const guardian_node_link_rollback_anchor_provider_t *anchor_provider,
    const guardian_node_link_freshness_identity_t *expected_identity,
    guardian_node_link_freshness_runtime_t *runtime,
    guardian_node_link_freshness_persisted_record_t *restored_record,
    guardian_node_link_persistent_anti_replay_evidence_t *evidence);

guardian_node_link_persistent_anti_replay_result_t
guardian_node_link_persistent_anti_replay_commit_continuation(
    const guardian_node_link_freshness_persistence_provider_t *persistence_provider,
    const guardian_node_link_rollback_anchor_provider_t *anchor_provider,
    const guardian_node_link_freshness_identity_t *expected_identity,
    const guardian_node_link_freshness_persisted_record_t *previous_record,
    uint32_t next_epoch,
    uint32_t next_sequence,
    guardian_node_link_freshness_runtime_t *runtime,
    guardian_node_link_freshness_persisted_record_t *committed_record,
    guardian_node_link_persistent_anti_replay_evidence_t *evidence);

guardian_node_link_persistent_anti_replay_result_t
guardian_node_link_persistent_anti_replay_can_actuate(uint8_t *allowed);

#endif