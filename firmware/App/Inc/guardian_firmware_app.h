#ifndef GUARDIAN_FIRMWARE_APP_H
#define GUARDIAN_FIRMWARE_APP_H

/* Include the published Guardian application-state registry. */
#include "guardian_device_service.h"

/* Include fixed-width integer types for baud and tick APIs. */
#include <stdint.h>

/* Initialize the physical STM32F401 Guardian command channel. */
int guardian_firmware_app_init(
    uint32_t baud_rate,
    uint32_t uart_irq_priority);

/* Execute bounded foreground Guardian protocol work. */
void guardian_firmware_app_poll(void);

/* Advance the middleware monotonic uptime source by one millisecond. */
void guardian_firmware_app_tick_1ms(void);

/* Change the application state published by GET_STATUS. */
void guardian_firmware_app_set_state(
    guardian_device_state_t state);

#endif
