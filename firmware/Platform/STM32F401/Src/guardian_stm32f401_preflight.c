/* Include the public STM32F401 hardware preflight API. */
#include "guardian_stm32f401_preflight.h"

/* Include official ST CMSIS clock, RCC and UID definitions supplied by the device pack. */
#include "stm32f4xx.h"

/* Include memory helpers for deterministic report initialization. */
#include <string.h>

/* Decode one STM32 APB peripheral clock from an RCC PPRE field. */
static uint32_t guardian_stm32f401_decode_pclk(
    uint32_t hclk_hz,
    uint32_t encoded_prescaler)
{
    /* Return HCLK directly when APB is not divided. */
    if (encoded_prescaler < 4U)
    {
        /* Preserve the undivided clock. */
        return hclk_hz;
    }

    /* Decode APB divisor as 2^(PPRE - 3). */
    return hclk_hz >>
        (encoded_prescaler - 3U);
}

/* Return whether one factory half-word is plausibly programmed. */
static int guardian_stm32f401_factory_u16_valid(
    uint16_t value)
{
    /* Reject erased or impossible zero factory values. */
    return
        (value != 0U) &&
        (value != 0xFFFFU);
}

/* Run one non-invasive target/clock/identity/calibration preflight. */
int guardian_stm32f401_preflight_run(
    uint32_t requested_baud,
    guardian_stm32f401_preflight_report_t *report)
{
    /* Reject missing caller report storage. */
    if (report == NULL)
    {
        /* Report invalid caller state. */
        return 0;
    }

    /* Clear every report field before reading hardware state. */
    (void)memset(
        report,
        0,
        sizeof(*report));

    /* Refresh CMSIS SystemCoreClock after board clock initialization. */
    SystemCoreClockUpdate();

    /* Snapshot the current HCLK value. */
    report->hclk_hz =
        SystemCoreClock;

    /* Extract the APB1 prescaler encoding. */
    uint32_t ppre1 =
        (RCC->CFGR &
         RCC_CFGR_PPRE1) >>
        RCC_CFGR_PPRE1_Pos;

    /* Extract the APB2 prescaler encoding. */
    uint32_t ppre2 =
        (RCC->CFGR &
         RCC_CFGR_PPRE2) >>
        RCC_CFGR_PPRE2_Pos;

    /* Decode the active APB1 peripheral clock. */
    report->pclk1_hz =
        guardian_stm32f401_decode_pclk(
            report->hclk_hz,
            ppre1);

    /* Decode the active APB2 peripheral clock. */
    report->pclk2_hz =
        guardian_stm32f401_decode_pclk(
            report->hclk_hz,
            ppre2);

    /* Preserve the requested physical Guardian baud rate. */
    report->requested_baud =
        requested_baud;

    /* Flag an unavailable HCLK immediately. */
    if (report->hclk_hz == 0U)
    {
        /* Record the failed system-clock contract. */
        report->failure_flags |=
            (uint32_t)GUARDIAN_STM32F401_PREFLIGHT_HCLK_ZERO;
    }

    /* Enforce the target maximum HCLK. */
    if (report->hclk_hz >
        GUARDIAN_STM32F401_MAX_HCLK_HZ)
    {
        /* Record target overclocking. */
        report->failure_flags |=
            (uint32_t)GUARDIAN_STM32F401_PREFLIGHT_HCLK_HIGH;
    }

    /* Enforce the target maximum APB1 clock. */
    if (report->pclk1_hz >
        GUARDIAN_STM32F401_MAX_PCLK1_HZ)
    {
        /* Record APB1 overclocking. */
        report->failure_flags |=
            (uint32_t)GUARDIAN_STM32F401_PREFLIGHT_PCLK1_HIGH;
    }

    /* Enforce the target maximum APB2 clock. */
    if (report->pclk2_hz >
        GUARDIAN_STM32F401_MAX_PCLK2_HZ)
    {
        /* Record APB2 overclocking. */
        report->failure_flags |=
            (uint32_t)GUARDIAN_STM32F401_PREFLIGHT_PCLK2_HIGH;
    }

    /* Validate the requested UART baud before calculating a divisor. */
    if ((requested_baud == 0U) ||
        (report->pclk1_hz == 0U))
    {
        /* Record an unrepresentable UART configuration. */
        report->failure_flags |=
            (uint32_t)GUARDIAN_STM32F401_PREFLIGHT_UART_DIVISOR;
    }
    else
    {
        /* Calculate the rounded OVER8=0 USART BRR value. */
        report->uart_brr =
            (
                report->pclk1_hz +
                (requested_baud / 2U)
            ) /
            requested_baud;

        /* Reject zero or wider-than-16-bit divisors. */
        if ((report->uart_brr == 0U) ||
            (report->uart_brr > 0xFFFFU))
        {
            /* Record an unrepresentable UART divisor. */
            report->failure_flags |=
                (uint32_t)GUARDIAN_STM32F401_PREFLIGHT_UART_DIVISOR;
        }
        else
        {
            /* Estimate the resulting integer UART baud rate. */
            report->actual_baud =
                report->pclk1_hz /
                report->uart_brr;

            /* Store absolute baud-rate difference. */
            uint32_t baud_difference =
                (
                    report->actual_baud >
                    requested_baud
                )
                ? (
                    report->actual_baud -
                    requested_baud
                )
                : (
                    requested_baud -
                    report->actual_baud
                );

            /* Convert absolute baud error to parts per million with 64-bit intermediate math. */
            report->uart_error_ppm =
                (uint32_t)(
                    (
                        (uint64_t)baud_difference *
                        1000000ULL
                    ) /
                    (uint64_t)requested_baud
                );

            /* Reject baud error beyond the explicit hardware bring-up tolerance. */
            if (report->uart_error_ppm >
                GUARDIAN_STM32F401_UART_MAX_ERROR_PPM)
            {
                /* Record excessive UART timing error. */
                report->failure_flags |=
                    (uint32_t)GUARDIAN_STM32F401_PREFLIGHT_UART_ERROR;
            }
        }
    }

    /* Point to the official CMSIS 96-bit unique-ID base address. */
    const volatile uint32_t *uid =
        (const volatile uint32_t *)UID_BASE;

    /* Snapshot every factory unique-ID word. */
    report->uid_words[0] =
        uid[0];

    /* Snapshot the second UID word. */
    report->uid_words[1] =
        uid[1];

    /* Snapshot the third UID word. */
    report->uid_words[2] =
        uid[2];

    /* Reject a fully zeroed or fully erased UID. */
    if (
        (
            report->uid_words[0] == 0U &&
            report->uid_words[1] == 0U &&
            report->uid_words[2] == 0U
        ) ||
        (
            report->uid_words[0] == 0xFFFFFFFFUL &&
            report->uid_words[1] == 0xFFFFFFFFUL &&
            report->uid_words[2] == 0xFFFFFFFFUL
        )
    )
    {
        /* Record invalid factory identity data. */
        report->failure_flags |=
            (uint32_t)GUARDIAN_STM32F401_PREFLIGHT_UID_INVALID;
    }

    /* Read the documented factory VREFINT calibration half-word. */
    report->vrefint_cal =
        *(const volatile uint16_t *)
            GUARDIAN_STM32F401_VREFINT_CAL_ADDRESS;

    /* Read the documented factory 30 C temperature calibration half-word. */
    report->temp_cal_30 =
        *(const volatile uint16_t *)
            GUARDIAN_STM32F401_TEMP_CAL_30_ADDRESS;

    /* Read the documented factory 110 C temperature calibration half-word. */
    report->temp_cal_110 =
        *(const volatile uint16_t *)
            GUARDIAN_STM32F401_TEMP_CAL_110_ADDRESS;

    /* Validate VREFINT factory calibration presence. */
    if (guardian_stm32f401_factory_u16_valid(
            report->vrefint_cal) == 0)
    {
        /* Record invalid VREFINT calibration. */
        report->failure_flags |=
            (uint32_t)GUARDIAN_STM32F401_PREFLIGHT_VREF_INVALID;
    }

    /* Validate both temperature calibration points and require distinct factory codes. */
    if (
        guardian_stm32f401_factory_u16_valid(
            report->temp_cal_30) == 0 ||
        guardian_stm32f401_factory_u16_valid(
            report->temp_cal_110) == 0 ||
        report->temp_cal_110 ==
            report->temp_cal_30
    )
    {
        /* Record invalid temperature calibration metadata without assuming sensor slope direction. */
        report->failure_flags |=
            (uint32_t)GUARDIAN_STM32F401_PREFLIGHT_TEMP_CAL_INVALID;
    }

    /* Report success only when no target qualification failure bit was recorded. */
    return
        (report->failure_flags == 0U)
        ? 1
        : 0;
}
