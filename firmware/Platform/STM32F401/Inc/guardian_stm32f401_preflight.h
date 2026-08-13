#ifndef GUARDIAN_STM32F401_PREFLIGHT_H
#define GUARDIAN_STM32F401_PREFLIGHT_H

/* Include the compile-time STM32F401CDU6 target contract. */
#include "guardian_stm32f401_target.h"

/* Include fixed-width integer types for diagnostics. */
#include <stdint.h>

/* Define hardware preflight failure bits. */
typedef enum
{
    /* Indicate no detected preflight failure. */
    GUARDIAN_STM32F401_PREFLIGHT_OK = 0x00000000UL,

    /* Indicate an unavailable or zero AHB/SystemCoreClock value. */
    GUARDIAN_STM32F401_PREFLIGHT_HCLK_ZERO = 0x00000001UL,

    /* Indicate HCLK exceeds the STM32F401 target limit. */
    GUARDIAN_STM32F401_PREFLIGHT_HCLK_HIGH = 0x00000002UL,

    /* Indicate APB1 exceeds the STM32F401 target limit. */
    GUARDIAN_STM32F401_PREFLIGHT_PCLK1_HIGH = 0x00000004UL,

    /* Indicate APB2 exceeds the STM32F401 target limit. */
    GUARDIAN_STM32F401_PREFLIGHT_PCLK2_HIGH = 0x00000008UL,

    /* Indicate the requested UART baud cannot be represented safely. */
    GUARDIAN_STM32F401_PREFLIGHT_UART_DIVISOR = 0x00000010UL,

    /* Indicate the calculated USART2 baud error exceeds the reference tolerance. */
    GUARDIAN_STM32F401_PREFLIGHT_UART_ERROR = 0x00000020UL,

    /* Indicate the 96-bit factory unique ID appears erased or invalid. */
    GUARDIAN_STM32F401_PREFLIGHT_UID_INVALID = 0x00000040UL,

    /* Indicate VREFINT factory calibration appears erased or invalid. */
    GUARDIAN_STM32F401_PREFLIGHT_VREF_INVALID = 0x00000080UL,

    /* Indicate temperature factory calibration appears erased, invalid or identical. */
    GUARDIAN_STM32F401_PREFLIGHT_TEMP_CAL_INVALID = 0x00000100UL
} guardian_stm32f401_preflight_flag_t;

/* Store one non-invasive hardware startup qualification snapshot. */
typedef struct
{
    /* Store all detected preflight failure bits. */
    uint32_t failure_flags;

    /* Store the CMSIS HCLK/SystemCoreClock value. */
    uint32_t hclk_hz;

    /* Store the decoded APB1 peripheral clock. */
    uint32_t pclk1_hz;

    /* Store the decoded APB2 peripheral clock. */
    uint32_t pclk2_hz;

    /* Store the requested USART2 baud rate. */
    uint32_t requested_baud;

    /* Store the rounded USART2 BRR divisor. */
    uint32_t uart_brr;

    /* Store the resulting integer USART2 baud estimate. */
    uint32_t actual_baud;

    /* Store absolute USART2 baud error in parts per million. */
    uint32_t uart_error_ppm;

    /* Store all three STM32 factory UID words. */
    uint32_t uid_words[3];

    /* Store the factory VREFINT calibration half-word. */
    uint16_t vrefint_cal;

    /* Store the factory 30 C temperature calibration half-word. */
    uint16_t temp_cal_30;

    /* Store the factory 110 C temperature calibration half-word. */
    uint16_t temp_cal_110;
} guardian_stm32f401_preflight_report_t;

/* Run one non-invasive target/clock/identity/calibration preflight. */
int guardian_stm32f401_preflight_run(
    uint32_t requested_baud,
    guardian_stm32f401_preflight_report_t *report);

#endif
