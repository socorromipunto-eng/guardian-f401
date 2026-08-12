/* Include the public M8 machine-health declarations. */
#include "guardian_health.h"

/* Include square root support for baseline standard deviations. */
#include <math.h>

/* Include memory initialization support for deterministic resets. */
#include <string.h>

/* Define the weighted normalized deviation that begins warning qualification. */
#define GUARDIAN_HEALTH_WARNING_Z (3.0F)

/* Define the weighted normalized deviation that begins alarm qualification. */
#define GUARDIAN_HEALTH_ALARM_Z (6.0F)

/* Define the lower recovery threshold used for warning/alarm hysteresis. */
#define GUARDIAN_HEALTH_RECOVERY_Z (2.0F)

/* Require this many warning-level blocks before WARNING latches. */
#define GUARDIAN_HEALTH_WARNING_STREAK ((uint8_t)3U)

/* Require this many alarm-level blocks before ALARM latches. */
#define GUARDIAN_HEALTH_ALARM_STREAK ((uint8_t)2U)

/* Require this many normal blocks before WARNING or ALARM returns to READY. */
#define GUARDIAN_HEALTH_RECOVERY_STREAK ((uint8_t)5U)

/* Define acquisition-quality conditions that invalidate health-model input. */
#define GUARDIAN_HEALTH_REJECTED_ACQUISITION_FLAGS \
    (GUARDIAN_ACQUISITION_STATUS_ADC_SATURATED | \
     GUARDIAN_ACQUISITION_STATUS_DMA_DROP | \
     GUARDIAN_ACQUISITION_STATUS_ADC_ERROR | \
     GUARDIAN_ACQUISITION_STATUS_DMA_ERROR)

/* Store per-feature minimum standard deviations that prevent zero-variance overreaction. */
static const float guardian_health_sigma_floor[GUARDIAN_HEALTH_FEATURE_COUNT] =
{
    /* RMS floor: 5 milli-g. */
    5.0F,

    /* Crest-factor floor: 0.050 represented as 50 milli-units. */
    50.0F,

    /* Dominant-frequency floor: 5 Hz represented as 500 centi-hertz. */
    500.0F,

    /* Spectral-centroid floor: 5 Hz represented as 500 centi-hertz. */
    500.0F,

    /* Low-band energy-share floor: 10 permille. */
    10.0F,

    /* Mid-band energy-share floor: 10 permille. */
    10.0F,

    /* High-band energy-share floor: 10 permille. */
    10.0F
};

/* Store feature weights used only for ranking machine-health deviations. */
static const float guardian_health_weight[GUARDIAN_HEALTH_FEATURE_COUNT] =
{
    /* Give vibration RMS the strongest direct severity weight. */
    1.50F,

    /* Give crest factor additional impulsiveness sensitivity. */
    1.20F,

    /* Preserve full weight for dominant-frequency drift. */
    1.00F,

    /* Preserve slightly lower weight for centroid drift. */
    0.90F,

    /* Preserve balanced spectral-band redistribution sensitivity. */
    0.80F,

    /* Preserve balanced spectral-band redistribution sensitivity. */
    0.80F,

    /* Preserve balanced spectral-band redistribution sensitivity. */
    0.80F
};

/* Saturating-increment one unsigned 32-bit diagnostic counter. */
static void guardian_health_increment_u32(
    uint32_t *value)
{
    /* Ignore missing storage defensively. */
    if (value == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Avoid diagnostic counter wrap. */
    if (*value != 0xFFFFFFFFUL)
    {
        /* Increment only while representable. */
        *value += 1U;
    }
}

/* Saturate one non-negative floating-point value to unsigned 16-bit. */
static uint16_t guardian_health_u16(
    float value)
{
    /* Clamp non-positive and unordered values to zero. */
    if (!(value > 0.0F))
    {
        /* Return the lower bound. */
        return 0U;
    }

    /* Clamp values beyond the wire-field maximum. */
    if (value >= 65535.0F)
    {
        /* Return the upper bound. */
        return 65535U;
    }

    /* Round to nearest integer. */
    return (uint16_t)(value + 0.5F);
}

/* Return absolute floating-point magnitude without depending on fabsf. */
static float guardian_health_abs(
    float value)
{
    /* Negate negative values only. */
    return (value < 0.0F)
        ? -value
        : value;
}

/* Convert one DSP snapshot into the fixed seven-feature model vector. */
static void guardian_health_vector(
    const guardian_dsp_features_t *features,
    float *vector)
{
    /* Map AC RMS directly. */
    vector[GUARDIAN_HEALTH_FEATURE_RMS] =
        (float)features->rms_mg;

    /* Map fixed-point crest factor directly. */
    vector[GUARDIAN_HEALTH_FEATURE_CREST] =
        (float)features->crest_factor_milli;

    /* Map dominant frequency in centi-hertz. */
    vector[GUARDIAN_HEALTH_FEATURE_DOMINANT_FREQUENCY] =
        (float)features->dominant_frequency_centi_hz;

    /* Map spectral centroid in centi-hertz. */
    vector[GUARDIAN_HEALTH_FEATURE_SPECTRAL_CENTROID] =
        (float)features->spectral_centroid_centi_hz;

    /* Map low-band energy share. */
    vector[GUARDIAN_HEALTH_FEATURE_LOW_BAND] =
        (float)features->low_band_permille;

    /* Map middle-band energy share. */
    vector[GUARDIAN_HEALTH_FEATURE_MID_BAND] =
        (float)features->mid_band_permille;

    /* Map high-band energy share. */
    vector[GUARDIAN_HEALTH_FEATURE_HIGH_BAND] =
        (float)features->high_band_permille;
}

/* Return whether one DSP snapshot is trustworthy enough for baseline or scoring. */
static int guardian_health_input_valid(
    const guardian_dsp_features_t *features)
{
    /* Reject a missing feature pointer. */
    if (features == NULL)
    {
        /* Report unusable input. */
        return 0;
    }

    /* Require successful M7 DSP analysis. */
    if ((features->dsp_status_flags &
         GUARDIAN_DSP_STATUS_VALID) == 0U)
    {
        /* Reject incomplete DSP output. */
        return 0;
    }

    /* Require successful M6 acquisition conversion. */
    if ((features->acquisition_status_flags &
         GUARDIAN_ACQUISITION_STATUS_VALID) == 0U)
    {
        /* Reject unvalidated acquisition input. */
        return 0;
    }

    /* Reject saturation, dropped DMA blocks and explicit ADC/DMA failures. */
    if ((features->acquisition_status_flags &
         GUARDIAN_HEALTH_REJECTED_ACQUISITION_FLAGS) != 0U)
    {
        /* Reject compromised input quality. */
        return 0;
    }

    /* Require a physical sample rate for frequency features. */
    if (features->sample_rate_hz == 0U)
    {
        /* Reject semantically incomplete features. */
        return 0;
    }

    /* Report trustworthy input. */
    return 1;
}

/* Return one sample standard deviation with the configured feature floor. */
static float guardian_health_sigma(
    const guardian_health_t *health,
    uint8_t feature)
{
    /* Start with the feature-specific minimum denominator. */
    float sigma =
        guardian_health_sigma_floor[feature];

    /* Calculate sample variance only after at least two accepted baseline samples. */
    if (health->status.baseline_samples > 1U)
    {
        /* Calculate Welford sample variance. */
        float variance =
            health->stats[feature].m2 /
            (float)(health->status.baseline_samples - 1U);

        /* Protect square root from tiny negative floating-point roundoff. */
        if (variance > 0.0F)
        {
            /* Convert variance into standard deviation. */
            float candidate =
                sqrtf(variance);

            /* Use measured variability only when it exceeds the safety floor. */
            if (candidate > sigma)
            {
                /* Preserve the larger denominator. */
                sigma = candidate;
            }
        }
    }

    /* Return the effective deviation denominator. */
    return sigma;
}

/* Refresh host-visible learned RMS summary fields. */
static void guardian_health_update_baseline_summary(
    guardian_health_t *health)
{
    /* Publish zero summary before any accepted baseline sample. */
    if (health->status.baseline_samples == 0U)
    {
        /* Clear the learned RMS mean. */
        health->status.baseline_rms_mean_mg = 0U;

        /* Clear the learned RMS standard deviation. */
        health->status.baseline_rms_std_mg = 0U;

        /* Return without variance calculation. */
        return;
    }

    /* Publish the learned RMS mean. */
    health->status.baseline_rms_mean_mg =
        guardian_health_u16(
            health->stats[
                GUARDIAN_HEALTH_FEATURE_RMS
            ].mean);

    /* Publish the effective learned RMS standard deviation. */
    health->status.baseline_rms_std_mg =
        guardian_health_u16(
            guardian_health_sigma(
                health,
                GUARDIAN_HEALTH_FEATURE_RMS));
}

/* Reset runtime model state while preserving no previous baseline. */
void guardian_health_reset(
    guardian_health_t *health)
{
    /* Ignore a missing model pointer defensively. */
    if (health == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Clear every runtime statistic and diagnostic. */
    (void)memset(
        health,
        0,
        sizeof(*health));

    /* Publish the explicit initial model state. */
    health->status.state =
        GUARDIAN_HEALTH_STATE_UNTRAINED;

    /* Publish a perfect neutral score before a baseline exists. */
    health->status.health_score = 1000U;

    /* Publish no dominant anomaly feature. */
    health->status.dominant_feature =
        GUARDIAN_HEALTH_FEATURE_NONE;
}

/* Initialize one runtime health model. */
void guardian_health_init(
    guardian_health_t *health)
{
    /* Reuse the deterministic complete reset path. */
    guardian_health_reset(
        health);
}

/* Start a fresh explicit baseline learning session. */
int guardian_health_start_baseline(
    guardian_health_t *health,
    uint16_t target_samples)
{
    /* Reject a missing model pointer. */
    if (health == NULL)
    {
        /* Report invalid caller state. */
        return 0;
    }

    /* Enforce the bounded explicit baseline sample-count policy. */
    if ((target_samples <
         GUARDIAN_HEALTH_BASELINE_MIN_SAMPLES) ||
        (target_samples >
         GUARDIAN_HEALTH_BASELINE_MAX_SAMPLES))
    {
        /* Report invalid baseline configuration. */
        return 0;
    }

    /* Erase every previous model, score and streak. */
    guardian_health_reset(
        health);

    /* Enter explicit baseline learning. */
    health->status.state =
        GUARDIAN_HEALTH_STATE_LEARNING;

    /* Preserve the requested target sample count. */
    health->status.baseline_target =
        target_samples;

    /* Report accepted baseline configuration. */
    return 1;
}

/* Record one rejected feature input without disturbing a trained anomaly state. */
static void guardian_health_reject_input(
    guardian_health_t *health,
    uint16_t quality_flag)
{
    /* Saturating-increment the full internal rejection counter. */
    guardian_health_increment_u32(
        &health->rejected_inputs_total);

    /* Saturate the host-visible 16-bit rejection count. */
    health->status.rejected_inputs =
        (health->rejected_inputs_total > 65535UL)
        ? 65535U
        : (uint16_t)health->rejected_inputs_total;

    /* Preserve the caller-selected quality reason. */
    health->status.quality_flags |=
        quality_flag;
}

/* Learn one accepted feature vector using Welford updates. */
static void guardian_health_learn(
    guardian_health_t *health,
    const guardian_dsp_features_t *features)
{
    /* Store the fixed seven-feature model vector. */
    float vector[GUARDIAN_HEALTH_FEATURE_COUNT] = {0};

    /* Track the current modeled feature. */
    uint8_t feature = 0U;

    /* Convert the current DSP snapshot into scalar model features. */
    guardian_health_vector(
        features,
        vector);

    /* Capture the baseline sample rate from the first accepted sample. */
    if (health->status.baseline_samples == 0U)
    {
        /* Preserve physical sample rate for future consistency checks. */
        health->baseline_sample_rate_hz =
            features->sample_rate_hz;
    }

    /* Calculate the accepted baseline sample count after this update. */
    uint16_t next_count =
        (uint16_t)(
            health->status.baseline_samples +
            1U
        );

    /* Update each independent feature statistic. */
    for (feature = 0U;
         feature < GUARDIAN_HEALTH_FEATURE_COUNT;
         ++feature)
    {
        /* Calculate deviation from the previous mean. */
        float delta =
            vector[feature] -
            health->stats[feature].mean;

        /* Update the running mean. */
        health->stats[feature].mean +=
            delta /
            (float)next_count;

        /* Calculate deviation from the updated mean. */
        float delta_after =
            vector[feature] -
            health->stats[feature].mean;

        /* Update Welford's squared-deviation accumulator. */
        health->stats[feature].m2 +=
            delta *
            delta_after;
    }

    /* Publish the accepted baseline sample count. */
    health->status.baseline_samples =
        next_count;

    /* Remember when baseline training used M6 reference external calibration. */
    if ((features->acquisition_status_flags &
         GUARDIAN_ACQUISITION_STATUS_DEFAULT_CALIBRATION) != 0U)
    {
        /* Preserve the calibration-quality caveat. */
        health->status.quality_flags |=
            GUARDIAN_HEALTH_QUALITY_REFERENCE_CALIBRATION;
    }

    /* Publish the latest accepted learning block for operator visibility. */
    health->status.block_sequence =
        features->block_sequence;

    /* Publish current RMS while baseline learning is active. */
    health->status.current_rms_mg =
        features->rms_mg;

    /* Publish current crest factor while baseline learning is active. */
    health->status.current_crest_factor_milli =
        features->crest_factor_milli;

    /* Publish current dominant frequency while baseline learning is active. */
    health->status.current_dominant_frequency_centi_hz =
        features->dominant_frequency_centi_hz;

    /* Refresh host-visible learned RMS summary values. */
    guardian_health_update_baseline_summary(
        health);

    /* Finalize the baseline automatically when its explicit target is reached. */
    if (health->status.baseline_samples >=
        health->status.baseline_target)
    {
        /* Enter normal trained monitoring. */
        health->status.state =
            GUARDIAN_HEALTH_STATE_READY;

        /* Publish baseline readiness. */
        health->status.quality_flags |=
            GUARDIAN_HEALTH_QUALITY_BASELINE_READY;

        /* Start trained monitoring from a neutral anomaly score. */
        health->status.anomaly_score = 0U;

        /* Start trained monitoring from maximum health score. */
        health->status.health_score = 1000U;

        /* Publish no dominant anomaly feature yet. */
        health->status.dominant_feature =
            GUARDIAN_HEALTH_FEATURE_NONE;
    }
}

/* Score one accepted feature snapshot against the frozen learned baseline. */
static void guardian_health_score(
    guardian_health_t *health,
    const guardian_dsp_features_t *features)
{
    /* Store the fixed seven-feature current vector. */
    float vector[GUARDIAN_HEALTH_FEATURE_COUNT] = {0};

    /* Store the largest weighted normalized deviation. */
    float maximum_weighted_z = 0.0F;

    /* Store the feature producing the largest weighted deviation. */
    uint8_t dominant_feature =
        GUARDIAN_HEALTH_FEATURE_NONE;

    /* Store the current warning-threshold feature mask. */
    uint16_t exceeded_mask = 0U;

    /* Track the current modeled feature. */
    uint8_t feature = 0U;

    /* Convert the DSP snapshot into scalar model features. */
    guardian_health_vector(
        features,
        vector);

    /* Evaluate every modeled feature independently. */
    for (feature = 0U;
         feature < GUARDIAN_HEALTH_FEATURE_COUNT;
         ++feature)
    {
        /* Calculate absolute deviation from the learned baseline mean. */
        float deviation =
            guardian_health_abs(
                vector[feature] -
                health->stats[feature].mean);

        /* Convert deviation into a normalized z-like distance using the variance floor. */
        float normalized =
            deviation /
            guardian_health_sigma(
                health,
                feature);

        /* Apply the feature-specific health-ranking weight. */
        float weighted =
            normalized *
            guardian_health_weight[feature];

        /* Mark features whose weighted distance reaches warning level. */
        if (weighted >=
            GUARDIAN_HEALTH_WARNING_Z)
        {
            /* Publish one bit per exceeded feature. */
            exceeded_mask |=
                (uint16_t)(
                    (uint16_t)1U <<
                    feature
                );
        }

        /* Preserve the largest weighted deviation. */
        if (weighted >
            maximum_weighted_z)
        {
            /* Update the current worst feature distance. */
            maximum_weighted_z =
                weighted;

            /* Preserve the corresponding feature identifier. */
            dominant_feature =
                feature;
        }
    }

    /* Preserve the current M7 block identity. */
    health->status.block_sequence =
        features->block_sequence;

    /* Publish current directly interpretable vibration features. */
    health->status.current_rms_mg =
        features->rms_mg;

    /* Publish current fixed-point crest factor. */
    health->status.current_crest_factor_milli =
        features->crest_factor_milli;

    /* Publish current dominant frequency. */
    health->status.current_dominant_frequency_centi_hz =
        features->dominant_frequency_centi_hz;

    /* Publish the warning-threshold feature mask. */
    health->status.exceeded_feature_mask =
        exceeded_mask;

    /* Publish the current worst feature identifier. */
    health->status.dominant_feature =
        dominant_feature;

    /* Publish the largest weighted normalized deviation in milli-units. */
    health->status.max_deviation_milli =
        guardian_health_u16(
            maximum_weighted_z *
            1000.0F);

    /* Map six weighted sigma units to the maximum anomaly score. */
    float anomaly_score =
        (
            maximum_weighted_z /
            GUARDIAN_HEALTH_ALARM_Z
        ) *
        1000.0F;

    /* Clamp anomaly severity to its published 0-1000 range. */
    if (anomaly_score > 1000.0F)
    {
        /* Saturate severe anomalies. */
        anomaly_score = 1000.0F;
    }

    /* Publish bounded anomaly severity. */
    health->status.anomaly_score =
        guardian_health_u16(
            anomaly_score);

    /* Publish inverse bounded health score. */
    health->status.health_score =
        (uint16_t)(
            1000U -
            health->status.anomaly_score
        );

    /* Publish current anomaly-presence quality state. */
    if (maximum_weighted_z >=
        GUARDIAN_HEALTH_WARNING_Z)
    {
        /* Mark current deviation as anomalous. */
        health->status.quality_flags |=
            GUARDIAN_HEALTH_QUALITY_ANOMALY_PRESENT;
    }
    else
    {
        /* Clear only the current anomaly-presence bit. */
        health->status.quality_flags &=
            (uint16_t)~GUARDIAN_HEALTH_QUALITY_ANOMALY_PRESENT;
    }

    /* Handle alarm-level persistence first. */
    if (maximum_weighted_z >=
        GUARDIAN_HEALTH_ALARM_Z)
    {
        /* Reset recovery qualification. */
        health->recovery_streak = 0U;

        /* Saturating-increment anomaly persistence. */
        if (health->anomalous_streak != 255U)
        {
            /* Advance the anomaly streak. */
            health->anomalous_streak += 1U;
        }

        /* Enter ALARM after the shorter severe-deviation persistence threshold. */
        if (health->anomalous_streak >=
            GUARDIAN_HEALTH_ALARM_STREAK)
        {
            /* Latch the severe machine-health state. */
            health->status.state =
                GUARDIAN_HEALTH_STATE_ALARM;
        }
        else if (health->status.state ==
                 GUARDIAN_HEALTH_STATE_READY)
        {
            /* Escalate immediately to WARNING while alarm persistence is qualifying. */
            health->status.state =
                GUARDIAN_HEALTH_STATE_WARNING;
        }
    }
    else if (maximum_weighted_z >=
             GUARDIAN_HEALTH_WARNING_Z)
    {
        /* Reset recovery qualification. */
        health->recovery_streak = 0U;

        /* Saturating-increment anomaly persistence. */
        if (health->anomalous_streak != 255U)
        {
            /* Advance the anomaly streak. */
            health->anomalous_streak += 1U;
        }

        /* Enter WARNING after persistent warning-level deviation. */
        if ((health->anomalous_streak >=
             GUARDIAN_HEALTH_WARNING_STREAK) &&
            (health->status.state !=
             GUARDIAN_HEALTH_STATE_ALARM))
        {
            /* Publish warning machine-health state. */
            health->status.state =
                GUARDIAN_HEALTH_STATE_WARNING;
        }
    }
    else
    {
        /* Clear anomaly qualification below warning threshold. */
        health->anomalous_streak = 0U;

        /* Qualify recovery only below the lower hysteresis threshold. */
        if (maximum_weighted_z <
            GUARDIAN_HEALTH_RECOVERY_Z)
        {
            /* Saturating-increment normal recovery persistence. */
            if (health->recovery_streak != 255U)
            {
                /* Advance normal recovery qualification. */
                health->recovery_streak += 1U;
            }

            /* Return WARNING or ALARM to READY only after sustained normal input. */
            if ((health->recovery_streak >=
                 GUARDIAN_HEALTH_RECOVERY_STREAK) &&
                ((health->status.state ==
                  GUARDIAN_HEALTH_STATE_WARNING) ||
                 (health->status.state ==
                  GUARDIAN_HEALTH_STATE_ALARM)))
            {
                /* Publish recovered normal monitoring state. */
                health->status.state =
                    GUARDIAN_HEALTH_STATE_READY;
            }
        }
        else
        {
            /* Reset recovery qualification inside the hysteresis band. */
            health->recovery_streak = 0U;
        }
    }

    /* Publish the current anomaly streak for diagnostics. */
    health->status.consecutive_anomalous =
        health->anomalous_streak;
}

/* Ingest one successfully analyzed M7 feature snapshot. */
void guardian_health_ingest(
    guardian_health_t *health,
    const guardian_dsp_features_t *features)
{
    /* Ignore a missing model pointer. */
    if (health == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Reject compromised or incomplete feature input. */
    if (guardian_health_input_valid(features) == 0)
    {
        /* Record the input-quality failure. */
        guardian_health_reject_input(
            health,
            GUARDIAN_HEALTH_QUALITY_INPUT_REJECTED);

        /* Return without contaminating baseline or score. */
        return;
    }

    /* Enforce sample-rate consistency after the first accepted baseline sample. */
    if ((health->baseline_sample_rate_hz != 0U) &&
        (features->sample_rate_hz !=
         health->baseline_sample_rate_hz))
    {
        /* Record sample-rate mismatch separately. */
        guardian_health_reject_input(
            health,
            GUARDIAN_HEALTH_QUALITY_INPUT_REJECTED |
            GUARDIAN_HEALTH_QUALITY_SAMPLE_RATE_MISMATCH);

        /* Return without comparing incompatible frequency features. */
        return;
    }

    /* Learn only during an explicit bounded baseline session. */
    if (health->status.state ==
        GUARDIAN_HEALTH_STATE_LEARNING)
    {
        /* Add this trustworthy sample to the online baseline. */
        guardian_health_learn(
            health,
            features);

        /* Return because learning samples are not scored as anomalies. */
        return;
    }

    /* Score only after a complete baseline exists. */
    if ((health->status.state ==
         GUARDIAN_HEALTH_STATE_READY) ||
        (health->status.state ==
         GUARDIAN_HEALTH_STATE_WARNING) ||
        (health->status.state ==
         GUARDIAN_HEALTH_STATE_ALARM))
    {
        /* Compare the current feature vector with the frozen baseline. */
        guardian_health_score(
            health,
            features);
    }
}

/* Return one immutable host-visible health snapshot by value. */
guardian_health_status_t guardian_health_status(
    const guardian_health_t *health)
{
    /* Create deterministic empty status for a missing model pointer. */
    guardian_health_status_t empty = {0};

    /* Reject a missing model pointer safely. */
    if (health == NULL)
    {
        /* Publish the explicit untrained state. */
        empty.state =
            GUARDIAN_HEALTH_STATE_UNTRAINED;

        /* Return deterministic empty status. */
        return empty;
    }

    /* Return the current bounded status by value. */
    return health->status;
}

/* Write one unsigned 16-bit integer using Guardian big-endian wire order. */
static void guardian_health_write_u16_be(
    uint8_t *output,
    uint16_t value)
{
    /* Write most-significant byte first. */
    output[0] =
        (uint8_t)((value >> 8U) & 0xFFU);

    /* Write least-significant byte second. */
    output[1] =
        (uint8_t)(value & 0xFFU);
}

/* Write one unsigned 32-bit integer using Guardian big-endian wire order. */
static void guardian_health_write_u32_be(
    uint8_t *output,
    uint32_t value)
{
    /* Write byte 3 first. */
    output[0] =
        (uint8_t)((value >> 24U) & 0xFFU);

    /* Write byte 2. */
    output[1] =
        (uint8_t)((value >> 16U) & 0xFFU);

    /* Write byte 1. */
    output[2] =
        (uint8_t)((value >> 8U) & 0xFFU);

    /* Write byte 0 last. */
    output[3] =
        (uint8_t)(value & 0xFFU);
}

/* Read one unsigned 16-bit integer using Guardian big-endian wire order. */
static uint16_t guardian_health_read_u16_be(
    const uint8_t *input)
{
    /* Combine the two wire bytes explicitly. */
    return (uint16_t)(
        ((uint16_t)input[0] << 8U) |
        (uint16_t)input[1]
    );
}

/* Build one request-correlated Guardian ERROR frame. */
static void guardian_health_make_error(
    const guardian_frame_t *request,
    guardian_frame_t *response,
    guardian_error_code_t error)
{
    /* Publish ERROR message semantics. */
    response->message_type =
        GUARDIAN_MESSAGE_ERROR;

    /* Preserve the original command identifier. */
    response->command =
        request->command;

    /* Use the only flags value defined by protocol v0.1. */
    response->flags =
        GUARDIAN_SUPPORTED_FLAGS;

    /* Preserve request/response correlation. */
    response->sequence =
        request->sequence;

    /* Publish the frozen one-byte error payload. */
    response->payload_length = 1U;

    /* Store the selected Guardian error code. */
    response->payload[0] =
        (uint8_t)error;
}

/* Encode one fixed GET_HEALTH_STATUS payload. */
guardian_protocol_result_t guardian_health_encode_status_payload(
    const guardian_health_status_t *status,
    uint8_t *payload,
    uint16_t payload_capacity,
    uint16_t *payload_length)
{
    /* Reject missing required storage. */
    if ((status == NULL) ||
        (payload == NULL) ||
        (payload_length == NULL))
    {
        /* Report canonical missing-argument failure. */
        return GUARDIAN_PROTOCOL_ERROR_NULL_ARGUMENT;
    }

    /* Require the complete fixed schema output capacity. */
    if (payload_capacity <
        GUARDIAN_HEALTH_STATUS_PAYLOAD_SIZE)
    {
        /* Report bounded output failure. */
        return GUARDIAN_PROTOCOL_ERROR_OUTPUT_TOO_SMALL;
    }

    /* Publish schema revision one. */
    payload[0] =
        GUARDIAN_HEALTH_SCHEMA_VERSION;

    /* Publish model state. */
    payload[1] =
        (uint8_t)status->state;

    /* Encode accepted baseline sample count. */
    guardian_health_write_u16_be(
        &payload[2],
        status->baseline_samples);

    /* Encode requested baseline target. */
    guardian_health_write_u16_be(
        &payload[4],
        status->baseline_target);

    /* Encode anomaly severity. */
    guardian_health_write_u16_be(
        &payload[6],
        status->anomaly_score);

    /* Encode inverse health score. */
    guardian_health_write_u16_be(
        &payload[8],
        status->health_score);

    /* Encode largest weighted normalized deviation. */
    guardian_health_write_u16_be(
        &payload[10],
        status->max_deviation_milli);

    /* Publish dominant anomaly feature identifier. */
    payload[12] =
        status->dominant_feature;

    /* Publish current consecutive anomaly count. */
    payload[13] =
        status->consecutive_anomalous;

    /* Encode health-model quality flags. */
    guardian_health_write_u16_be(
        &payload[14],
        status->quality_flags);

    /* Encode current M7 block sequence. */
    guardian_health_write_u32_be(
        &payload[16],
        status->block_sequence);

    /* Encode current RMS vibration. */
    guardian_health_write_u16_be(
        &payload[20],
        status->current_rms_mg);

    /* Encode current crest factor. */
    guardian_health_write_u16_be(
        &payload[22],
        status->current_crest_factor_milli);

    /* Encode current dominant frequency. */
    guardian_health_write_u32_be(
        &payload[24],
        status->current_dominant_frequency_centi_hz);

    /* Encode learned RMS baseline mean. */
    guardian_health_write_u16_be(
        &payload[28],
        status->baseline_rms_mean_mg);

    /* Encode learned RMS baseline standard deviation. */
    guardian_health_write_u16_be(
        &payload[30],
        status->baseline_rms_std_mg);

    /* Encode warning-threshold feature mask. */
    guardian_health_write_u16_be(
        &payload[32],
        status->exceeded_feature_mask);

    /* Encode saturated rejected-input count. */
    guardian_health_write_u16_be(
        &payload[34],
        status->rejected_inputs);

    /* Publish the exact fixed schema length. */
    *payload_length =
        GUARDIAN_HEALTH_STATUS_PAYLOAD_SIZE;

    /* Report successful serialization. */
    return GUARDIAN_PROTOCOL_OK;
}

/* Process GET_HEALTH_STATUS or BASELINE_CONTROL. */
guardian_protocol_result_t guardian_health_handle_request(
    guardian_health_t *health,
    const guardian_frame_t *request,
    guardian_frame_t *response)
{
    /* Reject missing required storage. */
    if ((health == NULL) ||
        (request == NULL) ||
        (response == NULL))
    {
        /* Report canonical missing-argument failure. */
        return GUARDIAN_PROTOCOL_ERROR_NULL_ARGUMENT;
    }

    /* Require host-to-device request semantics. */
    if (request->message_type !=
        GUARDIAN_MESSAGE_REQUEST)
    {
        /* Reject contradictory message class. */
        guardian_health_make_error(
            request,
            response,
            GUARDIAN_ERROR_MALFORMED_FRAME);

        /* Report successful ERROR frame construction. */
        return GUARDIAN_PROTOCOL_OK;
    }

    /* Handle GET_HEALTH_STATUS. */
    if (request->command ==
        (uint8_t)GUARDIAN_COMMAND_GET_HEALTH_STATUS)
    {
        /* Require the frozen empty request payload. */
        if (request->payload_length != 0U)
        {
            /* Reject undefined health-query bytes. */
            guardian_health_make_error(
                request,
                response,
                GUARDIAN_ERROR_INVALID_PAYLOAD);

            /* Report successful ERROR frame construction. */
            return GUARDIAN_PROTOCOL_OK;
        }

        /* Publish successful response semantics. */
        response->message_type =
            GUARDIAN_MESSAGE_RESPONSE;

        /* Preserve command identifier. */
        response->command =
            request->command;

        /* Use protocol v0.1 flags. */
        response->flags =
            GUARDIAN_SUPPORTED_FLAGS;

        /* Preserve correlation sequence. */
        response->sequence =
            request->sequence;

        /* Encode the current health snapshot directly into response storage. */
        return guardian_health_encode_status_payload(
            &health->status,
            response->payload,
            GUARDIAN_MAX_PAYLOAD_SIZE,
            &response->payload_length);
    }

    /* Handle explicit baseline lifecycle control. */
    if (request->command ==
        (uint8_t)GUARDIAN_COMMAND_BASELINE_CONTROL)
    {
        /* Require the exact fixed baseline-control payload. */
        if (request->payload_length !=
            GUARDIAN_BASELINE_CONTROL_PAYLOAD_SIZE)
        {
            /* Reject truncated or trailing baseline-control bytes. */
            guardian_health_make_error(
                request,
                response,
                GUARDIAN_ERROR_INVALID_PAYLOAD);

            /* Report successful ERROR frame construction. */
            return GUARDIAN_PROTOCOL_OK;
        }

        /* Require schema revision one. */
        if (request->payload[0] !=
            GUARDIAN_HEALTH_SCHEMA_VERSION)
        {
            /* Reject unsupported control semantics. */
            guardian_health_make_error(
                request,
                response,
                GUARDIAN_ERROR_INVALID_PAYLOAD);

            /* Report successful ERROR frame construction. */
            return GUARDIAN_PROTOCOL_OK;
        }

        /* Decode the explicit baseline action identifier. */
        uint8_t action =
            request->payload[1];

        /* Decode the bounded target sample count. */
        uint16_t target_samples =
            guardian_health_read_u16_be(
                &request->payload[2]);

        /* Start a fresh baseline when requested. */
        if (action ==
            (uint8_t)GUARDIAN_BASELINE_ACTION_START)
        {
            /* Validate and apply the bounded baseline target. */
            if (guardian_health_start_baseline(
                    health,
                    target_samples) == 0)
            {
                /* Reject an out-of-policy sample count. */
                guardian_health_make_error(
                    request,
                    response,
                    GUARDIAN_ERROR_INVALID_PAYLOAD);

                /* Report successful ERROR frame construction. */
                return GUARDIAN_PROTOCOL_OK;
            }
        }
        else if (action ==
                 (uint8_t)GUARDIAN_BASELINE_ACTION_RESET)
        {
            /* Require the reset action to carry a zero target field. */
            if (target_samples != 0U)
            {
                /* Reject contradictory reset semantics. */
                guardian_health_make_error(
                    request,
                    response,
                    GUARDIAN_ERROR_INVALID_PAYLOAD);

                /* Report successful ERROR frame construction. */
                return GUARDIAN_PROTOCOL_OK;
            }

            /* Erase the runtime baseline and anomaly state. */
            guardian_health_reset(
                health);
        }
        else
        {
            /* Reject undefined baseline actions. */
            guardian_health_make_error(
                request,
                response,
                GUARDIAN_ERROR_INVALID_PAYLOAD);

            /* Report successful ERROR frame construction. */
            return GUARDIAN_PROTOCOL_OK;
        }

        /* Build a successful normalized control response. */
        response->message_type =
            GUARDIAN_MESSAGE_RESPONSE;

        /* Preserve the baseline-control command identifier. */
        response->command =
            request->command;

        /* Use protocol v0.1 flags. */
        response->flags =
            GUARDIAN_SUPPORTED_FLAGS;

        /* Preserve request/response correlation. */
        response->sequence =
            request->sequence;

        /* Echo the supported schema version. */
        response->payload[0] =
            GUARDIAN_HEALTH_SCHEMA_VERSION;

        /* Echo the normalized action. */
        response->payload[1] =
            action;

        /* Echo the normalized active target for START or zero for RESET. */
        guardian_health_write_u16_be(
            &response->payload[2],
            (action ==
             (uint8_t)GUARDIAN_BASELINE_ACTION_START)
            ? health->status.baseline_target
            : 0U);

        /* Publish the fixed control payload length. */
        response->payload_length =
            GUARDIAN_BASELINE_CONTROL_PAYLOAD_SIZE;

        /* Report successful response construction. */
        return GUARDIAN_PROTOCOL_OK;
    }

    /* Reject accidental dispatch of another command. */
    guardian_health_make_error(
        request,
        response,
        GUARDIAN_ERROR_UNKNOWN_COMMAND);

    /* Report successful ERROR frame construction. */
    return GUARDIAN_PROTOCOL_OK;
}
