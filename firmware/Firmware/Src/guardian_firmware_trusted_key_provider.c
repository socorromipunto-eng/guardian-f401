#include "guardian_firmware_trusted_key_provider.h"

#include <stddef.h>

static const guardian_firmware_trusted_key_record_t g_guardian_firmware_trusted_keys[] = {
    {
        2u,
        GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519,
        GUARDIAN_FIRMWARE_TRUSTED_KEY_STATE_ACTIVE,
        {
            0x33u, 0x0Eu, 0x6Du, 0xB6u, 0xBBu, 0x72u, 0x3Fu, 0xFEu,
            0x2Bu, 0x98u, 0x7Au, 0xD6u, 0x0Eu, 0x07u, 0x73u, 0x97u,
            0xCCu, 0x33u, 0x3Fu, 0x04u, 0x7Bu, 0x4Bu, 0x77u, 0x06u,
            0x68u, 0x9Fu, 0xA2u, 0x49u, 0x69u, 0x26u, 0xCBu, 0xC4u
        }
    }
};

static const size_t g_guardian_firmware_trusted_key_count =
    sizeof(g_guardian_firmware_trusted_keys) /
    sizeof(g_guardian_firmware_trusted_keys[0]);

guardian_firmware_trusted_key_resolution_t
 guardian_firmware_trusted_key_provider_resolve(
    uint32_t key_id,
    guardian_firmware_trusted_key_algorithm_t required_algorithm,
    const guardian_firmware_trusted_key_record_t **out_record)
{
    size_t i;
    size_t match_count = 0u;
    const guardian_firmware_trusted_key_record_t *match = NULL;

    if (out_record == NULL) {
        return GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_INVALID_ARGUMENT;
    }

    *out_record = NULL;

    for (i = 0u; i < g_guardian_firmware_trusted_key_count; ++i) {
        if (g_guardian_firmware_trusted_keys[i].key_id == key_id) {
            match = &g_guardian_firmware_trusted_keys[i];
            ++match_count;
        }
    }

    if (match_count == 0u) {
        return GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_UNKNOWN_KEY;
    }

    if (match_count != 1u) {
        return GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_AMBIGUOUS_KEY;
    }

    if (match->state != GUARDIAN_FIRMWARE_TRUSTED_KEY_STATE_ACTIVE) {
        return GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_NON_ACTIVE_KEY;
    }

    if (match->algorithm != required_algorithm) {
        return GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_ALGORITHM_MISMATCH;
    }

    *out_record = match;
    return GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_OK;
}
