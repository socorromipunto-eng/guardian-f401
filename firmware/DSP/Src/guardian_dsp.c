/* Include the public M7 DSP declarations. */
#include "guardian_dsp.h"

/* Include standard floating-point square root support. */
#include <math.h>

/* Include memory initialization support for deterministic feature output. */
#include <string.h>

/* Define a tiny power threshold used only to classify an AC-silent spectrum. */
#define GUARDIAN_DSP_SILENCE_POWER_EPSILON (1.0e-9F)

/* Define the lower spectral-band upper edge in hertz. */
#define GUARDIAN_DSP_LOW_BAND_MAX_HZ (500.0F)

/* Define the middle spectral-band upper edge in hertz. */
#define GUARDIAN_DSP_MID_BAND_MAX_HZ (1500.0F)

/* Store the exact 64-point symmetric Hann window used by the M7 spectral pipeline. */
static const float guardian_dsp_hann[64] =
{
    0.000000000F, 0.002484612F, 0.009913756F, 0.022213597F,
    0.039261894F, 0.060889213F, 0.086880613F, 0.116977778F,
    0.150881591F, 0.188255099F, 0.228726868F, 0.271894671F,
    0.317329488F, 0.364579766F, 0.413175911F, 0.462634953F,
    0.512465346F, 0.562171852F, 0.611260467F, 0.659243325F,
    0.705643552F, 0.750000000F, 0.791871836F, 0.830842919F,
    0.866525936F, 0.898566254F, 0.926645441F, 0.950484434F,
    0.969846310F, 0.984538643F, 0.994415413F, 0.999378461F,
    0.999378461F, 0.994415413F, 0.984538643F, 0.969846310F,
    0.950484434F, 0.926645441F, 0.898566254F, 0.866525936F,
    0.830842919F, 0.791871836F, 0.750000000F, 0.705643552F,
    0.659243325F, 0.611260467F, 0.562171852F, 0.512465346F,
    0.462634953F, 0.413175911F, 0.364579766F, 0.317329488F,
    0.271894671F, 0.228726868F, 0.188255099F, 0.150881591F,
    0.116977778F, 0.086880613F, 0.060889213F, 0.039261894F,
    0.022213597F, 0.009913756F, 0.002484612F, 0.000000000F
};

/* Store real components of the 64-point forward FFT twiddle factors. */
static const float guardian_dsp_twiddle_real[32] =
{
    1.000000000F, 0.995184727F, 0.980785280F, 0.956940336F,
    0.923879533F, 0.881921264F, 0.831469612F, 0.773010453F,
    0.707106781F, 0.634393284F, 0.555570233F, 0.471396737F,
    0.382683432F, 0.290284677F, 0.195090322F, 0.098017140F,
    0.000000000F, -0.098017140F, -0.195090322F, -0.290284677F,
    -0.382683432F, -0.471396737F, -0.555570233F, -0.634393284F,
    -0.707106781F, -0.773010453F, -0.831469612F, -0.881921264F,
    -0.923879533F, -0.956940336F, -0.980785280F, -0.995184727F
};

/* Store imaginary components of the 64-point forward FFT twiddle factors. */
static const float guardian_dsp_twiddle_imag[32] =
{
    -0.000000000F, -0.098017140F, -0.195090322F, -0.290284677F,
    -0.382683432F, -0.471396737F, -0.555570233F, -0.634393284F,
    -0.707106781F, -0.773010453F, -0.831469612F, -0.881921264F,
    -0.923879533F, -0.956940336F, -0.980785280F, -0.995184727F,
    -1.000000000F, -0.995184727F, -0.980785280F, -0.956940336F,
    -0.923879533F, -0.881921264F, -0.831469612F, -0.773010453F,
    -0.707106781F, -0.634393284F, -0.555570233F, -0.471396737F,
    -0.382683432F, -0.290284677F, -0.195090322F, -0.098017140F
};

/* Saturate one non-negative floating-point value to an unsigned 16-bit integer. */
static uint16_t guardian_dsp_u16(
    float value)
{
    /* Clamp negative or non-positive values to zero. */
    if (!(value > 0.0F))
    {
        /* Return the lower telemetry bound. */
        return 0U;
    }

    /* Clamp values that exceed the published field width. */
    if (value >= 65535.0F)
    {
        /* Return the largest representable unsigned 16-bit value. */
        return 65535U;
    }

    /* Round to the nearest whole engineering unit. */
    return (uint16_t)(value + 0.5F);
}

/* Saturate one non-negative floating-point value to an unsigned 32-bit integer. */
static uint32_t guardian_dsp_u32(
    float value)
{
    /* Clamp negative or non-positive values to zero. */
    if (!(value > 0.0F))
    {
        /* Return the lower telemetry bound. */
        return 0U;
    }

    /* Clamp values that exceed the published field width. */
    if (value >= 4294967295.0F)
    {
        /* Return the largest representable unsigned 32-bit value. */
        return 0xFFFFFFFFUL;
    }

    /* Round to the nearest whole engineering unit. */
    return (uint32_t)(value + 0.5F);
}

/* Swap two floating-point values without dynamic storage. */
static void guardian_dsp_swap_float(
    float *left,
    float *right)
{
    /* Preserve the first value before overwriting it. */
    float temporary = *left;

    /* Move the second value into the first location. */
    *left = *right;

    /* Restore the preserved first value into the second location. */
    *right = temporary;
}

/* Perform in-place radix-2 bit reversal for exactly 64 complex samples. */
static void guardian_dsp_bit_reverse(
    float *real,
    float *imag)
{
    /* Track the source index through the fixed FFT array. */
    uint16_t index = 0U;

    /* Track the bit-reversed destination index. */
    uint16_t reversed = 0U;

    /* Visit every complex sample exactly once. */
    for (index = 1U;
         index < GUARDIAN_DSP_FFT_SIZE;
         ++index)
    {
        /* Start with the highest bit that participates in a 64-point index. */
        uint16_t bit =
            GUARDIAN_DSP_FFT_SIZE >> 1U;

        /* Walk backward through the binary carry chain. */
        while ((reversed & bit) != 0U)
        {
            /* Clear the carried bit. */
            reversed ^= bit;

            /* Advance to the next lower bit. */
            bit >>= 1U;
        }

        /* Set the first available bit. */
        reversed ^= bit;

        /* Swap only one side of each permutation pair. */
        if (index < reversed)
        {
            /* Swap real components. */
            guardian_dsp_swap_float(
                &real[index],
                &real[reversed]);

            /* Swap imaginary components. */
            guardian_dsp_swap_float(
                &imag[index],
                &imag[reversed]);
        }
    }
}

/* Execute one in-place 64-point forward complex radix-2 FFT. */
static void guardian_dsp_fft64(
    float *real,
    float *imag)
{
    /* Reorder input into bit-reversed order required by iterative butterflies. */
    guardian_dsp_bit_reverse(
        real,
        imag);

    /* Start with two-sample butterfly groups. */
    uint16_t length = 2U;

    /* Double the butterfly group length through the complete FFT. */
    while (length <= GUARDIAN_DSP_FFT_SIZE)
    {
        /* Calculate the twiddle-table stride for this FFT stage. */
        uint16_t twiddle_stride =
            GUARDIAN_DSP_FFT_SIZE /
            length;

        /* Track the first sample of the current butterfly group. */
        uint16_t base = 0U;

        /* Process every group at this stage. */
        for (base = 0U;
             base < GUARDIAN_DSP_FFT_SIZE;
             base = (uint16_t)(base + length))
        {
            /* Track one butterfly inside the current group. */
            uint16_t offset = 0U;

            /* Process the first half of each butterfly group. */
            for (offset = 0U;
                 offset < (length >> 1U);
                 ++offset)
            {
                /* Calculate the fixed 64-point twiddle index. */
                uint16_t twiddle_index =
                    (uint16_t)(offset * twiddle_stride);

                /* Calculate the upper butterfly sample index. */
                uint16_t even_index =
                    (uint16_t)(base + offset);

                /* Calculate the lower butterfly sample index. */
                uint16_t odd_index =
                    (uint16_t)(
                        even_index +
                        (length >> 1U)
                    );

                /* Read the stage twiddle real component. */
                float wr =
                    guardian_dsp_twiddle_real[
                        twiddle_index
                    ];

                /* Read the stage twiddle imaginary component. */
                float wi =
                    guardian_dsp_twiddle_imag[
                        twiddle_index
                    ];

                /* Multiply the odd complex sample by the stage twiddle. */
                float odd_real =
                    (real[odd_index] * wr) -
                    (imag[odd_index] * wi);

                /* Calculate the imaginary component of the twiddled odd sample. */
                float odd_imag =
                    (real[odd_index] * wi) +
                    (imag[odd_index] * wr);

                /* Preserve the even real component before butterfly overwrite. */
                float even_real =
                    real[even_index];

                /* Preserve the even imaginary component before butterfly overwrite. */
                float even_imag =
                    imag[even_index];

                /* Write the upper butterfly real output. */
                real[even_index] =
                    even_real +
                    odd_real;

                /* Write the upper butterfly imaginary output. */
                imag[even_index] =
                    even_imag +
                    odd_imag;

                /* Write the lower butterfly real output. */
                real[odd_index] =
                    even_real -
                    odd_real;

                /* Write the lower butterfly imaginary output. */
                imag[odd_index] =
                    even_imag -
                    odd_imag;
            }
        }

        /* Advance to the next radix-2 stage. */
        length = (uint16_t)(length << 1U);
    }
}

/* Write one unsigned 16-bit integer using Guardian big-endian wire order. */
static void guardian_dsp_write_u16_be(
    uint8_t *output,
    uint16_t value)
{
    /* Write the most-significant byte first. */
    output[0] =
        (uint8_t)((value >> 8U) & 0xFFU);

    /* Write the least-significant byte second. */
    output[1] =
        (uint8_t)(value & 0xFFU);
}

/* Write one unsigned 32-bit integer using Guardian big-endian wire order. */
static void guardian_dsp_write_u32_be(
    uint8_t *output,
    uint32_t value)
{
    /* Write the most-significant byte first. */
    output[0] =
        (uint8_t)((value >> 24U) & 0xFFU);

    /* Write the second byte. */
    output[1] =
        (uint8_t)((value >> 16U) & 0xFFU);

    /* Write the third byte. */
    output[2] =
        (uint8_t)((value >> 8U) & 0xFFU);

    /* Write the least-significant byte last. */
    output[3] =
        (uint8_t)(value & 0xFFU);
}

/* Build one request-correlated Guardian ERROR frame. */
static void guardian_dsp_make_error(
    const guardian_frame_t *request,
    guardian_frame_t *response,
    guardian_error_code_t error_code)
{
    /* Preserve the original command identifier. */
    response->command =
        request->command;

    /* Preserve the original request sequence. */
    response->sequence =
        request->sequence;

    /* Mark the output explicitly as an ERROR frame. */
    response->message_type =
        GUARDIAN_MESSAGE_ERROR;

    /* Use the only flags value defined by Guardian Protocol v0.1. */
    response->flags =
        GUARDIAN_SUPPORTED_FLAGS;

    /* Publish the frozen one-byte error payload. */
    response->payload_length = 1U;

    /* Store the published Guardian error identifier. */
    response->payload[0] =
        (uint8_t)error_code;
}

/* Analyze one complete calibrated M6 vibration block. */
guardian_dsp_result_t guardian_dsp_analyze(
    const guardian_acquisition_signal_block_t *signal_block,
    guardian_dsp_features_t *features)
{
    /* Store real FFT input and output components. */
    float real[GUARDIAN_DSP_FFT_SIZE] = {0};

    /* Store imaginary FFT input and output components. */
    float imag[GUARDIAN_DSP_FFT_SIZE] = {0};

    /* Store one-sided spectral power including the Nyquist bin. */
    float power[GUARDIAN_DSP_ONE_SIDED_BIN_COUNT] = {0};

    /* Accumulate the raw sample mean. */
    float mean = 0.0F;

    /* Accumulate unwindowed AC sample power for RMS. */
    float rms_sum = 0.0F;

    /* Track the largest absolute AC sample. */
    float peak = 0.0F;

    /* Accumulate total one-sided AC spectral power excluding DC. */
    float total_power = 0.0F;

    /* Accumulate frequency-weighted spectral power for centroid. */
    float centroid_numerator = 0.0F;

    /* Accumulate low-band spectral power. */
    float low_power = 0.0F;

    /* Accumulate middle-band spectral power. */
    float mid_power = 0.0F;

    /* Accumulate high-band spectral power. */
    float high_power = 0.0F;

    /* Track the largest one-sided spectral power. */
    float dominant_power = 0.0F;

    /* Track the dominant one-sided FFT bin. */
    uint16_t dominant_bin = 0U;

    /* Track the current sample or spectrum index. */
    uint16_t index = 0U;

    /* Reject missing required storage. */
    if ((signal_block == NULL) ||
        (features == NULL))
    {
        /* Report the canonical missing-argument failure. */
        return GUARDIAN_DSP_ERROR_NULL_ARGUMENT;
    }

    /* Require the exact M6/M7 fixed FFT block length. */
    if (signal_block->sample_count !=
        GUARDIAN_DSP_FFT_SIZE)
    {
        /* Report the deterministic unsupported-length failure. */
        return GUARDIAN_DSP_ERROR_LENGTH;
    }

    /* Reject a zero physical sample rate. */
    if (signal_block->sample_rate_hz == 0U)
    {
        /* Report the explicit sample-rate failure. */
        return GUARDIAN_DSP_ERROR_SAMPLE_RATE;
    }

    /* Clear every output field before analysis. */
    (void)memset(
        features,
        0,
        sizeof(*features));

    /* Accumulate the raw calibrated vibration sample mean. */
    for (index = 0U;
         index < GUARDIAN_DSP_FFT_SIZE;
         ++index)
    {
        /* Add one signed milli-g sample to the mean accumulator. */
        mean +=
            (float)signal_block->vibration_mg[index];
    }

    /* Convert the sum into the block arithmetic mean. */
    mean /=
        (float)GUARDIAN_DSP_FFT_SIZE;

    /* Remove DC, calculate time-domain features and prepare windowed FFT input. */
    for (index = 0U;
         index < GUARDIAN_DSP_FFT_SIZE;
         ++index)
    {
        /* Remove the block mean from one calibrated vibration sample. */
        float sample =
            (float)signal_block->vibration_mg[index] -
            mean;

        /* Accumulate squared AC magnitude. */
        rms_sum +=
            sample *
            sample;

        /* Calculate one absolute AC sample without an extra library dependency. */
        float absolute =
            (sample < 0.0F)
            ? -sample
            : sample;

        /* Preserve the largest absolute AC sample. */
        if (absolute > peak)
        {
            /* Update the time-domain peak. */
            peak = absolute;
        }

        /* Apply the fixed Hann window before spectral processing. */
        real[index] =
            sample *
            guardian_dsp_hann[index];

        /* Keep imaginary input at exactly zero for the real-valued sensor signal. */
        imag[index] = 0.0F;
    }

    /* Convert mean square into AC RMS magnitude. */
    float rms =
        sqrtf(
            rms_sum /
            (float)GUARDIAN_DSP_FFT_SIZE);

    /* Execute the fixed portable 64-point forward FFT. */
    guardian_dsp_fft64(
        real,
        imag);

    /* Calculate one-sided power for DC through Nyquist. */
    for (index = 0U;
         index < GUARDIAN_DSP_ONE_SIDED_BIN_COUNT;
         ++index)
    {
        /* Calculate squared complex magnitude without square root. */
        power[index] =
            (real[index] * real[index]) +
            (imag[index] * imag[index]);
    }

    /* Analyze only positive-frequency AC bins and the Nyquist bin. */
    for (index = 1U;
         index < GUARDIAN_DSP_ONE_SIDED_BIN_COUNT;
         ++index)
    {
        /* Convert this bin index into physical frequency. */
        float frequency_hz =
            (
                (float)index *
                (float)signal_block->sample_rate_hz
            ) /
            (float)GUARDIAN_DSP_FFT_SIZE;

        /* Read this bin power once. */
        float bin_power =
            power[index];

        /* Accumulate total non-DC one-sided spectral power. */
        total_power +=
            bin_power;

        /* Accumulate the centroid numerator. */
        centroid_numerator +=
            frequency_hz *
            bin_power;

        /* Classify the bin into one of the fixed M7 machine-health bands. */
        if (frequency_hz <=
            GUARDIAN_DSP_LOW_BAND_MAX_HZ)
        {
            /* Accumulate 0-500 Hz energy. */
            low_power +=
                bin_power;
        }
        else if (frequency_hz <=
                 GUARDIAN_DSP_MID_BAND_MAX_HZ)
        {
            /* Accumulate >500-1500 Hz energy. */
            mid_power +=
                bin_power;
        }
        else
        {
            /* Accumulate >1500 Hz through Nyquist energy. */
            high_power +=
                bin_power;
        }

        /* Preserve the largest spectral component. */
        if (bin_power > dominant_power)
        {
            /* Update the dominant power. */
            dominant_power =
                bin_power;

            /* Preserve the corresponding FFT bin. */
            dominant_bin =
                index;
        }
    }

    /* Preserve source block identity. */
    features->block_sequence =
        signal_block->sequence;

    /* Saturate the physical sample rate to the published 16-bit payload field. */
    features->sample_rate_hz =
        (signal_block->sample_rate_hz > 65535U)
        ? 65535U
        : (uint16_t)signal_block->sample_rate_hz;

    /* Publish AC RMS vibration. */
    features->rms_mg =
        guardian_dsp_u16(rms);

    /* Publish the largest absolute AC sample. */
    features->peak_mg =
        guardian_dsp_u16(peak);

    /* Publish crest factor only when RMS is non-zero. */
    if (rms > 0.0F)
    {
        /* Convert peak/RMS into a fixed three-decimal representation. */
        features->crest_factor_milli =
            guardian_dsp_u16(
                (peak / rms) *
                1000.0F);
    }

    /* Preserve exact M6 acquisition quality flags. */
    features->acquisition_status_flags =
        signal_block->status_flags;

    /* Publish the processing stages that were always executed successfully. */
    features->dsp_status_flags =
        GUARDIAN_DSP_STATUS_VALID |
        GUARDIAN_DSP_STATUS_DC_REMOVED |
        GUARDIAN_DSP_STATUS_HANN_WINDOW;

    /* Analyze spectral features only when measurable AC power exists. */
    if (total_power >
        GUARDIAN_DSP_SILENCE_POWER_EPSILON)
    {
        /* Start dominant-frequency interpolation at the integer FFT bin. */
        float interpolated_bin =
            (float)dominant_bin;

        /* Use parabolic power interpolation when two neighboring bins exist. */
        if ((dominant_bin > 1U) &&
            (dominant_bin <
             (GUARDIAN_DSP_ONE_SIDED_BIN_COUNT - 1U)))
        {
            /* Read the left neighboring power. */
            float left =
                power[dominant_bin - 1U];

            /* Read the center dominant power. */
            float center =
                power[dominant_bin];

            /* Read the right neighboring power. */
            float right =
                power[dominant_bin + 1U];

            /* Calculate the parabola curvature denominator. */
            float denominator =
                left -
                (2.0F * center) +
                right;

            /* Avoid unstable interpolation around a flat spectral peak. */
            if ((denominator > 1.0e-12F) ||
                (denominator < -1.0e-12F))
            {
                /* Calculate the standard three-point parabolic bin offset. */
                float delta =
                    0.5F *
                    (left - right) /
                    denominator;

                /* Clamp interpolation to the neighboring-bin interval. */
                if (delta > 0.5F)
                {
                    /* Limit an unstable positive extrapolation. */
                    delta = 0.5F;
                }
                else if (delta < -0.5F)
                {
                    /* Limit an unstable negative extrapolation. */
                    delta = -0.5F;
                }

                /* Apply the bounded sub-bin correction. */
                interpolated_bin +=
                    delta;
            }
        }

        /* Convert the interpolated dominant bin into hundredths of one hertz. */
        features->dominant_frequency_centi_hz =
            guardian_dsp_u32(
                (
                    interpolated_bin *
                    (float)signal_block->sample_rate_hz *
                    100.0F
                ) /
                (float)GUARDIAN_DSP_FFT_SIZE);

        /* Calculate the coherent gain of the fixed Hann window. */
        float hann_sum = 0.0F;

        /* Sum every window coefficient exactly once. */
        for (index = 0U;
             index < GUARDIAN_DSP_FFT_SIZE;
             ++index)
        {
            /* Accumulate one Hann coefficient. */
            hann_sum +=
                guardian_dsp_hann[index];
        }

        /* Estimate sinusoidal peak amplitude from the dominant complex magnitude. */
        features->dominant_peak_mg =
            guardian_dsp_u16(
                (
                    2.0F *
                    sqrtf(dominant_power)
                ) /
                hann_sum);

        /* Calculate the power-weighted spectral centroid. */
        features->spectral_centroid_centi_hz =
            guardian_dsp_u32(
                (
                    centroid_numerator /
                    total_power
                ) *
                100.0F);

        /* Calculate low-band energy share in permille. */
        features->low_band_permille =
            guardian_dsp_u16(
                (low_power / total_power) *
                1000.0F);

        /* Calculate middle-band energy share in permille. */
        features->mid_band_permille =
            guardian_dsp_u16(
                (mid_power / total_power) *
                1000.0F);

        /* Force the three published shares to sum to exactly 1000 when possible. */
        if ((uint32_t)features->low_band_permille +
            (uint32_t)features->mid_band_permille <=
            1000U)
        {
            /* Assign the rounding remainder to the high band. */
            features->high_band_permille =
                (uint16_t)(
                    1000U -
                    (uint32_t)features->low_band_permille -
                    (uint32_t)features->mid_band_permille
                );
        }
        else
        {
            /* Fall back to an independently rounded high-band value defensively. */
            features->high_band_permille =
                guardian_dsp_u16(
                    (high_power / total_power) *
                    1000.0F);
        }

        /* Mark the dominant frequency and amplitude fields valid. */
        features->dsp_status_flags |=
            GUARDIAN_DSP_STATUS_DOMINANT_VALID;
    }
    else
    {
        /* Mark an effectively silent AC block explicitly. */
        features->dsp_status_flags |=
            GUARDIAN_DSP_STATUS_SILENT;
    }

    /* Report successful deterministic feature extraction. */
    return GUARDIAN_DSP_OK;
}

/* Encode one deterministic GET_DSP_FEATURES payload. */
guardian_protocol_result_t guardian_dsp_encode_payload(
    const guardian_dsp_features_t *features,
    uint8_t *payload,
    uint16_t payload_capacity,
    uint16_t *payload_length)
{
    /* Reject missing required storage. */
    if ((features == NULL) ||
        (payload == NULL) ||
        (payload_length == NULL))
    {
        /* Report the canonical protocol missing-argument failure. */
        return GUARDIAN_PROTOCOL_ERROR_NULL_ARGUMENT;
    }

    /* Require enough bounded output storage for the fixed feature schema. */
    if (payload_capacity <
        GUARDIAN_DSP_PAYLOAD_SIZE)
    {
        /* Report the canonical bounded-output failure. */
        return GUARDIAN_PROTOCOL_ERROR_OUTPUT_TOO_SMALL;
    }

    /* Publish the first DSP feature payload schema revision. */
    payload[0] =
        GUARDIAN_DSP_SCHEMA_VERSION;

    /* Reserve one byte for future compatible feature capability flags. */
    payload[1] = 0U;

    /* Encode acquisition block sequence. */
    guardian_dsp_write_u32_be(
        &payload[2],
        features->block_sequence);

    /* Encode physical sample rate. */
    guardian_dsp_write_u16_be(
        &payload[6],
        features->sample_rate_hz);

    /* Encode AC RMS vibration. */
    guardian_dsp_write_u16_be(
        &payload[8],
        features->rms_mg);

    /* Encode time-domain peak vibration. */
    guardian_dsp_write_u16_be(
        &payload[10],
        features->peak_mg);

    /* Encode crest factor multiplied by 1000. */
    guardian_dsp_write_u16_be(
        &payload[12],
        features->crest_factor_milli);

    /* Encode dominant frequency in hundredths of one hertz. */
    guardian_dsp_write_u32_be(
        &payload[14],
        features->dominant_frequency_centi_hz);

    /* Encode estimated dominant sinusoidal peak amplitude. */
    guardian_dsp_write_u16_be(
        &payload[18],
        features->dominant_peak_mg);

    /* Encode spectral centroid in hundredths of one hertz. */
    guardian_dsp_write_u32_be(
        &payload[20],
        features->spectral_centroid_centi_hz);

    /* Encode low-band energy share. */
    guardian_dsp_write_u16_be(
        &payload[24],
        features->low_band_permille);

    /* Encode middle-band energy share. */
    guardian_dsp_write_u16_be(
        &payload[26],
        features->mid_band_permille);

    /* Encode high-band energy share. */
    guardian_dsp_write_u16_be(
        &payload[28],
        features->high_band_permille);

    /* Encode M6 acquisition quality flags. */
    guardian_dsp_write_u16_be(
        &payload[30],
        features->acquisition_status_flags);

    /* Publish the exact fixed payload length. */
    *payload_length =
        GUARDIAN_DSP_PAYLOAD_SIZE;

    /* Report successful payload serialization. */
    return GUARDIAN_PROTOCOL_OK;
}

/* Process GET_DSP_FEATURES and build one correlated RESPONSE or ERROR frame. */
guardian_protocol_result_t guardian_dsp_handle_request(
    const guardian_dsp_features_t *features,
    uint8_t features_valid,
    const guardian_frame_t *request,
    guardian_frame_t *response)
{
    /* Store payload encoding status. */
    guardian_protocol_result_t result =
        GUARDIAN_PROTOCOL_OK;

    /* Reject missing request or response storage. */
    if ((request == NULL) ||
        (response == NULL))
    {
        /* Report the canonical missing-argument failure. */
        return GUARDIAN_PROTOCOL_ERROR_NULL_ARGUMENT;
    }

    /* Reject accidental dispatch of another command. */
    if (request->command !=
        (uint8_t)GUARDIAN_COMMAND_GET_DSP_FEATURES)
    {
        /* Build a deterministic unknown-command error. */
        guardian_dsp_make_error(
            request,
            response,
            GUARDIAN_ERROR_UNKNOWN_COMMAND);

        /* Report successful ERROR frame construction. */
        return GUARDIAN_PROTOCOL_OK;
    }

    /* Require host-to-device REQUEST semantics. */
    if (request->message_type !=
        GUARDIAN_MESSAGE_REQUEST)
    {
        /* Reject contradictory message class. */
        guardian_dsp_make_error(
            request,
            response,
            GUARDIAN_ERROR_MALFORMED_FRAME);

        /* Report successful ERROR frame construction. */
        return GUARDIAN_PROTOCOL_OK;
    }

    /* Require the frozen empty GET_DSP_FEATURES request payload. */
    if (request->payload_length != 0U)
    {
        /* Reject undefined command-specific bytes. */
        guardian_dsp_make_error(
            request,
            response,
            GUARDIAN_ERROR_INVALID_PAYLOAD);

        /* Report successful ERROR frame construction. */
        return GUARDIAN_PROTOCOL_OK;
    }

    /* Require at least one successfully analyzed M7 block. */
    if ((features_valid == 0U) ||
        (features == NULL))
    {
        /* Report that deterministic DSP data is not ready yet. */
        guardian_dsp_make_error(
            request,
            response,
            GUARDIAN_ERROR_BUSY);

        /* Report successful ERROR frame construction. */
        return GUARDIAN_PROTOCOL_OK;
    }

    /* Build a successful correlated response. */
    response->message_type =
        GUARDIAN_MESSAGE_RESPONSE;

    /* Preserve the GET_DSP_FEATURES command identifier. */
    response->command =
        request->command;

    /* Use the only flags value defined by protocol v0.1. */
    response->flags =
        GUARDIAN_SUPPORTED_FLAGS;

    /* Preserve request/response sequence correlation. */
    response->sequence =
        request->sequence;

    /* Encode the fixed M7 feature payload directly into bounded frame storage. */
    result =
        guardian_dsp_encode_payload(
            features,
            response->payload,
            GUARDIAN_MAX_PAYLOAD_SIZE,
            &response->payload_length);

    /* Return the canonical payload encoding outcome. */
    return result;
}
