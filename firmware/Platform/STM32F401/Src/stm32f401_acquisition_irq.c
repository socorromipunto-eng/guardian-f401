/* Include the Guardian STM32F401 acquisition interrupt adapter declarations. */
#include "stm32f401_acquisition.h"

/* Provide the DMA2 Stream0 vector wrapper for standalone Guardian targets. */
void DMA2_Stream0_IRQHandler(void)
{
    /* Forward DMA completion and fault service into the Guardian acquisition adapter. */
    guardian_stm32f401_acquisition_dma_irq_handler();
}

/* Provide the ADC vector wrapper for standalone Guardian targets. */
void ADC_IRQHandler(void)
{
    /* Forward ADC overrun service into the Guardian acquisition adapter. */
    guardian_stm32f401_acquisition_adc_irq_handler();
}

/* Provide the TIM3 vector wrapper for standalone Guardian targets. */
void TIM3_IRQHandler(void)
{
    /* Forward RPM input-capture service into the Guardian acquisition adapter. */
    guardian_stm32f401_acquisition_tim3_irq_handler();
}
