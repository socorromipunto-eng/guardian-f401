/* Include the public STM32F401 USART2 adapter declarations. */
#include "stm32f401_uart2.h"

/* Include the shared STM32F401CDU6 compile-time target contract. */
#include "guardian_stm32f401_target.h"

/* Include official ST CMSIS peripheral definitions from the Keil/ST device pack. */
#include "stm32f4xx.h"

/* Define one two-bit GPIO field mask. */
#define GUARDIAN_GPIO_TWO_BIT_MASK ((uint32_t)0x3U)

/* Define the alternate-function GPIO mode encoding. */
#define GUARDIAN_GPIO_MODE_AF ((uint32_t)0x2U)

/* Define one four-bit alternate-function field mask. */
#define GUARDIAN_GPIO_AF_MASK ((uint32_t)0xFU)

/* Allocate the bounded interrupt-to-foreground RX queue. */
static volatile uint8_t guardian_rx_queue[GUARDIAN_STM32F401_UART2_RX_CAPACITY];

/* Store the next RX queue position written by the interrupt handler. */
static volatile uint16_t guardian_rx_head = 0U;

/* Store the next RX queue position read by foreground middleware. */
static volatile uint16_t guardian_rx_tail = 0U;

/* Allocate the bounded foreground-to-interrupt TX queue. */
static volatile uint8_t guardian_tx_queue[GUARDIAN_STM32F401_UART2_TX_CAPACITY];

/* Store the next TX queue position written by foreground middleware. */
static volatile uint16_t guardian_tx_head = 0U;

/* Store the next TX queue position transmitted by the interrupt handler. */
static volatile uint16_t guardian_tx_tail = 0U;

/* Store cumulative hardware and queue diagnostics. */
static volatile guardian_stm32f401_uart2_stats_t guardian_uart_stats = {0};

/* Saturating-increment one volatile unsigned 32-bit diagnostic counter. */
static void guardian_uart_increment_u32(volatile uint32_t *value)
{
    /* Ignore a missing diagnostic pointer defensively. */
    if (value == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Avoid counter wrap because diagnostics should remain monotonic. */
    if (*value != 0xFFFFFFFFUL)
    {
        /* Increment only while representable. */
        *value += 1U;
    }
}

/* Advance one RX queue index with deterministic wrap. */
static uint16_t guardian_rx_next(uint16_t index)
{
    /* Advance to the next queue element. */
    index = (uint16_t)(index + 1U);

    /* Wrap explicitly at the fixed queue capacity. */
    if (index >= GUARDIAN_STM32F401_UART2_RX_CAPACITY)
    {
        /* Return to the first queue element. */
        index = 0U;
    }

    /* Return the bounded next index. */
    return index;
}

/* Advance one TX queue index with deterministic wrap. */
static uint16_t guardian_tx_next(uint16_t index)
{
    /* Advance to the next queue element. */
    index = (uint16_t)(index + 1U);

    /* Wrap explicitly at the fixed queue capacity. */
    if (index >= GUARDIAN_STM32F401_UART2_TX_CAPACITY)
    {
        /* Return to the first queue element. */
        index = 0U;
    }

    /* Return the bounded next index. */
    return index;
}

/* Return the current APB1 peripheral clock from SystemCoreClock and RCC PPRE1. */
uint32_t guardian_stm32f401_uart2_apb1_clock_hz(void)
{
    /* Extract the three-bit APB1 prescaler field. */
    uint32_t ppre1 =
        (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;

    /* Return HCLK directly when APB1 is not divided. */
    if (ppre1 < 4U)
    {
        /* CMSIS SystemCoreClock represents HCLK on STM32F4. */
        return SystemCoreClock;
    }

    /* Decode APB divisor as 2^(PPRE1 - 3). */
    return SystemCoreClock >> (ppre1 - 3U);
}

/* Configure GPIOA pins 2/3 and USART2 using direct CMSIS register access. */
int guardian_stm32f401_uart2_init(
    uint32_t baud_rate,
    uint32_t irq_priority)
{
    /* Store the active APB1 clock. */
    uint32_t pclk1_hz = 0U;

    /* Store the rounded oversampling-by-16 BRR value. */
    uint32_t brr = 0U;

    /* Reject a zero baud rate. */
    if (baud_rate == 0U)
    {
        /* Report invalid configuration. */
        return 0;
    }

    /* Refresh CMSIS SystemCoreClock after application clock setup. */
    SystemCoreClockUpdate();

    /* Derive the active APB1 peripheral clock. */
    pclk1_hz = guardian_stm32f401_uart2_apb1_clock_hz();

    /* Reject an unavailable peripheral clock. */
    if (pclk1_hz == 0U)
    {
        /* Report invalid clock configuration. */
        return 0;
    }

    /* Round pclk/baud for OVER8 = 0. */
    brr = (pclk1_hz + (baud_rate / 2U)) / baud_rate;

    /* Reject unrepresentable baud divisors. */
    if ((brr == 0U) || (brr > 0xFFFFU))
    {
        /* Report unsupported clock/baud combination. */
        return 0;
    }

    /* Enable GPIOA clock. */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* Enable USART2 APB1 clock. */
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* Read back the enabled clock register before peripheral access. */
    (void)RCC->APB1ENR;

    /* Clear PA2 mode. */
    GPIOA->MODER &=
        ~(GUARDIAN_GPIO_TWO_BIT_MASK << (GUARDIAN_STM32F401_USART2_TX_PIN * 2U));

    /* Select alternate-function mode for PA2. */
    GPIOA->MODER |=
        (GUARDIAN_GPIO_MODE_AF << (GUARDIAN_STM32F401_USART2_TX_PIN * 2U));

    /* Clear PA3 mode. */
    GPIOA->MODER &=
        ~(GUARDIAN_GPIO_TWO_BIT_MASK << (GUARDIAN_STM32F401_USART2_RX_PIN * 2U));

    /* Select alternate-function mode for PA3. */
    GPIOA->MODER |=
        (GUARDIAN_GPIO_MODE_AF << (GUARDIAN_STM32F401_USART2_RX_PIN * 2U));

    /* Configure PA2 push-pull. */
    GPIOA->OTYPER &= ~(1UL << GUARDIAN_STM32F401_USART2_TX_PIN);

    /* Configure PA3 push-pull field consistently. */
    GPIOA->OTYPER &= ~(1UL << GUARDIAN_STM32F401_USART2_RX_PIN);

    /* Clear PA2 speed. */
    GPIOA->OSPEEDR &=
        ~(GUARDIAN_GPIO_TWO_BIT_MASK << (GUARDIAN_STM32F401_USART2_TX_PIN * 2U));

    /* Select medium speed for PA2. */
    GPIOA->OSPEEDR |=
        (1UL << (GUARDIAN_STM32F401_USART2_TX_PIN * 2U));

    /* Clear PA3 speed. */
    GPIOA->OSPEEDR &=
        ~(GUARDIAN_GPIO_TWO_BIT_MASK << (GUARDIAN_STM32F401_USART2_RX_PIN * 2U));

    /* Select medium speed for PA3. */
    GPIOA->OSPEEDR |=
        (1UL << (GUARDIAN_STM32F401_USART2_RX_PIN * 2U));

    /* Disable internal pull on PA2. */
    GPIOA->PUPDR &=
        ~(GUARDIAN_GPIO_TWO_BIT_MASK << (GUARDIAN_STM32F401_USART2_TX_PIN * 2U));

    /* Clear PA3 pull configuration. */
    GPIOA->PUPDR &=
        ~(GUARDIAN_GPIO_TWO_BIT_MASK << (GUARDIAN_STM32F401_USART2_RX_PIN * 2U));

    /* Enable a weak pull-up on PA3 to hold UART idle when disconnected. */
    GPIOA->PUPDR |=
        (1UL << (GUARDIAN_STM32F401_USART2_RX_PIN * 2U));

    /* Clear PA2 alternate-function field. */
    GPIOA->AFR[0] &=
        ~(GUARDIAN_GPIO_AF_MASK << (GUARDIAN_STM32F401_USART2_TX_PIN * 4U));

    /* Select AF7 USART2 for PA2. */
    GPIOA->AFR[0] |=
        (GUARDIAN_STM32F401_USART2_AF << (GUARDIAN_STM32F401_USART2_TX_PIN * 4U));

    /* Clear PA3 alternate-function field. */
    GPIOA->AFR[0] &=
        ~(GUARDIAN_GPIO_AF_MASK << (GUARDIAN_STM32F401_USART2_RX_PIN * 4U));

    /* Select AF7 USART2 for PA3. */
    GPIOA->AFR[0] |=
        (GUARDIAN_STM32F401_USART2_AF << (GUARDIAN_STM32F401_USART2_RX_PIN * 4U));

    /* Disable USART2 before configuration. */
    USART2->CR1 = 0U;

    /* Select one stop bit and asynchronous mode. */
    USART2->CR2 = 0U;

    /* Disable flow control and DMA for the M4 command channel. */
    USART2->CR3 = 0U;

    /* Configure the baud divisor. */
    USART2->BRR = brr;

    /* Read SR before DR to clear stale receive/error state. */
    (void)USART2->SR;

    /* Complete the SR/DR clear sequence. */
    (void)USART2->DR;

    /* Reset RX producer index. */
    guardian_rx_head = 0U;

    /* Reset RX consumer index. */
    guardian_rx_tail = 0U;

    /* Reset TX producer index. */
    guardian_tx_head = 0U;

    /* Reset TX consumer index. */
    guardian_tx_tail = 0U;

    /* Enable receiver, transmitter and RX interrupt. */
    USART2->CR1 =
        USART_CR1_RE |
        USART_CR1_TE |
        USART_CR1_RXNEIE;

    /* Configure USART2 IRQ priority. */
    NVIC_SetPriority(
        USART2_IRQn,
        irq_priority);

    /* Enable USART2 IRQ delivery. */
    NVIC_EnableIRQ(USART2_IRQn);

    /* Enable USART2 after configuration is complete. */
    USART2->CR1 |= USART_CR1_UE;

    /* Report successful hardware initialization. */
    return 1;
}

/* Read one interrupt-produced RX byte without blocking. */
int guardian_stm32f401_uart2_read_byte(
    void *context,
    uint8_t *byte)
{
    /* Store the foreground consumer index. */
    uint16_t tail = 0U;

    /* Mark the generic callback context as unused. */
    (void)context;

    /* Reject missing output storage. */
    if (byte == NULL)
    {
        /* Report no byte. */
        return 0;
    }

    /* Snapshot the consumer index. */
    tail = guardian_rx_tail;

    /* Report an empty queue. */
    if (tail == guardian_rx_head)
    {
        /* Return without blocking. */
        return 0;
    }

    /* Copy the next RX byte. */
    *byte = guardian_rx_queue[tail];

    /* Advance the consumer index. */
    guardian_rx_tail = guardian_rx_next(tail);

    /* Report one byte. */
    return 1;
}

/* Queue one complete Guardian frame for interrupt-driven transmission. */
size_t guardian_stm32f401_uart2_write(
    void *context,
    const uint8_t *data,
    size_t length)
{
    /* Store free queue slots. */
    size_t free_slots = 0U;

    /* Store the private producer index. */
    uint16_t head = 0U;

    /* Store the interrupt consumer index. */
    uint16_t tail = 0U;

    /* Track copied bytes. */
    size_t index = 0U;

    /* Mark generic context as unused. */
    (void)context;

    /* Reject missing data for a non-empty write. */
    if ((data == NULL) && (length != 0U))
    {
        /* Report rejection. */
        return 0U;
    }

    /* Accept an empty no-op. */
    if (length == 0U)
    {
        /* Report zero accepted bytes. */
        return 0U;
    }

    /* Reject a frame that can never fit. */
    if (length >= (size_t)GUARDIAN_STM32F401_UART2_TX_CAPACITY)
    {
        /* Count the rejection. */
        guardian_uart_increment_u32(
            &guardian_uart_stats.tx_queue_rejections);

        /* Reject atomically. */
        return 0U;
    }

    /* Snapshot producer index. */
    head = guardian_tx_head;

    /* Snapshot consumer index. */
    tail = guardian_tx_tail;

    /* Calculate free slots while preserving one sentinel slot. */
    if (head >= tail)
    {
        /* Calculate wrapped free space. */
        free_slots =
            (size_t)GUARDIAN_STM32F401_UART2_TX_CAPACITY -
            (size_t)(head - tail) -
            1U;
    }
    else
    {
        /* Calculate logical free space. */
        free_slots =
            (size_t)(tail - head) -
            1U;
    }

    /* Reject the complete frame when insufficient space exists. */
    if (free_slots < length)
    {
        /* Count the rejection. */
        guardian_uart_increment_u32(
            &guardian_uart_stats.tx_queue_rejections);

        /* Reject atomically. */
        return 0U;
    }

    /* Copy all bytes before publishing the producer index. */
    for (index = 0U; index < length; ++index)
    {
        /* Store one encoded byte. */
        guardian_tx_queue[head] = data[index];

        /* Advance private producer index. */
        head = guardian_tx_next(head);
    }

    /* Publish all copied bytes atomically. */
    guardian_tx_head = head;

    /* Enable TX-empty interrupts to drain the queue. */
    USART2->CR1 |= USART_CR1_TXEIE;

    /* Report complete acceptance. */
    return length;
}

/* Derive a public display identifier from the 96-bit factory unique ID. */
uint32_t guardian_stm32f401_public_device_id(void)
{
    /* Point to the official CMSIS unique-ID base address. */
    const volatile uint32_t *uid =
        (const volatile uint32_t *)UID_BASE;

    /* XOR all three words into a compact non-security display identifier. */
    return uid[0] ^ uid[1] ^ uid[2];
}

/* Return a copy of volatile UART diagnostics. */
guardian_stm32f401_uart2_stats_t guardian_stm32f401_uart2_stats(void)
{
    /* Create local diagnostic storage. */
    guardian_stm32f401_uart2_stats_t stats = {0};

    /* Copy parity errors. */
    stats.parity_errors = guardian_uart_stats.parity_errors;

    /* Copy framing errors. */
    stats.framing_errors = guardian_uart_stats.framing_errors;

    /* Copy noise errors. */
    stats.noise_errors = guardian_uart_stats.noise_errors;

    /* Copy hardware overruns. */
    stats.hardware_overruns = guardian_uart_stats.hardware_overruns;

    /* Copy software RX overruns. */
    stats.rx_queue_overruns = guardian_uart_stats.rx_queue_overruns;

    /* Copy TX rejections. */
    stats.tx_queue_rejections = guardian_uart_stats.tx_queue_rejections;

    /* Return the snapshot. */
    return stats;
}

/* Service USART2 line errors, RX bytes and TX queue progress. */
void guardian_stm32f401_uart2_irq_handler(void)
{
    /* Snapshot status before DR reads clear receive/error flags. */
    uint32_t status = USART2->SR;

    /* Store the data-register value when receive/error service requires a read. */
    uint8_t received = 0U;

    /* Store the next RX producer index when one valid byte is available. */
    uint16_t next = 0U;

    /* Handle receive or line-error conditions. */
    if ((status &
         (USART_SR_RXNE |
          USART_SR_PE |
          USART_SR_FE |
          USART_SR_NE |
          USART_SR_ORE)) != 0U)
    {
        /* Read DR exactly once. */
        received = (uint8_t)(USART2->DR & 0xFFU);

        /* Count parity errors. */
        if ((status & USART_SR_PE) != 0U)
        {
            /* Increment parity diagnostics. */
            guardian_uart_increment_u32(
                &guardian_uart_stats.parity_errors);
        }

        /* Count framing errors. */
        if ((status & USART_SR_FE) != 0U)
        {
            /* Increment framing diagnostics. */
            guardian_uart_increment_u32(
                &guardian_uart_stats.framing_errors);
        }

        /* Count noise errors. */
        if ((status & USART_SR_NE) != 0U)
        {
            /* Increment noise diagnostics. */
            guardian_uart_increment_u32(
                &guardian_uart_stats.noise_errors);
        }

        /* Count hardware overruns. */
        if ((status & USART_SR_ORE) != 0U)
        {
            /* Increment overrun diagnostics. */
            guardian_uart_increment_u32(
                &guardian_uart_stats.hardware_overruns);
        }

        /* Queue valid RX data only when no line-error bits accompany it. */
        if (((status & USART_SR_RXNE) != 0U) &&
            ((status &
              (USART_SR_PE |
               USART_SR_FE |
               USART_SR_NE |
               USART_SR_ORE)) == 0U))
        {
            /* Calculate next producer index. */
            next =
                guardian_rx_next(guardian_rx_head);

            /* Check for one free queue slot. */
            if (next != guardian_rx_tail)
            {
                /* Store the received byte. */
                guardian_rx_queue[guardian_rx_head] = received;

                /* Publish the producer index. */
                guardian_rx_head = next;
            }
            else
            {
                /* Count software RX queue overflow. */
                guardian_uart_increment_u32(
                    &guardian_uart_stats.rx_queue_overruns);
            }
        }
    }

    /* Service TX-empty only when its interrupt is enabled. */
    if (((USART2->CR1 & USART_CR1_TXEIE) != 0U) &&
        ((USART2->SR & USART_SR_TXE) != 0U))
    {
        /* Check whether TX data remains. */
        if (guardian_tx_tail != guardian_tx_head)
        {
            /* Write the next queued byte. */
            USART2->DR = guardian_tx_queue[guardian_tx_tail];

            /* Advance TX consumer index. */
            guardian_tx_tail =
                guardian_tx_next(guardian_tx_tail);
        }
        else
        {
            /* Disable TX-empty interrupts when the queue is drained. */
            USART2->CR1 &= ~USART_CR1_TXEIE;
        }
    }
}
