#ifndef GUARDIAN_HOST_HAL_COMPILE_CONTRACT_H
#define GUARDIAN_HOST_HAL_COMPILE_CONTRACT_H

/*
 * Declarations for the host-only main translation-unit compile check.
 * This file is not an ST vendor header and provides no HAL implementation.
 * It does not validate initialization, tick timing, linking or target behavior.
 * Production builds must use the controlled ST HAL headers and sources.
 */
#if !defined(GUARDIAN_HOST_HAL_COMPILE_CONTRACT)
#error "HAL declaration stub requires the explicit host compile contract."
#endif

typedef enum
{
    HAL_OK = 0x00U,
    HAL_ERROR = 0x01U,
    HAL_BUSY = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

HAL_StatusTypeDef HAL_Init(void);
void HAL_IncTick(void);

#endif
