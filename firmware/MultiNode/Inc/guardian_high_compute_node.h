#ifndef GUARDIAN_HIGH_COMPUTE_NODE_H
#define GUARDIAN_HIGH_COMPUTE_NODE_H

#include "guardian_multinode.h"

#include <stddef.h>
#include <stdint.h>

#define GUARDIAN_HIGH_COMPUTE_MAX_SAMPLES ((size_t)16U)
#define GUARDIAN_HIGH_COMPUTE_MAX_QUALITY ((uint8_t)100U)
#define GUARDIAN_HIGH_COMPUTE_NORMALIZED_LIMIT_MILLI ((int32_t)100000)
#define GUARDIAN_HIGH_COMPUTE_CORRELATION_SCALE ((int32_t)1000)
#define GUARDIAN_HIGH_COMPUTE_AUTHORITY_NONE ((uint8_t)0U)

typedef enum
{
    GUARDIAN_HIGH_COMPUTE_OK = 0,
    GUARDIAN_HIGH_COMPUTE_ERROR_NULL_ARGUMENT,
    GUARDIAN_HIGH_COMPUTE_ERROR_INVALID_NODE,
    GUARDIAN_HIGH_COMPUTE_ERROR_INVALID_POLICY,
    GUARDIAN_HIGH_COMPUTE_ERROR_INVALID_WINDOW,
    GUARDIAN_HIGH_COMPUTE_ERROR_SEQUENCE_REGRESSION,
    GUARDIAN_HIGH_COMPUTE_ERROR_TIMESTAMP_REGRESSION,
    GUARDIAN_HIGH_COMPUTE_ERROR_OUT_OF_RANGE,
    GUARDIAN_HIGH_COMPUTE_ERROR_SATURATED,
    GUARDIAN_HIGH_COMPUTE_ERROR_QUALITY,
    GUARDIAN_HIGH_COMPUTE_ERROR_CORRELATION_UNDEFINED,
    GUARDIAN_HIGH_COMPUTE_ERROR_ACTUATION_PROHIBITED
} guardian_high_compute_result_t;

typedef struct
{
    uint8_t minimum_quality;
    uint32_t drift_threshold_milli;
    int32_t minimum_correlation_milli;
} guardian_high_compute_policy_t;

typedef struct
{
    uint32_t sequence;
    uint64_t timestamp_us;
    int32_t normalized_milli;
    uint8_t quality;
    uint8_t saturated;
} guardian_high_compute_sample_t;

typedef struct
{
    uint32_t subject_node_id;
    uint32_t subject_producer_id;
    uint32_t signal_id;
    uint32_t epoch;
    int32_t baseline_milli;
    const guardian_high_compute_sample_t *samples;
    size_t sample_count;
} guardian_high_compute_window_t;

typedef struct
{
    uint32_t subject_node_id;
    uint32_t subject_producer_id;
    uint32_t signal_id;
    uint32_t epoch;
    uint32_t first_sequence;
    uint32_t last_sequence;
    uint64_t first_timestamp_us;
    uint64_t last_timestamp_us;
    size_t sample_count;
    int32_t mean_milli;
    int32_t minimum_milli;
    int32_t maximum_milli;
    uint32_t peak_to_peak_milli;
    uint64_t mean_square_milli2;
    uint32_t drift_milli;
    uint8_t minimum_quality;
    uint8_t drift_detected;
    uint8_t actuation_authorized;
} guardian_high_compute_analysis_t;

typedef struct
{
    int32_t correlation_milli;
    int32_t fused_mean_milli;
    uint8_t contradiction_detected;
    uint8_t actuation_authorized;
} guardian_high_compute_fusion_t;

typedef struct
{
    guardian_multinode_node_t self;
    guardian_high_compute_policy_t policy;
    uint32_t accepted_windows;
    uint32_t rejected_windows;
    uint8_t initialized;
} guardian_high_compute_node_t;

void guardian_high_compute_init(
    guardian_high_compute_node_t *node);

guardian_high_compute_result_t guardian_high_compute_configure(
    guardian_high_compute_node_t *node,
    const guardian_multinode_node_t *self,
    const guardian_high_compute_policy_t *policy);

guardian_high_compute_result_t guardian_high_compute_analyze(
    guardian_high_compute_node_t *node,
    const guardian_high_compute_window_t *window,
    guardian_high_compute_analysis_t *analysis);

guardian_high_compute_result_t guardian_high_compute_correlate_and_fuse(
    const guardian_high_compute_node_t *node,
    const guardian_high_compute_window_t *left,
    const guardian_high_compute_window_t *right,
    const guardian_high_compute_analysis_t *left_analysis,
    const guardian_high_compute_analysis_t *right_analysis,
    guardian_high_compute_fusion_t *fusion);

guardian_high_compute_result_t guardian_high_compute_can_actuate(
    const guardian_high_compute_node_t *node,
    uint8_t *allowed);

#endif
