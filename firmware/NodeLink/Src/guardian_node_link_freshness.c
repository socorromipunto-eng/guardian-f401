#include "guardian_node_link_freshness.h"
#include "guardian_node_link_freshness_persistence.h"

#include <string.h>

static int guardian_node_link_freshness_ref_valid(
    const char *reference,
    size_t capacity)
{
    size_t length = 0U;

    if ((reference == NULL) || (capacity == 0U))
    {
        return 0;
    }

    while ((length < capacity) &&
           (reference[length] != '\0'))
    {
        length += 1U;
    }

    if ((length == 0U) ||
        (length >= capacity))
    {
        return 0;
    }

    return 1;
}

static int guardian_node_link_freshness_condition_valid(
    guardian_node_link_freshness_condition_t condition)
{
    switch (condition)
    {
        case GUARDIAN_NODE_LINK_FRESHNESS_UNINITIALIZED:
        case GUARDIAN_NODE_LINK_FRESHNESS_ACTIVE:
        case GUARDIAN_NODE_LINK_FRESHNESS_UNKNOWN:
        case GUARDIAN_NODE_LINK_FRESHNESS_REJOIN_REQUIRED:
        case GUARDIAN_NODE_LINK_FRESHNESS_SESSION_REPLACEMENT_REQUIRED:
            return 1;

        default:
            return 0;
    }
}

void guardian_node_link_freshness_runtime_init(
    guardian_node_link_freshness_runtime_t *runtime)
{
    if (runtime == NULL)
    {
        return;
    }

    /*
     * Zero initialization is deliberately fail closed.
     *
     * No identity, observation, accepted epoch, or accepted sequence survives
     * initialization.
     */
    (void)memset(runtime, 0, sizeof(*runtime));

    runtime->condition =
        GUARDIAN_NODE_LINK_FRESHNESS_UNINITIALIZED;
}

guardian_node_link_freshness_result_t
guardian_node_link_freshness_runtime_validate(
    const guardian_node_link_freshness_runtime_t *runtime)
{
    if (runtime == NULL)
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT;
    }

    if (guardian_node_link_freshness_condition_valid(
            runtime->condition) == 0)
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_CONDITION;
    }

    /*
     * Validity indicators are bounded booleans.
     *
     * Values outside {0,1} are malformed state rather than aliases for true.
     */
    if ((runtime->identity_valid > 1U) ||
        (runtime->observation_valid > 1U) ||
        (runtime->accepted_epoch_valid > 1U) ||
        (runtime->accepted_sequence_valid > 1U))
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
    }

    /*
     * Observed or accepted freshness state has meaning only when it remains
     * bound to the authenticated + pre-freshness-compatible peer identity.
     *
     * Freshness state without identity is structurally invalid and must fail
     * closed.
     */
    if (((runtime->observation_valid != 0U) ||
         (runtime->accepted_epoch_valid != 0U) ||
         (runtime->accepted_sequence_valid != 0U)) &&
        (runtime->identity_valid == 0U))
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
    }

    /*
     * identity_valid means the runtime object contains a structurally valid
     * snapshot of the already authenticated and pre-freshness-compatible
     * identity context.
     *
     * This is structural validation only.
     *
     * It does not re-authenticate the peer and does not establish freshness.
     */
    if (runtime->identity_valid != 0U)
    {
        if ((runtime->identity.sender_node_id == 0U) ||
            (runtime->identity.producer_id == 0U) ||
            (runtime->identity.key_id == 0U) ||
            (runtime->identity.signature_algorithm !=
                GUARDIAN_NODE_LINK_SIGNATURE_ED25519))
        {
            return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
        }

        if (guardian_node_link_freshness_ref_valid(
                runtime->identity.producer_semantic_profile_id,
                sizeof(runtime->identity.producer_semantic_profile_id)) == 0)
        {
            return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
        }

        if (guardian_node_link_freshness_ref_valid(
                runtime->identity.consumer_semantic_profile_id,
                sizeof(runtime->identity.consumer_semantic_profile_id)) == 0)
        {
            return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
        }

        if (guardian_node_link_freshness_ref_valid(
                runtime->identity.compatibility_contract_id,
                sizeof(runtime->identity.compatibility_contract_id)) == 0)
        {
            return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
        }
    }

    /*
     * A valid observation must represent a structurally valid NodeLink
     * sequence. NodeLink sequence zero is already invalid at the wire layer
     * and remains invalid here.
     */
    if ((runtime->observation_valid != 0U) &&
        (runtime->observed_sequence == 0U))
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
    }

    /*
     * If accepted sequence state is declared valid, zero may not be treated
     * as an accepted sequence.
     */
    /*
     * An accepted epoch is a security-relevant established generation.
     *
     * R1 does not authenticate sender_epoch zero. R3 therefore also rejects
     * zero when runtime claims that an accepted epoch is valid, preventing a
     * forged or manually constructed typed object from creating a weaker
     * freshness boundary than the authenticated path.
     */
    if ((runtime->accepted_epoch_valid != 0U) &&
        (runtime->accepted_epoch == 0U))
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
    }

    if ((runtime->accepted_sequence_valid != 0U) &&
        (runtime->accepted_sequence == 0U))
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
    }
    /*
     * UNINITIALIZED is a fail-closed baseline.
     *
     * It must not simultaneously claim accepted freshness state.
     */
    if (runtime->condition ==
        GUARDIAN_NODE_LINK_FRESHNESS_UNINITIALIZED)
    {
        if ((runtime->accepted_epoch_valid != 0U) ||
            (runtime->accepted_sequence_valid != 0U))
        {
            return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
        }

        return GUARDIAN_NODE_LINK_FRESHNESS_OK;
    }

    /*
     * An accepted sequence is meaningless without an accepted epoch.
     */
    if ((runtime->accepted_sequence_valid != 0U) &&
        (runtime->accepted_epoch_valid == 0U))
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
    }

    /*
     * ACTIVE is structurally meaningful only when the future governed R3
     * evaluator has established an identity, an accepted epoch, and an
     * accepted non-zero sequence.
     *
     * C5-R3A intentionally contains no API that performs that promotion.
     */
    if (runtime->condition ==
        GUARDIAN_NODE_LINK_FRESHNESS_ACTIVE)
    {
        if ((runtime->identity_valid == 0U) ||
            (runtime->accepted_epoch_valid == 0U) ||
            (runtime->accepted_sequence_valid == 0U) ||
            (runtime->accepted_sequence == 0U))
        {
            return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
        }
    }



    return GUARDIAN_NODE_LINK_FRESHNESS_OK;
}

static int guardian_node_link_freshness_copy_ref(
    char *destination,
    size_t destination_capacity,
    const char *source)
{
    size_t index = 0U;

    if ((destination == NULL) ||
        (source == NULL) ||
        (destination_capacity == 0U))
    {
        return 0;
    }

    if (guardian_node_link_freshness_ref_valid(
            source,
            destination_capacity) == 0)
    {
        return 0;
    }

    (void)memset(destination, 0, destination_capacity);

    while ((index < (destination_capacity - 1U)) &&
           (source[index] != '\0'))
    {
        destination[index] = source[index];
        index += 1U;
    }

    destination[index] = '\0';

    return 1;
}

static int guardian_node_link_freshness_identity_equal(
    const guardian_node_link_freshness_identity_t *left,
    const guardian_node_link_freshness_identity_t *right)
{
    if ((left == NULL) || (right == NULL))
    {
        return 0;
    }

    if ((left->sender_node_id != right->sender_node_id) ||
        (left->producer_id != right->producer_id) ||
        (left->key_id != right->key_id) ||
        (left->signature_algorithm != right->signature_algorithm))
    {
        return 0;
    }

    if (strncmp(
            left->producer_semantic_profile_id,
            right->producer_semantic_profile_id,
            GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY) != 0)
    {
        return 0;
    }

    if (strncmp(
            left->consumer_semantic_profile_id,
            right->consumer_semantic_profile_id,
            GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY) != 0)
    {
        return 0;
    }

    if (strncmp(
            left->compatibility_contract_id,
            right->compatibility_contract_id,
            GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY) != 0)
    {
        return 0;
    }

    return 1;
}

static int guardian_node_link_freshness_identity_structurally_valid(
    const guardian_node_link_freshness_identity_t *identity)
{
    if (identity == NULL)
    {
        return 0;
    }

    if ((identity->sender_node_id == 0U) ||
        (identity->producer_id == 0U) ||
        (identity->key_id == 0U) ||
        (identity->signature_algorithm !=
            GUARDIAN_NODE_LINK_SIGNATURE_ED25519))
    {
        return 0;
    }

    if (guardian_node_link_freshness_ref_valid(
            identity->producer_semantic_profile_id,
            sizeof(identity->producer_semantic_profile_id)) == 0)
    {
        return 0;
    }

    if (guardian_node_link_freshness_ref_valid(
            identity->consumer_semantic_profile_id,
            sizeof(identity->consumer_semantic_profile_id)) == 0)
    {
        return 0;
    }

    if (guardian_node_link_freshness_ref_valid(
            identity->compatibility_contract_id,
            sizeof(identity->compatibility_contract_id)) == 0)
    {
        return 0;
    }

    return 1;
}

guardian_node_link_freshness_result_t
guardian_node_link_freshness_persistence_classify(
    guardian_node_link_freshness_persistence_status_t provider_status,
    const guardian_node_link_freshness_persisted_record_t *record,
    const guardian_node_link_freshness_identity_t *expected_identity,
    guardian_node_link_freshness_persistence_classification_t
        *classification)
{
    if (classification != NULL)
    {
        *classification =
            GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_INVALID;
    }

    if ((expected_identity == NULL) ||
        (classification == NULL))
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT;
    }

    if (guardian_node_link_freshness_identity_structurally_valid(
            expected_identity) == 0)
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
    }

    switch (provider_status)
    {
        case GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_NO_PERSISTED_STATE:
            *classification =
                GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_NO_STATE_BOOTSTRAP;

            return GUARDIAN_NODE_LINK_FRESHNESS_OK;

        case GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_CORRUPTED_STATE:
        case GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_TORN_OR_INCOMPLETE_UPDATE:
        case GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_ROLLBACK_SUSPECTED:
        case GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_UNAVAILABLE_STATE:
            *classification =
                GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_FRESHNESS_UNKNOWN;

            return GUARDIAN_NODE_LINK_FRESHNESS_OK;

        case GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE:
            break;

        case GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_INVALID:
        default:
            return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
    }

    if (record == NULL)
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT;
    }

    if (record->schema_version !=
        GUARDIAN_NODE_LINK_FRESHNESS_PERSISTENCE_SCHEMA_VERSION)
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
    }

    if (guardian_node_link_freshness_identity_structurally_valid(
            &record->identity) == 0)
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
    }

    if (guardian_node_link_freshness_identity_equal(
            &record->identity,
            expected_identity) == 0)
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
    }

    if ((record->accepted_epoch == 0U) ||
        (record->accepted_sequence == 0U))
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
    }

    /*
     * record_generation is deliberately not used as a trust decision.
     *
     * It remains ordering metadata only. Without a separately governed
     * independent rollback anchor, a generation value cannot demonstrate
     * rollback resistance or manufacture ROLLBACK_SUSPECTED.
     */

    *classification =
        GUARDIAN_NODE_LINK_PERSISTENCE_CLASSIFICATION_VALIDATED_RECORD_CANDIDATE;

    return GUARDIAN_NODE_LINK_FRESHNESS_OK;
}

static int guardian_node_link_freshness_identity_from_message(
    const guardian_node_link_pre_freshness_compatible_message_t *message,
    guardian_node_link_freshness_identity_t *identity)
{
    if ((message == NULL) || (identity == NULL))
    {
        return 0;
    }

    if ((message->authenticated_message.frame.sender_node_id == 0U) ||
        (message->authenticated_message.producer_id == 0U) ||
        (message->authenticated_message.key_id == 0U) ||
        (message->authenticated_message.signature_algorithm !=
            GUARDIAN_NODE_LINK_SIGNATURE_ED25519) ||
        (message->authenticated_message.frame.sequence == 0U) ||
        (message->authenticated_message.frame.sender_epoch == 0U))
    {
        return 0;
    }

    if (guardian_node_link_freshness_ref_valid(
            message->producer_semantic_profile_id,
            sizeof(message->producer_semantic_profile_id)) == 0)
    {
        return 0;
    }

    if (guardian_node_link_freshness_ref_valid(
            message->consumer_semantic_profile_id,
            sizeof(message->consumer_semantic_profile_id)) == 0)
    {
        return 0;
    }

    if (guardian_node_link_freshness_ref_valid(
            message->compatibility_contract_id,
            sizeof(message->compatibility_contract_id)) == 0)
    {
        return 0;
    }

    (void)memset(identity, 0, sizeof(*identity));

    identity->sender_node_id =
        message->authenticated_message.frame.sender_node_id;

    identity->producer_id =
        message->authenticated_message.producer_id;

    identity->key_id =
        message->authenticated_message.key_id;

    identity->signature_algorithm =
        message->authenticated_message.signature_algorithm;

    if (guardian_node_link_freshness_copy_ref(
            identity->producer_semantic_profile_id,
            sizeof(identity->producer_semantic_profile_id),
            message->producer_semantic_profile_id) == 0)
    {
        return 0;
    }

    if (guardian_node_link_freshness_copy_ref(
            identity->consumer_semantic_profile_id,
            sizeof(identity->consumer_semantic_profile_id),
            message->consumer_semantic_profile_id) == 0)
    {
        return 0;
    }

    if (guardian_node_link_freshness_copy_ref(
            identity->compatibility_contract_id,
            sizeof(identity->compatibility_contract_id),
            message->compatibility_contract_id) == 0)
    {
        return 0;
    }

    return 1;
}

static void guardian_node_link_freshness_bind_prior_state(
    const guardian_node_link_freshness_runtime_t *runtime,
    guardian_node_link_freshness_evaluation_t *evaluation)
{
    evaluation->prior_condition = runtime->condition;

    evaluation->prior_observation_valid =
        runtime->observation_valid;

    evaluation->prior_observed_epoch =
        runtime->observed_epoch;

    evaluation->prior_observed_sequence =
        runtime->observed_sequence;

    evaluation->prior_accepted_epoch_valid =
        runtime->accepted_epoch_valid;

    evaluation->prior_accepted_sequence_valid =
        runtime->accepted_sequence_valid;

    evaluation->prior_accepted_epoch =
        runtime->accepted_epoch;

    evaluation->prior_accepted_sequence =
        runtime->accepted_sequence;
}

static guardian_node_link_freshness_result_t
guardian_node_link_freshness_evaluate_internal(
    const guardian_node_link_freshness_policy_t *policy,
    const guardian_node_link_freshness_runtime_t *runtime,
    const guardian_node_link_pre_freshness_compatible_message_t *message,
    guardian_node_link_freshness_evaluation_t *evaluation)
{
    guardian_node_link_freshness_identity_t incoming_identity;
    guardian_node_link_freshness_result_t runtime_result;
    uint32_t incoming_epoch;
    uint32_t incoming_sequence;
    uint32_t delta;

    if ((policy == NULL) ||
        (runtime == NULL) ||
        (message == NULL) ||
        (evaluation == NULL))
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT;
    }

    if ((policy->configured != 1U) ||
        (policy->max_forward_gap == 0U))
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_POLICY;
    }

    runtime_result =
        guardian_node_link_freshness_runtime_validate(runtime);

    if (runtime_result != GUARDIAN_NODE_LINK_FRESHNESS_OK)
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
    }

    if (guardian_node_link_freshness_identity_from_message(
            message,
            &incoming_identity) == 0)
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_MESSAGE;
    }

    incoming_epoch =
        message->authenticated_message.frame.sender_epoch;

    incoming_sequence =
        message->authenticated_message.frame.sequence;

    evaluation->identity = incoming_identity;

    evaluation->observed_epoch = incoming_epoch;
    evaluation->observed_sequence = incoming_sequence;

    evaluation->evaluated_max_forward_gap =
        policy->max_forward_gap;

    guardian_node_link_freshness_bind_prior_state(
        runtime,
        evaluation);

    switch (runtime->condition)
    {
        case GUARDIAN_NODE_LINK_FRESHNESS_UNINITIALIZED:
            evaluation->decision =
                GUARDIAN_NODE_LINK_FRESHNESS_DECISION_OBSERVE_ONLY;

            return GUARDIAN_NODE_LINK_FRESHNESS_OK;

        case GUARDIAN_NODE_LINK_FRESHNESS_UNKNOWN:
        case GUARDIAN_NODE_LINK_FRESHNESS_REJOIN_REQUIRED:
        case GUARDIAN_NODE_LINK_FRESHNESS_SESSION_REPLACEMENT_REQUIRED:
            evaluation->decision =
                GUARDIAN_NODE_LINK_FRESHNESS_DECISION_STATE_BLOCKED;

            return GUARDIAN_NODE_LINK_FRESHNESS_OK;

        case GUARDIAN_NODE_LINK_FRESHNESS_ACTIVE:
            break;

        default:
            return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_STATE;
    }

    if (guardian_node_link_freshness_identity_equal(
            &runtime->identity,
            &incoming_identity) == 0)
    {
        evaluation->decision =
            GUARDIAN_NODE_LINK_FRESHNESS_DECISION_IDENTITY_MISMATCH;

        return GUARDIAN_NODE_LINK_FRESHNESS_OK;
    }

    /*
     * Once the accepted sequence reaches UINT32_MAX, that sequence generation
     * is exhausted. Ordinary traffic cannot revive or continue it.
     *
     * This precedence is evaluated before ordinary epoch-transition handling.
     */
    if (runtime->accepted_sequence == UINT32_MAX)
    {
        evaluation->decision =
            GUARDIAN_NODE_LINK_FRESHNESS_DECISION_SESSION_REPLACEMENT_REQUIRED;

        return GUARDIAN_NODE_LINK_FRESHNESS_OK;
    }

    if (incoming_epoch != runtime->accepted_epoch)
    {
        evaluation->decision =
            GUARDIAN_NODE_LINK_FRESHNESS_DECISION_EPOCH_TRANSITION_REQUIRED;

        return GUARDIAN_NODE_LINK_FRESHNESS_OK;
    }

    if (incoming_sequence == runtime->accepted_sequence)
    {
        evaluation->decision =
            GUARDIAN_NODE_LINK_FRESHNESS_DECISION_REPLAY_DUPLICATE;

        return GUARDIAN_NODE_LINK_FRESHNESS_OK;
    }

    if (incoming_sequence < runtime->accepted_sequence)
    {
        evaluation->decision =
            GUARDIAN_NODE_LINK_FRESHNESS_DECISION_REPLAY_REGRESSION;

        return GUARDIAN_NODE_LINK_FRESHNESS_OK;
    }

    delta = incoming_sequence - runtime->accepted_sequence;

    if (delta > policy->max_forward_gap)
    {
        evaluation->decision =
            GUARDIAN_NODE_LINK_FRESHNESS_DECISION_FORWARD_GAP_EXCEEDED;

        return GUARDIAN_NODE_LINK_FRESHNESS_OK;
    }

    evaluation->decision =
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_ACCEPT;

    return GUARDIAN_NODE_LINK_FRESHNESS_OK;
}

guardian_node_link_freshness_result_t
guardian_node_link_freshness_evaluate(
    const guardian_node_link_freshness_policy_t *policy,
    const guardian_node_link_freshness_runtime_t *runtime,
    const guardian_node_link_pre_freshness_compatible_message_t *message,
    guardian_node_link_freshness_evaluation_t *evaluation)
{
    guardian_node_link_freshness_result_t result;

    if (evaluation == NULL)
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT;
    }

    /*
     * INVALID is zero by contract.
     *
     * Therefore any subsequent failure leaves deterministic non-ACCEPT output.
     */
    (void)memset(evaluation, 0, sizeof(*evaluation));

    if ((policy == NULL) ||
        (runtime == NULL) ||
        (message == NULL))
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT;
    }

    result =
        guardian_node_link_freshness_evaluate_internal(
            policy,
            runtime,
            message,
            evaluation);

    if (result != GUARDIAN_NODE_LINK_FRESHNESS_OK)
    {
        (void)memset(evaluation, 0, sizeof(*evaluation));
    }

    return result;
}

guardian_node_link_freshness_result_t
guardian_node_link_freshness_evaluate_and_apply(
    const guardian_node_link_freshness_policy_t *policy,
    guardian_node_link_freshness_runtime_t *runtime,
    const guardian_node_link_pre_freshness_compatible_message_t *message,
    guardian_node_link_freshness_evaluation_t *evaluation)
{
    guardian_node_link_freshness_runtime_t candidate;
    guardian_node_link_freshness_result_t result;

    if (evaluation == NULL)
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT;
    }

    (void)memset(evaluation, 0, sizeof(*evaluation));

    if ((policy == NULL) ||
        (runtime == NULL) ||
        (message == NULL))
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_NULL_ARGUMENT;
    }

    result =
        guardian_node_link_freshness_evaluate_internal(
            policy,
            runtime,
            message,
            evaluation);

    if (result != GUARDIAN_NODE_LINK_FRESHNESS_OK)
    {
        (void)memset(evaluation, 0, sizeof(*evaluation));
        return result;
    }

    /*
     * Security decisions other than ACCEPT are observable but cannot mutate
     * accepted freshness state.
     */
    if (evaluation->decision !=
        GUARDIAN_NODE_LINK_FRESHNESS_DECISION_ACCEPT)
    {
        return GUARDIAN_NODE_LINK_FRESHNESS_OK;
    }

    candidate = *runtime;

    candidate.observation_valid = 1U;
    candidate.observed_epoch = evaluation->observed_epoch;
    candidate.observed_sequence = evaluation->observed_sequence;

    /*
     * evaluate_internal() permits ACCEPT only for the currently accepted
     * epoch. R3B therefore advances only same-epoch sequence state.
     */
    candidate.accepted_sequence_valid = 1U;
    candidate.accepted_sequence = evaluation->observed_sequence;

    result =
        guardian_node_link_freshness_runtime_validate(&candidate);

    if (result != GUARDIAN_NODE_LINK_FRESHNESS_OK)
    {
        (void)memset(evaluation, 0, sizeof(*evaluation));

        return GUARDIAN_NODE_LINK_FRESHNESS_ERROR_INVALID_EVALUATION;
    }

    /*
     * Logical commit occurs only after complete candidate validation.
     */
    *runtime = candidate;

    evaluation->transition_applied = 1U;

    return GUARDIAN_NODE_LINK_FRESHNESS_OK;
}
