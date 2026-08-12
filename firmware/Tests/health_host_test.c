/* Include the portable M8 machine-health engine under test. */
#include "guardian_health.h"

/* Include assertion support for deterministic host verification. */
#include <assert.h>

/* Include standard output for one concise CI success line. */
#include <stdio.h>

/* Build one trustworthy deterministic M7 feature snapshot. */
static guardian_dsp_features_t test_features(
    uint32_t sequence,
    uint16_t rms_mg)
{
    /* Create deterministic zero-initialized feature storage. */
    guardian_dsp_features_t features = {0};

    /* Publish the supplied acquisition block sequence. */
    features.block_sequence = sequence;

    /* Match the M6 reference physical sample rate. */
    features.sample_rate_hz = 4000U;

    /* Publish caller-selected vibration RMS. */
    features.rms_mg = rms_mg;

    /* Publish a stable time-domain peak. */
    features.peak_mg = 61U;

    /* Publish a stable crest factor. */
    features.crest_factor_milli = 1450U;

    /* Publish a stable 250 Hz dominant component. */
    features.dominant_frequency_centi_hz = 25000UL;

    /* Publish a stable dominant peak amplitude. */
    features.dominant_peak_mg = 58U;

    /* Publish a stable 400 Hz spectral centroid. */
    features.spectral_centroid_centi_hz = 40000UL;

    /* Publish a stable low-band energy share. */
    features.low_band_permille = 800U;

    /* Publish a stable middle-band energy share. */
    features.mid_band_permille = 150U;

    /* Publish a stable high-band energy share. */
    features.high_band_permille = 50U;

    /* Mark the source acquisition valid while preserving reference calibration. */
    features.acquisition_status_flags =
        GUARDIAN_ACQUISITION_STATUS_VALID |
        GUARDIAN_ACQUISITION_STATUS_DEFAULT_CALIBRATION;

    /* Mark the DSP analysis valid with its documented processing stages. */
    features.dsp_status_flags =
        GUARDIAN_DSP_STATUS_VALID |
        GUARDIAN_DSP_STATUS_DC_REMOVED |
        GUARDIAN_DSP_STATUS_HANN_WINDOW |
        GUARDIAN_DSP_STATUS_DOMINANT_VALID;

    /* Return the complete trustworthy feature snapshot. */
    return features;
}

/* Learn the minimum stable baseline and require READY state. */
static void test_baseline_learning(void)
{
    /* Create one fresh runtime model. */
    guardian_health_t health = {0};

    /* Track the accepted baseline sample. */
    uint16_t sample = 0U;

    /* Initialize the model explicitly. */
    guardian_health_init(
        &health);

    /* Require initial UNTRAINED state. */
    assert(
        health.status.state ==
        GUARDIAN_HEALTH_STATE_UNTRAINED);

    /* Start the minimum bounded baseline. */
    assert(
        guardian_health_start_baseline(
            &health,
            GUARDIAN_HEALTH_BASELINE_MIN_SAMPLES) == 1);

    /* Feed exactly the requested number of healthy feature vectors. */
    for (sample = 0U;
         sample < GUARDIAN_HEALTH_BASELINE_MIN_SAMPLES;
         ++sample)
    {
        /* Build one stable healthy feature vector. */
        guardian_dsp_features_t features =
            test_features(
                (uint32_t)sample + 1U,
                40U);

        /* Add the trustworthy vector to the baseline. */
        guardian_health_ingest(
            &health,
            &features);
    }

    /* Require automatic baseline finalization. */
    assert(
        health.status.state ==
        GUARDIAN_HEALTH_STATE_READY);

    /* Require exact accepted sample count. */
    assert(
        health.status.baseline_samples ==
        GUARDIAN_HEALTH_BASELINE_MIN_SAMPLES);

    /* Require exact learned RMS mean. */
    assert(
        health.status.baseline_rms_mean_mg ==
        40U);

    /* Require the configured RMS sigma floor for a zero-variance baseline. */
    assert(
        health.status.baseline_rms_std_mg ==
        5U);

    /* Require baseline readiness quality state. */
    assert(
        (health.status.quality_flags &
         GUARDIAN_HEALTH_QUALITY_BASELINE_READY) != 0U);

    /* Require the reference-calibration caveat to remain visible. */
    assert(
        (health.status.quality_flags &
         GUARDIAN_HEALTH_QUALITY_REFERENCE_CALIBRATION) != 0U);
}

/* Verify severe deviation requires persistence before ALARM. */
static void test_alarm_and_recovery_hysteresis(void)
{
    /* Create one fresh runtime model. */
    guardian_health_t health = {0};

    /* Track baseline samples. */
    uint16_t sample = 0U;

    /* Initialize and start the minimum baseline. */
    guardian_health_init(
        &health);

    /* Require accepted baseline start. */
    assert(
        guardian_health_start_baseline(
            &health,
            GUARDIAN_HEALTH_BASELINE_MIN_SAMPLES) == 1);

    /* Learn a zero-variance 40 milli-g baseline. */
    for (sample = 0U;
         sample < GUARDIAN_HEALTH_BASELINE_MIN_SAMPLES;
         ++sample)
    {
        /* Build one stable baseline vector. */
        guardian_dsp_features_t features =
            test_features(
                (uint32_t)sample + 1U,
                40U);

        /* Learn the vector. */
        guardian_health_ingest(
            &health,
            &features);
    }

    /* Create the first severe 80 milli-g deviation. */
    guardian_dsp_features_t anomaly =
        test_features(
            100U,
            80U);

    /* Score the first severe block. */
    guardian_health_ingest(
        &health,
        &anomaly);

    /* Require immediate warning while severe persistence qualifies. */
    assert(
        health.status.state ==
        GUARDIAN_HEALTH_STATE_WARNING);

    /* Require saturated anomaly severity. */
    assert(
        health.status.anomaly_score ==
        1000U);

    /* Require RMS to be the dominant anomaly feature. */
    assert(
        health.status.dominant_feature ==
        GUARDIAN_HEALTH_FEATURE_RMS);

    /* Score the second consecutive severe block. */
    anomaly.block_sequence = 101U;

    /* Feed the second severe vector. */
    guardian_health_ingest(
        &health,
        &anomaly);

    /* Require the persistent severe state to latch ALARM. */
    assert(
        health.status.state ==
        GUARDIAN_HEALTH_STATE_ALARM);

    /* Feed five normal blocks to qualify hysteretic recovery. */
    for (sample = 0U;
         sample < 5U;
         ++sample)
    {
        /* Build a normal baseline-like vector. */
        guardian_dsp_features_t normal =
            test_features(
                200U + (uint32_t)sample,
                40U);

        /* Score one normal recovery block. */
        guardian_health_ingest(
            &health,
            &normal);
    }

    /* Require recovery to trained READY state only after persistence. */
    assert(
        health.status.state ==
        GUARDIAN_HEALTH_STATE_READY);

    /* Require neutral anomaly score after recovery input. */
    assert(
        health.status.anomaly_score ==
        0U);
}

/* Verify acquisition errors never contaminate baseline or score. */
static void test_rejects_bad_acquisition_input(void)
{
    /* Create one fresh runtime model. */
    guardian_health_t health = {0};

    /* Initialize and start baseline learning. */
    guardian_health_init(
        &health);

    /* Require accepted baseline configuration. */
    assert(
        guardian_health_start_baseline(
            &health,
            GUARDIAN_HEALTH_BASELINE_MIN_SAMPLES) == 1);

    /* Build one otherwise valid feature snapshot. */
    guardian_dsp_features_t features =
        test_features(
            1U,
            40U);

    /* Add the M6 ADC saturation quality failure. */
    features.acquisition_status_flags |=
        GUARDIAN_ACQUISITION_STATUS_ADC_SATURATED;

    /* Attempt to ingest the compromised vector. */
    guardian_health_ingest(
        &health,
        &features);

    /* Require zero accepted baseline samples. */
    assert(
        health.status.baseline_samples ==
        0U);

    /* Require one rejected input diagnostic. */
    assert(
        health.status.rejected_inputs ==
        1U);

    /* Require explicit input-rejected quality state. */
    assert(
        (health.status.quality_flags &
         GUARDIAN_HEALTH_QUALITY_INPUT_REJECTED) != 0U);
}

/* Verify sample-rate mismatch fails closed after baseline rate capture. */
static void test_rejects_sample_rate_mismatch(void)
{
    /* Create one fresh runtime model. */
    guardian_health_t health = {0};

    /* Initialize and start baseline learning. */
    guardian_health_init(
        &health);

    /* Require accepted baseline configuration. */
    assert(
        guardian_health_start_baseline(
            &health,
            GUARDIAN_HEALTH_BASELINE_MIN_SAMPLES) == 1);

    /* Build the first trustworthy 4 kHz baseline feature vector. */
    guardian_dsp_features_t first =
        test_features(
            1U,
            40U);

    /* Capture the baseline sample-rate contract. */
    guardian_health_ingest(
        &health,
        &first);

    /* Build a second otherwise valid feature vector. */
    guardian_dsp_features_t mismatch =
        test_features(
            2U,
            40U);

    /* Change its sample rate to an incompatible value. */
    mismatch.sample_rate_hz = 8000U;

    /* Attempt to ingest incompatible frequency-domain features. */
    guardian_health_ingest(
        &health,
        &mismatch);

    /* Require the second vector not to count toward baseline completion. */
    assert(
        health.status.baseline_samples ==
        1U);

    /* Require explicit sample-rate mismatch quality state. */
    assert(
        (health.status.quality_flags &
         GUARDIAN_HEALTH_QUALITY_SAMPLE_RATE_MISMATCH) != 0U);
}

/* Verify M8 command handlers enforce explicit baseline lifecycle semantics. */
static void test_health_protocol_commands(void)
{
    /* Create one fresh runtime model. */
    guardian_health_t health = {0};

    /* Create request storage. */
    guardian_frame_t request = {0};

    /* Create response storage. */
    guardian_frame_t response = {0};

    /* Initialize the model. */
    guardian_health_init(
        &health);

    /* Build a BASELINE_CONTROL START request. */
    request.message_type =
        GUARDIAN_MESSAGE_REQUEST;

    /* Select the baseline-control command. */
    request.command =
        (uint8_t)GUARDIAN_COMMAND_BASELINE_CONTROL;

    /* Configure request sequence. */
    request.sequence = 500U;

    /* Publish the fixed four-byte control payload. */
    request.payload_length =
        GUARDIAN_BASELINE_CONTROL_PAYLOAD_SIZE;

    /* Publish schema revision one. */
    request.payload[0] =
        GUARDIAN_HEALTH_SCHEMA_VERSION;

    /* Select START. */
    request.payload[1] =
        (uint8_t)GUARDIAN_BASELINE_ACTION_START;

    /* Encode target most-significant byte. */
    request.payload[2] = 0U;

    /* Encode the 16-sample minimum target. */
    request.payload[3] =
        (uint8_t)GUARDIAN_HEALTH_BASELINE_MIN_SAMPLES;

    /* Process the baseline command. */
    assert(
        guardian_health_handle_request(
            &health,
            &request,
            &response) ==
        GUARDIAN_PROTOCOL_OK);

    /* Require successful response semantics. */
    assert(
        response.message_type ==
        GUARDIAN_MESSAGE_RESPONSE);

    /* Require the model to enter LEARNING. */
    assert(
        health.status.state ==
        GUARDIAN_HEALTH_STATE_LEARNING);

    /* Build an empty GET_HEALTH_STATUS request. */
    request.command =
        (uint8_t)GUARDIAN_COMMAND_GET_HEALTH_STATUS;

    /* Update correlation sequence. */
    request.sequence = 501U;

    /* GET_HEALTH_STATUS has no request payload. */
    request.payload_length = 0U;

    /* Process the health query. */
    assert(
        guardian_health_handle_request(
            &health,
            &request,
            &response) ==
        GUARDIAN_PROTOCOL_OK);

    /* Require successful health response semantics. */
    assert(
        response.message_type ==
        GUARDIAN_MESSAGE_RESPONSE);

    /* Require the exact fixed M8 status payload size. */
    assert(
        response.payload_length ==
        GUARDIAN_HEALTH_STATUS_PAYLOAD_SIZE);

    /* Require schema revision one on the wire. */
    assert(
        response.payload[0] ==
        GUARDIAN_HEALTH_SCHEMA_VERSION);

    /* Require LEARNING state on the wire. */
    assert(
        response.payload[1] ==
        (uint8_t)GUARDIAN_HEALTH_STATE_LEARNING);
}

/* Execute every portable M8 machine-health test. */
int main(void)
{
    /* Verify explicit bounded baseline learning. */
    test_baseline_learning();

    /* Verify anomaly persistence and recovery hysteresis. */
    test_alarm_and_recovery_hysteresis();

    /* Verify bad acquisition data never contaminates the model. */
    test_rejects_bad_acquisition_input();

    /* Verify frequency features reject sample-rate mismatch. */
    test_rejects_sample_rate_mismatch();

    /* Verify M8 protocol command semantics. */
    test_health_protocol_commands();

    /* Print one concise success line for local and CI logs. */
    (void)printf("Guardian M8 health host tests: PASS\n");

    /* Return conventional successful process status. */
    return 0;
}
