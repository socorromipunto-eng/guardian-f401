/* Include the public Guardian firmware integration API. */
#include "guardian_firmware_app.h"

/* Include the transport-independent embedded protocol middleware. */
#include "guardian_embedded_link.h"

/* Include the STM32F401 USART2 platform adapter. */
#include "stm32f401_uart2.h"

/* Define the firmware model name exposed to guardianctl. */
static const char guardian_model[] = "Guardian-F401-HW";

/* Store the transport-independent Guardian middleware instance. */
static guardian_embedded_link_t guardian_link;

/* Store the latest successfully analyzed M7 DSP feature snapshot. */
static guardian_dsp_features_t guardian_latest_dsp_features;

/* Store whether at least one M7 DSP feature snapshot is valid. */
static uint8_t guardian_latest_dsp_valid = 0U;

/* Store monotonic milliseconds advanced by the existing application tick. */
static volatile uint32_t guardian_uptime_ms = 0U;

/* Return whole monotonic uptime seconds to Guardian middleware. */
static uint32_t guardian_firmware_uptime_seconds(
    void *context)
{
    /* Mark the generic callback context as unused. */
    (void)context;

    /* Convert monotonic milliseconds into whole seconds. */
    return guardian_uptime_ms / 1000U;
}

/* Initialize UART, Guardian middleware and deterministic M6 acquisition. */
int guardian_firmware_app_init(
    uint32_t baud_rate,
    uint32_t uart_irq_priority)
{
    /* Store platform-independent Guardian transport callbacks. */
    guardian_embedded_io_t io = {0};

    /* Store immutable public device identity. */
    guardian_device_identity_t identity = {0};

    /* Store the M6 reference acquisition configuration. */
    guardian_stm32f401_acquisition_config_t acquisition_config = {0};

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

    /* Reset application uptime only after UART hardware initialization succeeds. */
    guardian_uptime_ms = 0U;

    /* Connect Guardian middleware RX to the interrupt-backed USART2 RX queue. */
    io.read_byte =
        guardian_stm32f401_uart2_read_byte;

    /* Connect Guardian middleware TX to the interrupt-backed USART2 TX queue. */
    io.write =
        guardian_stm32f401_uart2_write;

    /* Connect Guardian middleware uptime to application time. */
    io.uptime_seconds =
        guardian_firmware_uptime_seconds;

    /* No platform context is required by the current singleton USART2 adapter. */
    io.context = NULL;

    /* Publish the hardware-specific Guardian model name. */
    identity.model = guardian_model;

    /* Publish firmware milestone major version. */
    identity.firmware_major = 0U;

    /* Publish firmware milestone minor version. */
    identity.firmware_minor = 8U;

    /* Publish firmware milestone patch version. */
    identity.firmware_patch = 0U;

    /* Derive a non-security display identifier from the STM32 factory UID. */
    identity.device_id =
        guardian_stm32f401_public_device_id();

    /* Initialize parser, device service, telemetry and transport callbacks. */
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

    /* Start without a valid M7 DSP snapshot until the first acquisition block is analyzed. */
    guardian_latest_dsp_valid = 0U;

    /* Load documented M6 reference sampling and calibration defaults. */
    guardian_stm32f401_acquisition_default_config(
        &acquisition_config);

    /* Initialize TIM2-triggered ADC1 scan, DMA2 double buffering and TIM3 RPM capture. */
    if (guardian_stm32f401_acquisition_init(
            &acquisition_config) == 0)
    {
        /* Report deterministic acquisition initialization failure. */
        return 0;
    }

    /* Report successful physical Guardian startup. */
    return 1;
}

/* Execute bounded acquisition, command and telemetry work from the main loop. */
void guardian_firmware_app_poll(void)
{
    /* Store one coherent M6 engineering-unit measurement snapshot. */
    guardian_machine_measurements_t measurements = {0};

    /* Store the exact calibrated M6 vibration block consumed by M7 DSP. */
    guardian_acquisition_signal_block_t signal_block = {0};

    /* Store one newly analyzed M7 feature snapshot. */
    guardian_dsp_features_t dsp_features = {0};

    /* Process at most one completed ADC DMA block per foreground iteration. */
    int acquisition_result =
        guardian_stm32f401_acquisition_poll_ex(
            &measurements,
            &signal_block);

    /* Publish newly processed hardware measurements into M5 telemetry. */
    if (acquisition_result > 0)
    {
        /* Replace the previous telemetry snapshot atomically at the middleware boundary. */
        guardian_embedded_link_update_telemetry(
            &guardian_link,
            &measurements);

        /* Analyze the exact calibrated vibration block outside interrupt context. */
        if (guardian_dsp_analyze(
                &signal_block,
                &dsp_features) == GUARDIAN_DSP_OK)
        {
            /* Preserve the latest successful feature snapshot locally. */
            guardian_latest_dsp_features =
                dsp_features;

            /* Mark the local snapshot valid. */
            guardian_latest_dsp_valid = 1U;

            /* Expose the same immutable snapshot through GET_DSP_FEATURES. */
            guardian_embedded_link_update_dsp(
                &guardian_link,
                &dsp_features);
        }
    }

    /* Process bounded command RX work and emit at most one due telemetry frame. */
    guardian_embedded_link_poll(
        &guardian_link,
        GUARDIAN_EMBEDDED_DEFAULT_RX_BUDGET);
}

/* Advance monotonic firmware time, RPM freshness and telemetry scheduling. */
void guardian_firmware_app_tick_1ms(void)
{
    /* Avoid uptime counter wrap so GET_STATUS remains monotonic. */
    if (guardian_uptime_ms != 0xFFFFFFFFUL)
    {
        /* Advance one application millisecond. */
        guardian_uptime_ms += 1U;
    }

    /* Advance RPM staleness tracking for the M6 acquisition path. */
    guardian_stm32f401_acquisition_tick_1ms();

    /* Advance the independent M5 telemetry timestamp and scheduler. */
    guardian_embedded_link_tick_1ms(
        &guardian_link);
}

/* Change the state exposed by status and telemetry. */
void guardian_firmware_app_set_state(
    guardian_device_state_t state)
{
    /* Forward application state into Guardian middleware. */
    guardian_embedded_link_set_state(
        &guardian_link,
        state);
}

/* Replace the latest telemetry snapshot manually when an application override is required. */
void guardian_firmware_app_update_telemetry(
    const guardian_machine_measurements_t *measurements)
{
    /* Forward the explicit bounded snapshot into Guardian middleware. */
    guardian_embedded_link_update_telemetry(
        &guardian_link,
        measurements);
}

/* Return M6 hardware acquisition diagnostics. */
guardian_stm32f401_acquisition_stats_t guardian_firmware_app_acquisition_stats(void)
{
    /* Return the coherent hardware acquisition diagnostic snapshot by value. */
    return guardian_stm32f401_acquisition_stats();
}


/* Return the latest local M7 DSP feature snapshot. */
int guardian_firmware_app_dsp_features(
    guardian_dsp_features_t *features)
{
    /* Reject a missing caller buffer or a device that has not analyzed one block yet. */
    if ((features == NULL) ||
        (guardian_latest_dsp_valid == 0U))
    {
        /* Report that no valid feature snapshot was returned. */
        return 0;
    }

    /* Copy the complete immutable feature snapshot by value. */
    *features =
        guardian_latest_dsp_features;

    /* Report one valid feature snapshot. */
    return 1;
}


/* Return the current M8 runtime machine-health snapshot. */
guardian_health_status_t guardian_firmware_app_health_status(void)
{
    /* Return the transport-independent model snapshot by value. */
    return guardian_embedded_link_health_status(
        &guardian_link);
}
