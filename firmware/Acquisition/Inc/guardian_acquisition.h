#ifndef GUARDIAN_ACQUISITION_H
#define GUARDIAN_ACQUISITION_H

/* Include the M5 machine measurement structure consumed by telemetry. */
#include "guardian_telemetry.h"

/* Include size_t for bounded interleaved sample blocks. */
#include <stddef.h>

/* Include fixed-width integer types for deterministic ADC and calibration fields. */
#include <stdint.h>

/* Define the fixed 12-bit STM32F401 ADC full-scale code. */
#define GUARDIAN_ACQUISITION_ADC_MAX_CODE ((uint16_t)4095U)

/* Define the factory calibration reference voltage in millivolts. */
#define GUARDIAN_ACQUISITION_FACTORY_VDDA_MV ((uint32_t)3300UL)

/* Define the number of ADC conversions in one deterministic scan frame. */
#define GUARDIAN_ACQUISITION_CHANNEL_COUNT ((size_t)5U)

/* Define the default number of complete scan frames in one DMA target buffer. */
#define GUARDIAN_ACQUISITION_FRAMES_PER_BLOCK ((size_t)64U)

/* Define the number of half-word DMA transfers in one target buffer. */
#define GUARDIAN_ACQUISITION_SAMPLES_PER_BLOCK \
    (GUARDIAN_ACQUISITION_CHANNEL_COUNT * GUARDIAN_ACQUISITION_FRAMES_PER_BLOCK)

/* Publish one status bit when the measurement block is valid. */
#define GUARDIAN_ACQUISITION_STATUS_VALID ((uint16_t)0x0001U)

/* Publish one status bit when at least one ADC sample touched a rail. */
#define GUARDIAN_ACQUISITION_STATUS_ADC_SATURATED ((uint16_t)0x0002U)

/* Publish one status bit when the RPM capture is unavailable or stale. */
#define GUARDIAN_ACQUISITION_STATUS_RPM_STALE ((uint16_t)0x0004U)

/* Publish one status bit when hardware reported a dropped DMA block. */
#define GUARDIAN_ACQUISITION_STATUS_DMA_DROP ((uint16_t)0x0008U)

/* Publish one status bit while reference external-sensor calibration is still in use. */
#define GUARDIAN_ACQUISITION_STATUS_DEFAULT_CALIBRATION ((uint16_t)0x0010U)

/* Publish one status bit when the ADC hardware reported an overrun. */
#define GUARDIAN_ACQUISITION_STATUS_ADC_ERROR ((uint16_t)0x0020U)

/* Publish one status bit when the DMA hardware reported a transfer fault. */
#define GUARDIAN_ACQUISITION_STATUS_DMA_ERROR ((uint16_t)0x0040U)

/* Define the fixed conversion order used by ADC1 scan mode and DMA. */
typedef enum
{
    /* Store the vibration front-end conversion first. */
    GUARDIAN_ACQUISITION_CHANNEL_VIBRATION = 0,

    /* Store the current-sensor conversion second. */
    GUARDIAN_ACQUISITION_CHANNEL_CURRENT = 1,

    /* Store the external supply-divider conversion third. */
    GUARDIAN_ACQUISITION_CHANNEL_SUPPLY = 2,

    /* Store the internal temperature-sensor conversion fourth. */
    GUARDIAN_ACQUISITION_CHANNEL_TEMPERATURE = 3,

    /* Store the internal VREFINT conversion fifth. */
    GUARDIAN_ACQUISITION_CHANNEL_VREFINT = 4
} guardian_acquisition_channel_t;

/* Define deterministic portable acquisition outcomes. */
typedef enum
{
    /* Indicate successful block processing. */
    GUARDIAN_ACQUISITION_OK = 0,

    /* Indicate a missing required pointer. */
    GUARDIAN_ACQUISITION_ERROR_NULL_ARGUMENT,

    /* Indicate a block whose sample count is not a complete scan-frame multiple. */
    GUARDIAN_ACQUISITION_ERROR_LENGTH,

    /* Indicate unusable calibration constants. */
    GUARDIAN_ACQUISITION_ERROR_CALIBRATION,

    /* Indicate an invalid VREFINT sample that prevents voltage compensation. */
    GUARDIAN_ACQUISITION_ERROR_REFERENCE
} guardian_acquisition_result_t;

/* Store calibration constants used to convert raw ADC codes into engineering units. */
typedef struct
{
    /* Store the factory VREFINT raw code acquired at 3.3 V. */
    uint16_t vrefint_cal_code;

    /* Store the factory temperature raw code acquired at 30 degrees Celsius. */
    uint16_t temperature_cal_30_code;

    /* Store the factory temperature raw code acquired at 110 degrees Celsius. */
    uint16_t temperature_cal_110_code;

    /* Store the vibration front-end zero-g ADC code at the 3.3 V calibration reference. */
    uint16_t vibration_zero_code;

    /* Store the vibration milli-g numerator applied per normalized ADC code. */
    uint32_t vibration_mg_per_code_num;

    /* Store the vibration milli-g denominator applied per normalized ADC code. */
    uint32_t vibration_mg_per_code_den;

    /* Store the current-sensor zero-current ADC code at the 3.3 V calibration reference. */
    uint16_t current_zero_code;

    /* Store the current milliampere numerator applied per normalized ADC code. */
    uint32_t current_ma_per_code_num;

    /* Store the current milliampere denominator applied per normalized ADC code. */
    uint32_t current_ma_per_code_den;

    /* Store the external supply-divider multiplication numerator. */
    uint16_t supply_divider_num;

    /* Store the external supply-divider multiplication denominator. */
    uint16_t supply_divider_den;

    /* Store persistent status bits such as DEFAULT_CALIBRATION. */
    uint16_t base_status_flags;
} guardian_acquisition_calibration_t;

/* Store non-ADC measurements and hardware-quality flags captured with one block. */
typedef struct
{
    /* Store the most recent shaft speed in revolutions per minute. */
    uint16_t rpm;

    /* Store whether the RPM capture is fresh enough to publish. */
    uint8_t rpm_valid;

    /* Store hardware-quality flags that must accompany this processed block. */
    uint16_t hardware_status_flags;
} guardian_acquisition_aux_t;

/* Store cumulative portable acquisition diagnostics. */
typedef struct
{
    /* Count successfully processed ADC blocks. */
    uint32_t blocks_processed;

    /* Count rejected blocks that could not produce trustworthy measurements. */
    uint32_t invalid_blocks;

    /* Count processed blocks that contained one or more ADC rail samples. */
    uint32_t saturated_blocks;
} guardian_acquisition_stats_t;

/* Store portable acquisition calibration, latest measurements and diagnostics. */
typedef struct
{
    /* Store immutable conversion calibration copied during initialization. */
    guardian_acquisition_calibration_t calibration;

    /* Store the latest successfully processed engineering-unit measurement snapshot. */
    guardian_machine_measurements_t latest;

    /* Store cumulative portable acquisition diagnostics. */
    guardian_acquisition_stats_t stats;
} guardian_acquisition_t;

/* Initialize one portable acquisition processor from explicit calibration constants. */
guardian_acquisition_result_t guardian_acquisition_init(
    guardian_acquisition_t *acquisition,
    const guardian_acquisition_calibration_t *calibration);

/* Process one complete interleaved ADC scan block into M5 telemetry measurements. */
guardian_acquisition_result_t guardian_acquisition_process_block(
    guardian_acquisition_t *acquisition,
    const uint16_t *samples,
    size_t sample_count,
    const guardian_acquisition_aux_t *aux);

/* Return the latest successfully processed engineering-unit measurements. */
guardian_machine_measurements_t guardian_acquisition_latest(
    const guardian_acquisition_t *acquisition);

/* Return a copy of cumulative portable acquisition diagnostics. */
guardian_acquisition_stats_t guardian_acquisition_stats(
    const guardian_acquisition_t *acquisition);

#endif
