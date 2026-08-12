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

/* Return whole monotonic uptime seconds. */
static uint32_t guardian_firmware_uptime_seconds(void *context)
{
    /* Mark generic callback context as unused. */
    (void)context;

    /* Convert milliseconds into whole seconds. */
    return guardian_uptime_ms / 1000U;
}

/* Initialize the complete STM32F401 Guardian physical command path. */
int guardian_firmware_app_init(
    uint32_t baud_rate,
    uint32_t uart_irq_priority)
{
    /* Store platform callbacks. */
    guardian_embedded_io_t io = {0};

    /* Store public device identity. */
    guardian_device_identity_t identity = {0};

    /* Store middleware initialization status. */
    guardian_protocol_result_t result = GUARDIAN_PROTOCOL_OK;

    /* Initialize USART2. */
    if (guardian_stm32f401_uart2_init(
            baud_rate,
            uart_irq_priority) == 0)
    {
        /* Report hardware initialization failure. */
        return 0;
    }

    /* Reset uptime after hardware startup. */
    guardian_uptime_ms = 0U;

    /* Connect RX callback. */
    io.read_byte = guardian_stm32f401_uart2_read_byte;

    /* Connect TX callback. */
    io.write = guardian_stm32f401_uart2_write;

    /* Connect uptime callback. */
    io.uptime_seconds = guardian_firmware_uptime_seconds;

    /* No context is required by the singleton adapter. */
    io.context = NULL;

    /* Publish hardware model name. */
    identity.model = guardian_model;

    /* Publish firmware major version. */
    identity.firmware_major = 0U;

    /* Publish firmware minor version. */
    identity.firmware_minor = 4U;

    /* Publish firmware patch version. */
    identity.firmware_patch = 0U;

    /* Publish a non-security device display ID. */
    identity.device_id =
        guardian_stm32f401_public_device_id();

    /* Initialize middleware. */
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

    /* Report successful startup. */
    return 1;
}

/* Execute bounded protocol work from the main loop. */
void guardian_firmware_app_poll(void)
{
    /* Process the documented default RX byte budget. */
    guardian_embedded_link_poll(
        &guardian_link,
        GUARDIAN_EMBEDDED_DEFAULT_RX_BUDGET);
}

/* Advance monotonic uptime from an existing one-millisecond application tick. */
void guardian_firmware_app_tick_1ms(void)
{
    /* Avoid millisecond wrap. */
    if (guardian_uptime_ms != 0xFFFFFFFFUL)
    {
        /* Advance one millisecond. */
        guardian_uptime_ms += 1U;
    }
}

/* Change the state exposed by GET_STATUS. */
void guardian_firmware_app_set_state(
    guardian_device_state_t state)
{
    /* Forward the application state into middleware. */
    guardian_embedded_link_set_state(
        &guardian_link,
        state);
}
