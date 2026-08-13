#ifndef GUARDIAN_TEST_STM32F4XX_H
#define GUARDIAN_TEST_STM32F4XX_H

/* Include fixed-width integer types used by CMSIS-style peripheral declarations. */
#include <stdint.h>

/* Require the same device-family selector as the real ST device pack. */
#if !defined(STM32F401xE)

/* Fail the compile contract when the target define is missing. */
#error "CMSIS compile contract requires STM32F401xE."

#endif

/* Model CMSIS volatile read/write register fields. */
#define __IO volatile

/* Define a CMSIS-shaped GPIO register block. */
typedef struct
{
    /* Store GPIO mode fields. */
    __IO uint32_t MODER;

    /* Store output type fields. */
    __IO uint32_t OTYPER;

    /* Store output speed fields. */
    __IO uint32_t OSPEEDR;

    /* Store pull-up/pull-down fields. */
    __IO uint32_t PUPDR;

    /* Store input data. */
    __IO uint32_t IDR;

    /* Store output data. */
    __IO uint32_t ODR;

    /* Store atomic bit set/reset data. */
    __IO uint32_t BSRR;

    /* Store GPIO lock state. */
    __IO uint32_t LCKR;

    /* Store alternate-function fields. */
    __IO uint32_t AFR[2];
} GPIO_TypeDef;

/* Define a CMSIS-shaped RCC register block containing fields used by Guardian. */
typedef struct
{
    /* Preserve unused leading clock-control fields. */
    __IO uint32_t CR;

    /* Preserve unused PLL configuration. */
    __IO uint32_t PLLCFGR;

    /* Store system/bus clock configuration. */
    __IO uint32_t CFGR;

    /* Preserve unused interrupt state. */
    __IO uint32_t CIR;

    /* Preserve AHB1 reset state. */
    __IO uint32_t AHB1RSTR;

    /* Preserve AHB2 reset state. */
    __IO uint32_t AHB2RSTR;

    /* Preserve address-space layout. */
    uint32_t RESERVED0[2];

    /* Preserve APB1 reset state. */
    __IO uint32_t APB1RSTR;

    /* Preserve APB2 reset state. */
    __IO uint32_t APB2RSTR;

    /* Preserve address-space layout. */
    uint32_t RESERVED1[2];

    /* Store AHB1 peripheral clock enables. */
    __IO uint32_t AHB1ENR;

    /* Preserve AHB2 peripheral clock enables. */
    __IO uint32_t AHB2ENR;

    /* Preserve address-space layout. */
    uint32_t RESERVED2[2];

    /* Store APB1 peripheral clock enables. */
    __IO uint32_t APB1ENR;

    /* Store APB2 peripheral clock enables. */
    __IO uint32_t APB2ENR;
} RCC_TypeDef;

/* Define a CMSIS-shaped USART register block. */
typedef struct
{
    /* Store USART status. */
    __IO uint32_t SR;

    /* Store USART data. */
    __IO uint32_t DR;

    /* Store USART baud divisor. */
    __IO uint32_t BRR;

    /* Store USART control register one. */
    __IO uint32_t CR1;

    /* Store USART control register two. */
    __IO uint32_t CR2;

    /* Store USART control register three. */
    __IO uint32_t CR3;

    /* Preserve guard-time/prescaler storage. */
    __IO uint32_t GTPR;
} USART_TypeDef;

/* Define a CMSIS-shaped ADC register block. */
typedef struct
{
    /* Store ADC status. */
    __IO uint32_t SR;

    /* Store ADC control register one. */
    __IO uint32_t CR1;

    /* Store ADC control register two. */
    __IO uint32_t CR2;

    /* Store sample-time register one. */
    __IO uint32_t SMPR1;

    /* Store sample-time register two. */
    __IO uint32_t SMPR2;

    /* Preserve injected offsets. */
    __IO uint32_t JOFR1;

    /* Preserve injected offsets. */
    __IO uint32_t JOFR2;

    /* Preserve injected offsets. */
    __IO uint32_t JOFR3;

    /* Preserve injected offsets. */
    __IO uint32_t JOFR4;

    /* Preserve watchdog high threshold. */
    __IO uint32_t HTR;

    /* Preserve watchdog low threshold. */
    __IO uint32_t LTR;

    /* Store regular sequence register one. */
    __IO uint32_t SQR1;

    /* Store regular sequence register two. */
    __IO uint32_t SQR2;

    /* Store regular sequence register three. */
    __IO uint32_t SQR3;

    /* Preserve injected sequence register. */
    __IO uint32_t JSQR;

    /* Preserve injected data registers. */
    __IO uint32_t JDR1;

    /* Preserve injected data registers. */
    __IO uint32_t JDR2;

    /* Preserve injected data registers. */
    __IO uint32_t JDR3;

    /* Preserve injected data registers. */
    __IO uint32_t JDR4;

    /* Store regular conversion data. */
    __IO uint32_t DR;
} ADC_TypeDef;

/* Define the CMSIS-shaped ADC common register block used by Guardian. */
typedef struct
{
    /* Preserve common status. */
    __IO uint32_t CSR;

    /* Store ADC common control. */
    __IO uint32_t CCR;

    /* Preserve common data. */
    __IO uint32_t CDR;
} ADC_Common_TypeDef;

/* Define a CMSIS-shaped DMA controller register block. */
typedef struct
{
    /* Store low interrupt status. */
    __IO uint32_t LISR;

    /* Store high interrupt status. */
    __IO uint32_t HISR;

    /* Store low interrupt flag clear. */
    __IO uint32_t LIFCR;

    /* Store high interrupt flag clear. */
    __IO uint32_t HIFCR;
} DMA_TypeDef;

/* Define a CMSIS-shaped DMA stream register block. */
typedef struct
{
    /* Store stream control. */
    __IO uint32_t CR;

    /* Store transfer count. */
    __IO uint32_t NDTR;

    /* Store peripheral address. */
    __IO uint32_t PAR;

    /* Store first memory address. */
    __IO uint32_t M0AR;

    /* Store second memory address. */
    __IO uint32_t M1AR;

    /* Store FIFO control. */
    __IO uint32_t FCR;
} DMA_Stream_TypeDef;

/* Define a CMSIS-shaped timer register block. */
typedef struct
{
    /* Store timer control register one. */
    __IO uint32_t CR1;

    /* Store timer control register two. */
    __IO uint32_t CR2;

    /* Store slave-mode control. */
    __IO uint32_t SMCR;

    /* Store DMA/interrupt enables. */
    __IO uint32_t DIER;

    /* Store timer status. */
    __IO uint32_t SR;

    /* Store event generation. */
    __IO uint32_t EGR;

    /* Store capture/compare mode register one. */
    __IO uint32_t CCMR1;

    /* Store capture/compare mode register two. */
    __IO uint32_t CCMR2;

    /* Store capture/compare enable. */
    __IO uint32_t CCER;

    /* Store counter. */
    __IO uint32_t CNT;

    /* Store prescaler. */
    __IO uint32_t PSC;

    /* Store auto-reload. */
    __IO uint32_t ARR;

    /* Preserve repetition counter layout. */
    __IO uint32_t RCR;

    /* Store capture/compare register one. */
    __IO uint32_t CCR1;

    /* Preserve remaining capture/compare registers. */
    __IO uint32_t CCR2;

    /* Preserve remaining capture/compare registers. */
    __IO uint32_t CCR3;

    /* Preserve remaining capture/compare registers. */
    __IO uint32_t CCR4;
} TIM_TypeDef;

/* Declare compile-only peripheral singleton objects. */
extern RCC_TypeDef guardian_stub_rcc;

/* Declare GPIOA compile-only storage. */
extern GPIO_TypeDef guardian_stub_gpioa;

/* Declare USART2 compile-only storage. */
extern USART_TypeDef guardian_stub_usart2;

/* Declare ADC1 compile-only storage. */
extern ADC_TypeDef guardian_stub_adc1;

/* Declare ADC common compile-only storage. */
extern ADC_Common_TypeDef guardian_stub_adc_common;

/* Declare DMA2 compile-only storage. */
extern DMA_TypeDef guardian_stub_dma2;

/* Declare DMA2 Stream0 compile-only storage. */
extern DMA_Stream_TypeDef guardian_stub_dma2_stream0;

/* Declare TIM2 compile-only storage. */
extern TIM_TypeDef guardian_stub_tim2;

/* Declare TIM3 compile-only storage. */
extern TIM_TypeDef guardian_stub_tim3;

/* Publish CMSIS-shaped peripheral pointers. */
#define RCC (&guardian_stub_rcc)

/* Publish the GPIOA peripheral pointer. */
#define GPIOA (&guardian_stub_gpioa)

/* Publish the USART2 peripheral pointer. */
#define USART2 (&guardian_stub_usart2)

/* Publish the ADC1 peripheral pointer. */
#define ADC1 (&guardian_stub_adc1)

/* Publish the ADC common peripheral pointer. */
#define ADC (&guardian_stub_adc_common)

/* Publish the DMA2 peripheral pointer. */
#define DMA2 (&guardian_stub_dma2)

/* Publish the DMA2 Stream0 peripheral pointer. */
#define DMA2_Stream0 (&guardian_stub_dma2_stream0)

/* Publish the TIM2 peripheral pointer. */
#define TIM2 (&guardian_stub_tim2)

/* Publish the TIM3 peripheral pointer. */
#define TIM3 (&guardian_stub_tim3)

/* Define the APB1 prescaler field mask used by Guardian. */
#define RCC_CFGR_PPRE1 ((uint32_t)0x00001C00UL)

/* Define the APB1 prescaler field position used by Guardian. */
#define RCC_CFGR_PPRE1_Pos ((uint32_t)10U)

/* Define the APB2 prescaler field mask used by M13 preflight. */
#define RCC_CFGR_PPRE2 ((uint32_t)0x0000E000UL)

/* Define the APB2 prescaler field position used by M13 preflight. */
#define RCC_CFGR_PPRE2_Pos ((uint32_t)13U)

/* Define the GPIOA AHB1 clock-enable bit. */
#define RCC_AHB1ENR_GPIOAEN ((uint32_t)0x00000001UL)

/* Define the USART2 APB1 clock-enable bit. */
#define RCC_APB1ENR_USART2EN ((uint32_t)0x00020000UL)

/* Define USART receiver enable. */
#define USART_CR1_RE ((uint32_t)0x00000004UL)

/* Define USART transmitter enable. */
#define USART_CR1_TE ((uint32_t)0x00000008UL)

/* Define USART RX-not-empty interrupt enable. */
#define USART_CR1_RXNEIE ((uint32_t)0x00000020UL)

/* Define USART TX-empty interrupt enable. */
#define USART_CR1_TXEIE ((uint32_t)0x00000080UL)

/* Define USART peripheral enable. */
#define USART_CR1_UE ((uint32_t)0x00002000UL)

/* Define USART parity error status. */
#define USART_SR_PE ((uint32_t)0x00000001UL)

/* Define USART framing error status. */
#define USART_SR_FE ((uint32_t)0x00000002UL)

/* Define USART noise error status. */
#define USART_SR_NE ((uint32_t)0x00000004UL)

/* Define USART overrun error status. */
#define USART_SR_ORE ((uint32_t)0x00000008UL)

/* Define USART RX-not-empty status. */
#define USART_SR_RXNE ((uint32_t)0x00000020UL)

/* Define USART TX-empty status. */
#define USART_SR_TXE ((uint32_t)0x00000080UL)

/* Define one compile-only USART2 interrupt number. */
#define USART2_IRQn ((int32_t)38)

/* Define one compile-only DMA2 Stream0 interrupt number. */
#define DMA2_Stream0_IRQn ((int32_t)56)

/* Define one compile-only ADC interrupt number. */
#define ADC_IRQn ((int32_t)18)

/* Define one compile-only TIM3 interrupt number. */
#define TIM3_IRQn ((int32_t)29)

/* Declare the CMSIS HCLK variable. */
extern uint32_t SystemCoreClock;

/* Declare the CMSIS system-clock refresh function. */
void SystemCoreClockUpdate(void);

/* Declare the CMSIS Cortex-M SysTick configuration helper. */
uint32_t SysTick_Config(
    uint32_t ticks);

/* Declare the CMSIS NVIC priority function. */
void NVIC_SetPriority(
    int32_t irqn,
    uint32_t priority);

/* Declare the CMSIS NVIC enable function. */
void NVIC_EnableIRQ(
    int32_t irqn);

/* Declare the CMSIS PRIMASK accessor. */
uint32_t __get_PRIMASK(void);

/* Declare the CMSIS global interrupt disable intrinsic. */
void __disable_irq(void);

/* Declare the CMSIS global interrupt enable intrinsic. */
void __enable_irq(void);

/* Declare the CMSIS no-operation intrinsic used by bounded startup delay loops. */
void __NOP(void);

/* Publish one compile-only aligned UID base address. */
#define UID_BASE ((uintptr_t)0x1FFF7A10UL)

#endif
