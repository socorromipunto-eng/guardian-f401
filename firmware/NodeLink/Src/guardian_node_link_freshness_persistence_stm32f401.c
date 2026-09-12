#include "guardian_node_link_freshness_persistence_stm32f401.h"

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_flash_ex.h"

#include <stddef.h>
#include <stdint.h>

static void guardian_stm32f401_bytes_zero(
    void *data,
    size_t length)
{
    uint8_t *bytes;
    size_t index;

    if (data == NULL)
    {
        return;
    }

    bytes = (uint8_t *)data;

    for (index = 0U; index < length; index += 1U)
    {
        bytes[index] = 0U;
    }
}

static void guardian_stm32f401_bytes_copy(
    uint8_t *destination,
    const uint8_t *source,
    size_t length)
{
    size_t index;

    if ((destination == NULL) || (source == NULL))
    {
        return;
    }

    for (index = 0U; index < length; index += 1U)
    {
        destination[index] = source[index];
    }
}

static int guardian_stm32f401_slot_geometry(
    guardian_node_link_persistence_slot_t slot,
    uint32_t *base_address,
    size_t *capacity,
    uint32_t *sector)
{
    if ((base_address == NULL) ||
        (capacity == NULL) ||
        (sector == NULL))
    {
        return 0;
    }

    switch (slot)
    {
        case GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A:
            *base_address =
                GUARDIAN_STM32F401_PERSISTENCE_SLOT_A_ADDRESS;

            *capacity =
                (size_t)
                GUARDIAN_STM32F401_PERSISTENCE_SLOT_A_SIZE;

            *sector = FLASH_SECTOR_3;

            return 1;

        case GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_B:
            *base_address =
                GUARDIAN_STM32F401_PERSISTENCE_SLOT_B_ADDRESS;

            *capacity =
                (size_t)
                GUARDIAN_STM32F401_PERSISTENCE_SLOT_B_SIZE;

            *sector = FLASH_SECTOR_4;

            return 1;

        default:
            break;
    }

    return 0;
}

static int guardian_stm32f401_range_valid(
    size_t capacity,
    size_t offset,
    size_t length)
{
    if (offset > capacity)
    {
        return 0;
    }

    if (length > (capacity - offset))
    {
        return 0;
    }

    return 1;
}

static guardian_node_link_persistence_media_result_t
guardian_stm32f401_flash_read(
    void *opaque_context,
    guardian_node_link_persistence_slot_t slot,
    size_t offset,
    uint8_t *output,
    size_t length)
{
    uint32_t base_address;
    uint32_t sector;
    size_t capacity;
    const uint8_t *source;

    (void)opaque_context;

    if ((output == NULL) && (length != 0U))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_INVALID;
    }

    if (!guardian_stm32f401_slot_geometry(
            slot,
            &base_address,
            &capacity,
            &sector))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_INVALID;
    }

    (void)sector;

    if (!guardian_stm32f401_range_valid(
            capacity,
            offset,
            length))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE;
    }

    if (length == 0U)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_OK;
    }

    source =
        (const uint8_t *)(uintptr_t)(
            base_address + (uint32_t)offset);

    guardian_stm32f401_bytes_copy(
        output,
        source,
        length);

    return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_OK;
}

static guardian_node_link_persistence_media_result_t
guardian_stm32f401_flash_erase(
    void *opaque_context,
    guardian_node_link_persistence_slot_t slot)
{
    guardian_node_link_stm32f401_flash_context_t *context;

    FLASH_EraseInitTypeDef erase;
    uint32_t sector_error;

    uint32_t base_address;
    uint32_t sector;
    size_t capacity;

    HAL_StatusTypeDef unlock_status;
    HAL_StatusTypeDef erase_status;
    HAL_StatusTypeDef lock_status;

    if (opaque_context == NULL)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_INVALID;
    }

    context =
        (guardian_node_link_stm32f401_flash_context_t *)
        opaque_context;

    if (!guardian_stm32f401_slot_geometry(
            slot,
            &base_address,
            &capacity,
            &sector))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_INVALID;
    }

    (void)base_address;
    (void)capacity;

    guardian_stm32f401_bytes_zero(
        &erase,
        sizeof(erase));

    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Sector = sector;
    erase.NbSectors = 1U;
    erase.VoltageRange = context->erase_voltage_range;

    sector_error = UINT32_MAX;

    unlock_status = HAL_FLASH_Unlock();

    if (unlock_status != HAL_OK)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE;
    }

    erase_status =
        HAL_FLASHEx_Erase(
            &erase,
            &sector_error);

    lock_status = HAL_FLASH_Lock();

    if ((erase_status != HAL_OK) ||
        (lock_status != HAL_OK))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE;
    }

    if (sector_error != UINT32_MAX)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE;
    }

    return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_OK;
}

static guardian_node_link_persistence_media_result_t
guardian_stm32f401_flash_program(
    void *opaque_context,
    guardian_node_link_persistence_slot_t slot,
    size_t offset,
    const uint8_t *data,
    size_t length)
{
    uint32_t base_address;
    uint32_t sector;
    size_t capacity;

    size_t index;

    HAL_StatusTypeDef unlock_status;
    HAL_StatusTypeDef program_status;
    HAL_StatusTypeDef lock_status;

    (void)opaque_context;

    if ((data == NULL) && (length != 0U))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_INVALID;
    }

    if (!guardian_stm32f401_slot_geometry(
            slot,
            &base_address,
            &capacity,
            &sector))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_INVALID;
    }

    (void)sector;

    if (!guardian_stm32f401_range_valid(
            capacity,
            offset,
            length))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE;
    }

    if (length == 0U)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_OK;
    }

    unlock_status = HAL_FLASH_Unlock();

    if (unlock_status != HAL_OK)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE;
    }

    program_status = HAL_OK;

    for (index = 0U; index < length; index += 1U)
    {
        program_status =
            HAL_FLASH_Program(
                FLASH_TYPEPROGRAM_BYTE,
                base_address +
                    (uint32_t)offset +
                    (uint32_t)index,
                (uint64_t)data[index]);

        if (program_status != HAL_OK)
        {
            break;
        }
    }

    lock_status = HAL_FLASH_Lock();

    if ((program_status != HAL_OK) ||
        (lock_status != HAL_OK))
    {
        /*
         * The physical media may have been partially programmed.
         * Higher layers therefore must not infer atomic completion.
         */
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE;
    }

    return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_OK;
}

guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_stm32f401_flash_media_init(
    guardian_node_link_stm32f401_flash_context_t *context,
    uint32_t erase_voltage_range,
    guardian_node_link_persistence_media_t *media)
{
    if ((context == NULL) || (media == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    if (!IS_VOLTAGERANGE(erase_voltage_range))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    guardian_stm32f401_bytes_zero(
        context,
        sizeof(*context));

    guardian_stm32f401_bytes_zero(
        media,
        sizeof(*media));

    context->erase_voltage_range =
        erase_voltage_range;

    media->context = context;
    media->read =
        guardian_stm32f401_flash_read;
    media->erase =
        guardian_stm32f401_flash_erase;
    media->program =
        guardian_stm32f401_flash_program;

    return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
}