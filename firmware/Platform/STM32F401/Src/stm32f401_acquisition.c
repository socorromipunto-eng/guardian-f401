/* Include the public STM32F401 acquisition adapter declarations. */
#include "stm32f401_acquisition.h"

/* Include official ST CMSIS peripheral declarations supplied by the Keil device pack. */
#include "stm32f4xx.h"

/* Include uintptr_t for portable pointer-to-register address conversion. */
#include <stdint.h>

/* Define the factory VREFINT calibration half-word address from the STM32F401xD/xE datasheet. */
#define GUARDIAN_VREFINT_CAL_ADDRESS ((uintptr_t)0x1FFF7A2AUL)

/* Define the factory 30 C temperature calibration half-word address. */
#define GUARDIAN_TEMP_CAL_30_ADDRESS ((uintptr_t)0x1FFF7A2CUL)

/* Define the factory 110 C temperature calibration half-word address. */
#define GUARDIAN_TEMP_CAL_110_ADDRESS ((uintptr_t)0x1FFF7A2EUL)

/* Define ADC input pin numbers used by the M6 reference design. */
#define GUARDIAN_ADC_VIBRATION_PIN ((uint32_t)0U)
#define GUARDIAN_ADC_CURRENT_PIN ((uint32_t)1U)
#define GUARDIAN_ADC_SUPPLY_PIN ((uint32_t)4U)

/* Define the RPM input pin and alternate-function selection. */
#define GUARDIAN_RPM_PIN ((uint32_t)6U)
#define GUARDIAN_RPM_AF ((uint32_t)2U)

/* Define GPIO register field helpers. */
#define GUARDIAN_GPIO_MODE_MASK ((uint32_t)0x3U)
#define GUARDIAN_GPIO_MODE_ANALOG ((uint32_t)0x3U)
#define GUARDIAN_GPIO_MODE_AF ((uint32_t)0x2U)
#define GUARDIAN_GPIO_AF_MASK ((uint32_t)0xFU)

/* Define RCC enable bits consumed by the acquisition path. */
#define GUARDIAN_RCC_AHB1_GPIOAEN ((uint32_t)1UL << 0U)
#define GUARDIAN_RCC_AHB1_DMA2EN ((uint32_t)1UL << 22U)
#define GUARDIAN_RCC_APB1_TIM2EN ((uint32_t)1UL << 0U)
#define GUARDIAN_RCC_APB1_TIM3EN ((uint32_t)1UL << 1U)
#define GUARDIAN_RCC_APB2_ADC1EN ((uint32_t)1UL << 8U)

/* Define RCC APB1 prescaler field geometry. */
#define GUARDIAN_RCC_PPRE1_SHIFT ((uint32_t)10U)
#define GUARDIAN_RCC_PPRE1_MASK ((uint32_t)0x7U << GUARDIAN_RCC_PPRE1_SHIFT)

/* Define RCC APB2 prescaler field geometry. */
#define GUARDIAN_RCC_PPRE2_SHIFT ((uint32_t)13U)
#define GUARDIAN_RCC_PPRE2_MASK ((uint32_t)0x7U << GUARDIAN_RCC_PPRE2_SHIFT)

/* Define TIM control bits used by the deterministic trigger and RPM capture path. */
#define GUARDIAN_TIM_CR1_CEN ((uint32_t)1UL << 0U)
#define GUARDIAN_TIM_CR1_ARPE ((uint32_t)1UL << 7U)
#define GUARDIAN_TIM_CR2_MMS_UPDATE ((uint32_t)0x2U << 4U)
#define GUARDIAN_TIM_DIER_UIE ((uint32_t)1UL << 0U)
#define GUARDIAN_TIM_DIER_CC1IE ((uint32_t)1UL << 1U)
#define GUARDIAN_TIM_SR_UIF ((uint32_t)1UL << 0U)
#define GUARDIAN_TIM_SR_CC1IF ((uint32_t)1UL << 1U)
#define GUARDIAN_TIM_SR_CC1OF ((uint32_t)1UL << 9U)
#define GUARDIAN_TIM_EGR_UG ((uint32_t)1UL << 0U)
#define GUARDIAN_TIM_CCMR1_CC1S_TI1 ((uint32_t)0x1U << 0U)
#define GUARDIAN_TIM_CCMR1_IC1F_8_SAMPLES ((uint32_t)0x3U << 4U)
#define GUARDIAN_TIM_CCER_CC1E ((uint32_t)1UL << 0U)

/* Define ADC control bits and external-trigger selection. */
#define GUARDIAN_ADC_CR1_SCAN ((uint32_t)1UL << 8U)
#define GUARDIAN_ADC_CR1_OVRIE ((uint32_t)1UL << 26U)
#define GUARDIAN_ADC_CR2_ADON ((uint32_t)1UL << 0U)
#define GUARDIAN_ADC_CR2_DMA ((uint32_t)1UL << 8U)
#define GUARDIAN_ADC_CR2_DDS ((uint32_t)1UL << 9U)
#define GUARDIAN_ADC_CR2_EXTSEL_TIM2_TRGO ((uint32_t)0x6U << 24U)
#define GUARDIAN_ADC_CR2_EXTEN_RISING ((uint32_t)0x1U << 28U)
#define GUARDIAN_ADC_SR_OVR ((uint32_t)1UL << 5U)

/* Define ADC common-control fields used by clocking, VREFINT and temperature. */
#define GUARDIAN_ADC_CCR_ADCPRE_MASK ((uint32_t)0x3U << 16U)
#define GUARDIAN_ADC_CCR_ADCPRE_DIV2 ((uint32_t)0x0U << 16U)
#define GUARDIAN_ADC_CCR_ADCPRE_DIV4 ((uint32_t)0x1U << 16U)
#define GUARDIAN_ADC_CCR_ADCPRE_DIV6 ((uint32_t)0x2U << 16U)
#define GUARDIAN_ADC_CCR_ADCPRE_DIV8 ((uint32_t)0x3U << 16U)
#define GUARDIAN_ADC_CCR_VBATE ((uint32_t)1UL << 22U)
#define GUARDIAN_ADC_CCR_TSVREFE ((uint32_t)1UL << 23U)

/* Keep ADC clock at or below 18 MHz so the reference remains conservative across VDDA range. */
#define GUARDIAN_ADC_CONSERVATIVE_MAX_CLOCK_HZ ((uint32_t)18000000UL)

/* Count complete 12-bit ADC cycles in one five-channel M6 regular scan sequence. */
#define GUARDIAN_ADC_SEQUENCE_CYCLES ((uint32_t)1188UL)

/* Define ADC sample-time encodings from RM0368. */
#define GUARDIAN_ADC_SAMPLE_56_CYCLES ((uint32_t)0x3U)
#define GUARDIAN_ADC_SAMPLE_480_CYCLES ((uint32_t)0x7U)

/* Define the regular scan sequence length field for five conversions. */
#define GUARDIAN_ADC_SQR1_LENGTH_5 ((uint32_t)4U << 20U)

/* Define the fixed ADC1 channel numbers used by the reference scan. */
#define GUARDIAN_ADC_CHANNEL_VIBRATION ((uint32_t)0U)
#define GUARDIAN_ADC_CHANNEL_CURRENT ((uint32_t)1U)
#define GUARDIAN_ADC_CHANNEL_SUPPLY ((uint32_t)4U)
#define GUARDIAN_ADC_CHANNEL_TEMPERATURE ((uint32_t)16U)
#define GUARDIAN_ADC_CHANNEL_VREFINT ((uint32_t)17U)

/* Define DMA2 Stream0 control bits for ADC1 Channel 0 double-buffer acquisition. */
#define GUARDIAN_DMA_CR_EN ((uint32_t)1UL << 0U)
#define GUARDIAN_DMA_CR_DMEIE ((uint32_t)1UL << 1U)
#define GUARDIAN_DMA_CR_TEIE ((uint32_t)1UL << 2U)
#define GUARDIAN_DMA_CR_TCIE ((uint32_t)1UL << 4U)
#define GUARDIAN_DMA_CR_CIRC ((uint32_t)1UL << 8U)
#define GUARDIAN_DMA_CR_MINC ((uint32_t)1UL << 10U)
#define GUARDIAN_DMA_CR_PSIZE_16 ((uint32_t)0x1U << 11U)
#define GUARDIAN_DMA_CR_MSIZE_16 ((uint32_t)0x1U << 13U)
#define GUARDIAN_DMA_CR_PRIORITY_HIGH ((uint32_t)0x2U << 16U)
#define GUARDIAN_DMA_CR_DBM ((uint32_t)1UL << 18U)
#define GUARDIAN_DMA_CR_CT ((uint32_t)1UL << 19U)

/* Define DMA2 Stream0 low-status and flag-clear bits. */
#define GUARDIAN_DMA_FEIF0 ((uint32_t)1UL << 0U)
#define GUARDIAN_DMA_DMEIF0 ((uint32_t)1UL << 2U)
#define GUARDIAN_DMA_TEIF0 ((uint32_t)1UL << 3U)
#define GUARDIAN_DMA_HTIF0 ((uint32_t)1UL << 4U)
#define GUARDIAN_DMA_TCIF0 ((uint32_t)1UL << 5U)
#define GUARDIAN_DMA_STREAM0_ALL_FLAGS \
    (GUARDIAN_DMA_FEIF0 | \
     GUARDIAN_DMA_DMEIF0 | \
     GUARDIAN_DMA_TEIF0 | \
     GUARDIAN_DMA_HTIF0 | \
     GUARDIAN_DMA_TCIF0)

/* Define ready-mask bits for the two DMA target buffers. */
#define GUARDIAN_DMA_BUFFER_0_READY ((uint8_t)0x01U)
#define GUARDIAN_DMA_BUFFER_1_READY ((uint8_t)0x02U)

/* Allocate the first DMA target buffer for 64 complete five-channel scan frames. */
static uint16_t guardian_dma_buffer_0[GUARDIAN_ACQUISITION_SAMPLES_PER_BLOCK];

/* Allocate the second DMA target buffer for hardware double-buffer mode. */
static uint16_t guardian_dma_buffer_1[GUARDIAN_ACQUISITION_SAMPLES_PER_BLOCK];

/* Allocate one foreground snapshot buffer so DMA never mutates data during conversion math. */
static uint16_t guardian_process_buffer[GUARDIAN_ACQUISITION_SAMPLES_PER_BLOCK];

/* Store the portable raw-to-engineering-unit processor. */
static guardian_acquisition_t guardian_acquisition;

/* Store the active explicit hardware configuration. */
static guardian_stm32f401_acquisition_config_t guardian_config;

/* Store the active TIM3 counter frequency after prescaler rounding. */
static uint32_t guardian_rpm_timer_hz = 0U;

/* Store the runtime-selected ADC common prescaler register encoding. */
static uint32_t guardian_adc_prescaler_bits =
    GUARDIAN_ADC_CCR_ADCPRE_DIV2;

/* Store the resulting ADC kernel clock in hertz. */
static uint32_t guardian_adc_clock_hz = 0U;

/* Store the most recent software-extended TIM3 capture timestamp. */
static volatile uint32_t guardian_last_rpm_timestamp = 0U;

/* Store the software extension count for TIM3 16-bit counter overflows. */
static volatile uint32_t guardian_rpm_overflow_count = 0U;

/* Store whether one previous RPM edge exists for period calculation. */
static volatile uint8_t guardian_has_rpm_capture = 0U;

/* Store the most recently calculated shaft speed. */
static volatile uint16_t guardian_rpm = 0U;

/* Store elapsed milliseconds since the most recent valid RPM period. */
static volatile uint32_t guardian_rpm_age_ms = UINT32_MAX;

/* Store whether the most recent RPM value is fresh. */
static volatile uint8_t guardian_rpm_valid = 0U;

/* Store completed DMA target buffers awaiting foreground processing. */
static volatile uint8_t guardian_ready_mask = 0U;

/* Store quality flags that must accompany the next successfully processed block. */
static volatile uint16_t guardian_pending_quality_flags = 0U;

/* Store whether ADC/DMA state must be rebuilt in foreground code. */
static volatile uint8_t guardian_recovery_required = 0U;

/* Store whether hardware acquisition initialization completed successfully. */
static volatile uint8_t guardian_initialized = 0U;

/* Store cumulative hardware diagnostics. */
static volatile guardian_stm32f401_acquisition_stats_t guardian_hw_stats = {0};

/* Saturating-increment one volatile unsigned 32-bit diagnostic counter. */
static void guardian_stm32f401_increment_u32(
    volatile uint32_t *value)
{
    /* Ignore a missing diagnostic pointer defensively. */
    if (value == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Avoid counter wrap because diagnostics should remain monotonic. */
    if (*value != UINT32_MAX)
    {
        /* Increment only while representable. */
        *value += 1U;
    }
}

/* Read one factory calibration half-word from the documented system-memory address. */
static uint16_t guardian_stm32f401_factory_u16(
    uintptr_t address)
{
    /* Convert the fixed system-memory address into a read-only volatile half-word pointer. */
    const volatile uint16_t *value =
        (const volatile uint16_t *)address;

    /* Return the factory-programmed calibration value. */
    return *value;
}

/* Return the current APB1 peripheral clock derived from SystemCoreClock and RCC PPRE1. */
static uint32_t guardian_stm32f401_pclk1_hz(void)
{
    /* Extract the three-bit APB1 prescaler field. */
    uint32_t ppre1 =
        (RCC->CFGR & GUARDIAN_RCC_PPRE1_MASK) >>
        GUARDIAN_RCC_PPRE1_SHIFT;

    /* Return HCLK directly when APB1 is not divided. */
    if (ppre1 < 4U)
    {
        /* CMSIS SystemCoreClock represents HCLK on STM32F4. */
        return SystemCoreClock;
    }

    /* Decode the APB divisor as 2^(PPRE1 - 3). */
    return SystemCoreClock >> (ppre1 - 3U);
}

/* Return the current APB2 peripheral clock derived from SystemCoreClock and RCC PPRE2. */
static uint32_t guardian_stm32f401_pclk2_hz(void)
{
    /* Extract the three-bit APB2 prescaler field. */
    uint32_t ppre2 =
        (RCC->CFGR & GUARDIAN_RCC_PPRE2_MASK) >>
        GUARDIAN_RCC_PPRE2_SHIFT;

    /* Return HCLK directly when APB2 is not divided. */
    if (ppre2 < 4U)
    {
        /* CMSIS SystemCoreClock represents HCLK on STM32F4. */
        return SystemCoreClock;
    }

    /* Decode the APB divisor as 2^(PPRE2 - 3). */
    return SystemCoreClock >> (ppre2 - 3U);
}

/* Select the fastest ADC prescaler that never exceeds the conservative 18 MHz ceiling. */
static int guardian_stm32f401_select_adc_clock(void)
{
    /* Read the active APB2 peripheral clock that feeds the ADC common prescaler. */
    uint32_t pclk2_hz =
        guardian_stm32f401_pclk2_hz();

    /* Define the four hardware ADC prescaler divisors in fastest-first order. */
    static const uint8_t divisors[4] =
    {
        2U,
        4U,
        6U,
        8U
    };

    /* Define the register encoding corresponding to each hardware divisor. */
    static const uint32_t encodings[4] =
    {
        GUARDIAN_ADC_CCR_ADCPRE_DIV2,
        GUARDIAN_ADC_CCR_ADCPRE_DIV4,
        GUARDIAN_ADC_CCR_ADCPRE_DIV6,
        GUARDIAN_ADC_CCR_ADCPRE_DIV8
    };

    /* Track the current prescaler candidate. */
    size_t index = 0U;

    /* Reject an unavailable APB2 clock. */
    if (pclk2_hz == 0U)
    {
        /* Report invalid clock configuration. */
        return 0;
    }

    /* Select the fastest legal ADC clock. */
    for (index = 0U; index < 4U; ++index)
    {
        /* Calculate the ADC kernel clock for this prescaler. */
        uint32_t candidate_hz =
            pclk2_hz /
            (uint32_t)divisors[index];

        /* Accept the first candidate that satisfies the conservative ceiling. */
        if ((candidate_hz != 0U) &&
            (candidate_hz <=
             GUARDIAN_ADC_CONSERVATIVE_MAX_CLOCK_HZ))
        {
            /* Preserve the selected ADC common-control encoding. */
            guardian_adc_prescaler_bits =
                encodings[index];

            /* Preserve the resulting ADC kernel clock for throughput validation. */
            guardian_adc_clock_hz =
                candidate_hz;

            /* Report successful ADC clock selection. */
            return 1;
        }
    }

    /* Report that no hardware prescaler can satisfy the conservative ADC clock ceiling. */
    return 0;
}

/* Return the timer kernel clock associated with APB1. */
static uint32_t guardian_stm32f401_apb1_timer_hz(void)
{
    /* Read the active APB1 peripheral clock. */
    uint32_t pclk1_hz =
        guardian_stm32f401_pclk1_hz();

    /* Extract the three-bit APB1 prescaler field. */
    uint32_t ppre1 =
        (RCC->CFGR & GUARDIAN_RCC_PPRE1_MASK) >>
        GUARDIAN_RCC_PPRE1_SHIFT;

    /* Timers run at PCLK1 when APB1 is not divided. */
    if (ppre1 < 4U)
    {
        /* Return the undoubled timer clock. */
        return pclk1_hz;
    }

    /* STM32F4 APB timers run at twice PCLK when the APB prescaler is greater than one. */
    return pclk1_hz * 2U;
}

/* Execute a conservative short startup delay using CMSIS NOP instructions. */
static void guardian_stm32f401_startup_delay(void)
{
    /* Calculate a deliberately conservative loop count from the current HCLK. */
    volatile uint32_t loops =
        (SystemCoreClock / 1000000UL) * 50UL;

    /* Consume the bounded delay before TIM2 begins triggering internal ADC channels. */
    while (loops != 0U)
    {
        /* Prevent the compiler from removing the timing loop completely. */
        __NOP();

        /* Advance toward delay completion. */
        loops -= 1U;
    }
}

/* Configure one GPIOA pin for analog mode with no internal pulls. */
static void guardian_stm32f401_gpio_analog(
    uint32_t pin)
{
    /* Clear the two-bit mode field before selecting analog mode. */
    GPIOA->MODER &=
        ~(GUARDIAN_GPIO_MODE_MASK << (pin * 2U));

    /* Select analog mode. */
    GPIOA->MODER |=
        (GUARDIAN_GPIO_MODE_ANALOG << (pin * 2U));

    /* Disable internal pull-up and pull-down resistors. */
    GPIOA->PUPDR &=
        ~(GUARDIAN_GPIO_MODE_MASK << (pin * 2U));
}

/* Configure PA6 for TIM3 channel-1 input capture on AF2. */
static void guardian_stm32f401_gpio_rpm(void)
{
    /* Clear the PA6 mode field. */
    GPIOA->MODER &=
        ~(GUARDIAN_GPIO_MODE_MASK << (GUARDIAN_RPM_PIN * 2U));

    /* Select alternate-function mode for PA6. */
    GPIOA->MODER |=
        (GUARDIAN_GPIO_MODE_AF << (GUARDIAN_RPM_PIN * 2U));

    /* Disable internal pulls so the external sensor/front-end defines the logic level. */
    GPIOA->PUPDR &=
        ~(GUARDIAN_GPIO_MODE_MASK << (GUARDIAN_RPM_PIN * 2U));

    /* Clear the PA6 AFRL field. */
    GPIOA->AFR[0] &=
        ~(GUARDIAN_GPIO_AF_MASK << (GUARDIAN_RPM_PIN * 4U));

    /* Select AF2 TIM3_CH1 for PA6. */
    GPIOA->AFR[0] |=
        (GUARDIAN_RPM_AF << (GUARDIAN_RPM_PIN * 4U));
}

/* Configure TIM2 update events as the deterministic ADC scan trigger. */
static int guardian_stm32f401_configure_sample_timer(void)
{
    /* Read the active TIM2 kernel clock. */
    uint32_t timer_hz =
        guardian_stm32f401_apb1_timer_hz();

    /* Store the rounded number of timer clocks in one sample period. */
    uint64_t period_ticks = 0U;

    /* Reject an unavailable timer clock. */
    if (timer_hz == 0U)
    {
        /* Report invalid clock configuration. */
        return 0;
    }

    /* Calculate the nearest whole-number TIM2 period without a prescaler. */
    period_ticks =
        (
            (uint64_t)timer_hz +
            ((uint64_t)guardian_config.sample_rate_hz / 2U)
        ) /
        (uint64_t)guardian_config.sample_rate_hz;

    /* Reject impossible or unrepresentable sample periods. */
    if ((period_ticks == 0U) ||
        (period_ticks > ((uint64_t)UINT32_MAX + 1U)))
    {
        /* Report unsupported sample timing. */
        return 0;
    }

    /* Stop TIM2 before changing its timebase. */
    TIM2->CR1 = 0U;

    /* Use the full APB1 timer clock for maximum trigger precision. */
    TIM2->PSC = 0U;

    /* Program the rounded 32-bit auto-reload period. */
    TIM2->ARR =
        (uint32_t)(period_ticks - 1U);

    /* Reset the counter before publishing the new period. */
    TIM2->CNT = 0U;

    /* Select update event as TIM2 TRGO for ADC1. */
    TIM2->CR2 =
        GUARDIAN_TIM_CR2_MMS_UPDATE;

    /* Generate one update so the new prescaler and auto-reload values become active. */
    TIM2->EGR =
        GUARDIAN_TIM_EGR_UG;

    /* Clear the software-generated update flag before acquisition starts. */
    TIM2->SR = 0U;

    /* Enable auto-reload preload while leaving the timer stopped. */
    TIM2->CR1 =
        GUARDIAN_TIM_CR1_ARPE;

    /* Report successful sample timer configuration. */
    return 1;
}

/* Configure TIM3 channel-1 rising-edge input capture for RPM measurement. */
static int guardian_stm32f401_configure_rpm_timer(void)
{
    /* Read the active TIM3 kernel clock. */
    uint32_t timer_hz =
        guardian_stm32f401_apb1_timer_hz();

    /* Store the nearest prescaler divisor for the reference RPM counter rate. */
    uint32_t divisor = 0U;

    /* Reject an unavailable timer clock. */
    if (timer_hz == 0U)
    {
        /* Report invalid clock configuration. */
        return 0;
    }

    /* Round the timer prescaler divisor to the nearest whole value. */
    divisor =
        (
            timer_hz +
            (GUARDIAN_STM32F401_RPM_TIMER_HZ / 2U)
        ) /
        GUARDIAN_STM32F401_RPM_TIMER_HZ;

    /* Require a divisor representable by the 16-bit TIM3 prescaler. */
    if ((divisor == 0U) || (divisor > 65536U))
    {
        /* Report unsupported RPM timer timing. */
        return 0;
    }

    /* Store the actual counter rate after integer prescaler rounding. */
    guardian_rpm_timer_hz =
        timer_hz / divisor;

    /* Stop TIM3 before changing input capture configuration. */
    TIM3->CR1 = 0U;

    /* Program the 16-bit prescaler. */
    TIM3->PSC =
        divisor - 1U;

    /* Use the full 16-bit counter range so unsigned capture subtraction handles one wrap. */
    TIM3->ARR = 0xFFFFU;

    /* Reset the free-running capture counter. */
    TIM3->CNT = 0U;

    /* Route TI1 into capture channel 1 and apply a modest digital input filter. */
    TIM3->CCMR1 =
        GUARDIAN_TIM_CCMR1_CC1S_TI1 |
        GUARDIAN_TIM_CCMR1_IC1F_8_SAMPLES;

    /* Enable rising-edge channel-1 capture with default non-inverted polarity. */
    TIM3->CCER =
        GUARDIAN_TIM_CCER_CC1E;

    /* Enable update and channel-1 capture interrupts for software-extended timestamps. */
    TIM3->DIER =
        GUARDIAN_TIM_DIER_UIE |
        GUARDIAN_TIM_DIER_CC1IE;

    /* Generate an update so the prescaler becomes active immediately. */
    TIM3->EGR =
        GUARDIAN_TIM_EGR_UG;

    /* Clear the generated update and stale capture flags. */
    TIM3->SR = 0U;

    /* Configure the requested TIM3 interrupt priority. */
    NVIC_SetPriority(
        TIM3_IRQn,
        guardian_config.rpm_irq_priority);

    /* Enable TIM3 interrupt delivery. */
    NVIC_EnableIRQ(
        TIM3_IRQn);

    /* Start the free-running RPM capture counter. */
    TIM3->CR1 =
        GUARDIAN_TIM_CR1_CEN;

    /* Report successful RPM timer configuration. */
    return 1;
}

/* Configure ADC1 scan order, sample times, VREFINT and temperature sensor. */
static void guardian_stm32f401_configure_adc(void)
{
    /* Stop ADC1 before replacing conversion configuration. */
    ADC1->CR2 = 0U;

    /* Enable scan mode and ADC overrun interrupts. */
    ADC1->CR1 =
        GUARDIAN_ADC_CR1_SCAN |
        GUARDIAN_ADC_CR1_OVRIE;

    /* Configure 56-cycle sample time for ADC1 channel 0. */
    ADC1->SMPR2 &=
        ~((uint32_t)0x7U << (GUARDIAN_ADC_CHANNEL_VIBRATION * 3U));

    /* Apply the external-channel sample time for vibration. */
    ADC1->SMPR2 |=
        GUARDIAN_ADC_SAMPLE_56_CYCLES <<
        (GUARDIAN_ADC_CHANNEL_VIBRATION * 3U);

    /* Configure 56-cycle sample time for ADC1 channel 1. */
    ADC1->SMPR2 &=
        ~((uint32_t)0x7U << (GUARDIAN_ADC_CHANNEL_CURRENT * 3U));

    /* Apply the external-channel sample time for current. */
    ADC1->SMPR2 |=
        GUARDIAN_ADC_SAMPLE_56_CYCLES <<
        (GUARDIAN_ADC_CHANNEL_CURRENT * 3U);

    /* Configure 56-cycle sample time for ADC1 channel 4. */
    ADC1->SMPR2 &=
        ~((uint32_t)0x7U << (GUARDIAN_ADC_CHANNEL_SUPPLY * 3U));

    /* Apply the external-channel sample time for the supply divider. */
    ADC1->SMPR2 |=
        GUARDIAN_ADC_SAMPLE_56_CYCLES <<
        (GUARDIAN_ADC_CHANNEL_SUPPLY * 3U);

    /* Clear the channel-16 internal temperature sample-time field. */
    ADC1->SMPR1 &=
        ~((uint32_t)0x7U <<
          ((GUARDIAN_ADC_CHANNEL_TEMPERATURE - 10U) * 3U));

    /* Use the longest 480-cycle sample time for the internal temperature sensor. */
    ADC1->SMPR1 |=
        GUARDIAN_ADC_SAMPLE_480_CYCLES <<
        ((GUARDIAN_ADC_CHANNEL_TEMPERATURE - 10U) * 3U);

    /* Clear the channel-17 VREFINT sample-time field. */
    ADC1->SMPR1 &=
        ~((uint32_t)0x7U <<
          ((GUARDIAN_ADC_CHANNEL_VREFINT - 10U) * 3U));

    /* Use the longest 480-cycle sample time for VREFINT. */
    ADC1->SMPR1 |=
        GUARDIAN_ADC_SAMPLE_480_CYCLES <<
        ((GUARDIAN_ADC_CHANNEL_VREFINT - 10U) * 3U);

    /* Configure exactly five regular conversions. */
    ADC1->SQR1 =
        GUARDIAN_ADC_SQR1_LENGTH_5;

    /* Encode vibration channel 0 as regular sequence slot 1. */
    ADC1->SQR3 =
        (GUARDIAN_ADC_CHANNEL_VIBRATION << 0U) |

        /* Encode current channel 1 as regular sequence slot 2. */
        (GUARDIAN_ADC_CHANNEL_CURRENT << 5U) |

        /* Encode supply channel 4 as regular sequence slot 3. */
        (GUARDIAN_ADC_CHANNEL_SUPPLY << 10U) |

        /* Encode temperature channel 16 as regular sequence slot 4. */
        (GUARDIAN_ADC_CHANNEL_TEMPERATURE << 15U) |

        /* Encode VREFINT channel 17 as regular sequence slot 5. */
        (GUARDIAN_ADC_CHANNEL_VREFINT << 20U);

    /* Disable injected-sequence use in this acquisition milestone. */
    ADC1->JSQR = 0U;

    /* Apply the runtime-selected conservative ADC clock, disable VBAT and enable internal channels. */
    ADC->CCR =
        (ADC->CCR &
         ~(GUARDIAN_ADC_CCR_ADCPRE_MASK |
           GUARDIAN_ADC_CCR_VBATE)) |
        guardian_adc_prescaler_bits |
        GUARDIAN_ADC_CCR_TSVREFE;

    /* Select TIM2 TRGO rising edge, DMA mode and continuous DMA requests. */
    ADC1->CR2 =
        GUARDIAN_ADC_CR2_DMA |
        GUARDIAN_ADC_CR2_DDS |
        GUARDIAN_ADC_CR2_EXTSEL_TIM2_TRGO |
        GUARDIAN_ADC_CR2_EXTEN_RISING;

    /* Clear stale ADC status flags before enabling conversion. */
    ADC1->SR = 0U;

    /* Enable ADC1 while leaving conversions controlled exclusively by TIM2 TRGO. */
    ADC1->CR2 |=
        GUARDIAN_ADC_CR2_ADON;
}

/* Disable DMA2 Stream0 and wait for hardware acknowledgement. */
static int guardian_stm32f401_disable_dma(void)
{
    /* Request DMA2 Stream0 disable. */
    DMA2_Stream0->CR &=
        ~GUARDIAN_DMA_CR_EN;

    /* Bound the hardware-disable wait so a peripheral fault cannot hang firmware forever. */
    uint32_t timeout = 1000000UL;

    /* Wait until hardware clears the stream enable bit. */
    while ((DMA2_Stream0->CR & GUARDIAN_DMA_CR_EN) != 0U)
    {
        /* Fail when the bounded wait is exhausted. */
        if (timeout == 0U)
        {
            /* Report failed DMA shutdown. */
            return 0;
        }

        /* Advance toward the bounded timeout. */
        timeout -= 1U;
    }

    /* Report successful DMA shutdown. */
    return 1;
}

/* Configure DMA2 Stream0 Channel 0 for ADC1 peripheral-to-memory double buffering. */
static int guardian_stm32f401_configure_dma(void)
{
    /* Disable the stream before writing protected configuration fields. */
    if (guardian_stm32f401_disable_dma() == 0)
    {
        /* Report failed DMA reconfiguration. */
        return 0;
    }

    /* Clear every stale Stream0 status flag. */
    DMA2->LIFCR =
        GUARDIAN_DMA_STREAM0_ALL_FLAGS;

    /* Point the fixed peripheral address at ADC1 regular data register. */
    DMA2_Stream0->PAR =
        (uint32_t)(uintptr_t)&ADC1->DR;

    /* Point memory target zero at the first bounded raw sample buffer. */
    DMA2_Stream0->M0AR =
        (uint32_t)(uintptr_t)guardian_dma_buffer_0;

    /* Point memory target one at the second bounded raw sample buffer. */
    DMA2_Stream0->M1AR =
        (uint32_t)(uintptr_t)guardian_dma_buffer_1;

    /* Transfer one complete 64-frame interleaved scan block before target swapping. */
    DMA2_Stream0->NDTR =
        (uint32_t)GUARDIAN_ACQUISITION_SAMPLES_PER_BLOCK;

    /* Configure Channel 0, peripheral-to-memory, half-words, increment, high priority and DBM. */
    DMA2_Stream0->CR =
        GUARDIAN_DMA_CR_DMEIE |
        GUARDIAN_DMA_CR_TEIE |
        GUARDIAN_DMA_CR_TCIE |
        GUARDIAN_DMA_CR_CIRC |
        GUARDIAN_DMA_CR_MINC |
        GUARDIAN_DMA_CR_PSIZE_16 |
        GUARDIAN_DMA_CR_MSIZE_16 |
        GUARDIAN_DMA_CR_PRIORITY_HIGH |
        GUARDIAN_DMA_CR_DBM;

    /* Use direct mode because ADC half-word transfers do not require FIFO buffering. */
    DMA2_Stream0->FCR = 0U;

    /* Clear pending software ownership of both DMA targets before restart. */
    guardian_ready_mask = 0U;

    /* Configure the requested DMA2 Stream0 NVIC priority. */
    NVIC_SetPriority(
        DMA2_Stream0_IRQn,
        guardian_config.dma_irq_priority);

    /* Enable DMA2 Stream0 interrupt delivery. */
    NVIC_EnableIRQ(
        DMA2_Stream0_IRQn);

    /* Enable the fully configured double-buffer stream. */
    DMA2_Stream0->CR |=
        GUARDIAN_DMA_CR_EN;

    /* Report successful DMA configuration. */
    return 1;
}

/* Start or restart the deterministic TIM2 -> ADC1 -> DMA2 acquisition stream. */
static int guardian_stm32f401_start_stream(void)
{
    /* Stop TIM2 so no trigger arrives during ADC/DMA reconstruction. */
    TIM2->CR1 &=
        ~GUARDIAN_TIM_CR1_CEN;

    /* Disable ADC1 before rebuilding its DMA relationship. */
    ADC1->CR2 &=
        ~GUARDIAN_ADC_CR2_ADON;

    /* Rebuild ADC scan and trigger configuration. */
    guardian_stm32f401_configure_adc();

    /* Rebuild DMA target addresses, count and stream state. */
    if (guardian_stm32f401_configure_dma() == 0)
    {
        /* Report failed acquisition restart. */
        return 0;
    }

    /* Wait conservatively for ADC and internal temperature/VREFINT startup. */
    guardian_stm32f401_startup_delay();

    /* Reset the deterministic trigger counter before starting a new acquisition epoch. */
    TIM2->CNT = 0U;

    /* Clear timer status before enabling periodic triggers. */
    TIM2->SR = 0U;

    /* Start TIM2 while preserving auto-reload preload. */
    TIM2->CR1 |=
        GUARDIAN_TIM_CR1_CEN;

    /* Report successful acquisition stream start. */
    return 1;
}

/* Fill one configuration with reference hardware and factory calibration defaults. */
void guardian_stm32f401_acquisition_default_config(
    guardian_stm32f401_acquisition_config_t *config)
{
    /* Ignore a missing caller pointer defensively. */
    if (config == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Select the M6 reference ADC scan-frame rate. */
    config->sample_rate_hz =
        GUARDIAN_STM32F401_ACQUISITION_DEFAULT_SAMPLE_RATE_HZ;

    /* Assume one RPM sensor edge per revolution until hardware defines otherwise. */
    config->rpm_pulses_per_revolution = 1U;

    /* Keep DMA below the command-channel UART priority in the reference configuration. */
    config->dma_irq_priority = 6U;

    /* Keep RPM capture at the same bounded acquisition priority. */
    config->rpm_irq_priority = 6U;

    /* Keep ADC overrun reporting at the same bounded acquisition priority. */
    config->adc_irq_priority = 6U;

    /* Read the factory VREFINT calibration acquired at 3.3 V. */
    config->calibration.vrefint_cal_code =
        guardian_stm32f401_factory_u16(
            GUARDIAN_VREFINT_CAL_ADDRESS);

    /* Read the factory temperature calibration acquired at 30 C. */
    config->calibration.temperature_cal_30_code =
        guardian_stm32f401_factory_u16(
            GUARDIAN_TEMP_CAL_30_ADDRESS);

    /* Read the factory temperature calibration acquired at 110 C. */
    config->calibration.temperature_cal_110_code =
        guardian_stm32f401_factory_u16(
            GUARDIAN_TEMP_CAL_110_ADDRESS);

    /* Use midscale as a reference zero-g placeholder for an analog vibration front-end. */
    config->calibration.vibration_zero_code = 2048U;

    /* Use one milli-g per normalized ADC code until the actual vibration sensor is calibrated. */
    config->calibration.vibration_mg_per_code_num = 1U;

    /* Keep the reference vibration scale denominator explicit. */
    config->calibration.vibration_mg_per_code_den = 1U;

    /* Use ground-referenced zero current until the actual sensor front-end is calibrated. */
    config->calibration.current_zero_code = 0U;

    /* Map full-scale normalized ADC code to 5 A as a reference current-sensor placeholder. */
    config->calibration.current_ma_per_code_num = 5000U;

    /* Divide by 12-bit full scale for the reference current mapping. */
    config->calibration.current_ma_per_code_den =
        GUARDIAN_ACQUISITION_ADC_MAX_CODE;

    /* Assume a 1:1 resistor divider so the measured supply equals twice the PA4 pin voltage. */
    config->calibration.supply_divider_num = 2U;

    /* Keep the divider denominator explicit. */
    config->calibration.supply_divider_den = 1U;

    /* Mark external sensor scaling as reference calibration until real hardware values replace it. */
    config->calibration.base_status_flags =
        GUARDIAN_ACQUISITION_STATUS_DEFAULT_CALIBRATION;
}

/* Initialize TIM2-triggered ADC1 scan, DMA2 double buffer and TIM3 RPM capture. */
int guardian_stm32f401_acquisition_init(
    const guardian_stm32f401_acquisition_config_t *config)
{
    /* Store portable acquisition initialization status. */
    guardian_acquisition_result_t result =
        GUARDIAN_ACQUISITION_OK;

    /* Mark hardware unavailable until this complete initialization succeeds. */
    guardian_initialized = 0U;

    /* Reset cumulative hardware diagnostics for the new acquisition epoch. */
    guardian_hw_stats.dma_blocks_completed = 0U;
    guardian_hw_stats.dma_blocks_dropped = 0U;
    guardian_hw_stats.dma_errors = 0U;
    guardian_hw_stats.adc_overruns = 0U;
    guardian_hw_stats.recoveries = 0U;
    guardian_hw_stats.rpm_edges = 0U;

    /* Reject a missing configuration pointer. */
    if (config == NULL)
    {
        /* Report invalid initialization. */
        return 0;
    }

    /* Enforce the documented deterministic ADC scan-frame rate range. */
    if ((config->sample_rate_hz <
         GUARDIAN_STM32F401_ACQUISITION_MIN_SAMPLE_RATE_HZ) ||
        (config->sample_rate_hz >
         GUARDIAN_STM32F401_ACQUISITION_MAX_SAMPLE_RATE_HZ))
    {
        /* Report unsupported sample timing. */
        return 0;
    }

    /* Require at least one RPM sensor pulse per shaft revolution. */
    if (config->rpm_pulses_per_revolution == 0U)
    {
        /* Report invalid RPM configuration. */
        return 0;
    }

    /* Refresh CMSIS SystemCoreClock after the application's clock setup. */
    SystemCoreClockUpdate();

    /* Copy the complete validated hardware configuration by value. */
    guardian_config = *config;

    /* Select the fastest ADC clock that remains within the conservative datasheet limit. */
    if (guardian_stm32f401_select_adc_clock() == 0)
    {
        /* Report an unsupported APB2/ADC clock relationship. */
        return 0;
    }

    /* Require enough ADC cycles to finish all five conversions before the next TIM2 trigger. */
    if ((uint64_t)guardian_adc_clock_hz <
        (
            (uint64_t)guardian_config.sample_rate_hz *
            (uint64_t)GUARDIAN_ADC_SEQUENCE_CYCLES
        ))
    {
        /* Report a sample rate that would overlap regular scan sequences. */
        return 0;
    }

    /* Initialize the portable acquisition processor before enabling peripherals. */
    result =
        guardian_acquisition_init(
            &guardian_acquisition,
            &guardian_config.calibration);

    /* Reject unusable calibration before acquiring raw data. */
    if (result != GUARDIAN_ACQUISITION_OK)
    {
        /* Report failed initialization. */
        return 0;
    }

    /* Enable GPIOA clock for PA0, PA1, PA4 and PA6. */
    RCC->AHB1ENR |=
        GUARDIAN_RCC_AHB1_GPIOAEN;

    /* Enable DMA2 clock for ADC1 transfers. */
    RCC->AHB1ENR |=
        GUARDIAN_RCC_AHB1_DMA2EN;

    /* Enable TIM2 and TIM3 clocks on APB1. */
    RCC->APB1ENR |=
        GUARDIAN_RCC_APB1_TIM2EN |
        GUARDIAN_RCC_APB1_TIM3EN;

    /* Enable ADC1 clock on APB2. */
    RCC->APB2ENR |=
        GUARDIAN_RCC_APB2_ADC1EN;

    /* Perform a readback so peripheral clocks are visible before register access. */
    (void)RCC->APB2ENR;

    /* Configure PA0 as analog vibration input. */
    guardian_stm32f401_gpio_analog(
        GUARDIAN_ADC_VIBRATION_PIN);

    /* Configure PA1 as analog current input. */
    guardian_stm32f401_gpio_analog(
        GUARDIAN_ADC_CURRENT_PIN);

    /* Configure PA4 as analog supply-divider input. */
    guardian_stm32f401_gpio_analog(
        GUARDIAN_ADC_SUPPLY_PIN);

    /* Configure PA6 AF2 as TIM3_CH1 RPM input capture. */
    guardian_stm32f401_gpio_rpm();

    /* Configure the deterministic TIM2 ADC trigger. */
    if (guardian_stm32f401_configure_sample_timer() == 0)
    {
        /* Report failed sample timer configuration. */
        return 0;
    }

    /* Configure TIM3 RPM input capture. */
    if (guardian_stm32f401_configure_rpm_timer() == 0)
    {
        /* Report failed RPM timer configuration. */
        return 0;
    }

    /* Configure ADC overrun interrupt priority. */
    NVIC_SetPriority(
        ADC_IRQn,
        guardian_config.adc_irq_priority);

    /* Enable ADC overrun interrupt delivery. */
    NVIC_EnableIRQ(
        ADC_IRQn);

    /* Reset asynchronous hardware state before the first stream start. */
    guardian_last_rpm_timestamp = 0U;

    /* Reset the software extension of the TIM3 16-bit counter. */
    guardian_rpm_overflow_count = 0U;

    /* Reset RPM period history. */
    guardian_has_rpm_capture = 0U;

    /* Start without a valid RPM measurement. */
    guardian_rpm = 0U;

    /* Start RPM age at the stale sentinel. */
    guardian_rpm_age_ms = UINT32_MAX;

    /* Mark RPM invalid until two edges establish one complete period. */
    guardian_rpm_valid = 0U;

    /* Clear pending hardware quality flags. */
    guardian_pending_quality_flags = 0U;

    /* Clear pending recovery state. */
    guardian_recovery_required = 0U;

    /* Start the deterministic ADC/DMA acquisition stream. */
    if (guardian_stm32f401_start_stream() == 0)
    {
        /* Report failed acquisition stream startup. */
        return 0;
    }

    /* Publish successful hardware initialization. */
    guardian_initialized = 1U;

    /* Report successful acquisition initialization. */
    return 1;
}

/* Process at most one completed DMA target buffer in foreground code. */
int guardian_stm32f401_acquisition_poll(
    guardian_machine_measurements_t *measurements)
{
    /* Store the selected stable DMA target index. */
    uint8_t selected_buffer = 0xFFU;

    /* Store a snapshot of pending hardware quality flags. */
    uint16_t hardware_flags = 0U;

    /* Store a coherent RPM snapshot. */
    guardian_acquisition_aux_t aux = {0};

    /* Store portable processing outcome. */
    guardian_acquisition_result_t result =
        GUARDIAN_ACQUISITION_OK;

    /* Store interrupt state before a short critical section. */
    uint32_t primask = 0U;

    /* Track the current raw half-word while snapshotting one stable DMA target. */
    size_t sample_index = 0U;

    /* Store the selected DMA target pointer after ownership validation. */
    const uint16_t *selected_samples = NULL;

    /* Reject a missing output pointer or uninitialized hardware. */
    if ((measurements == NULL) ||
        (guardian_initialized == 0U))
    {
        /* Report no fresh measurement. */
        return 0;
    }

    /* Recover ADC/DMA state in foreground rather than inside an interrupt handler. */
    if (guardian_recovery_required != 0U)
    {
        /* Attempt a complete deterministic acquisition stream rebuild. */
        if (guardian_stm32f401_start_stream() == 0)
        {
            /* Report hardware recovery failure. */
            return -1;
        }

        /* Enter a short critical section before clearing the recovery request. */
        primask = __get_PRIMASK();

        /* Prevent IRQ state from changing while the recovery flag is acknowledged. */
        __disable_irq();

        /* Clear the recovery request after successful stream reconstruction. */
        guardian_recovery_required = 0U;

        /* Restore interrupts only when they were enabled before this critical section. */
        if (primask == 0U)
        {
            /* Re-enable interrupt delivery. */
            __enable_irq();
        }

        /* Count the successful acquisition recovery. */
        guardian_stm32f401_increment_u32(
            &guardian_hw_stats.recoveries);
    }

    /* Preserve the caller's previous interrupt-enable state. */
    primask = __get_PRIMASK();

    /* Enter a very short ownership critical section. */
    __disable_irq();

    /* Prefer the oldest conventional buffer ordering when buffer zero is ready. */
    if ((guardian_ready_mask &
         GUARDIAN_DMA_BUFFER_0_READY) != 0U)
    {
        /* Claim buffer zero for foreground processing. */
        selected_buffer = 0U;

        /* Clear its pending-ready bit before processing. */
        guardian_ready_mask &=
            (uint8_t)~GUARDIAN_DMA_BUFFER_0_READY;
    }
    else if ((guardian_ready_mask &
              GUARDIAN_DMA_BUFFER_1_READY) != 0U)
    {
        /* Claim buffer one for foreground processing. */
        selected_buffer = 1U;

        /* Clear its pending-ready bit before processing. */
        guardian_ready_mask &=
            (uint8_t)~GUARDIAN_DMA_BUFFER_1_READY;
    }

    /* Snapshot pending quality flags for the next processed block. */
    hardware_flags =
        guardian_pending_quality_flags;

    /* Clear pending flags only when one actual raw block was claimed. */
    if (selected_buffer != 0xFFU)
    {
        /* Transfer hardware-quality ownership to this block. */
        guardian_pending_quality_flags = 0U;
    }

    /* Snapshot the latest RPM value. */
    aux.rpm = guardian_rpm;

    /* Snapshot RPM freshness. */
    aux.rpm_valid = guardian_rpm_valid;

    /* Attach all hardware-quality flags to this block. */
    aux.hardware_status_flags =
        hardware_flags;

    /* Restore interrupts only when they were enabled before this critical section. */
    if (primask == 0U)
    {
        /* Re-enable interrupt delivery. */
        __enable_irq();
    }

    /* Return immediately when no complete DMA block is pending. */
    if (selected_buffer == 0xFFU)
    {
        /* Report no fresh measurement. */
        return 0;
    }

    /* Select the claimed stable DMA target pointer. */
    selected_samples =
        (selected_buffer == 0U)
        ? guardian_dma_buffer_0
        : guardian_dma_buffer_1;

    /* Reject a stale ownership token if DMA has already switched back to this target. */
    if (((selected_buffer == 0U) &&
         ((DMA2_Stream0->CR & GUARDIAN_DMA_CR_CT) == 0U)) ||
        ((selected_buffer == 1U) &&
         ((DMA2_Stream0->CR & GUARDIAN_DMA_CR_CT) != 0U)))
    {
        /* Preserve caller interrupt state before updating shared drop diagnostics. */
        primask = __get_PRIMASK();

        /* Enter a short critical section. */
        __disable_irq();

        /* Count the block that became active before foreground could snapshot it. */
        guardian_stm32f401_increment_u32(
            &guardian_hw_stats.dma_blocks_dropped);

        /* Preserve previous quality events and attach data loss to the next trustworthy block. */
        guardian_pending_quality_flags |=
            hardware_flags |
            GUARDIAN_ACQUISITION_STATUS_DMA_DROP;

        /* Restore interrupts only when they were previously enabled. */
        if (primask == 0U)
        {
            /* Re-enable interrupt delivery. */
            __enable_irq();
        }

        /* Report no fresh trustworthy measurement. */
        return 0;
    }

    /* Copy the completed DMA target into foreground-owned memory before conversion math. */
    for (sample_index = 0U;
         sample_index < GUARDIAN_ACQUISITION_SAMPLES_PER_BLOCK;
         ++sample_index)
    {
        /* Snapshot exactly one raw half-word. */
        guardian_process_buffer[sample_index] =
            selected_samples[sample_index];
    }

    /* Verify DMA did not switch back to the selected target during the bounded snapshot copy. */
    if (((selected_buffer == 0U) &&
         ((DMA2_Stream0->CR & GUARDIAN_DMA_CR_CT) == 0U)) ||
        ((selected_buffer == 1U) &&
         ((DMA2_Stream0->CR & GUARDIAN_DMA_CR_CT) != 0U)))
    {
        /* Preserve caller interrupt state before updating shared drop diagnostics. */
        primask = __get_PRIMASK();

        /* Enter a short critical section. */
        __disable_irq();

        /* Count the snapshot that raced the next DMA target reuse. */
        guardian_stm32f401_increment_u32(
            &guardian_hw_stats.dma_blocks_dropped);

        /* Preserve previous quality events and attach data loss to the next trustworthy block. */
        guardian_pending_quality_flags |=
            hardware_flags |
            GUARDIAN_ACQUISITION_STATUS_DMA_DROP;

        /* Restore interrupts only when they were previously enabled. */
        if (primask == 0U)
        {
            /* Re-enable interrupt delivery. */
            __enable_irq();
        }

        /* Report no fresh trustworthy measurement. */
        return 0;
    }

    /* Convert the immutable foreground snapshot into engineering units. */
    result =
        guardian_acquisition_process_block(
            &guardian_acquisition,
            guardian_process_buffer,
            GUARDIAN_ACQUISITION_SAMPLES_PER_BLOCK,
            &aux);

    /* Reject blocks that fail portable reference or calibration validation. */
    if (result != GUARDIAN_ACQUISITION_OK)
    {
        /* Preserve caller interrupt state before requeueing quality diagnostics. */
        primask = __get_PRIMASK();

        /* Enter a short critical section. */
        __disable_irq();

        /* Carry forward prior hardware events and mark the rejected ADC measurement. */
        guardian_pending_quality_flags |=
            hardware_flags |
            GUARDIAN_ACQUISITION_STATUS_ADC_ERROR;

        /* Restore interrupts only when they were previously enabled. */
        if (primask == 0U)
        {
            /* Re-enable interrupt delivery. */
            __enable_irq();
        }

        /* Report the foreground processing failure. */
        return -1;
    }

    /* Return the latest coherent engineering-unit snapshot to the firmware application. */
    *measurements =
        guardian_acquisition_latest(
            &guardian_acquisition);

    /* Report exactly one fresh measurement block. */
    return 1;
}

/* Advance RPM staleness tracking by one millisecond. */
void guardian_stm32f401_acquisition_tick_1ms(void)
{
    /* Ignore ticks before hardware initialization completes. */
    if (guardian_initialized == 0U)
    {
        /* Return without touching acquisition state. */
        return;
    }

    /* Saturate RPM age instead of wrapping. */
    if (guardian_rpm_age_ms != UINT32_MAX)
    {
        /* Advance one millisecond. */
        guardian_rpm_age_ms += 1U;
    }

    /* Mark RPM stale after the documented timeout. */
    if (guardian_rpm_age_ms >
        GUARDIAN_STM32F401_RPM_STALE_MS)
    {
        /* Prevent stale RPM from being published as current machine speed. */
        guardian_rpm_valid = 0U;
    }
}

/* Return a coherent copy of hardware acquisition diagnostics. */
guardian_stm32f401_acquisition_stats_t guardian_stm32f401_acquisition_stats(void)
{
    /* Create local non-volatile diagnostic storage. */
    guardian_stm32f401_acquisition_stats_t stats = {0};

    /* Preserve caller interrupt state before copying ISR-updated counters. */
    uint32_t primask =
        __get_PRIMASK();

    /* Enter a short diagnostic snapshot critical section. */
    __disable_irq();

    /* Copy completed DMA block count. */
    stats.dma_blocks_completed =
        guardian_hw_stats.dma_blocks_completed;

    /* Copy dropped DMA block count. */
    stats.dma_blocks_dropped =
        guardian_hw_stats.dma_blocks_dropped;

    /* Copy DMA error count. */
    stats.dma_errors =
        guardian_hw_stats.dma_errors;

    /* Copy ADC overrun count. */
    stats.adc_overruns =
        guardian_hw_stats.adc_overruns;

    /* Copy successful recovery count. */
    stats.recoveries =
        guardian_hw_stats.recoveries;

    /* Copy valid RPM edge count. */
    stats.rpm_edges =
        guardian_hw_stats.rpm_edges;

    /* Restore interrupts only when they were enabled before this critical section. */
    if (primask == 0U)
    {
        /* Re-enable interrupt delivery. */
        __enable_irq();
    }

    /* Return the coherent diagnostic snapshot by value. */
    return stats;
}

/* Service DMA2 Stream0 completion and transfer-error events. */
void guardian_stm32f401_acquisition_dma_irq_handler(void)
{
    /* Snapshot the Stream0 low interrupt status bits. */
    uint32_t status =
        DMA2->LISR;

    /* Detect DMA transfer or direct-mode errors. */
    if ((status &
         (GUARDIAN_DMA_TEIF0 |
          GUARDIAN_DMA_DMEIF0 |
          GUARDIAN_DMA_FEIF0)) != 0U)
    {
        /* Count one DMA fault event. */
        guardian_stm32f401_increment_u32(
            &guardian_hw_stats.dma_errors);

        /* Mark the next trustworthy measurement with a DMA error. */
        guardian_pending_quality_flags |=
            GUARDIAN_ACQUISITION_STATUS_DMA_ERROR;

        /* Request full ADC/DMA recovery in foreground code. */
        guardian_recovery_required = 1U;

        /* Discard any pending target ownership because the failed transfer may be incomplete. */
        guardian_ready_mask = 0U;

        /* Clear every Stream0 flag observed by this handler. */
        DMA2->LIFCR =
            status &
            GUARDIAN_DMA_STREAM0_ALL_FLAGS;

        /* Return without publishing a potentially corrupted DMA target. */
        return;
    }

    /* Detect one completed DMA target buffer. */
    if ((status &
         GUARDIAN_DMA_TCIF0) != 0U)
    {
        /* Store which target just completed after hardware swapped CT. */
        uint8_t completed_mask = 0U;

        /* Store which target hardware has already started filling next. */
        uint8_t active_mask = 0U;

        /* CT=1 means DMA now writes M1, so M0 just completed. */
        if ((DMA2_Stream0->CR &
             GUARDIAN_DMA_CR_CT) != 0U)
        {
            /* Select the completed first target buffer. */
            completed_mask =
                GUARDIAN_DMA_BUFFER_0_READY;

            /* Identify the newly active second target buffer. */
            active_mask =
                GUARDIAN_DMA_BUFFER_1_READY;
        }
        else
        {
            /* CT=0 means DMA now writes M0, so M1 just completed. */
            completed_mask =
                GUARDIAN_DMA_BUFFER_1_READY;

            /* Identify the newly active first target buffer. */
            active_mask =
                GUARDIAN_DMA_BUFFER_0_READY;
        }

        /* Invalidate any unprocessed block whose memory target is being reused now. */
        if ((guardian_ready_mask &
             active_mask) != 0U)
        {
            /* Remove stale ownership before foreground can read a buffer being overwritten. */
            guardian_ready_mask &=
                (uint8_t)~active_mask;

            /* Count the overwritten unprocessed block. */
            guardian_stm32f401_increment_u32(
                &guardian_hw_stats.dma_blocks_dropped);

            /* Mark the next trustworthy processed block with the data-loss diagnostic. */
            guardian_pending_quality_flags |=
                GUARDIAN_ACQUISITION_STATUS_DMA_DROP;
        }

        /* Detect impossible duplicate completion ownership defensively. */
        if ((guardian_ready_mask &
             completed_mask) != 0U)
        {
            /* Count the stale completed-target ownership. */
            guardian_stm32f401_increment_u32(
                &guardian_hw_stats.dma_blocks_dropped);

            /* Preserve the data-loss diagnostic. */
            guardian_pending_quality_flags |=
                GUARDIAN_ACQUISITION_STATUS_DMA_DROP;
        }

        /* Publish only the completed target that is no longer active. */
        guardian_ready_mask |=
            completed_mask;

        /* Count the completed DMA block. */
        guardian_stm32f401_increment_u32(
            &guardian_hw_stats.dma_blocks_completed);
    }

    /* Clear every Stream0 flag observed by this handler. */
    DMA2->LIFCR =
        status &
        GUARDIAN_DMA_STREAM0_ALL_FLAGS;
}

/* Service ADC1 overrun events that require deterministic stream recovery. */
void guardian_stm32f401_acquisition_adc_irq_handler(void)
{
    /* Snapshot ADC1 status before clearing writable flags. */
    uint32_t status =
        ADC1->SR;

    /* Handle only the overrun condition used by M6 diagnostics. */
    if ((status &
         GUARDIAN_ADC_SR_OVR) != 0U)
    {
        /* Count the ADC overrun event. */
        guardian_stm32f401_increment_u32(
            &guardian_hw_stats.adc_overruns);

        /* Mark the next trustworthy block with an ADC error diagnostic. */
        guardian_pending_quality_flags |=
            GUARDIAN_ACQUISITION_STATUS_ADC_ERROR;

        /* Request the RM0368-required ADC/DMA reconstruction in foreground code. */
        guardian_recovery_required = 1U;

        /* Clear the write-zero-to-clear OVR status bit. */
        ADC1->SR &=
            ~GUARDIAN_ADC_SR_OVR;
    }
}

/* Service TIM3 channel-1 RPM input-capture and counter-overflow events. */
void guardian_stm32f401_acquisition_tim3_irq_handler(void)
{
    /* Snapshot TIM3 status before clearing update or capture flags. */
    uint32_t status =
        TIM3->SR;

    /* Snapshot the software overflow epoch associated with the current interrupt. */
    uint32_t overflow_snapshot =
        guardian_rpm_overflow_count;

    /* Process one valid channel-1 capture. */
    if ((status &
         GUARDIAN_TIM_SR_CC1IF) != 0U)
    {
        /* Read the captured 16-bit free-running timer value. */
        uint16_t capture =
            (uint16_t)TIM3->CCR1;

        /* Start with the overflow epoch visible when this interrupt began. */
        uint32_t capture_overflows =
            overflow_snapshot;

        /* Resolve a capture that occurred just after an unserviced timer wrap. */
        if (((status &
              GUARDIAN_TIM_SR_UIF) != 0U) &&
            (capture < 0x8000U))
        {
            /* Associate the low capture value with the newly completed timer epoch. */
            capture_overflows += 1U;
        }

        /* Combine the software epoch and hardware CCR1 into one 32-bit modulo timestamp. */
        uint32_t capture_timestamp =
            (capture_overflows << 16U) |
            (uint32_t)capture;

        /* Count the observed shaft-sensor edge. */
        guardian_stm32f401_increment_u32(
            &guardian_hw_stats.rpm_edges);

        /* Calculate RPM only after one previous edge establishes a complete period. */
        if (guardian_has_rpm_capture != 0U)
        {
            /* Unsigned 32-bit subtraction supports many TIM3 wraps between shaft edges. */
            uint32_t period_ticks =
                capture_timestamp -
                guardian_last_rpm_timestamp;

            /* Reject a zero-width period caused by duplicate or noisy captures. */
            if (period_ticks != 0U)
            {
                /* Build the denominator from measured timer ticks and pulses per revolution. */
                uint64_t denominator =
                    (uint64_t)period_ticks *
                    (uint64_t)guardian_config.rpm_pulses_per_revolution;

                /* Calculate RPM with nearest-integer rounding. */
                uint64_t rpm =
                    (
                        ((uint64_t)guardian_rpm_timer_hz * 60ULL) +
                        (denominator / 2ULL)
                    ) /
                    denominator;

                /* Saturate the published telemetry RPM field. */
                if (rpm > UINT16_MAX)
                {
                    /* Clamp unexpectedly high speed to the wire-field maximum. */
                    rpm = UINT16_MAX;
                }

                /* Publish the new shaft speed. */
                guardian_rpm =
                    (uint16_t)rpm;

                /* Reset RPM freshness age. */
                guardian_rpm_age_ms = 0U;

                /* Mark the RPM value fresh. */
                guardian_rpm_valid = 1U;
            }
        }

        /* Preserve this extended edge timestamp for the next period measurement. */
        guardian_last_rpm_timestamp =
            capture_timestamp;

        /* Mark that one previous capture now exists. */
        guardian_has_rpm_capture = 1U;
    }

    /* Extend the 16-bit TIM3 counter whenever one update event occurred. */
    if ((status &
         GUARDIAN_TIM_SR_UIF) != 0U)
    {
        /* Advance the software timer epoch modulo 2^32 capture timestamps. */
        guardian_rpm_overflow_count += 1U;
    }

    /* Treat capture overrun as stale RPM rather than publishing an ambiguous period. */
    if ((status &
         GUARDIAN_TIM_SR_CC1OF) != 0U)
    {
        /* Invalidate the current RPM measurement. */
        guardian_rpm_valid = 0U;

        /* Require two new clean edges before another period is published. */
        guardian_has_rpm_capture = 0U;
    }

    /* Clear every TIM3 status flag handled by this adapter. */
    TIM3->SR &=
        ~(GUARDIAN_TIM_SR_CC1IF |
          GUARDIAN_TIM_SR_CC1OF |
          GUARDIAN_TIM_SR_UIF);
}
