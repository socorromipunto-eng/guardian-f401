#include "guardian_st_measurement_node.h"

#include <limits.h>
#include <string.h>

static int
guardian_st_signal_type_valid(
    guardian_st_signal_type_t signal_type)
{
    return
        (signal_type >
            GUARDIAN_ST_SIGNAL_INVALID) &&
        (signal_type <=
            GUARDIAN_ST_SIGNAL_TRANSIENT);
}

void guardian_st_measurement_node_init(
    guardian_st_measurement_node_t *node)
{
    if (node == NULL)
    {
        return;
    }

    (void)memset(
        node,
        0,
        sizeof(*node));
}

guardian_st_measurement_result_t
guardian_st_measurement_node_configure(
    guardian_st_measurement_node_t *node,
    const guardian_multinode_node_t *self,
    const guardian_st_measurement_policy_t *policy)
{
    guardian_multinode_result_t node_result;

    if ((node == NULL) ||
        (self == NULL) ||
        (policy == NULL))
    {
        return
            GUARDIAN_ST_MEASUREMENT_ERROR_NULL_ARGUMENT;
    }

    node_result =
        guardian_multinode_validate_node(
            self);

    if (node_result !=
        GUARDIAN_MULTINODE_OK)
    {
        return
            GUARDIAN_ST_MEASUREMENT_ERROR_INVALID_NODE;
    }

    if (self->role !=
        GUARDIAN_NODE_ROLE_ST_MEASUREMENT_REFERENCE)
    {
        return
            GUARDIAN_ST_MEASUREMENT_ERROR_INVALID_NODE;
    }

    if (self->authority_class !=
        GUARDIAN_AUTHORITY_CLASS_OBSERVE)
    {
        return
            GUARDIAN_ST_MEASUREMENT_ERROR_INVALID_NODE;
    }

    if ((self->capabilities &
         GUARDIAN_CAP_SENSOR_ACQUISITION) == 0U)
    {
        return
            GUARDIAN_ST_MEASUREMENT_ERROR_INVALID_NODE;
    }

    if ((self->capabilities &
         GUARDIAN_CAP_ACTUATION_GATE) != 0U)
    {
        return
            GUARDIAN_ST_MEASUREMENT_ERROR_INVALID_NODE;
    }

    if ((policy->max_uncertainty_milli == 0U) ||
        (policy->minimum_quality == 0U) ||
        (policy->minimum_quality >
            GUARDIAN_ST_MEASUREMENT_MAX_QUALITY))
    {
        return
            GUARDIAN_ST_MEASUREMENT_ERROR_INVALID_POLICY;
    }

    guardian_st_measurement_node_init(
        node);

    node->self =
        *self;

    node->policy =
        *policy;

    node->initialized =
        1U;

    return
        GUARDIAN_ST_MEASUREMENT_OK;
}

guardian_st_measurement_result_t
guardian_st_measurement_validate_sample(
    const guardian_st_measurement_node_t *node,
    const guardian_st_measurement_sample_t *sample)
{
    if ((node == NULL) ||
        (sample == NULL))
    {
        return
            GUARDIAN_ST_MEASUREMENT_ERROR_NULL_ARGUMENT;
    }

    if (node->initialized != 1U)
    {
        return
            GUARDIAN_ST_MEASUREMENT_ERROR_INVALID_NODE;
    }

    if ((sample->sensor_id == 0U) ||
        (sample->sequence == 0U) ||
        (sample->timestamp_us == 0U) ||
        (guardian_st_signal_type_valid(
            sample->signal_type) == 0))
    {
        return
            GUARDIAN_ST_MEASUREMENT_ERROR_INVALID_SAMPLE;
    }

    if (sample->valid_min_milli >
        sample->valid_max_milli)
    {
        return
            GUARDIAN_ST_MEASUREMENT_ERROR_INVALID_SAMPLE;
    }

    if (sample->saturated > 1U)
    {
        return
            GUARDIAN_ST_MEASUREMENT_ERROR_INVALID_SAMPLE;
    }

    if (sample->quality >
        GUARDIAN_ST_MEASUREMENT_MAX_QUALITY)
    {
        return
            GUARDIAN_ST_MEASUREMENT_ERROR_INVALID_SAMPLE;
    }

    if (sample->saturated == 1U)
    {
        return
            GUARDIAN_ST_MEASUREMENT_ERROR_SATURATED;
    }

    if ((sample->value_milli <
            sample->valid_min_milli) ||
        (sample->value_milli >
            sample->valid_max_milli))
    {
        return
            GUARDIAN_ST_MEASUREMENT_ERROR_OUT_OF_RANGE;
    }

    if (sample->quality <
        node->policy.minimum_quality)
    {
        return
            GUARDIAN_ST_MEASUREMENT_ERROR_QUALITY;
    }

    if ((sample->uncertainty_milli == 0U) ||
        (sample->uncertainty_milli >
            node->policy.max_uncertainty_milli))
    {
        return
            GUARDIAN_ST_MEASUREMENT_ERROR_UNCERTAINTY;
    }

    if (node->accepted_samples > 0U)
    {
        if (sample->sequence <=
            node->last_sequence)
        {
            return
                GUARDIAN_ST_MEASUREMENT_ERROR_SEQUENCE_REGRESSION;
        }

        if (sample->timestamp_us <=
            node->last_timestamp_us)
        {
            return
                GUARDIAN_ST_MEASUREMENT_ERROR_TIMESTAMP_REGRESSION;
        }
    }

    return
        GUARDIAN_ST_MEASUREMENT_OK;
}

guardian_st_measurement_result_t
guardian_st_measurement_accept_sample(
    guardian_st_measurement_node_t *node,
    const guardian_st_measurement_sample_t *sample,
    guardian_st_measurement_observation_t *observation)
{
    guardian_st_measurement_result_t result;

    if ((node == NULL) ||
        (sample == NULL) ||
        (observation == NULL))
    {
        return
            GUARDIAN_ST_MEASUREMENT_ERROR_NULL_ARGUMENT;
    }

    (void)memset(
        observation,
        0,
        sizeof(*observation));

    result =
        guardian_st_measurement_validate_sample(
            node,
            sample);

    if (result !=
        GUARDIAN_ST_MEASUREMENT_OK)
    {
        if (node->rejected_samples <
            UINT32_MAX)
        {
            node->rejected_samples += 1U;
        }

        return result;
    }

    node->last_sequence =
        sample->sequence;

    node->last_timestamp_us =
        sample->timestamp_us;

    if (node->accepted_samples <
        UINT32_MAX)
    {
        node->accepted_samples += 1U;
    }

    observation->producer_node_id =
        node->self.node_id;

    observation->sensor_id =
        sample->sensor_id;

    observation->signal_type =
        sample->signal_type;

    observation->sequence =
        sample->sequence;

    observation->timestamp_us =
        sample->timestamp_us;

    observation->value_milli =
        sample->value_milli;

    observation->uncertainty_milli =
        sample->uncertainty_milli;

    observation->quality =
        sample->quality;

    /*
     * Measurement evidence is not physical authority.
     */
    observation->actuation_authorized =
        0U;

    return
        GUARDIAN_ST_MEASUREMENT_OK;
}

guardian_st_measurement_result_t
guardian_st_measurement_can_actuate(
    const guardian_st_measurement_node_t *node,
    uint8_t *allowed)
{
    if ((node == NULL) ||
        (allowed == NULL))
    {
        return
            GUARDIAN_ST_MEASUREMENT_ERROR_NULL_ARGUMENT;
    }

    *allowed =
        0U;

    return
        GUARDIAN_ST_MEASUREMENT_ERROR_ACTUATION_PROHIBITED;
}
