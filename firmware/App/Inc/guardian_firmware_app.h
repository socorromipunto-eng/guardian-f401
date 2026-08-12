#ifndef GUARDIAN_FIRMWARE_APP_H
#define GUARDIAN_FIRMWARE_APP_H

/* Include the published Guardian application-state registry. */
#include "guardian_device_service.h"

/* Include the M5 application measurement snapshot. */
#include "guardian_telemetry.h"

/* Include fixed-width integer types for baud and tick APIs. */
#include <stdint.h>

/* Initialize the physical STM32F401 Guardian command channel. */
int guardian_firmware_app_init(
    uint32_t baud_rate,
    uint32_t uart_irq_priority);

/* Execute bounded foreground Guardian command and telemetry work. */
void guardian_firmware_app_poll(void);

/* Advance firmware uptime and telemetry scheduling by one millisecond. */
void guardian_firmware_app_tick_1ms(void);

/* Change the application state published by status and telemetry. */
void guardian_firmware_app_set_state(
    guardian_device_state_t state);

/* Replace the latest application-provided telemetry measurements. */
void guardian_firmware_app_update_telemetry(
    const guardian_machine_measurements_t *measurements);

#endif
