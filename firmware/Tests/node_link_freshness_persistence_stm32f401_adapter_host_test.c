#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * Minimal HAL surface for host contract testing.
 *
 * The production adapter is included directly below so its static callbacks
 * can be exercised without changing the governed production API.
 */

typedef enum
{
    HAL_OK = 0x00U,
    HAL_ERROR = 0x01U,
    HAL_BUSY = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

#define FLASH_TYPEPROGRAM_BYTE        0x00000000U
#define FLASH_TYPEERASE_SECTORS       0x00000000U
#define FLASH_SECTOR_3                3U
#define FLASH_SECTOR_4                4U

#define FLASH_VOLTAGE_RANGE_1         0x00000000U
#define FLASH_VOLTAGE_RANGE_2         0x00000001U
#define FLASH_VOLTAGE_RANGE_3         0x00000002U
#define FLASH_VOLTAGE_RANGE_4         0x00000003U

#define IS_VOLTAGERANGE(RANGE) \
    (((RANGE) == FLASH_VOLTAGE_RANGE_1) || \
     ((RANGE) == FLASH_VOLTAGE_RANGE_2) || \
     ((RANGE) == FLASH_VOLTAGE_RANGE_3) || \
     ((RANGE) == FLASH_VOLTAGE_RANGE_4))

typedef struct
{
    uint32_t TypeErase;
    uint32_t Banks;
    uint32_t Sector;
    uint32_t NbSectors;
    uint32_t VoltageRange;
} FLASH_EraseInitTypeDef;

static int g_unlock_calls;
static int g_lock_calls;
static int g_erase_calls;
static int g_program_calls;

static HAL_StatusTypeDef g_unlock_result;
static HAL_StatusTypeDef g_lock_result;
static HAL_StatusTypeDef g_erase_result;
static HAL_StatusTypeDef g_program_result;

static uint32_t g_sector_error;

static FLASH_EraseInitTypeDef g_last_erase;

static uint32_t g_last_program_type;
static uint32_t g_last_program_address;
static uint64_t g_last_program_data;

static uint32_t g_program_addresses[1024];
static uint64_t g_program_data[1024];

HAL_StatusTypeDef HAL_FLASH_Unlock(void)
{
    g_unlock_calls += 1;
    return g_unlock_result;
}

HAL_StatusTypeDef HAL_FLASH_Lock(void)
{
    g_lock_calls += 1;
    return g_lock_result;
}

HAL_StatusTypeDef HAL_FLASHEx_Erase(
    FLASH_EraseInitTypeDef *erase,
    uint32_t *sector_error)
{
    g_erase_calls += 1;

    if (erase != NULL)
    {
        g_last_erase = *erase;
    }

    if (sector_error != NULL)
    {
        *sector_error = g_sector_error;
    }

    return g_erase_result;
}

HAL_StatusTypeDef HAL_FLASH_Program(
    uint32_t type_program,
    uint32_t address,
    uint64_t data)
{
    if (g_program_calls < 1024)
    {
        g_program_addresses[g_program_calls] = address;
        g_program_data[g_program_calls] = data;
    }

    g_program_calls += 1;

    g_last_program_type = type_program;
    g_last_program_address = address;
    g_last_program_data = data;

    return g_program_result;
}

/*
 * Prevent production STM32 HAL headers from being pulled into this host test.
 */
#define STM32F4xx_HAL_H
#define STM32F4xx_HAL_FLASH_H
#define STM32F4xx_HAL_FLASH_EX_H

#include "guardian_node_link_freshness_persistence_stm32f401.h"

/*
 * Include the production adapter directly so its internal static media
 * callbacks are contract-tested without changing production visibility.
 */
#include "../NodeLink/Src/guardian_node_link_freshness_persistence_stm32f401.c"

static int td_total;
static int td_pass;

static void td(
    const char *name,
    int condition)
{
    td_total += 1;

    if (condition)
    {
        td_pass += 1;
        (void)printf(
            "TD-%02d %s=PASS\n",
            td_total,
            name);
    }
    else
    {
        (void)printf(
            "TD-%02d %s=FAIL\n",
            td_total,
            name);
    }
}

static void reset_hal(void)
{
    (void)memset(
        &g_last_erase,
        0,
        sizeof(g_last_erase));

    (void)memset(
        g_program_addresses,
        0,
        sizeof(g_program_addresses));

    (void)memset(
        g_program_data,
        0,
        sizeof(g_program_data));

    g_unlock_calls = 0;
    g_lock_calls = 0;
    g_erase_calls = 0;
    g_program_calls = 0;

    g_unlock_result = HAL_OK;
    g_lock_result = HAL_OK;
    g_erase_result = HAL_OK;
    g_program_result = HAL_OK;

    g_sector_error = UINT32_MAX;

    g_last_program_type = UINT32_MAX;
    g_last_program_address = 0U;
    g_last_program_data = 0U;
}

int main(void)
{
    guardian_node_link_stm32f401_flash_context_t context;
    guardian_node_link_persistence_media_t media;

    guardian_node_link_persistence_media_result_t media_result;
    guardian_node_link_freshness_persistence_operation_result_t init_result;

    uint8_t data[4];

    reset_hal();

    init_result =
        guardian_node_link_stm32f401_flash_media_init(
            &context,
            FLASH_VOLTAGE_RANGE_3,
            &media);

    td(
        "init_valid_voltage",
        init_result ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK);

    td(
        "init_context_preserves_voltage",
        context.erase_voltage_range ==
        FLASH_VOLTAGE_RANGE_3);

    td(
        "media_callbacks_non_null",
        (media.read != NULL) &&
        (media.erase != NULL) &&
        (media.program != NULL));

    init_result =
        guardian_node_link_stm32f401_flash_media_init(
            NULL,
            FLASH_VOLTAGE_RANGE_3,
            &media);

    td(
        "init_null_context_rejected",
        init_result ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID);

    init_result =
        guardian_node_link_stm32f401_flash_media_init(
            &context,
            0xFFFFFFFFU,
            &media);

    td(
        "init_invalid_voltage_rejected",
        init_result ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID);

    reset_hal();

    init_result =
        guardian_node_link_stm32f401_flash_media_init(
            &context,
            FLASH_VOLTAGE_RANGE_2,
            &media);

    td(
        "reinit_for_erase",
        init_result ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK);

    media_result =
        media.erase(
            media.context,
            GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A);

    td(
        "erase_slot_a_ok",
        media_result ==
        GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_OK);

    td(
        "erase_slot_a_unlock_once",
        g_unlock_calls == 1);

    td(
        "erase_slot_a_lock_once",
        g_lock_calls == 1);

    td(
        "erase_slot_a_sector_3",
        g_last_erase.Sector == FLASH_SECTOR_3);

    td(
        "erase_slot_a_one_sector",
        g_last_erase.NbSectors == 1U);

    td(
        "erase_slot_a_voltage_forwarded",
        g_last_erase.VoltageRange ==
        FLASH_VOLTAGE_RANGE_2);

    td(
        "erase_type_sector",
        g_last_erase.TypeErase ==
        FLASH_TYPEERASE_SECTORS);

    reset_hal();

    (void)guardian_node_link_stm32f401_flash_media_init(
        &context,
        FLASH_VOLTAGE_RANGE_1,
        &media);

    media_result =
        media.erase(
            media.context,
            GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_B);

    td(
        "erase_slot_b_sector_4",
        (media_result ==
         GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_OK) &&
        (g_last_erase.Sector == FLASH_SECTOR_4));

    reset_hal();

    (void)guardian_node_link_stm32f401_flash_media_init(
        &context,
        FLASH_VOLTAGE_RANGE_3,
        &media);

    g_unlock_result = HAL_ERROR;

    media_result =
        media.erase(
            media.context,
            GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A);

    td(
        "erase_unlock_failure_propagates",
        (media_result ==
         GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE) &&
        (g_erase_calls == 0) &&
        (g_lock_calls == 0));

    reset_hal();

    (void)guardian_node_link_stm32f401_flash_media_init(
        &context,
        FLASH_VOLTAGE_RANGE_3,
        &media);

    g_erase_result = HAL_ERROR;

    media_result =
        media.erase(
            media.context,
            GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A);

    td(
        "erase_hal_failure_locks_and_fails",
        (media_result ==
         GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE) &&
        (g_erase_calls == 1) &&
        (g_lock_calls == 1));

    reset_hal();

    (void)guardian_node_link_stm32f401_flash_media_init(
        &context,
        FLASH_VOLTAGE_RANGE_3,
        &media);

    g_sector_error = FLASH_SECTOR_3;

    media_result =
        media.erase(
            media.context,
            GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A);

    td(
        "erase_sector_error_rejected",
        media_result ==
        GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE);

    reset_hal();

    (void)guardian_node_link_stm32f401_flash_media_init(
        &context,
        FLASH_VOLTAGE_RANGE_3,
        &media);

    data[0] = 0x11U;
    data[1] = 0x22U;
    data[2] = 0x33U;
    data[3] = 0x44U;

    media_result =
        media.program(
            media.context,
            GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A,
            5U,
            data,
            sizeof(data));

    td(
        "program_four_bytes_ok",
        media_result ==
        GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_OK);

    td(
        "program_unlock_once",
        g_unlock_calls == 1);

    td(
        "program_lock_once",
        g_lock_calls == 1);

    td(
        "program_call_count_four",
        g_program_calls == 4);

    td(
        "program_byte_type_used",
        g_last_program_type ==
        FLASH_TYPEPROGRAM_BYTE);

    td(
        "program_address_sequence",
        (g_program_addresses[0] == 0x0800C005U) &&
        (g_program_addresses[1] == 0x0800C006U) &&
        (g_program_addresses[2] == 0x0800C007U) &&
        (g_program_addresses[3] == 0x0800C008U));

    td(
        "program_data_sequence",
        (g_program_data[0] == 0x11U) &&
        (g_program_data[1] == 0x22U) &&
        (g_program_data[2] == 0x33U) &&
        (g_program_data[3] == 0x44U));

    reset_hal();

    (void)guardian_node_link_stm32f401_flash_media_init(
        &context,
        FLASH_VOLTAGE_RANGE_3,
        &media);

    media_result =
        media.program(
            media.context,
            GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A,
            GUARDIAN_STM32F401_PERSISTENCE_SLOT_A_SIZE,
            NULL,
            0U);

    td(
        "zero_length_at_end_allowed",
        (media_result ==
         GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_OK) &&
        (g_unlock_calls == 0) &&
        (g_program_calls == 0));

    data[0] = 0xAAU;

    media_result =
        media.program(
            media.context,
            GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A,
            GUARDIAN_STM32F401_PERSISTENCE_SLOT_A_SIZE,
            data,
            1U);

    td(
        "program_past_slot_a_rejected",
        (media_result ==
         GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE) &&
        (g_unlock_calls == 0));

    media_result =
        media.program(
            media.context,
            (guardian_node_link_persistence_slot_t)99,
            0U,
            data,
            1U);

    td(
        "invalid_slot_rejected",
        media_result ==
        GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_INVALID);

    reset_hal();

    (void)guardian_node_link_stm32f401_flash_media_init(
        &context,
        FLASH_VOLTAGE_RANGE_3,
        &media);

    g_unlock_result = HAL_ERROR;

    media_result =
        media.program(
            media.context,
            GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A,
            0U,
            data,
            1U);

    td(
        "program_unlock_failure_propagates",
        (media_result ==
         GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE) &&
        (g_program_calls == 0) &&
        (g_lock_calls == 0));

    reset_hal();

    (void)guardian_node_link_stm32f401_flash_media_init(
        &context,
        FLASH_VOLTAGE_RANGE_3,
        &media);

    g_program_result = HAL_ERROR;

    media_result =
        media.program(
            media.context,
            GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A,
            0U,
            data,
            4U);

    td(
        "program_failure_stops_immediately",
        (media_result ==
         GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE) &&
        (g_program_calls == 1) &&
        (g_lock_calls == 1));

    reset_hal();

    (void)guardian_node_link_stm32f401_flash_media_init(
        &context,
        FLASH_VOLTAGE_RANGE_3,
        &media);

    g_lock_result = HAL_ERROR;

    media_result =
        media.program(
            media.context,
            GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A,
            0U,
            data,
            1U);

    td(
        "program_lock_failure_fails_closed",
        (media_result ==
         GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE) &&
        (g_program_calls == 1));

    (void)printf("TD_TOTAL=%d\n", td_total);
    (void)printf("TD_PASS_COUNT=%d\n", td_pass);
    (void)printf(
        "TD_FAIL_COUNT=%d\n",
        td_total - td_pass);

    if (td_total != td_pass)
    {
        (void)printf(
            "R3C_E_C3C_HAL_ADAPTER_CONTRACT_TESTS=FAIL\n");

        return 1;
    }

    (void)printf(
        "R3C_E_C3C_HAL_ADAPTER_CONTRACT_TESTS=PASS\n");

    (void)printf(
        "HARDWARE_FLASH_WRITE_VALIDATED=NO\n");

    (void)printf(
        "HARDWARE_POWER_LOSS_VALIDATED=NO\n");

    return 0;
}