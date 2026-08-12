#ifndef GUARDIAN_HEALTH_H
#define GUARDIAN_HEALTH_H

/* Include M7 DSP feature snapshots consumed by the health model. */
#include "guardian_dsp.h"

/* Include Guardian frame and protocol result types for host commands. */
#include "guardian_protocol.h"

/* Include fixed-width integer types for deterministic model and wire fields. */
#include <stdint.h>

/* Define the first machine-health payload schema revision. */
#define GUARDIAN_HEALTH_SCHEMA_VERSION ((uint8_t)0x01U)

/* Define the fixed GET_HEALTH_STATUS response payload size. */
#define GUARDIAN_HEALTH_STATUS_PAYLOAD_SIZE ((uint16_t)36U)

/* Define the fixed BASELINE_CONTROL request and response payload size. */
#define GUARDIAN_BASELINE_CONTROL_PAYLOAD_SIZE ((uint16_t)4U)

/* Define the minimum explicit baseline sample count. */
#define GUARDIAN_HEALTH_BASELINE_MIN_SAMPLES ((uint16_t)16U)

/* Define the maximum bounded baseline sample count. */
#define GUARDIAN_HEALTH_BASELINE_MAX_SAMPLES ((uint16_t)1024U)

/* Define the number of independent M7 features modeled by M8. */
#define GUARDIAN_HEALTH_FEATURE_COUNT ((uint8_t)7U)

/* Define a sentinel used when no dominant anomaly feature exists. */
#define GUARDIAN_HEALTH_FEATURE_NONE ((uint8_t)0xFFU)

/* Mark a baseline that was learned while M6 reference calibration remained active. */
#define GUARDIAN_HEALTH_QUALITY_REFERENCE_CALIBRATION ((uint16_t)0x0001U)

/* Mark that at least one feature input was rejected for acquisition/DSP quality. */
#define GUARDIAN_HEALTH_QUALITY_INPUT_REJECTED ((uint16_t)0x0002U)

/* Mark that a feature block used a sample rate inconsistent with the baseline. */
#define GUARDIAN_HEALTH_QUALITY_SAMPLE_RATE_MISMATCH ((uint16_t)0x0004U)

/* Mark that the baseline reached its explicit target and is ready for scoring. */
#define GUARDIAN_HEALTH_QUALITY_BASELINE_READY ((uint16_t)0x0008U)

/* Mark that the current feature vector exceeds at least one warning threshold. */
#define GUARDIAN_HEALTH_QUALITY_ANOMALY_PRESENT ((uint16_t)0x0010U)

/* Define explicit M8 model lifecycle and anomaly states. */
typedef enum
{
    /* No baseline has been requested. */
    GUARDIAN_HEALTH_STATE_UNTRAINED = 0,

    /* A bounded explicit baseline is currently being learned. */
    GUARDIAN_HEALTH_STATE_LEARNING = 1,

    /* A trained baseline exists and current input is normal. */
    GUARDIAN_HEALTH_STATE_READY = 2,

    /* Persistent warning-level deviation has been observed. */
    GUARDIAN_HEALTH_STATE_WARNING = 3,

    /* Persistent alarm-level deviation has been observed. */
    GUARDIAN_HEALTH_STATE_ALARM = 4
} guardian_health_state_t;

/* Define explicit baseline-control operations. */
typedef enum
{
    /* Start a fresh bounded baseline learning session. */
    GUARDIAN_BASELINE_ACTION_START = 1,

    /* Erase the runtime baseline and return to UNTRAINED. */
    GUARDIAN_BASELINE_ACTION_RESET = 2
} guardian_baseline_action_t;

/* Identify modeled M7 features for anomaly masks and dominant-feature reporting. */
typedef enum
{
    /* Model vibration AC RMS. */
    GUARDIAN_HEALTH_FEATURE_RMS = 0,

    /* Model vibration crest factor. */
    GUARDIAN_HEALTH_FEATURE_CREST = 1,

    /* Model dominant vibration frequency. */
    GUARDIAN_HEALTH_FEATURE_DOMINANT_FREQUENCY = 2,

    /* Model spectral centroid. */
    GUARDIAN_HEALTH_FEATURE_SPECTRAL_CENTROID = 3,

    /* Model low-band energy share. */
    GUARDIAN_HEALTH_FEATURE_LOW_BAND = 4,

    /* Model middle-band energy share. */
    GUARDIAN_HEALTH_FEATURE_MID_BAND = 5,

    /* Model high-band energy share. */
    GUARDIAN_HEALTH_FEATURE_HIGH_BAND = 6
} guardian_health_feature_t;

/* Store one online baseline statistic using Welford's numerically stable update. */
typedef struct
{
    /* Store the current baseline mean. */
    float mean;

    /* Store the sum of squared deviations required for sample variance. */
    float m2;
} guardian_health_stat_t;

/* Store the host-visible current health snapshot. */
typedef struct
{
    /* Store the current model lifecycle/anomaly state. */
    guardian_health_state_t state;

    /* Store the number of accepted baseline samples. */
    uint16_t baseline_samples;

    /* Store the requested baseline target sample count. */
    uint16_t baseline_target;

    /* Store bounded current anomaly severity from 0 through 1000. */
    uint16_t anomaly_score;

    /* Store inverse bounded health score from 1000 through 0. */
    uint16_t health_score;

    /* Store the largest weighted normalized deviation multiplied by 1000. */
    uint16_t max_deviation_milli;

    /* Store the feature identifier responsible for the largest deviation. */
    uint8_t dominant_feature;

    /* Store the current consecutive warning/alarm deviation count. */
    uint8_t consecutive_anomalous;

    /* Store baseline and input-quality diagnostics. */
    uint16_t quality_flags;

    /* Store the M7 block sequence associated with the current score. */
    uint32_t block_sequence;

    /* Store the current vibration AC RMS in milli-g. */
    uint16_t current_rms_mg;

    /* Store the current crest factor multiplied by 1000. */
    uint16_t current_crest_factor_milli;

    /* Store the current dominant frequency in hundredths of one hertz. */
    uint32_t current_dominant_frequency_centi_hz;

    /* Store the learned RMS baseline mean in milli-g. */
    uint16_t baseline_rms_mean_mg;

    /* Store the learned RMS baseline standard deviation in milli-g. */
    uint16_t baseline_rms_std_mg;

    /* Store one bit per feature whose weighted deviation exceeds warning level. */
    uint16_t exceeded_feature_mask;

    /* Store a saturated count of rejected feature inputs. */
    uint16_t rejected_inputs;
} guardian_health_status_t;

/* Store the complete bounded runtime machine-health model. */
typedef struct
{
    /* Store the current host-visible health snapshot. */
    guardian_health_status_t status;

    /* Store one Welford baseline statistic per modeled M7 feature. */
    guardian_health_stat_t stats[GUARDIAN_HEALTH_FEATURE_COUNT];

    /* Store the physical sample rate captured by the first accepted baseline sample. */
    uint32_t baseline_sample_rate_hz;

    /* Store warning/alarm persistence count. */
    uint8_t anomalous_streak;

    /* Store normal recovery persistence count. */
    uint8_t recovery_streak;

    /* Store the full monotonic rejected-input counter before wire saturation. */
    uint32_t rejected_inputs_total;
} guardian_health_t;

/* Initialize one runtime health model in the UNTRAINED state. */
void guardian_health_init(
    guardian_health_t *health);

/* Start a fresh explicit baseline learning session. */
int guardian_health_start_baseline(
    guardian_health_t *health,
    uint16_t target_samples);

/* Erase runtime baseline and anomaly state. */
void guardian_health_reset(
    guardian_health_t *health);

/* Ingest one successfully analyzed M7 feature snapshot. */
void guardian_health_ingest(
    guardian_health_t *health,
    const guardian_dsp_features_t *features);

/* Return one immutable host-visible health snapshot by value. */
guardian_health_status_t guardian_health_status(
    const guardian_health_t *health);

/* Encode one fixed GET_HEALTH_STATUS payload. */
guardian_protocol_result_t guardian_health_encode_status_payload(
    const guardian_health_status_t *status,
    uint8_t *payload,
    uint16_t payload_capacity,
    uint16_t *payload_length);

/* Process GET_HEALTH_STATUS or BASELINE_CONTROL and build a correlated response. */
guardian_protocol_result_t guardian_health_handle_request(
    guardian_health_t *health,
    const guardian_frame_t *request,
    guardian_frame_t *response);

#endif
