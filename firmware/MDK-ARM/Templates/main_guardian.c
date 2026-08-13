/* Include the complete Guardian firmware integration API. */
#include "guardian_firmware_app.h"

/* Include official ST CMSIS core/device declarations from the Keil Device Pack. */
#include "stm32f4xx.h"

/* Define the physical Guardian UART baud used by the reference hardware target. */
#define GUARDIAN_TARGET_UART_BAUD ((uint32_t)115200UL)

/* Define a conservative USART2 interrupt priority below highest-priority safety work. */
#define GUARDIAN_TARGET_UART_IRQ_PRIORITY ((uint32_t)5U)

/* Enter one debugger-visible fail-stop state after startup qualification failure. */
static void guardian_target_fail_stop(void)
{
    /* Prevent partially initialized interrupt-driven hardware from continuing. */
    __disable_irq();

    /* Remain stopped so a debugger can inspect guardian_firmware_app_preflight(). */
    for (;;)
    {
        /* Keep the loop explicit without generating useful peripheral activity. */
        __NOP();
    }
}

/* Forward the existing one-millisecond Cortex-M SysTick into Guardian timekeeping. */
void SysTick_Handler(void)
{
    /* Advance Guardian uptime, RPM freshness and telemetry scheduling. */
    guardian_firmware_app_tick_1ms();
}

/* Start the minimal Guardian STM32F401CDU6 hardware-validation application. */
int main(void)
{
    /* Refresh the CMSIS clock variable after startup clock configuration. */
    SystemCoreClockUpdate();

    /* Reject an unavailable clock before configuring a one-millisecond SysTick. */
    if (SystemCoreClock < 1000U)
    {
        /* Preserve the failed target state for debugger inspection. */
        guardian_target_fail_stop();
    }

    /* Configure one-millisecond Cortex-M SysTick using the current HCLK value. */
    if (SysTick_Config(
            SystemCoreClock / 1000U) != 0U)
    {
        /* Preserve invalid SysTick configuration for debugger inspection. */
        guardian_target_fail_stop();
    }

    /* Initialize preflight, USART2, Guardian middleware and acquisition hardware. */
    if (guardian_firmware_app_init(
            GUARDIAN_TARGET_UART_BAUD,
            GUARDIAN_TARGET_UART_IRQ_PRIORITY) == 0)
    {
        /* Preserve the M13 preflight/startup failure for debugger inspection. */
        guardian_target_fail_stop();
    }

    /* Run all protocol, acquisition, DSP and telemetry work in foreground context. */
    for (;;)
    {
        /* Execute one bounded Guardian foreground iteration. */
        guardian_firmware_app_poll();
    }
}
