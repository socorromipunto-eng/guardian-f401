/* Include the portable M6 acquisition processor under test. */
#include "guardian_acquisition.h"

/* Include assertion support for deterministic host-side verification. */
#include <assert.h>

/* Include fixed-width integer types used by synthetic ADC blocks. */
#include <stdint.h>

/* Include standard output for one concise CI success message. */
#include <stdio.h>

/* Define the exact number of synthetic raw samples in one default M6 block. */
#define TEST_SAMPLE_COUNT GUARDIAN_ACQUISITION_SAMPLES_PER_BLOCK

/* Fill one deterministic five-channel interleaved ADC block. */
static void test_fill_block(
    uint16_t *samples)
{
    /* Track the current complete ADC scan frame. */
    size_t frame = 0U;

    /* Fill every complete synthetic scan frame. */
    for (frame = 0U;
         frame < GUARDIAN_ACQUISITION_FRAMES_PER_BLOCK;
         ++frame)
    {
        /* Calculate the first sample index for this scan frame. */
        size_t base =
            frame *
            GUARDIAN_ACQUISITION_CHANNEL_COUNT;

        /* Alternate vibration around the configured midscale by exactly ten codes. */
        samples[
            base +
            GUARDIAN_ACQUISITION_CHANNEL_VIBRATION
        ] =
            ((frame & 1U) == 0U)
            ? 2058U
            : 2038U;

        /* Hold current-sensor code constant. */
        samples[
            base +
            GUARDIAN_ACQUISITION_CHANNEL_CURRENT
        ] = 1000U;

        /* Hold supply-divider input close to half-scale. */
        samples[
            base +
            GUARDIAN_ACQUISITION_CHANNEL_SUPPLY
        ] = 2048U;

        /* Use the midpoint between synthetic 30 C and 110 C calibration codes. */
        samples[
            base +
            GUARDIAN_ACQUISITION_CHANNEL_TEMPERATURE
        ] = 900U;

        /* Match the synthetic factory VREFINT calibration code exactly. */
        samples[
            base +
            GUARDIAN_ACQUISITION_CHANNEL_VREFINT
        ] = 1500U;
    }
}

/* Return deterministic calibration with simple exact engineering-unit math. */
static guardian_acquisition_calibration_t test_calibration(void)
{
    /* Create deterministic zero-initialized calibration storage. */
    guardian_acquisition_calibration_t calibration = {0};

    /* Configure VREFINT factory code at 3.3 V. */
    calibration.vrefint_cal_code = 1500U;

    /* Configure synthetic 30 C temperature calibration code. */
    calibration.temperature_cal_30_code = 1000U;

    /* Configure synthetic 110 C temperature calibration code. */
    calibration.temperature_cal_110_code = 800U;

    /* Configure vibration midpoint. */
    calibration.vibration_zero_code = 2048U;

    /* Map each normalized vibration code to two milli-g. */
    calibration.vibration_mg_per_code_num = 2U;

    /* Keep the vibration scale denominator at one. */
    calibration.vibration_mg_per_code_den = 1U;

    /* Configure current-sensor zero point. */
    calibration.current_zero_code = 100U;

    /* Map each normalized current code to two milliamperes. */
    calibration.current_ma_per_code_num = 2U;

    /* Keep the current scale denominator at one. */
    calibration.current_ma_per_code_den = 1U;

    /* Model a 1:1 external resistor divider. */
    calibration.supply_divider_num = 2U;

    /* Keep the divider denominator at one. */
    calibration.supply_divider_den = 1U;

    /* Start without persistent hardware quality flags. */
    calibration.base_status_flags = 0U;

    /* Return the complete deterministic calibration by value. */
    return calibration;
}

/* Verify one valid raw block converts into the expected engineering values. */
static void test_process_known_block(void)
{
    /* Allocate one complete synthetic DMA target block. */
    uint16_t samples[TEST_SAMPLE_COUNT] = {0};

    /* Create deterministic portable acquisition state. */
    guardian_acquisition_t acquisition = {0};

    /* Create deterministic calibration. */
    guardian_acquisition_calibration_t calibration =
        test_calibration();

    /* Create fresh auxiliary RPM state. */
    guardian_acquisition_aux_t aux = {0};

    /* Store the converted engineering-unit snapshot. */
    guardian_machine_measurements_t measurements = {0};

    /* Fill all raw scan frames. */
    test_fill_block(samples);

    /* Configure one fresh 1500 RPM capture. */
    aux.rpm = 1500U;

    /* Mark the RPM capture valid. */
    aux.rpm_valid = 1U;

    /* Initialize the portable processor. */
    assert(
        guardian_acquisition_init(
            &acquisition,
            &calibration) ==
        GUARDIAN_ACQUISITION_OK);

    /* Process the complete interleaved block. */
    assert(
        guardian_acquisition_process_block(
            &acquisition,
            samples,
            TEST_SAMPLE_COUNT,
            &aux) ==
        GUARDIAN_ACQUISITION_OK);

    /* Read the coherent converted snapshot. */
    measurements =
        guardian_acquisition_latest(
            &acquisition);

    /* Require 70.00 C from midpoint factory calibration interpolation. */
    assert(
        measurements.temperature_centi_c ==
        7000);

    /* Require exactly 20 milli-g RMS from alternating plus/minus ten codes. */
    assert(
        measurements.vibration_mg_rms ==
        20U);

    /* Require 1800 mA from a 900-code current delta at two mA per code. */
    assert(
        measurements.current_ma ==
        1800U);

    /* Require the fresh RPM capture unchanged. */
    assert(
        measurements.rpm ==
        1500U);

    /* Require approximately 3.3 V after the reference 1:1 divider compensation. */
    assert(
        (measurements.supply_mv >= 3298U) &&
        (measurements.supply_mv <= 3302U));

    /* Require the VALID quality bit. */
    assert(
        (measurements.status_flags &
         GUARDIAN_ACQUISITION_STATUS_VALID) != 0U);

    /* Require no stale RPM indication. */
    assert(
        (measurements.status_flags &
         GUARDIAN_ACQUISITION_STATUS_RPM_STALE) == 0U);
}


/* Verify VREFINT compensation keeps normalized sensor engineering values stable at lower VDDA. */
static void test_vdda_compensation(void)
{
    /* Allocate one complete synthetic DMA target block. */
    uint16_t samples[TEST_SAMPLE_COUNT] = {0};

    /* Create portable acquisition state. */
    guardian_acquisition_t acquisition = {0};

    /* Create deterministic factory-domain calibration. */
    guardian_acquisition_calibration_t calibration =
        test_calibration();

    /* Create fresh auxiliary RPM state. */
    guardian_acquisition_aux_t aux = {0};

    /* Store the converted measurement snapshot. */
    guardian_machine_measurements_t measurements = {0};

    /* Track the current complete ADC scan frame. */
    size_t frame = 0U;

    /* Fill every scan frame with raw codes corresponding to approximately 3.0 V VDDA. */
    for (frame = 0U;
         frame < GUARDIAN_ACQUISITION_FRAMES_PER_BLOCK;
         ++frame)
    {
        /* Calculate the first raw sample position for this scan frame. */
        size_t base =
            frame *
            GUARDIAN_ACQUISITION_CHANNEL_COUNT;

        /* Encode normalized vibration code 2058 at approximately 3.0 V VDDA. */
        samples[
            base +
            GUARDIAN_ACQUISITION_CHANNEL_VIBRATION
        ] =
            ((frame & 1U) == 0U)
            ? 2264U
            : 2242U;

        /* Encode normalized current code 1000 at approximately 3.0 V VDDA. */
        samples[
            base +
            GUARDIAN_ACQUISITION_CHANNEL_CURRENT
        ] = 1100U;

        /* Encode approximately 1.65 V at the PA4 divider input with 3.0 V VDDA. */
        samples[
            base +
            GUARDIAN_ACQUISITION_CHANNEL_SUPPLY
        ] = 2252U;

        /* Encode normalized temperature code 900 at approximately 3.0 V VDDA. */
        samples[
            base +
            GUARDIAN_ACQUISITION_CHANNEL_TEMPERATURE
        ] = 990U;

        /* Encode the expected VREFINT raw code for 3.0 V from a 1500-code 3.3 V calibration. */
        samples[
            base +
            GUARDIAN_ACQUISITION_CHANNEL_VREFINT
        ] = 1650U;
    }

    /* Configure one fresh RPM capture. */
    aux.rpm = 1500U;

    /* Mark the RPM capture valid. */
    aux.rpm_valid = 1U;

    /* Initialize the portable acquisition processor. */
    assert(
        guardian_acquisition_init(
            &acquisition,
            &calibration) ==
        GUARDIAN_ACQUISITION_OK);

    /* Process the lower-VDDA synthetic block. */
    assert(
        guardian_acquisition_process_block(
            &acquisition,
            samples,
            TEST_SAMPLE_COUNT,
            &aux) ==
        GUARDIAN_ACQUISITION_OK);

    /* Read the coherent engineering-unit snapshot. */
    measurements =
        guardian_acquisition_latest(
            &acquisition);

    /* Require temperature compensation to preserve the 70.00 C factory-domain result. */
    assert(
        measurements.temperature_centi_c ==
        7000);

    /* Require vibration compensation to preserve approximately 20 milli-g RMS. */
    assert(
        (measurements.vibration_mg_rms >= 18U) &&
        (measurements.vibration_mg_rms <= 22U));

    /* Require current compensation to preserve the 1800 mA calibrated result. */
    assert(
        measurements.current_ma ==
        1800U);

    /* Require the resistor-divider result to remain approximately 3.3 V. */
    assert(
        (measurements.supply_mv >= 3297U) &&
        (measurements.supply_mv <= 3303U));
}

/* Verify stale RPM state is explicit and does not publish an old speed. */
static void test_stale_rpm_flag(void)
{
    /* Allocate one complete synthetic DMA target block. */
    uint16_t samples[TEST_SAMPLE_COUNT] = {0};

    /* Create portable acquisition state. */
    guardian_acquisition_t acquisition = {0};

    /* Create deterministic calibration. */
    guardian_acquisition_calibration_t calibration =
        test_calibration();

    /* Create deliberately stale RPM state. */
    guardian_acquisition_aux_t aux = {0};

    /* Store the converted measurement snapshot. */
    guardian_machine_measurements_t measurements = {0};

    /* Fill the raw ADC block. */
    test_fill_block(samples);

    /* Leave rpm_valid cleared intentionally. */
    aux.rpm_valid = 0U;

    /* Initialize the processor. */
    assert(
        guardian_acquisition_init(
            &acquisition,
            &calibration) ==
        GUARDIAN_ACQUISITION_OK);

    /* Process the block successfully despite missing RPM. */
    assert(
        guardian_acquisition_process_block(
            &acquisition,
            samples,
            TEST_SAMPLE_COUNT,
            &aux) ==
        GUARDIAN_ACQUISITION_OK);

    /* Read the converted snapshot. */
    measurements =
        guardian_acquisition_latest(
            &acquisition);

    /* Require stale RPM to publish zero speed rather than old data. */
    assert(measurements.rpm == 0U);

    /* Require the explicit RPM_STALE quality flag. */
    assert(
        (measurements.status_flags &
         GUARDIAN_ACQUISITION_STATUS_RPM_STALE) != 0U);
}

/* Verify ADC rail contact is preserved as a telemetry quality flag. */
static void test_adc_saturation_flag(void)
{
    /* Allocate one complete synthetic DMA target block. */
    uint16_t samples[TEST_SAMPLE_COUNT] = {0};

    /* Create portable acquisition state. */
    guardian_acquisition_t acquisition = {0};

    /* Create deterministic calibration. */
    guardian_acquisition_calibration_t calibration =
        test_calibration();

    /* Create valid auxiliary RPM state. */
    guardian_acquisition_aux_t aux = {0};

    /* Store the converted measurement snapshot. */
    guardian_machine_measurements_t measurements = {0};

    /* Fill the normal raw ADC block first. */
    test_fill_block(samples);

    /* Force exactly one vibration sample to the upper ADC rail. */
    samples[
        GUARDIAN_ACQUISITION_CHANNEL_VIBRATION
    ] =
        GUARDIAN_ACQUISITION_ADC_MAX_CODE;

    /* Mark the RPM capture fresh. */
    aux.rpm_valid = 1U;

    /* Initialize the processor. */
    assert(
        guardian_acquisition_init(
            &acquisition,
            &calibration) ==
        GUARDIAN_ACQUISITION_OK);

    /* Process the rail-contact block. */
    assert(
        guardian_acquisition_process_block(
            &acquisition,
            samples,
            TEST_SAMPLE_COUNT,
            &aux) ==
        GUARDIAN_ACQUISITION_OK);

    /* Read the converted snapshot. */
    measurements =
        guardian_acquisition_latest(
            &acquisition);

    /* Require the explicit ADC saturation quality flag. */
    assert(
        (measurements.status_flags &
         GUARDIAN_ACQUISITION_STATUS_ADC_SATURATED) != 0U);

    /* Require one saturated-block diagnostic count. */
    assert(
        guardian_acquisition_stats(
            &acquisition
        ).saturated_blocks == 1U);
}

/* Verify a zero VREFINT block is rejected before voltage-dependent conversion. */
static void test_invalid_reference_rejected(void)
{
    /* Allocate one complete synthetic DMA target block. */
    uint16_t samples[TEST_SAMPLE_COUNT] = {0};

    /* Create portable acquisition state. */
    guardian_acquisition_t acquisition = {0};

    /* Create deterministic calibration. */
    guardian_acquisition_calibration_t calibration =
        test_calibration();

    /* Create auxiliary input state. */
    guardian_acquisition_aux_t aux = {0};

    /* Track the current scan frame. */
    size_t frame = 0U;

    /* Fill a valid raw ADC block first. */
    test_fill_block(samples);

    /* Force every VREFINT conversion to zero. */
    for (frame = 0U;
         frame < GUARDIAN_ACQUISITION_FRAMES_PER_BLOCK;
         ++frame)
    {
        /* Calculate the VREFINT position in this scan frame. */
        size_t index =
            (frame *
             GUARDIAN_ACQUISITION_CHANNEL_COUNT) +
            GUARDIAN_ACQUISITION_CHANNEL_VREFINT;

        /* Remove the required voltage reference measurement. */
        samples[index] = 0U;
    }

    /* Initialize the processor. */
    assert(
        guardian_acquisition_init(
            &acquisition,
            &calibration) ==
        GUARDIAN_ACQUISITION_OK);

    /* Require explicit reference failure. */
    assert(
        guardian_acquisition_process_block(
            &acquisition,
            samples,
            TEST_SAMPLE_COUNT,
            &aux) ==
        GUARDIAN_ACQUISITION_ERROR_REFERENCE);

    /* Require one invalid-block diagnostic count. */
    assert(
        guardian_acquisition_stats(
            &acquisition
        ).invalid_blocks == 1U);
}

/* Verify malformed interleaved lengths fail closed. */
static void test_invalid_block_length_rejected(void)
{
    /* Allocate one small synthetic sample array. */
    uint16_t samples[6] = {0};

    /* Create portable acquisition state. */
    guardian_acquisition_t acquisition = {0};

    /* Create deterministic calibration. */
    guardian_acquisition_calibration_t calibration =
        test_calibration();

    /* Create auxiliary input state. */
    guardian_acquisition_aux_t aux = {0};

    /* Initialize the processor. */
    assert(
        guardian_acquisition_init(
            &acquisition,
            &calibration) ==
        GUARDIAN_ACQUISITION_OK);

    /* Six samples cannot contain an integer number of five-channel scan frames. */
    assert(
        guardian_acquisition_process_block(
            &acquisition,
            samples,
            6U,
            &aux) ==
        GUARDIAN_ACQUISITION_ERROR_LENGTH);
}

/* Execute every M6 portable acquisition test. */
int main(void)
{
    /* Verify deterministic engineering-unit conversion. */
    test_process_known_block();

    /* Verify VREFINT compensation across a lower VDDA operating point. */
    test_vdda_compensation();

    /* Verify stale RPM quality reporting. */
    test_stale_rpm_flag();

    /* Verify ADC saturation quality reporting. */
    test_adc_saturation_flag();

    /* Verify VREFINT validation. */
    test_invalid_reference_rejected();

    /* Verify interleaved block framing validation. */
    test_invalid_block_length_rejected();

    /* Print one concise success message for local and CI logs. */
    (void)printf("Guardian M6 acquisition host tests: PASS\n");

    /* Return the conventional successful process status. */
    return 0;
}
