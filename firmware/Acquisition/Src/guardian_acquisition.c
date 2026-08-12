/* Include the public portable acquisition declarations. */
#include "guardian_acquisition.h"

/* Include limits used for deterministic engineering-unit saturation. */
#include <limits.h>

/* Include memory initialization support for deterministic startup. */
#include <string.h>

/* Saturating-increment one unsigned 32-bit diagnostic counter. */
static void guardian_acquisition_increment_u32(
    uint32_t *value)
{
    /* Ignore a missing diagnostic pointer defensively. */
    if (value == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Avoid counter wrap because diagnostics should remain monotonic. */
    if (*value != UINT32_MAX)
    {
        /* Increment only while representable. */
        *value += 1U;
    }
}

/* Return the nearest unsigned integer square root for one non-negative 64-bit value. */
static uint32_t guardian_acquisition_isqrt_u64(
    uint64_t value)
{
    /* Store the current binary restoring algorithm remainder. */
    uint64_t remainder = value;

    /* Store the current trial/result value. */
    uint64_t result = 0U;

    /* Start at the highest even power of four representable in 64 bits. */
    uint64_t bit = (uint64_t)1U << 62U;

    /* Move the initial bit down until it is not larger than the input. */
    while (bit > remainder)
    {
        /* Advance by one radix-four digit. */
        bit >>= 2U;
    }

    /* Process every radix-four digit. */
    while (bit != 0U)
    {
        /* Check whether the next result bit fits inside the remaining value. */
        if (remainder >= (result + bit))
        {
            /* Subtract the accepted trial value. */
            remainder -= result + bit;

            /* Shift the partial result and add the accepted bit. */
            result = (result >> 1U) + bit;
        }
        else
        {
            /* Shift the partial result when the trial bit does not fit. */
            result >>= 1U;
        }

        /* Advance to the next radix-four input digit. */
        bit >>= 2U;
    }

    /* Saturate the mathematically bounded result to the public return width. */
    if (result > UINT32_MAX)
    {
        /* Return the largest representable unsigned 32-bit value. */
        return UINT32_MAX;
    }

    /* Return the exact floor square root. */
    return (uint32_t)result;
}

/* Saturate one unsigned 64-bit engineering value to an unsigned 16-bit field. */
static uint16_t guardian_acquisition_u16(
    uint64_t value)
{
    /* Clamp values that exceed the telemetry field width. */
    if (value > UINT16_MAX)
    {
        /* Return the largest representable telemetry value. */
        return UINT16_MAX;
    }

    /* Return the safely representable value. */
    return (uint16_t)value;
}

/* Saturate one signed 64-bit engineering value to a signed 16-bit field. */
static int16_t guardian_acquisition_i16(
    int64_t value)
{
    /* Clamp values above the telemetry field maximum. */
    if (value > INT16_MAX)
    {
        /* Return the largest representable signed telemetry value. */
        return INT16_MAX;
    }

    /* Clamp values below the telemetry field minimum. */
    if (value < INT16_MIN)
    {
        /* Return the smallest representable signed telemetry value. */
        return INT16_MIN;
    }

    /* Return the safely representable value. */
    return (int16_t)value;
}

/* Normalize one unsigned ADC code to the 3.3 V factory calibration reference. */
static uint32_t guardian_acquisition_normalize_code(
    uint32_t code,
    uint32_t vdda_mv)
{
    /* Scale the raw code so factory 3.3 V calibration constants remain comparable. */
    uint64_t scaled =
        (uint64_t)code *
        (uint64_t)vdda_mv;

    /* Round to the nearest normalized ADC code. */
    scaled +=
        (uint64_t)GUARDIAN_ACQUISITION_FACTORY_VDDA_MV /
        2U;

    /* Divide by the factory calibration supply voltage. */
    scaled /=
        (uint64_t)GUARDIAN_ACQUISITION_FACTORY_VDDA_MV;

    /* Clamp unexpected out-of-range values defensively. */
    if (scaled > UINT32_MAX)
    {
        /* Return the largest representable portable code. */
        return UINT32_MAX;
    }

    /* Return the normalized code. */
    return (uint32_t)scaled;
}

/* Validate explicit calibration constants before processing any ADC block. */
static int guardian_acquisition_calibration_valid(
    const guardian_acquisition_calibration_t *calibration)
{
    /* Reject missing calibration storage. */
    if (calibration == NULL)
    {
        /* Report invalid calibration. */
        return 0;
    }

    /* Require a usable factory VREFINT calibration code. */
    if (calibration->vrefint_cal_code == 0U)
    {
        /* Report invalid calibration. */
        return 0;
    }

    /* Require distinct temperature calibration endpoints. */
    if (calibration->temperature_cal_30_code ==
        calibration->temperature_cal_110_code)
    {
        /* Report invalid calibration. */
        return 0;
    }

    /* Require a non-zero vibration scale denominator. */
    if (calibration->vibration_mg_per_code_den == 0U)
    {
        /* Report invalid calibration. */
        return 0;
    }

    /* Require a non-zero current scale denominator. */
    if (calibration->current_ma_per_code_den == 0U)
    {
        /* Report invalid calibration. */
        return 0;
    }

    /* Require a non-zero supply divider denominator. */
    if (calibration->supply_divider_den == 0U)
    {
        /* Report invalid calibration. */
        return 0;
    }

    /* Report valid calibration. */
    return 1;
}

/* Initialize one portable acquisition processor. */
guardian_acquisition_result_t guardian_acquisition_init(
    guardian_acquisition_t *acquisition,
    const guardian_acquisition_calibration_t *calibration)
{
    /* Reject missing acquisition or calibration storage. */
    if ((acquisition == NULL) || (calibration == NULL))
    {
        /* Report the canonical missing-argument failure. */
        return GUARDIAN_ACQUISITION_ERROR_NULL_ARGUMENT;
    }

    /* Reject incomplete or mathematically unsafe calibration. */
    if (guardian_acquisition_calibration_valid(calibration) == 0)
    {
        /* Report the explicit calibration failure. */
        return GUARDIAN_ACQUISITION_ERROR_CALIBRATION;
    }

    /* Clear all state before applying immutable calibration. */
    (void)memset(
        acquisition,
        0,
        sizeof(*acquisition));

    /* Copy explicit calibration by value. */
    acquisition->calibration = *calibration;

    /* Report successful initialization. */
    return GUARDIAN_ACQUISITION_OK;
}

/* Process one complete interleaved ADC scan block into telemetry measurements. */
guardian_acquisition_result_t guardian_acquisition_process_block(
    guardian_acquisition_t *acquisition,
    const uint16_t *samples,
    size_t sample_count,
    const guardian_acquisition_aux_t *aux)
{
    /* Accumulate raw channel sums using widths that cannot overflow one practical DMA block. */
    uint64_t channel_sum[GUARDIAN_ACQUISITION_CHANNEL_COUNT] = {0};

    /* Accumulate normalized squared vibration deltas for RMS calculation. */
    uint64_t vibration_square_sum = 0U;

    /* Store one rounded average for each channel. */
    uint32_t average[GUARDIAN_ACQUISITION_CHANNEL_COUNT] = {0};

    /* Store the number of complete ADC scan frames. */
    size_t frame_count = 0U;

    /* Track the current frame while accumulating channel sums. */
    size_t frame_index = 0U;

    /* Track the current channel within one interleaved scan frame. */
    size_t channel_index = 0U;

    /* Track whether any sample touched a 12-bit ADC rail. */
    uint8_t saturated = 0U;

    /* Store the VREFINT-compensated VDDA estimate in millivolts. */
    uint32_t vdda_mv = 0U;

    /* Store the normalized average temperature code. */
    uint32_t normalized_temperature_code = 0U;

    /* Store the signed factory temperature calibration span. */
    int32_t temperature_span = 0;

    /* Store the signed normalized temperature offset from the 30 C calibration point. */
    int32_t temperature_offset = 0;

    /* Store the calculated temperature in hundredths of one degree Celsius. */
    int64_t temperature_centi_c = 0;

    /* Store the normalized current-sensor code delta. */
    int32_t current_delta = 0;

    /* Store the calculated machine current in milliamperes. */
    uint64_t current_ma = 0U;

    /* Store the calculated external supply voltage in millivolts. */
    uint64_t supply_mv = 0U;

    /* Store the integer RMS vibration magnitude in normalized ADC codes. */
    uint32_t vibration_rms_code = 0U;

    /* Store the calculated vibration magnitude in milli-g. */
    uint64_t vibration_mg = 0U;

    /* Store the final bounded measurement snapshot before publishing it. */
    guardian_machine_measurements_t measurements = {0};

    /* Reject missing required storage. */
    if ((acquisition == NULL) || (samples == NULL) || (aux == NULL))
    {
        /* Report the canonical missing-argument failure. */
        return GUARDIAN_ACQUISITION_ERROR_NULL_ARGUMENT;
    }

    /* Revalidate calibration in case caller memory was corrupted after initialization. */
    if (guardian_acquisition_calibration_valid(
            &acquisition->calibration) == 0)
    {
        /* Count the rejected block. */
        guardian_acquisition_increment_u32(
            &acquisition->stats.invalid_blocks);

        /* Report the explicit calibration failure. */
        return GUARDIAN_ACQUISITION_ERROR_CALIBRATION;
    }

    /* Require at least one complete fixed-width ADC scan frame. */
    if ((sample_count == 0U) ||
        ((sample_count % GUARDIAN_ACQUISITION_CHANNEL_COUNT) != 0U))
    {
        /* Count the malformed block. */
        guardian_acquisition_increment_u32(
            &acquisition->stats.invalid_blocks);

        /* Report the deterministic block-length failure. */
        return GUARDIAN_ACQUISITION_ERROR_LENGTH;
    }

    /* Calculate the number of complete interleaved scan frames. */
    frame_count =
        sample_count /
        GUARDIAN_ACQUISITION_CHANNEL_COUNT;

    /* Accumulate raw averages and detect ADC rail values. */
    for (frame_index = 0U;
         frame_index < frame_count;
         ++frame_index)
    {
        /* Visit every fixed-position channel inside this scan frame. */
        for (channel_index = 0U;
             channel_index < GUARDIAN_ACQUISITION_CHANNEL_COUNT;
             ++channel_index)
        {
            /* Read the current interleaved 12-bit sample. */
            uint16_t sample =
                samples[
                    (frame_index *
                     GUARDIAN_ACQUISITION_CHANNEL_COUNT) +
                    channel_index
                ];

            /* Add the sample to its channel accumulator. */
            channel_sum[channel_index] +=
                (uint64_t)sample;

            /* Mark blocks that touch either ADC rail. */
            if ((sample == 0U) ||
                (sample >= GUARDIAN_ACQUISITION_ADC_MAX_CODE))
            {
                /* Preserve the rail-contact diagnostic for the entire block. */
                saturated = 1U;
            }
        }
    }

    /* Calculate rounded average codes for every scan channel. */
    for (channel_index = 0U;
         channel_index < GUARDIAN_ACQUISITION_CHANNEL_COUNT;
         ++channel_index)
    {
        /* Add half the divisor for nearest-integer rounding. */
        uint64_t rounded =
            channel_sum[channel_index] +
            ((uint64_t)frame_count / 2U);

        /* Divide by the number of complete scan frames. */
        rounded /=
            (uint64_t)frame_count;

        /* Store the bounded rounded average. */
        average[channel_index] =
            (uint32_t)rounded;
    }

    /* Reject a zero VREFINT conversion because VDDA cannot be derived safely. */
    if (average[GUARDIAN_ACQUISITION_CHANNEL_VREFINT] == 0U)
    {
        /* Count the untrustworthy block. */
        guardian_acquisition_increment_u32(
            &acquisition->stats.invalid_blocks);

        /* Report the explicit reference failure. */
        return GUARDIAN_ACQUISITION_ERROR_REFERENCE;
    }

    /* Derive actual VDDA from the factory VREFINT code acquired at 3.3 V. */
    vdda_mv =
        (uint32_t)(
            (
                (uint64_t)GUARDIAN_ACQUISITION_FACTORY_VDDA_MV *
                (uint64_t)acquisition->calibration.vrefint_cal_code +
                ((uint64_t)average[
                    GUARDIAN_ACQUISITION_CHANNEL_VREFINT] /
                 2U)
            ) /
            (uint64_t)average[
                GUARDIAN_ACQUISITION_CHANNEL_VREFINT]
        );

    /* Normalize temperature ADC code into the factory 3.3 V calibration domain. */
    normalized_temperature_code =
        guardian_acquisition_normalize_code(
            average[
                GUARDIAN_ACQUISITION_CHANNEL_TEMPERATURE],
            vdda_mv);

    /* Calculate the signed 30 C to 110 C factory code span. */
    temperature_span =
        (int32_t)acquisition->calibration.temperature_cal_110_code -
        (int32_t)acquisition->calibration.temperature_cal_30_code;

    /* Calculate the signed normalized offset from the 30 C calibration point. */
    temperature_offset =
        (int32_t)normalized_temperature_code -
        (int32_t)acquisition->calibration.temperature_cal_30_code;

    /* Linearly interpolate factory calibration over the documented 80 C span. */
    temperature_centi_c =
        3000LL +
        (
            ((int64_t)temperature_offset * 8000LL) /
            (int64_t)temperature_span
        );

    /* Calculate vibration RMS from every individual sample rather than only the average. */
    for (frame_index = 0U;
         frame_index < frame_count;
         ++frame_index)
    {
        /* Read the vibration sample at the fixed first scan position. */
        uint16_t raw_vibration =
            samples[
                frame_index *
                GUARDIAN_ACQUISITION_CHANNEL_COUNT
            ];

        /* Normalize the raw vibration sample into the factory 3.3 V code domain. */
        uint32_t normalized_vibration =
            guardian_acquisition_normalize_code(
                raw_vibration,
                vdda_mv);

        /* Subtract the calibrated zero point only after both values share the same voltage domain. */
        int32_t normalized_delta =
            (int32_t)normalized_vibration -
            (int32_t)acquisition->calibration.vibration_zero_code;

        /* Promote before multiplication so squaring cannot overflow signed 32-bit arithmetic. */
        int64_t wide_delta =
            (int64_t)normalized_delta;

        /* Accumulate squared normalized vibration magnitude. */
        vibration_square_sum +=
            (uint64_t)(wide_delta * wide_delta);
    }

    /* Calculate the mean squared normalized vibration code. */
    vibration_square_sum /=
        (uint64_t)frame_count;

    /* Calculate integer RMS without requiring libm in embedded builds. */
    vibration_rms_code =
        guardian_acquisition_isqrt_u64(
            vibration_square_sum);

    /* Convert normalized RMS code into configured milli-g units. */
    vibration_mg =
        (
            (uint64_t)vibration_rms_code *
            (uint64_t)acquisition->calibration.vibration_mg_per_code_num
        ) /
        (uint64_t)acquisition->calibration.vibration_mg_per_code_den;

    /* Normalize the average current sample into the factory 3.3 V code domain. */
    uint32_t normalized_current_code =
        guardian_acquisition_normalize_code(
            average[
                GUARDIAN_ACQUISITION_CHANNEL_CURRENT],
            vdda_mv);

    /* Subtract the calibrated current zero point in the same normalized voltage domain. */
    current_delta =
        (int32_t)normalized_current_code -
        (int32_t)acquisition->calibration.current_zero_code;

    /* Clamp a unidirectional current sensor below its configured zero point. */
    if (current_delta < 0)
    {
        /* Publish zero current instead of wrapping a signed delta. */
        current_delta = 0;
    }

    /* Convert normalized current code into configured milliamperes. */
    current_ma =
        (
            (uint64_t)(uint32_t)current_delta *
            (uint64_t)acquisition->calibration.current_ma_per_code_num
        ) /
        (uint64_t)acquisition->calibration.current_ma_per_code_den;

    /* Convert the external supply-divider ADC input into its pin voltage. */
    supply_mv =
        (
            (uint64_t)average[
                GUARDIAN_ACQUISITION_CHANNEL_SUPPLY] *
            (uint64_t)vdda_mv
        ) /
        (uint64_t)GUARDIAN_ACQUISITION_ADC_MAX_CODE;

    /* Apply the explicit external resistor-divider ratio. */
    supply_mv =
        (
            supply_mv *
            (uint64_t)acquisition->calibration.supply_divider_num
        ) /
        (uint64_t)acquisition->calibration.supply_divider_den;

    /* Publish bounded signed temperature. */
    measurements.temperature_centi_c =
        guardian_acquisition_i16(
            temperature_centi_c);

    /* Publish bounded vibration RMS magnitude. */
    measurements.vibration_mg_rms =
        guardian_acquisition_u16(
            vibration_mg);

    /* Publish bounded machine current. */
    measurements.current_ma =
        guardian_acquisition_u16(
            current_ma);

    /* Publish fresh RPM only when the timer capture is valid. */
    measurements.rpm =
        (aux->rpm_valid != 0U)
        ? aux->rpm
        : 0U;

    /* Publish bounded measured external supply voltage. */
    measurements.supply_mv =
        guardian_acquisition_u16(
            supply_mv);

    /* Start with calibration and hardware-quality flags supplied by the caller. */
    measurements.status_flags =
        acquisition->calibration.base_status_flags |
        aux->hardware_status_flags;

    /* Mark every successfully processed block as valid. */
    measurements.status_flags |=
        GUARDIAN_ACQUISITION_STATUS_VALID;

    /* Mark missing or stale RPM capture explicitly. */
    if (aux->rpm_valid == 0U)
    {
        /* Preserve data-quality visibility in M5 telemetry. */
        measurements.status_flags |=
            GUARDIAN_ACQUISITION_STATUS_RPM_STALE;
    }

    /* Mark ADC rail contact explicitly. */
    if (saturated != 0U)
    {
        /* Preserve data-quality visibility in M5 telemetry. */
        measurements.status_flags |=
            GUARDIAN_ACQUISITION_STATUS_ADC_SATURATED;

        /* Count the saturated block once. */
        guardian_acquisition_increment_u32(
            &acquisition->stats.saturated_blocks);
    }

    /* Publish the complete coherent measurement snapshot only after successful processing. */
    acquisition->latest = measurements;

    /* Count the successfully processed block. */
    guardian_acquisition_increment_u32(
        &acquisition->stats.blocks_processed);

    /* Report successful processing. */
    return GUARDIAN_ACQUISITION_OK;
}

/* Return the latest successfully processed measurement snapshot. */
guardian_machine_measurements_t guardian_acquisition_latest(
    const guardian_acquisition_t *acquisition)
{
    /* Create deterministic zero measurements for an invalid caller pointer. */
    guardian_machine_measurements_t empty = {0};

    /* Handle a missing acquisition pointer safely. */
    if (acquisition == NULL)
    {
        /* Return deterministic empty measurements. */
        return empty;
    }

    /* Return the latest coherent snapshot by value. */
    return acquisition->latest;
}

/* Return a copy of cumulative portable acquisition diagnostics. */
guardian_acquisition_stats_t guardian_acquisition_stats(
    const guardian_acquisition_t *acquisition)
{
    /* Create deterministic zero diagnostics for an invalid caller pointer. */
    guardian_acquisition_stats_t empty = {0};

    /* Handle a missing acquisition pointer safely. */
    if (acquisition == NULL)
    {
        /* Return deterministic empty diagnostics. */
        return empty;
    }

    /* Return cumulative diagnostics by value. */
    return acquisition->stats;
}
