/* Include the public Guardian firmware integration API. */
#include "guardian_firmware_app.h"

/* Include the transport-independent embedded protocol middleware. */
#include "guardian_embedded_link.h"

/* Include the STM32F401 USART2 platform adapter. */
#include "stm32f401_uart2.h"

/* Define the firmware model name exposed to guardianctl. */
static const char guardian_model[] = "Guardian-F401-HW";

/* Store the transport-independent middleware instance. */
static guardian_embedded_link_t guardian_link;

/* Store monotonic milliseconds advanced by the existing application tick. */
static volatile uint32_t guardian_uptime_ms = 0U;

/* Return whole monotonic uptime seconds to middleware. */
static uint32_t guardian_firmware_uptime_seconds(
    void *context)
{
    /* Mark the generic callback context as unused. */
    (void)context;

    /* Convert monotonic milliseconds into whole seconds. */
    return guardian_uptime_ms / 1000U;
}

/* Initialize the complete STM32F401 Guardian physical path. */
int guardian_firmware_app_init(
    uint32_t baud_rate,
    uint32_t uart_irq_priority)
{
    /* Store platform-independent I/O callbacks. */
    guardian_embedded_io_t io = {0};

    /* Store immutable public device identity. */
    guardian_device_identity_t identity = {0};

    /* Store middleware initialization status. */
    guardian_protocol_result_t result =
        GUARDIAN_PROTOCOL_OK;

    /* Initialize the physical USART2 command channel first. */
    if (guardian_stm32f401_uart2_init(
            baud_rate,
            uart_irq_priority) == 0)
    {
        /* Report hardware initialization failure. */
        return 0;
    }

    /* Reset application uptime only after hardware initialization succeeds. */
    guardian_uptime_ms = 0U;

    /* Connect middleware RX to the interrupt-backed USART2 RX queue. */
    io.read_byte =
        guardian_stm32f401_uart2_read_byte;

    /* Connect middleware TX to the interrupt-backed USART2 TX queue. */
    io.write =
        guardian_stm32f401_uart2_write;

    /* Connect middleware uptime to application time. */
    io.uptime_seconds =
        guardian_firmware_uptime_seconds;

    /* No platform context is required by the singleton USART2 adapter. */
    io.context = NULL;

    /* Publish the hardware-specific Guardian model name. */
    identity.model = guardian_model;

    /* Publish firmware milestone major version. */
    identity.firmware_major = 0U;

    /* Publish firmware milestone minor version. */
    identity.firmware_minor = 5U;

    /* Publish firmware milestone patch version. */
    identity.firmware_patch = 0U;

    /* Derive a non-security display identifier from the STM32 factory UID. */
    identity.device_id =
        guardian_stm32f401_public_device_id();

    /* Initialize parser, command service, telemetry and callbacks. */
    result =
        guardian_embedded_link_init(
            &guardian_link,
            &io,
            &identity);

    /* Report middleware initialization failure. */
    if (result != GUARDIAN_PROTOCOL_OK)
    {
        /* Report failed startup. */
        return 0;
    }

    /* Report successful physical Guardian startup. */
    return 1;
}

/* Execute bounded command and telemetry work from the main loop. */
void guardian_firmware_app_poll(void)
{
    /* Process bounded RX work and at most one due telemetry frame. */
    guardian_embedded_link_poll(
        &guardian_link,
        GUARDIAN_EMBEDDED_DEFAULT_RX_BUDGET);
}

/* Advance monotonic firmware time and telemetry scheduling. */
void guardian_firmware_app_tick_1ms(void)
{
    /* Avoid uptime counter wrap so GET_STATUS remains monotonic. */
    if (guardian_uptime_ms != 0xFFFFFFFFUL)
    {
        /* Advance one application millisecond. */
        guardian_uptime_ms += 1U;
    }

    /* Advance the independent M5 telemetry timestamp and scheduler. */
    guardian_embedded_link_tick_1ms(
        &guardian_link);
}

/* Change the state exposed by status and telemetry. */
void guardian_firmware_app_set_state(
    guardian_device_state_t state)
{
    /* Forward application state into middleware. */
    guardian_embedded_link_set_state(
        &guardian_link,
        state);
}

/* Replace the latest application-provided telemetry measurements. */
void guardian_firmware_app_update_telemetry(
    const guardian_machine_measurements_t *measurements)
{
    /* Forward the bounded snapshot into middleware. */
    guardian_embedded_link_update_telemetry(
        &guardian_link,
        measurements);
}
