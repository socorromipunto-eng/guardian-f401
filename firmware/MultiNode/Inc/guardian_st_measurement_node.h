#ifndef GUARDIAN_ST_MEASUREMENT_NODE_H
#define GUARDIAN_ST_MEASUREMENT_NODE_H

#include "guardian_multinode.h"

#include <stdint.h>

#define GUARDIAN_ST_MEASUREMENT_MAX_QUALITY ((uint8_t)100U)

typedef enum
{
    GUARDIAN_ST_SIGNAL_INVALID = 0,

    GUARDIAN_ST_SIGNAL_VIBRATION_X,

    GUARDIAN_ST_SIGNAL_VIBRATION_Y,

    GUARDIAN_ST_SIGNAL_VIBRATION_Z,

    GUARDIAN_ST_SIGNAL_CURRENT,

    GUARDIAN_ST_SIGNAL_VOLTAGE,

    GUARDIAN_ST_SIGNAL_SPEED,

    GUARDIAN_ST_SIGNAL_PHASE,

    GUARDIAN_ST_SIGNAL_TEMPERATURE,

    GUARDIAN_ST_SIGNAL_ACOUSTIC_EMISSION,

    GUARDIAN_ST_SIGNAL_TRANSIENT

} guardian_st_signal_type_t;

typedef enum
{
    GUARDIAN_ST_MEASUREMENT_OK = 0,

    GUARDIAN_ST_MEASUREMENT_ERROR_NULL_ARGUMENT,

    GUARDIAN_ST_MEASUREMENT_ERROR_INVALID_NODE,

    GUARDIAN_ST_MEASUREMENT_ERROR_INVALID_POLICY,

    GUARDIAN_ST_MEASUREMENT_ERROR_INVALID_SAMPLE,

    GUARDIAN_ST_MEASUREMENT_ERROR_SEQUENCE_REGRESSION,

    GUARDIAN_ST_MEASUREMENT_ERROR_TIMESTAMP_REGRESSION,

    GUARDIAN_ST_MEASUREMENT_ERROR_OUT_OF_RANGE,

    GUARDIAN_ST_MEASUREMENT_ERROR_SATURATED,

    GUARDIAN_ST_MEASUREMENT_ERROR_QUALITY,

    GUARDIAN_ST_MEASUREMENT_ERROR_UNCERTAINTY,

    GUARDIAN_ST_MEASUREMENT_ERROR_ACTUATION_PROHIBITED

} guardian_st_measurement_result_t;

typedef struct
{
    uint32_t max_uncertainty_milli;

    uint8_t minimum_quality;

} guardian_st_measurement_policy_t;

typedef struct
{
    uint32_t sensor_id;

    guardian_st_signal_type_t signal_type;

    uint32_t sequence;

    uint64_t timestamp_us;

    /*
     * Deterministic fixed-point representation.
     *
     * value_milli means engineering-unit * 1000.
     */
    int32_t value_milli;

    int32_t valid_min_milli;

    int32_t valid_max_milli;

    uint32_t uncertainty_milli;

    uint8_t quality;

    uint8_t saturated;

} guardian_st_measurement_sample_t;

typedef struct
{
    uint32_t producer_node_id;

    uint32_t sensor_id;

    guardian_st_signal_type_t signal_type;

    uint32_t sequence;

    uint64_t timestamp_us;

    int32_t value_milli;

    uint32_t uncertainty_milli;

    uint8_t quality;

    /*
     * Observation != authorization.
     */
    uint8_t actuation_authorized;

} guardian_st_measurement_observation_t;

typedef struct
{
    guardian_multinode_node_t self;

    guardian_st_measurement_policy_t policy;

    uint32_t last_sequence;

    uint64_t last_timestamp_us;

    uint32_t accepted_samples;

    uint32_t rejected_samples;

    uint8_t initialized;

} guardian_st_measurement_node_t;

void guardian_st_measurement_node_init(
    guardian_st_measurement_node_t *node);

guardian_st_measurement_result_t
guardian_st_measurement_node_configure(
    guardian_st_measurement_node_t *node,
    const guardian_multinode_node_t *self,
    const guardian_st_measurement_policy_t *policy);

guardian_st_measurement_result_t
guardian_st_measurement_validate_sample(
    const guardian_st_measurement_node_t *node,
    const guardian_st_measurement_sample_t *sample);

guardian_st_measurement_result_t
guardian_st_measurement_accept_sample(
    guardian_st_measurement_node_t *node,
    const guardian_st_measurement_sample_t *sample,
    guardian_st_measurement_observation_t *observation);

guardian_st_measurement_result_t
guardian_st_measurement_can_actuate(
    const guardian_st_measurement_node_t *node,
    uint8_t *allowed);

#endif
