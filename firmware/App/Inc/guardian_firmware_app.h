#ifndef GUARDIAN_FIRMWARE_APP_H
#define GUARDIAN_FIRMWARE_APP_H

/* Include the published Guardian application-state registry. */
#include "guardian_device_service.h"

/* Include the M5 machine measurement snapshot consumed by telemetry. */
#include "guardian_telemetry.h"

/* Include M6 STM32F401 acquisition diagnostics. */
#include "stm32f401_acquisition.h"

/* Include M7 DSP feature snapshots. */
#include "guardian_dsp.h"

/* Include fixed-width integer types for baud and tick APIs. */
#include <stdint.h>

/* Initialize UART, Guardian middleware and M6 deterministic acquisition. */
int guardian_firmware_app_init(
    uint32_t baud_rate,
    uint32_t uart_irq_priority);

/* Execute bounded acquisition, command and telemetry foreground work. */
void guardian_firmware_app_poll(void);

/* Advance firmware uptime, RPM freshness and telemetry scheduling by one millisecond. */
void guardian_firmware_app_tick_1ms(void);

/* Change the application state published by status and telemetry. */
void guardian_firmware_app_set_state(
    guardian_device_state_t state);

/* Replace the latest telemetry snapshot manually when an application override is required. */
void guardian_firmware_app_update_telemetry(
    const guardian_machine_measurements_t *measurements);

/* Return M6 hardware acquisition diagnostics for debugger or future protocol use. */
guardian_stm32f401_acquisition_stats_t guardian_firmware_app_acquisition_stats(void);

/* Return the latest M7 DSP feature snapshot and whether it is valid. */
int guardian_firmware_app_dsp_features(
    guardian_dsp_features_t *features);

#endif
