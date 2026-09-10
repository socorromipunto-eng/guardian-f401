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

    GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE,

    GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_POLICY,

    GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_MESSAGE,

    GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_EVALUATION
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
 * C5-R3B bounded freshness decisions.
 *
 * INVALID is deliberately zero so that zero-initialized or invalidated
 * evaluation output can never be interpreted as ACCEPT.
 *
 * These decisions classify session-local freshness only.
 *
 * FRESH != TRUTHFUL
 * FRESH != AUTHORIZED
 * FRESH != ACTUATION_AUTHORIZED
 */
typedef enum
{
    GUARDIAN_NODE_LINK_FRESHNESS_DECISION_INVALID = 0,

    GUARDIAN_NODE_LINK_FRESHNESS_DECISION_ACCEPT,

    GUARDIAN_NODE_LINK_FRESHNESS_DECISION_OBSERVE_ONLY,

    GUARDIAN_NODE_LINK_FRESHNESS_DECISION_REPLAY_DUPLICATE,

    GUARDIAN_NODE_LINK_FRESHNESS_DECISION_REPLAY_REGRESSION,

    GUARDIAN_NODE_LINK_FRESHNESS_DECISION_FORWARD_GAP_EXCEEDED,

    GUARDIAN_NODE_LINK_FRESHNESS_DECISION_EPOCH_TRANSITION_REQUIRED,

    GUARDIAN_NODE_LINK_FRESHNESS_DECISION_SESSION_REPLACEMENT_REQUIRED,

    GUARDIAN_NODE_LINK_FRESHNESS_DECISION_IDENTITY_MISMATCH,

    GUARDIAN_NODE_LINK_FRESHNESS_DECISION_STATE_BLOCKED
} guardian_node_link_freshness_decision_t;

/*
 * Explicit bounded session-local forward progression policy.
 *
 * configured must equal exactly 1.
 * max_forward_gap must be non-zero.
 *
 * R3B does not select a deployment-specific gap value.
 */
typedef struct
{
    uint32_t max_forward_gap;
    uint8_t configured;
} guardian_node_link_freshness_policy_t;

/*
 * R3B evaluation evidence.
 *
 * This object is output only.
 *
 * It is not an authorization token and cannot itself be supplied to a public
 * state-mutation API.
 */
typedef struct
{
    guardian_node_link_freshness_decision_t decision;

    guardian_node_link_freshness_identity_t identity;

    uint32_t observed_epoch;
    uint32_t observed_sequence;

    guardian_node_link_freshness_condition_t prior_condition;

    uint32_t prior_observed_epoch;
    uint32_t prior_observed_sequence;

    uint32_t prior_accepted_epoch;
    uint32_t prior_accepted_sequence;

    uint32_t evaluated_max_forward_gap;

    uint8_t prior_observation_valid;
    uint8_t prior_accepted_epoch_valid;
    uint8_t prior_accepted_sequence_valid;

    /*
     * Set to 1 only when evaluate_and_apply() actually commits an ACCEPT
     * transition to runtime.
     *
     * evaluate() always leaves this value at zero.
     */
    uint8_t transition_applied;
} guardian_node_link_freshness_evaluation_t;
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

/*
 * Evaluate one authenticated + pre-freshness-compatible message against the
 * current runtime state.
 *
 * This function never mutates runtime.
 *
 * On any API/structural error, evaluation is deterministically invalidated
 * when a non-NULL evaluation pointer is supplied.
 */
guardian_node_link_freshness_result_t
guardian_node_link_freshness_evaluate(
    const guardian_node_link_freshness_policy_t *policy,
    const guardian_node_link_freshness_runtime_t *runtime,
    const guardian_node_link_pre_freshness_compatible_message_t *message,
    guardian_node_link_freshness_evaluation_t *evaluation);

/*
 * Evaluate against the CURRENT policy, CURRENT runtime, and original
 * authenticated + pre-freshness-compatible message, then atomically commit
 * only a same-epoch DECISION_ACCEPT transition.
 *
 * Caller-supplied evaluation objects are never consumed as mutation
 * authorization.
 *
 * Non-ACCEPT security decisions leave runtime unchanged.
 *
 * This function does not establish:
 * - initial epoch acceptance;
 * - epoch replacement;
 * - persistent anti-replay;
 * - rollback detection;
 * - rejoin;
 * - authority;
 * - actuation authority.
 */
guardian_node_link_freshness_result_t
guardian_node_link_freshness_evaluate_and_apply(
    const guardian_node_link_freshness_policy_t *policy,
    guardian_node_link_freshness_runtime_t *runtime,
    const guardian_node_link_pre_freshness_compatible_message_t *message,
    guardian_node_link_freshness_evaluation_t *evaluation);
#endif