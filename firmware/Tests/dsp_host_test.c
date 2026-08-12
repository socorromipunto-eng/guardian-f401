/* Include the portable M7 DSP engine under test. */
#include "guardian_dsp.h"

/* Include assertion support for deterministic host verification. */
#include <assert.h>

/* Include math support for synthetic sine-wave test signals. */
#include <math.h>

/* Include standard output for one concise CI success line. */
#include <stdio.h>

/* Define pi locally so the tests remain independent from non-standard M_PI macros. */
#define TEST_PI (3.14159265358979323846F)

/* Fill one complete calibrated vibration signal block with a sine wave. */
static guardian_acquisition_signal_block_t test_sine_block(
    float frequency_hz,
    float amplitude_mg)
{
    /* Create deterministic zero-initialized signal storage. */
    guardian_acquisition_signal_block_t block = {0};

    /* Track the current sample. */
    uint16_t index = 0U;

    /* Publish a deterministic block sequence. */
    block.sequence = 17U;

    /* Match the M6 reference physical sample rate. */
    block.sample_rate_hz = 4000U;

    /* Publish the exact M7 FFT input length. */
    block.sample_count = GUARDIAN_DSP_FFT_SIZE;

    /* Preserve one representative M6 acquisition status bit. */
    block.status_flags =
        GUARDIAN_ACQUISITION_STATUS_VALID;

    /* Generate one calibrated sine-wave block. */
    for (index = 0U;
         index < GUARDIAN_DSP_FFT_SIZE;
         ++index)
    {
        /* Calculate this sample phase in radians. */
        float phase =
            (
                2.0F *
                TEST_PI *
                frequency_hz *
                (float)index
            ) /
            (float)block.sample_rate_hz;

        /* Convert the floating synthetic sample into signed milli-g. */
        block.vibration_mg[index] =
            (int16_t)(
                (amplitude_mg * sinf(phase)) +
                (
                    (sinf(phase) >= 0.0F)
                    ? 0.5F
                    : -0.5F
                )
            );
    }

    /* Return the complete deterministic signal block by value. */
    return block;
}

/* Verify constant input becomes a silent AC spectrum after DC removal. */
static void test_dc_only_block(void)
{
    /* Create one deterministic signal block. */
    guardian_acquisition_signal_block_t block = {0};

    /* Create output feature storage. */
    guardian_dsp_features_t features = {0};

    /* Track the current sample. */
    uint16_t index = 0U;

    /* Publish valid metadata. */
    block.sequence = 1U;

    /* Publish the reference M6 sample rate. */
    block.sample_rate_hz = 4000U;

    /* Publish the fixed M7 sample count. */
    block.sample_count = GUARDIAN_DSP_FFT_SIZE;

    /* Fill every sample with the same DC acceleration offset. */
    for (index = 0U;
         index < GUARDIAN_DSP_FFT_SIZE;
         ++index)
    {
        /* Store a constant 250 milli-g offset. */
        block.vibration_mg[index] = 250;
    }

    /* Analyze the block successfully. */
    assert(
        guardian_dsp_analyze(
            &block,
            &features) ==
        GUARDIAN_DSP_OK);

    /* Require exact zero AC RMS after mean removal. */
    assert(features.rms_mg == 0U);

    /* Require exact zero AC peak after mean removal. */
    assert(features.peak_mg == 0U);

    /* Require explicit silent classification. */
    assert(
        (features.dsp_status_flags &
         GUARDIAN_DSP_STATUS_SILENT) != 0U);

    /* Require no false dominant frequency. */
    assert(
        features.dominant_frequency_centi_hz ==
        0U);
}

/* Verify an exact 250 Hz FFT-bin tone is identified accurately. */
static void test_low_band_250_hz(void)
{
    /* Generate a 100 milli-g peak sine wave at FFT bin four. */
    guardian_acquisition_signal_block_t block =
        test_sine_block(
            250.0F,
            100.0F);

    /* Create output feature storage. */
    guardian_dsp_features_t features = {0};

    /* Analyze the synthetic machine signal. */
    assert(
        guardian_dsp_analyze(
            &block,
            &features) ==
        GUARDIAN_DSP_OK);

    /* Require approximately 70.7 milli-g RMS. */
    assert(
        (features.rms_mg >= 70U) &&
        (features.rms_mg <= 72U));

    /* Require approximately 100 milli-g time-domain peak. */
    assert(
        (features.peak_mg >= 99U) &&
        (features.peak_mg <= 101U));

    /* Require a crest factor close to sqrt(2). */
    assert(
        (features.crest_factor_milli >= 1380U) &&
        (features.crest_factor_milli <= 1450U));

    /* Require dominant frequency within one hertz of 250 Hz. */
    assert(
        (features.dominant_frequency_centi_hz >= 24900U) &&
        (features.dominant_frequency_centi_hz <= 25100U));

    /* Require dominant amplitude close to the synthetic 100 milli-g peak. */
    assert(
        (features.dominant_peak_mg >= 95U) &&
        (features.dominant_peak_mg <= 105U));

    /* Require the low-frequency band to contain nearly all signal energy. */
    assert(features.low_band_permille >= 980U);

    /* Require the three energy shares to sum to exactly 1000 permille. */
    assert(
        (uint32_t)features.low_band_permille +
        (uint32_t)features.mid_band_permille +
        (uint32_t)features.high_band_permille ==
        1000U);
}

/* Verify a 1000 Hz tone maps into the middle spectral band. */
static void test_mid_band_1000_hz(void)
{
    /* Generate a deterministic 1000 Hz machine vibration tone. */
    guardian_acquisition_signal_block_t block =
        test_sine_block(
            1000.0F,
            120.0F);

    /* Create output feature storage. */
    guardian_dsp_features_t features = {0};

    /* Analyze the synthetic machine signal. */
    assert(
        guardian_dsp_analyze(
            &block,
            &features) ==
        GUARDIAN_DSP_OK);

    /* Require dominant frequency near 1000 Hz. */
    assert(
        (features.dominant_frequency_centi_hz >= 99900U) &&
        (features.dominant_frequency_centi_hz <= 100100U));

    /* Require the middle band to contain nearly all signal energy. */
    assert(features.mid_band_permille >= 980U);
}

/* Verify a 1750 Hz tone maps into the high spectral band. */
static void test_high_band_1750_hz(void)
{
    /* Generate a deterministic high-frequency vibration tone. */
    guardian_acquisition_signal_block_t block =
        test_sine_block(
            1750.0F,
            80.0F);

    /* Create output feature storage. */
    guardian_dsp_features_t features = {0};

    /* Analyze the synthetic machine signal. */
    assert(
        guardian_dsp_analyze(
            &block,
            &features) ==
        GUARDIAN_DSP_OK);

    /* Require dominant frequency near 1750 Hz. */
    assert(
        (features.dominant_frequency_centi_hz >= 174900U) &&
        (features.dominant_frequency_centi_hz <= 175100U));

    /* Require the high band to contain nearly all signal energy. */
    assert(features.high_band_permille >= 980U);
}

/* Verify GET_DSP_FEATURES protocol response encoding and BUSY behavior. */
static void test_dsp_protocol_handler(void)
{
    /* Create one representative valid feature snapshot. */
    guardian_dsp_features_t features = {0};

    /* Create one empty host request. */
    guardian_frame_t request = {0};

    /* Create response storage. */
    guardian_frame_t response = {0};

    /* Publish one deterministic feature sequence. */
    features.block_sequence = 9U;

    /* Publish the reference sample rate. */
    features.sample_rate_hz = 4000U;

    /* Publish one deterministic RMS value. */
    features.rms_mg = 42U;

    /* Configure the host request message type. */
    request.message_type =
        GUARDIAN_MESSAGE_REQUEST;

    /* Configure the M7 command identifier. */
    request.command =
        (uint8_t)GUARDIAN_COMMAND_GET_DSP_FEATURES;

    /* Configure request correlation sequence. */
    request.sequence = 77U;

    /* Require BUSY before a valid feature snapshot exists. */
    assert(
        guardian_dsp_handle_request(
            &features,
            0U,
            &request,
            &response) ==
        GUARDIAN_PROTOCOL_OK);

    /* Require explicit ERROR semantics. */
    assert(
        response.message_type ==
        GUARDIAN_MESSAGE_ERROR);

    /* Require the published BUSY error. */
    assert(
        response.payload[0] ==
        (uint8_t)GUARDIAN_ERROR_BUSY);

    /* Process the same request with valid features available. */
    assert(
        guardian_dsp_handle_request(
            &features,
            1U,
            &request,
            &response) ==
        GUARDIAN_PROTOCOL_OK);

    /* Require successful synchronous response semantics. */
    assert(
        response.message_type ==
        GUARDIAN_MESSAGE_RESPONSE);

    /* Require exact request correlation. */
    assert(response.sequence == 77U);

    /* Require the fixed M7 payload size. */
    assert(
        response.payload_length ==
        GUARDIAN_DSP_PAYLOAD_SIZE);

    /* Require payload schema version one. */
    assert(
        response.payload[0] ==
        GUARDIAN_DSP_SCHEMA_VERSION);
}

/* Execute every portable M7 DSP test. */
int main(void)
{
    /* Verify DC removal and silence classification. */
    test_dc_only_block();

    /* Verify low-band FFT feature extraction. */
    test_low_band_250_hz();

    /* Verify middle-band FFT feature extraction. */
    test_mid_band_1000_hz();

    /* Verify high-band FFT feature extraction. */
    test_high_band_1750_hz();

    /* Verify protocol exposure of the latest DSP snapshot. */
    test_dsp_protocol_handler();

    /* Print one concise success line for local and CI logs. */
    (void)printf("Guardian M7 DSP host tests: PASS\n");

    /* Return the conventional successful process status. */
    return 0;
}
