#include "guardian_st_measurement_node.h"

#include <stdio.h>

#define TEST_CHECK(condition) \
    do \
    { \
        if (!(condition)) \
        { \
            return __LINE__; \
        } \
    } \
    while (0)

static guardian_multinode_node_t
make_st_node(void)
{
    guardian_multinode_node_t node;

    node.node_id =
        3U;

    node.producer_id =
        103U;

    node.key_id =
        1003U;

    node.role =
        GUARDIAN_NODE_ROLE_ST_MEASUREMENT_REFERENCE;

    node.authority_class =
        GUARDIAN_AUTHORITY_CLASS_OBSERVE;

    node.capabilities =
        GUARDIAN_CAP_OBSERVATION |
        GUARDIAN_CAP_SENSOR_ACQUISITION;

    return node;
}

static guardian_st_measurement_sample_t
make_valid_sample(
    uint32_t sequence,
    uint64_t timestamp_us)
{
    guardian_st_measurement_sample_t sample;

    sample.sensor_id =
        501U;

    sample.signal_type =
        GUARDIAN_ST_SIGNAL_VIBRATION_X;

    sample.sequence =
        sequence;

    sample.timestamp_us =
        timestamp_us;

    sample.value_milli =
        1250;

    sample.valid_min_milli =
        -5000;

    sample.valid_max_milli =
        5000;

    sample.uncertainty_milli =
        25U;

    sample.quality =
        95U;

    sample.saturated =
        0U;

    return sample;
}

int main(void)
{
    guardian_st_measurement_node_t node;

    guardian_st_measurement_policy_t policy;

    guardian_multinode_node_t identity =
        make_st_node();

    guardian_st_measurement_sample_t sample;

    guardian_st_measurement_observation_t observation;

    uint8_t allowed =
        1U;

    policy.max_uncertainty_milli =
        100U;

    policy.minimum_quality =
        80U;

    /*
     * PT01 - deterministic initialization.
     */
    guardian_st_measurement_node_init(
        &node);

    TEST_CHECK(
        node.initialized ==
        0U);

    /*
     * PT02 - valid measurement-node configuration.
     */
    TEST_CHECK(
        guardian_st_measurement_node_configure(
            &node,
            &identity,
            &policy) ==
        GUARDIAN_ST_MEASUREMENT_OK);

    TEST_CHECK(
        node.initialized ==
        1U);

    /*
     * PT03 - valid sample is accepted.
     */
    sample =
        make_valid_sample(
            1U,
            1000U);

    TEST_CHECK(
        guardian_st_measurement_accept_sample(
            &node,
            &sample,
            &observation) ==
        GUARDIAN_ST_MEASUREMENT_OK);

    TEST_CHECK(
        observation.producer_node_id ==
        3U);

    TEST_CHECK(
        observation.sequence ==
        1U);

    TEST_CHECK(
        observation.actuation_authorized ==
        0U);

    /*
     * PT04 - second monotonic sample accepted.
     */
    sample =
        make_valid_sample(
            2U,
            2000U);

    TEST_CHECK(
        guardian_st_measurement_accept_sample(
            &node,
            &sample,
            &observation) ==
        GUARDIAN_ST_MEASUREMENT_OK);

    TEST_CHECK(
        node.accepted_samples ==
        2U);

    /*
     * NT01 - duplicate sequence rejected.
     */
    sample =
        make_valid_sample(
            2U,
            3000U);

    TEST_CHECK(
        guardian_st_measurement_accept_sample(
            &node,
            &sample,
            &observation) ==
        GUARDIAN_ST_MEASUREMENT_ERROR_SEQUENCE_REGRESSION);

    /*
     * NT02 - timestamp regression rejected.
     */
    sample =
        make_valid_sample(
            3U,
            1500U);

    TEST_CHECK(
        guardian_st_measurement_accept_sample(
            &node,
            &sample,
            &observation) ==
        GUARDIAN_ST_MEASUREMENT_ERROR_TIMESTAMP_REGRESSION);

    /*
     * NT03 - saturated sample rejected.
     */
    sample =
        make_valid_sample(
            3U,
            3000U);

    sample.saturated =
        1U;

    TEST_CHECK(
        guardian_st_measurement_accept_sample(
            &node,
            &sample,
            &observation) ==
        GUARDIAN_ST_MEASUREMENT_ERROR_SATURATED);

    /*
     * NT04 - out-of-range sample rejected.
     */
    sample =
        make_valid_sample(
            3U,
            3000U);

    sample.value_milli =
        6000;

    TEST_CHECK(
        guardian_st_measurement_accept_sample(
            &node,
            &sample,
            &observation) ==
        GUARDIAN_ST_MEASUREMENT_ERROR_OUT_OF_RANGE);

    /*
     * NT05 - insufficient sample quality rejected.
     */
    sample =
        make_valid_sample(
            3U,
            3000U);

    sample.quality =
        50U;

    TEST_CHECK(
        guardian_st_measurement_accept_sample(
            &node,
            &sample,
            &observation) ==
        GUARDIAN_ST_MEASUREMENT_ERROR_QUALITY);

    /*
     * NT06 - excessive uncertainty rejected.
     */
    sample =
        make_valid_sample(
            3U,
            3000U);

    sample.uncertainty_milli =
        101U;

    TEST_CHECK(
        guardian_st_measurement_accept_sample(
            &node,
            &sample,
            &observation) ==
        GUARDIAN_ST_MEASUREMENT_ERROR_UNCERTAINTY);

    /*
     * NT07 - zero uncertainty is not silently treated as exact.
     */
    sample =
        make_valid_sample(
            3U,
            3000U);

    sample.uncertainty_milli =
        0U;

    TEST_CHECK(
        guardian_st_measurement_accept_sample(
            &node,
            &sample,
            &observation) ==
        GUARDIAN_ST_MEASUREMENT_ERROR_UNCERTAINTY);

    /*
     * NT08 - measurement node cannot actuate.
     */
    allowed =
        1U;

    TEST_CHECK(
        guardian_st_measurement_can_actuate(
            &node,
            &allowed) ==
        GUARDIAN_ST_MEASUREMENT_ERROR_ACTUATION_PROHIBITED);

    TEST_CHECK(
        allowed ==
        0U);

    /*
     * NT09 - identity cannot acquire F401 actuation capability.
     */
    identity.capabilities |=
        GUARDIAN_CAP_ACTUATION_GATE;

    TEST_CHECK(
        guardian_st_measurement_node_configure(
            &node,
            &identity,
            &policy) ==
        GUARDIAN_ST_MEASUREMENT_ERROR_INVALID_NODE);

    puts(
        "M17_ST_MEASUREMENT_NODE_HOST_TEST=PASS");

    return 0;
}
