#ifndef GUARDIAN_NODE_LINK_FRESHNESS_H
#define GUARDIAN_NODE_LINK_FRESHNESS_H

#include "guardian_node_link_semantic.h"

#include <stdint.h>

/*
 * C5-R3A runtime freshness state model.
 *
 * This module deliberately does not establish freshness.
 *
 * It materializes the bounded state required by later C5-R3 phases while
 * preserving the following architectural separations:
 *
 * AUTHENTICATED != FRESH
 * SIGNED_EPOCH != CURRENT_EPOCH
 * OBSERVED_EPOCH != ACCEPTED_EPOCH
 * OBSERVED_SEQUENCE != ACCEPTED_SEQUENCE
 * SEQUENCE_MONOTONICITY != PERSISTENT_FRESHNESS
 * SESSION_SEQUENCE != CROSS_REBOOT_ANTI_REPLAY
 * FRESH != AUTHORIZED
 * FRESH != ACTUATION_AUTHORIZED
 *
 * C5-R3A does not implement:
 *
 * - epoch acceptance;
 * - replay evaluation;
 * - sequence acceptance;
 * - persistent freshness;
 * - persistence-provider behavior;
 * - rollback detection;
 * - rejoin;
 * - NodeSupervisor consumption;
 * - authority;
 * - actuation.
 */

/*
 * Freshness/security conditions are deliberately distinct from the Guardian
 * operational node-state vocabulary.
 *
 * These values SHALL NOT be interpreted as new Guardian operational states.
 */
typedef enum
{
    /*
     * No accepted freshness generation has been established.
     */
    GUARDIAN_NODE_LINK_FRESHNESS_UNINITIALIZED = 0,

    /*
     * Represents a future successfully established freshness generation.
     *
     * C5-R3A intentionally provides no API that promotes a runtime object
     * into ACTIVE. That transition belongs to later governed R3 evaluation.
     */
    GUARDIAN_NODE_LINK_FRESHNESS_ACTIVE,

    /*
     * Required freshness state cannot currently be trusted.
     */
    GUARDIAN_NODE_LINK_FRESHNESS_UNKNOWN,

    /*
     * Normal freshness continuation is prohibited until a separately
     * governed rejoin procedure succeeds.
     */
    GUARDIAN_NODE_LINK_FRESHNESS_REJOIN_REQUIRED,

    /*
     * The current sequence generation cannot continue and requires an
     * accepted epoch transition or equivalent separately governed boundary.
     */
    GUARDIAN_NODE_LINK_FRESHNESS_SESSION_REPLACEMENT_REQUIRED
} guardian_node_link_freshness_condition_t;

/*
 * Bounded R3A API results.
 *
 * These results describe only manipulation or validation of the runtime state
 * model. GUARDIAN_NODE_LINK_FRESHNESS_OK does not mean that any peer message
 * has been proven fresh.
 */
typedef enum
{
    GUARDIAN_NODE_LINK_FRESHNESS_OK = 0,

    GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT,

    GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_CONDITION,

    GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE
} guardian_node_link_freshness_result_t;

/*
 * Exact authenticated + pre-freshness-compatible identity context that later
 * R3 evaluation must preserve.
 *
 * R3A defines the storage model only. It does not dynamically bind this
 * identity from an incoming message and does not establish trust from it.
 */
typedef struct
{
    uint32_t sender_node_id;
    uint32_t producer_id;
    uint32_t key_id;
    uint8_t signature_algorithm;

    char producer_semantic_profile_id[
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY];

    char consumer_semantic_profile_id[
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY];

    char compatibility_contract_id[
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY];
} guardian_node_link_freshness_identity_t;

/*
 * Runtime state for one accepted freshness generation.
 *
 * C5-R3 session-model adjudication:
 *
 * ONE_ACCEPTED_EPOCH = ONE_SEQUENCE_GENERATION
 *
 * A same-epoch sequence reset is not a new session and must fail closed in
 * the future evaluator.
 *
 * A new observed authenticated epoch is not automatically accepted.
 */
typedef struct
{
    guardian_node_link_freshness_condition_t condition;

    guardian_node_link_freshness_identity_t identity;

    /*
     * Observed values are peer-originated authenticated observations.
     *
     * They are intentionally stored separately from accepted values.
     */
    uint32_t observed_epoch;
    uint32_t observed_sequence;

    /*
     * Accepted values represent state that a later governed R3 evaluator has
     * explicitly accepted.
     *
     * R3A itself never advances these values.
     */
    uint32_t accepted_epoch;
    uint32_t accepted_sequence;

    /*
     * Explicit validity flags prevent zero-initialized storage from being
     * silently interpreted as established freshness state.
     */
    uint8_t identity_valid;
    uint8_t observation_valid;
    uint8_t accepted_epoch_valid;
    uint8_t accepted_sequence_valid;
} guardian_node_link_freshness_runtime_t;

/*
 * Initialize runtime state to a deterministic fail-closed baseline.
 *
 * After initialization:
 *
 * condition               = UNINITIALIZED
 * identity_valid          = 0
 * observation_valid       = 0
 * accepted_epoch_valid    = 0
 * accepted_sequence_valid = 0
 *
 * No peer state is accepted by this operation.
 */
void guardian_node_link_freshness_runtime_init(
    guardian_node_link_freshness_runtime_t *runtime);

/*
 * Validate only the structural invariants of an R3A runtime object.
 *
 * This function does not evaluate peer freshness and cannot promote the
 * runtime state.
 */
guardian_node_link_freshness_result_t
guardian_node_link_freshness_runtime_validate(
    const guardian_node_link_freshness_runtime_t *runtime);

#endif