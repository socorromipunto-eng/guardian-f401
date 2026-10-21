#include "guardian_high_compute_node.h"

#include <limits.h>
#include <string.h>

static uint64_t guardian_high_compute_isqrt_u64(uint64_t value)
{
    uint64_t result = 0U;
    uint64_t bit = UINT64_C(1) << 62U;

    while (bit > value)
    {
        bit >>= 2U;
    }

    while (bit != 0U)
    {
        if (value >= (result + bit))
        {
            value -= result + bit;
            result = (result >> 1U) + bit;
        }
        else
        {
            result >>= 1U;
        }

        bit >>= 2U;
    }

    return result;
}

static uint32_t guardian_high_compute_abs_i64_to_u32(int64_t value)
{
    uint64_t magnitude;

    if (value < 0)
    {
        magnitude = (uint64_t)(-value);
    }
    else
    {
        magnitude = (uint64_t)value;
    }

    if (magnitude > (uint64_t)UINT32_MAX)
    {
        return UINT32_MAX;
    }

    return (uint32_t)magnitude;
}

static int guardian_high_compute_window_basic_valid(
    const guardian_high_compute_window_t *window)
{
    if ((window == NULL) ||
        (window->samples == NULL) ||
        (window->subject_node_id == 0U) ||
        (window->subject_producer_id == 0U) ||
        (window->signal_id == 0U) ||
        (window->epoch == 0U) ||
        (window->sample_count < 2U) ||
        (window->sample_count > GUARDIAN_HIGH_COMPUTE_MAX_SAMPLES))
    {
        return 0;
    }

    if ((window->baseline_milli < -GUARDIAN_HIGH_COMPUTE_NORMALIZED_LIMIT_MILLI) ||
        (window->baseline_milli > GUARDIAN_HIGH_COMPUTE_NORMALIZED_LIMIT_MILLI))
    {
        return 0;
    }

    return 1;
}

void guardian_high_compute_init(
    guardian_high_compute_node_t *node)
{
    if (node == NULL)
    {
        return;
    }

    (void)memset(node, 0, sizeof(*node));
}

guardian_high_compute_result_t guardian_high_compute_configure(
    guardian_high_compute_node_t *node,
    const guardian_multinode_node_t *self,
    const guardian_high_compute_policy_t *policy)
{
    guardian_multinode_result_t node_result;

    if ((node == NULL) || (self == NULL) || (policy == NULL))
    {
        return GUARDIAN_HIGH_COMPUTE_ERROR_NULL_ARGUMENT;
    }

    node_result = guardian_multinode_validate_node(self);

    if (node_result != GUARDIAN_MULTINODE_OK)
    {
        return GUARDIAN_HIGH_COMPUTE_ERROR_INVALID_NODE;
    }

    if ((self->role != GUARDIAN_NODE_ROLE_HIGH_COMPUTE_PROCESSING) ||
        (self->authority_class != GUARDIAN_AUTHORITY_CLASS_OBSERVE) ||
        ((self->capabilities & GUARDIAN_CAP_SIGNAL_PROCESSING) == 0U) ||
        ((self->capabilities & GUARDIAN_CAP_ACTUATION_GATE) != 0U))
    {
        return GUARDIAN_HIGH_COMPUTE_ERROR_INVALID_NODE;
    }

    if ((policy->minimum_quality == 0U) ||
        (policy->minimum_quality > GUARDIAN_HIGH_COMPUTE_MAX_QUALITY) ||
        (policy->drift_threshold_milli == 0U) ||
        (policy->drift_threshold_milli > (uint32_t)GUARDIAN_HIGH_COMPUTE_NORMALIZED_LIMIT_MILLI) ||
        (policy->minimum_correlation_milli < -GUARDIAN_HIGH_COMPUTE_CORRELATION_SCALE) ||
        (policy->minimum_correlation_milli > GUARDIAN_HIGH_COMPUTE_CORRELATION_SCALE))
    {
        return GUARDIAN_HIGH_COMPUTE_ERROR_INVALID_POLICY;
    }

    guardian_high_compute_init(node);
    node->self = *self;
    node->policy = *policy;
    node->initialized = 1U;

    return GUARDIAN_HIGH_COMPUTE_OK;
}

guardian_high_compute_result_t guardian_high_compute_analyze(
    guardian_high_compute_node_t *node,
    const guardian_high_compute_window_t *window,
    guardian_high_compute_analysis_t *analysis)
{
    size_t index;
    int64_t sum = 0;
    uint64_t sum_square = 0U;
    int32_t minimum_value;
    int32_t maximum_value;
    uint8_t minimum_quality;

    if ((node == NULL) || (window == NULL) || (analysis == NULL))
    {
        return GUARDIAN_HIGH_COMPUTE_ERROR_NULL_ARGUMENT;
    }

    (void)memset(analysis, 0, sizeof(*analysis));

    if (node->initialized != 1U)
    {
        return GUARDIAN_HIGH_COMPUTE_ERROR_INVALID_NODE;
    }

    if (guardian_high_compute_window_basic_valid(window) == 0)
    {
        node->rejected_windows += 1U;
        return GUARDIAN_HIGH_COMPUTE_ERROR_INVALID_WINDOW;
    }

    minimum_value = window->samples[0].normalized_milli;
    maximum_value = window->samples[0].normalized_milli;
    minimum_quality = window->samples[0].quality;

    for (index = 0U; index < window->sample_count; index += 1U)
    {
        const guardian_high_compute_sample_t *sample = &window->samples[index];
        int64_t value64;

        if ((sample->sequence == 0U) || (sample->timestamp_us == 0U))
        {
            node->rejected_windows += 1U;
            return GUARDIAN_HIGH_COMPUTE_ERROR_INVALID_WINDOW;
        }

        if ((sample->normalized_milli < -GUARDIAN_HIGH_COMPUTE_NORMALIZED_LIMIT_MILLI) ||
            (sample->normalized_milli > GUARDIAN_HIGH_COMPUTE_NORMALIZED_LIMIT_MILLI))
        {
            node->rejected_windows += 1U;
            return GUARDIAN_HIGH_COMPUTE_ERROR_OUT_OF_RANGE;
        }

        if (sample->saturated != 0U)
        {
            node->rejected_windows += 1U;
            return GUARDIAN_HIGH_COMPUTE_ERROR_SATURATED;
        }

        if ((sample->quality == 0U) ||
            (sample->quality > GUARDIAN_HIGH_COMPUTE_MAX_QUALITY) ||
            (sample->quality < node->policy.minimum_quality))
        {
            node->rejected_windows += 1U;
            return GUARDIAN_HIGH_COMPUTE_ERROR_QUALITY;
        }

        if (index > 0U)
        {
            if (sample->sequence <= window->samples[index - 1U].sequence)
            {
                node->rejected_windows += 1U;
                return GUARDIAN_HIGH_COMPUTE_ERROR_SEQUENCE_REGRESSION;
            }

            if (sample->timestamp_us <= window->samples[index - 1U].timestamp_us)
            {
                node->rejected_windows += 1U;
                return GUARDIAN_HIGH_COMPUTE_ERROR_TIMESTAMP_REGRESSION;
            }
        }

        if (sample->normalized_milli < minimum_value)
        {
            minimum_value = sample->normalized_milli;
        }

        if (sample->normalized_milli > maximum_value)
        {
            maximum_value = sample->normalized_milli;
        }

        if (sample->quality < minimum_quality)
        {
            minimum_quality = sample->quality;
        }

        value64 = (int64_t)sample->normalized_milli;
        sum += value64;
        sum_square += (uint64_t)(value64 * value64);
    }

    analysis->subject_node_id = window->subject_node_id;
    analysis->subject_producer_id = window->subject_producer_id;
    analysis->signal_id = window->signal_id;
    analysis->epoch = window->epoch;
    analysis->first_sequence = window->samples[0].sequence;
    analysis->last_sequence = window->samples[window->sample_count - 1U].sequence;
    analysis->first_timestamp_us = window->samples[0].timestamp_us;
    analysis->last_timestamp_us = window->samples[window->sample_count - 1U].timestamp_us;
    analysis->sample_count = window->sample_count;
    analysis->mean_milli = (int32_t)(sum / (int64_t)window->sample_count);
    analysis->minimum_milli = minimum_value;
    analysis->maximum_milli = maximum_value;
    analysis->peak_to_peak_milli = (uint32_t)((int64_t)maximum_value - (int64_t)minimum_value);
    analysis->mean_square_milli2 = sum_square / (uint64_t)window->sample_count;
    analysis->drift_milli = guardian_high_compute_abs_i64_to_u32(
        (int64_t)analysis->mean_milli - (int64_t)window->baseline_milli);
    analysis->minimum_quality = minimum_quality;
    analysis->drift_detected =
        (analysis->drift_milli >= node->policy.drift_threshold_milli) ? 1U : 0U;
    analysis->actuation_authorized = GUARDIAN_HIGH_COMPUTE_AUTHORITY_NONE;

    node->accepted_windows += 1U;

    return GUARDIAN_HIGH_COMPUTE_OK;
}

guardian_high_compute_result_t guardian_high_compute_correlate_and_fuse(
    const guardian_high_compute_node_t *node,
    const guardian_high_compute_window_t *left,
    const guardian_high_compute_window_t *right,
    const guardian_high_compute_analysis_t *left_analysis,
    const guardian_high_compute_analysis_t *right_analysis,
    guardian_high_compute_fusion_t *fusion)
{
    size_t index;
    int64_t left_sum = 0;
    int64_t right_sum = 0;
    int64_t dot = 0;
    uint64_t left_square = 0U;
    uint64_t right_square = 0U;
    uint64_t denominator;
    int64_t scaled_dot;
    int64_t correlation;
    int64_t fused_sum;
    int64_t sample_count64;

    if ((node == NULL) ||
        (left == NULL) ||
        (right == NULL) ||
        (left_analysis == NULL) ||
        (right_analysis == NULL) ||
        (fusion == NULL))
    {
        return GUARDIAN_HIGH_COMPUTE_ERROR_NULL_ARGUMENT;
    }

    (void)memset(fusion, 0, sizeof(*fusion));

    if (node->initialized != 1U)
    {
        return GUARDIAN_HIGH_COMPUTE_ERROR_INVALID_NODE;
    }

    if ((guardian_high_compute_window_basic_valid(left) == 0) ||
        (guardian_high_compute_window_basic_valid(right) == 0) ||
        (left->sample_count != right->sample_count) ||
        (left_analysis->sample_count != left->sample_count) ||
        (right_analysis->sample_count != right->sample_count) ||
        (left_analysis->actuation_authorized != GUARDIAN_HIGH_COMPUTE_AUTHORITY_NONE) ||
        (right_analysis->actuation_authorized != GUARDIAN_HIGH_COMPUTE_AUTHORITY_NONE))
    {
        return GUARDIAN_HIGH_COMPUTE_ERROR_INVALID_WINDOW;
    }

    sample_count64 = (int64_t)left->sample_count;

    for (index = 0U; index < left->sample_count; index += 1U)
    {
        left_sum += (int64_t)left->samples[index].normalized_milli;
        right_sum += (int64_t)right->samples[index].normalized_milli;
    }

    for (index = 0U; index < left->sample_count; index += 1U)
    {
        int64_t left_centered =
            ((int64_t)left->samples[index].normalized_milli * sample_count64) - left_sum;

        int64_t right_centered =
            ((int64_t)right->samples[index].normalized_milli * sample_count64) - right_sum;

        dot += left_centered * right_centered;
        left_square += (uint64_t)(left_centered * left_centered);
        right_square += (uint64_t)(right_centered * right_centered);
    }

    if ((left_square == 0U) || (right_square == 0U))
    {
        return GUARDIAN_HIGH_COMPUTE_ERROR_CORRELATION_UNDEFINED;
    }

    denominator =
        guardian_high_compute_isqrt_u64(left_square) *
        guardian_high_compute_isqrt_u64(right_square);

    if (denominator == 0U)
    {
        return GUARDIAN_HIGH_COMPUTE_ERROR_CORRELATION_UNDEFINED;
    }

    scaled_dot = dot * (int64_t)GUARDIAN_HIGH_COMPUTE_CORRELATION_SCALE;
    correlation = scaled_dot / (int64_t)denominator;

    if (correlation > (int64_t)GUARDIAN_HIGH_COMPUTE_CORRELATION_SCALE)
    {
        correlation = (int64_t)GUARDIAN_HIGH_COMPUTE_CORRELATION_SCALE;
    }
    else if (correlation < -(int64_t)GUARDIAN_HIGH_COMPUTE_CORRELATION_SCALE)
    {
        correlation = -(int64_t)GUARDIAN_HIGH_COMPUTE_CORRELATION_SCALE;
    }

    fused_sum =
        (int64_t)left_analysis->mean_milli +
        (int64_t)right_analysis->mean_milli;

    fusion->correlation_milli = (int32_t)correlation;
    fusion->fused_mean_milli = (int32_t)(fused_sum / 2);
    fusion->contradiction_detected =
        (fusion->correlation_milli < node->policy.minimum_correlation_milli) ? 1U : 0U;
    fusion->actuation_authorized = GUARDIAN_HIGH_COMPUTE_AUTHORITY_NONE;

    return GUARDIAN_HIGH_COMPUTE_OK;
}

guardian_high_compute_result_t guardian_high_compute_can_actuate(
    const guardian_high_compute_node_t *node,
    uint8_t *allowed)
{
    if ((node == NULL) || (allowed == NULL))
    {
        return GUARDIAN_HIGH_COMPUTE_ERROR_NULL_ARGUMENT;
    }

    *allowed = 0U;

    return GUARDIAN_HIGH_COMPUTE_ERROR_ACTUATION_PROHIBITED;
}
