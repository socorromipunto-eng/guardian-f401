#include "guardian_high_compute_node.h"

#include <stdio.h>
#include <string.h>

#define TEST_CHECK(condition) \
    do \
    { \
        if (!(condition)) \
        { \
            return __LINE__; \
        } \
    } \
    while (0)

static guardian_multinode_node_t make_high_compute_node(void)
{
    guardian_multinode_node_t node;

    node.node_id = 5U;
    node.producer_id = 105U;
    node.key_id = 1005U;
    node.role = GUARDIAN_NODE_ROLE_HIGH_COMPUTE_PROCESSING;
    node.authority_class = GUARDIAN_AUTHORITY_CLASS_OBSERVE;
    node.capabilities =
        GUARDIAN_CAP_OBSERVATION |
        GUARDIAN_CAP_SIGNAL_PROCESSING;

    return node;
}

static guardian_high_compute_policy_t make_policy(void)
{
    guardian_high_compute_policy_t policy;

    policy.minimum_quality = 80U;
    policy.drift_threshold_milli = 500U;
    policy.minimum_correlation_milli = 500;

    return policy;
}

static void fill_samples(
    guardian_high_compute_sample_t *samples,
    const int32_t *values,
    size_t count,
    uint32_t sequence_base,
    uint64_t timestamp_base)
{
    size_t index;

    for (index = 0U; index < count; index += 1U)
    {
        samples[index].sequence = sequence_base + (uint32_t)index;
        samples[index].timestamp_us = timestamp_base + ((uint64_t)index * UINT64_C(1000));
        samples[index].normalized_milli = values[index];
        samples[index].quality = 95U;
        samples[index].saturated = 0U;
    }
}

int main(void)
{
    static const int32_t values_a[4] = {1000, 2000, 3000, 4000};
    static const int32_t values_b[4] = {1100, 2100, 3100, 4100};
    static const int32_t values_inverse[4] = {4000, 3000, 2000, 1000};
    static const int32_t values_constant[4] = {2000, 2000, 2000, 2000};

    guardian_high_compute_node_t node;
    guardian_multinode_node_t identity = make_high_compute_node();
    guardian_high_compute_policy_t policy = make_policy();
    guardian_high_compute_sample_t samples_a[4];
    guardian_high_compute_sample_t samples_b[4];
    guardian_high_compute_sample_t samples_inverse[4];
    guardian_high_compute_sample_t samples_constant[4];
    guardian_high_compute_window_t window_a;
    guardian_high_compute_window_t window_b;
    guardian_high_compute_window_t window_inverse;
    guardian_high_compute_window_t window_constant;
    guardian_high_compute_analysis_t analysis_a;
    guardian_high_compute_analysis_t analysis_b;
    guardian_high_compute_analysis_t analysis_inverse;
    guardian_high_compute_analysis_t analysis_constant;
    guardian_high_compute_fusion_t fusion;
    uint8_t allowed = 1U;

    guardian_high_compute_init(&node);
    TEST_CHECK(node.initialized == 0U);

    TEST_CHECK(
        guardian_high_compute_configure(&node, &identity, &policy) ==
        GUARDIAN_HIGH_COMPUTE_OK);

    fill_samples(samples_a, values_a, 4U, 1U, UINT64_C(100000));
    fill_samples(samples_b, values_b, 4U, 11U, UINT64_C(200000));
    fill_samples(samples_inverse, values_inverse, 4U, 21U, UINT64_C(300000));
    fill_samples(samples_constant, values_constant, 4U, 31U, UINT64_C(400000));

    window_a.subject_node_id = 3U;
    window_a.subject_producer_id = 103U;
    window_a.signal_id = 1U;
    window_a.epoch = 1U;
    window_a.baseline_milli = 2000;
    window_a.samples = samples_a;
    window_a.sample_count = 4U;

    window_b = window_a;
    window_b.signal_id = 2U;
    window_b.samples = samples_b;

    window_inverse = window_a;
    window_inverse.signal_id = 3U;
    window_inverse.samples = samples_inverse;

    window_constant = window_a;
    window_constant.signal_id = 4U;
    window_constant.samples = samples_constant;

    TEST_CHECK(
        guardian_high_compute_analyze(&node, &window_a, &analysis_a) ==
        GUARDIAN_HIGH_COMPUTE_OK);

    TEST_CHECK(analysis_a.mean_milli == 2500);
    TEST_CHECK(analysis_a.minimum_milli == 1000);
    TEST_CHECK(analysis_a.maximum_milli == 4000);
    TEST_CHECK(analysis_a.peak_to_peak_milli == 3000U);
    TEST_CHECK(analysis_a.mean_square_milli2 == UINT64_C(7500000));
    TEST_CHECK(analysis_a.drift_milli == 500U);
    TEST_CHECK(analysis_a.drift_detected == 1U);
    TEST_CHECK(analysis_a.actuation_authorized == 0U);

    TEST_CHECK(
        guardian_high_compute_analyze(&node, &window_b, &analysis_b) ==
        GUARDIAN_HIGH_COMPUTE_OK);

    TEST_CHECK(
        guardian_high_compute_correlate_and_fuse(
            &node,
            &window_a,
            &window_b,
            &analysis_a,
            &analysis_b,
            &fusion) ==
        GUARDIAN_HIGH_COMPUTE_OK);

    TEST_CHECK(fusion.correlation_milli >= 999);
    TEST_CHECK(fusion.contradiction_detected == 0U);
    TEST_CHECK(fusion.actuation_authorized == 0U);

    TEST_CHECK(
        guardian_high_compute_analyze(&node, &window_inverse, &analysis_inverse) ==
        GUARDIAN_HIGH_COMPUTE_OK);

    TEST_CHECK(
        guardian_high_compute_correlate_and_fuse(
            &node,
            &window_a,
            &window_inverse,
            &analysis_a,
            &analysis_inverse,
            &fusion) ==
        GUARDIAN_HIGH_COMPUTE_OK);

    TEST_CHECK(fusion.correlation_milli <= -999);
    TEST_CHECK(fusion.contradiction_detected == 1U);

    samples_a[2].sequence = samples_a[1].sequence;
    TEST_CHECK(
        guardian_high_compute_analyze(&node, &window_a, &analysis_a) ==
        GUARDIAN_HIGH_COMPUTE_ERROR_SEQUENCE_REGRESSION);
    samples_a[2].sequence = 3U;

    samples_a[2].timestamp_us = samples_a[1].timestamp_us;
    TEST_CHECK(
        guardian_high_compute_analyze(&node, &window_a, &analysis_a) ==
        GUARDIAN_HIGH_COMPUTE_ERROR_TIMESTAMP_REGRESSION);
    samples_a[2].timestamp_us = UINT64_C(102000);

    samples_a[1].saturated = 1U;
    TEST_CHECK(
        guardian_high_compute_analyze(&node, &window_a, &analysis_a) ==
        GUARDIAN_HIGH_COMPUTE_ERROR_SATURATED);
    samples_a[1].saturated = 0U;

    samples_a[1].quality = 70U;
    TEST_CHECK(
        guardian_high_compute_analyze(&node, &window_a, &analysis_a) ==
        GUARDIAN_HIGH_COMPUTE_ERROR_QUALITY);
    samples_a[1].quality = 95U;

    samples_a[1].normalized_milli = 100001;
    TEST_CHECK(
        guardian_high_compute_analyze(&node, &window_a, &analysis_a) ==
        GUARDIAN_HIGH_COMPUTE_ERROR_OUT_OF_RANGE);
    samples_a[1].normalized_milli = 2000;

    TEST_CHECK(
        guardian_high_compute_analyze(&node, &window_a, &analysis_a) ==
        GUARDIAN_HIGH_COMPUTE_OK);

    TEST_CHECK(
        guardian_high_compute_analyze(&node, &window_constant, &analysis_constant) ==
        GUARDIAN_HIGH_COMPUTE_OK);

    TEST_CHECK(
        guardian_high_compute_correlate_and_fuse(
            &node,
            &window_a,
            &window_constant,
            &analysis_a,
            &analysis_constant,
            &fusion) ==
        GUARDIAN_HIGH_COMPUTE_ERROR_CORRELATION_UNDEFINED);

    allowed = 1U;
    TEST_CHECK(
        guardian_high_compute_can_actuate(&node, &allowed) ==
        GUARDIAN_HIGH_COMPUTE_ERROR_ACTUATION_PROHIBITED);
    TEST_CHECK(allowed == 0U);

    identity.capabilities |= GUARDIAN_CAP_ACTUATION_GATE;
    TEST_CHECK(
        guardian_high_compute_configure(&node, &identity, &policy) ==
        GUARDIAN_HIGH_COMPUTE_ERROR_INVALID_NODE);

    puts("M17_HIGH_COMPUTE_NODE_HOST_TEST=PASS");
    return 0;
}
