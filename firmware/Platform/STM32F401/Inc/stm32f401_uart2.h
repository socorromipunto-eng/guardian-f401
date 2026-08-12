#ifndef STM32F401_UART2_H
#define STM32F401_UART2_H

/* Include size_t for non-blocking byte-queue APIs. */
#include <stddef.h>

/* Include fixed-width integer types for baud, bytes and counters. */
#include <stdint.h>

/* Define the default Guardian physical UART speed. */
#define GUARDIAN_STM32F401_UART2_DEFAULT_BAUD ((uint32_t)115200UL)

/* Define the fixed RX queue capacity used between ISR and foreground code. */
#define GUARDIAN_STM32F401_UART2_RX_CAPACITY ((uint16_t)512U)

/* Define the fixed TX queue capacity used between foreground code and ISR. */
#define GUARDIAN_STM32F401_UART2_TX_CAPACITY ((uint16_t)512U)

/* Store UART hardware and software queue diagnostics. */
typedef struct
{
    /* Count hardware parity-error indications. */
    uint32_t parity_errors;

    /* Count hardware framing-error indications. */
    uint32_t framing_errors;

    /* Count hardware noise-error indications. */
    uint32_t noise_errors;

    /* Count hardware overrun-error indications. */
    uint32_t hardware_overruns;

    /* Count bytes dropped because the RX software queue was full. */
    uint32_t rx_queue_overruns;

    /* Count write calls that could not queue a complete requested block. */
    uint32_t tx_queue_rejections;
} guardian_stm32f401_uart2_stats_t;

/* Configure PA2/PA3 AF7 and USART2 for 8-N-1 interrupt-driven Guardian traffic. */
int guardian_stm32f401_uart2_init(
    uint32_t baud_rate,
    uint32_t irq_priority);

/* Read one queued RX byte without blocking. */
int guardian_stm32f401_uart2_read_byte(
    void *context,
    uint8_t *byte);

/* Atomically queue one complete Guardian frame or reject it. */
size_t guardian_stm32f401_uart2_write(
    void *context,
    const uint8_t *data,
    size_t length);

/* Return the current APB1 peripheral clock derived from CMSIS clock state. */
uint32_t guardian_stm32f401_uart2_apb1_clock_hz(void);

/* Derive a public non-security display identifier from the STM32 unique ID. */
uint32_t guardian_stm32f401_public_device_id(void);

/* Return a copy of UART hardware/queue diagnostics. */
guardian_stm32f401_uart2_stats_t guardian_stm32f401_uart2_stats(void);

/* Service USART2 RX, TX and line-error events from an application IRQ wrapper. */
void guardian_stm32f401_uart2_irq_handler(void);

#endif
