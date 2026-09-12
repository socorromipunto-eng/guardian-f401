#ifndef GUARDIAN_NODE_LINK_FRESHNESS_PERSISTENCE_STM32F401_H
#define GUARDIAN_NODE_LINK_FRESHNESS_PERSISTENCE_STM32F401_H

#include "guardian_node_link_freshness_persistence_backend.h"

#include <stdint.h>

/*
 * C5-R3C-E STM32F401 physical-media adapter.
 *
 * This adapter maps the portable dual-slot R3C-E backend onto:
 *
 * SLOT A:
 *   STM32F401 FLASH sector 3
 *   0x0800C000
 *   16 KiB
 *
 * SLOT B:
 *   STM32F401 FLASH sector 4
 *   0x08010000
 *   64 KiB
 *
 * The application linker reservation of these sectors is NOT established by
 * this module and must be demonstrated independently.
 *
 * STM32_HAL_ADAPTER != LINKER_RESERVATION
 * STM32_HAL_ADAPTER != HARDWARE_DURABILITY_EVIDENCE
 * FLASH_STORAGE != AUTHENTICATED_STORAGE
 * DUAL_SLOT_RECOVERY != ROLLBACK_RESISTANCE
 * PERSISTED_STATE != FRESHNESS
 * FRESHNESS != AUTHORITY
 */

#define GUARDIAN_STM32F401_PERSISTENCE_SLOT_A_ADDRESS \
    ((uint32_t)0x0800C000U)

#define GUARDIAN_STM32F401_PERSISTENCE_SLOT_A_SIZE \
    ((uint32_t)0x00004000U)

#define GUARDIAN_STM32F401_PERSISTENCE_SLOT_B_ADDRESS \
    ((uint32_t)0x08010000U)

#define GUARDIAN_STM32F401_PERSISTENCE_SLOT_B_SIZE \
    ((uint32_t)0x00010000U)

/*
 * Caller-selected erase voltage range.
 *
 * No deployment voltage is inferred by Guardian.
 * The value must be one of the STM32 HAL FLASH_VOLTAGE_RANGE_* values.
 */
typedef struct
{
    uint32_t erase_voltage_range;
} guardian_node_link_stm32f401_flash_context_t;

/*
 * Initialize the STM32F401 media adapter.
 *
 * Initialization validates configuration only.
 * It does not unlock, erase, program, restore, or promote persisted state.
 */
guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_stm32f401_flash_media_init(
    guardian_node_link_stm32f401_flash_context_t *context,
    uint32_t erase_voltage_range,
    guardian_node_link_persistence_media_t *media);

#endif