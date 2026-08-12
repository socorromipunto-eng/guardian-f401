#ifndef GUARDIAN_TELEMETRY_H
#define GUARDIAN_TELEMETRY_H

/* Include the application state registry and Guardian frame types. */
#include "guardian_device_service.h"

/* Include fixed-width integer types for deterministic telemetry fields. */
#include <stdint.h>

/* Define the first M5 telemetry payload schema version. */
#define GUARDIAN_TELEMETRY_SCHEMA_VERSION ((uint8_t)0x01U)

/* Define the minimum allowed telemetry period. */
#define GUARDIAN_TELEMETRY_MIN_PERIOD_MS ((uint16_t)100U)

/* Define the maximum allowed telemetry period. */
#define GUARDIAN_TELEMETRY_MAX_PERIOD_MS ((uint16_t)60000U)

/* Define the default telemetry period. */
#define GUARDIAN_TELEMETRY_DEFAULT_PERIOD_MS ((uint16_t)1000U)

/* Store one application-provided machine measurement snapshot. */
typedef struct
{
    /* Store temperature in signed hundredths of one degree Celsius. */
    int16_t temperature_centi_c;

    /* Store RMS vibration magnitude in milli-g. */
    uint16_t vibration_mg_rms;

    /* Store measured machine current in milliamperes. */
    uint16_t current_ma;

    /* Store shaft speed in revolutions per minute. */
    uint16_t rpm;

    /* Store measured supply voltage in millivolts. */
    uint16_t supply_mv;

    /* Store application-defined telemetry status flags. */
    uint16_t status_flags;
} guardian_machine_measurements_t;

/* Store asynchronous telemetry scheduling and latest measurements. */
typedef struct
{
    /* Store whether asynchronous telemetry transmission is enabled. */
    uint8_t enabled;

    /* Store the bounded configured transmission period. */
    uint16_t period_ms;

    /* Store elapsed enabled time since the most recent emission. */
    uint32_t elapsed_ms;

    /* Store a monotonic millisecond timestamp modulo 2^32. */
    uint32_t timestamp_ms;

    /* Store the next non-zero telemetry frame sequence. */
    uint32_t next_sequence;

    /* Store the latest application-provided measurement snapshot. */
    guardian_machine_measurements_t measurements;
} guardian_telemetry_t;

/* Initialize deterministic telemetry state. */
void guardian_telemetry_init(
    guardian_telemetry_t *telemetry);

/* Advance telemetry scheduling time by one millisecond. */
void guardian_telemetry_tick_1ms(
    guardian_telemetry_t *telemetry);

/* Replace the latest application measurement snapshot. */
void guardian_telemetry_set_measurements(
    guardian_telemetry_t *telemetry,
    const guardian_machine_measurements_t *measurements);

/* Process SET_TELEMETRY and create one correlated response or ERROR frame. */
guardian_protocol_result_t guardian_telemetry_handle_request(
    guardian_telemetry_t *telemetry,
    const guardian_frame_t *request,
    guardian_frame_t *response);

/* Create one due MACHINE_TELEMETRY frame without burst catch-up. */
int guardian_telemetry_make_due_frame(
    guardian_telemetry_t *telemetry,
    guardian_device_state_t state,
    guardian_frame_t *frame);

#endif
