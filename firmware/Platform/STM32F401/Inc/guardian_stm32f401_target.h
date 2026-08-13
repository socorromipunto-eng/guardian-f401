#ifndef GUARDIAN_STM32F401_TARGET_H
#define GUARDIAN_STM32F401_TARGET_H

/* Require the official ST CMSIS device-family selector used for STM32F401CD. */
#if !defined(STM32F401xE)

/* Stop target compilation before a wrong MCU definition can produce unsafe register assumptions. */
#error "Guardian F401 hardware target requires the STM32F401xE CMSIS device define."

#endif

/* Include fixed-width integer types for hardware limits and pin mappings. */
#include <stdint.h>

/* Define the exact Guardian hardware ordering-code family. */
#define GUARDIAN_STM32F401_TARGET_NAME "STM32F401CDU6"

/* Define the maximum STM32F401 CPU/AHB clock supported by the target. */
#define GUARDIAN_STM32F401_MAX_HCLK_HZ ((uint32_t)84000000UL)

/* Define the maximum APB1 peripheral clock supported by the target. */
#define GUARDIAN_STM32F401_MAX_PCLK1_HZ ((uint32_t)42000000UL)

/* Define the maximum APB2 peripheral clock supported by the target. */
#define GUARDIAN_STM32F401_MAX_PCLK2_HZ ((uint32_t)84000000UL)

/* Define the physical flash capacity of STM32F401CD devices. */
#define GUARDIAN_STM32F401_FLASH_BYTES ((uint32_t)(384UL * 1024UL))

/* Define the total SRAM capacity published for the STM32F401xD target family. */
#define GUARDIAN_STM32F401_SRAM_BYTES ((uint32_t)(96UL * 1024UL))

/* Define the reference USART2 TX GPIO pin number on GPIOA. */
#define GUARDIAN_STM32F401_USART2_TX_PIN ((uint32_t)2U)

/* Define the reference USART2 RX GPIO pin number on GPIOA. */
#define GUARDIAN_STM32F401_USART2_RX_PIN ((uint32_t)3U)

/* Define the AF7 alternate-function selection used by USART2 on PA2/PA3. */
#define GUARDIAN_STM32F401_USART2_AF ((uint32_t)7U)

/* Define the reference vibration ADC pin number on GPIOA. */
#define GUARDIAN_STM32F401_VIBRATION_PIN ((uint32_t)0U)

/* Define the reference current ADC pin number on GPIOA. */
#define GUARDIAN_STM32F401_CURRENT_PIN ((uint32_t)1U)

/* Define the reference supply-divider ADC pin number on GPIOA. */
#define GUARDIAN_STM32F401_SUPPLY_PIN ((uint32_t)4U)

/* Define the reference RPM input-capture pin number on GPIOA. */
#define GUARDIAN_STM32F401_RPM_PIN ((uint32_t)6U)

/* Define the AF2 alternate-function selection used by TIM3_CH1 on PA6. */
#define GUARDIAN_STM32F401_RPM_AF ((uint32_t)2U)

/* Define the factory VREFINT calibration half-word address. */
#define GUARDIAN_STM32F401_VREFINT_CAL_ADDRESS ((uintptr_t)0x1FFF7A2AUL)

/* Define the factory 30 C temperature calibration half-word address. */
#define GUARDIAN_STM32F401_TEMP_CAL_30_ADDRESS ((uintptr_t)0x1FFF7A2CUL)

/* Define the factory 110 C temperature calibration half-word address. */
#define GUARDIAN_STM32F401_TEMP_CAL_110_ADDRESS ((uintptr_t)0x1FFF7A2EUL)

/* Define the maximum accepted UART baud-rate error in parts per million. */
#define GUARDIAN_STM32F401_UART_MAX_ERROR_PPM ((uint32_t)20000UL)

#endif
