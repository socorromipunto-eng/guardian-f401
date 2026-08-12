/* Include the Guardian STM32F401 USART2 adapter declaration. */
#include "stm32f401_uart2.h"

/* Provide the USART2 vector wrapper for targets that do not already own this IRQ handler. */
void USART2_IRQHandler(void)
{
    /* Forward interrupt service into the Guardian UART adapter. */
    guardian_stm32f401_uart2_irq_handler();
}
