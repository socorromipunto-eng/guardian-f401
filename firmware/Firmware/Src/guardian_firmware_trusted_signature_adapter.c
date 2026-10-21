#include "guardian_firmware_trusted_signature_adapter.h"

#include <stddef.h>

static int guardian_firmware_provenance_digest_is_zero(
    const uint8_t digest[GUARDIAN_FIRMWARE_TRUSTED_KEY_PROVENANCE_DIGEST_BYTES])
{
    size_t i;
    uint8_t combined = 0u;

    if (digest == NULL) {
        return 1;
    }

    for (i = 0u; i < GUARDIAN_FIRMWARE_TRUSTED_KEY_PROVENANCE_DIGEST_BYTES; ++i) {
        combined = (uint8_t)(combined | digest[i]);
    }

    return combined == 0u ? 1 : 0;
}

int guardian_firmware_trusted_signature_verify(
    void *context,
    uint8_t signature_algorithm,
    uint32_t key_id,
    const uint8_t *signed_message,
    size_t signed_message_length,
    const uint8_t *signature,
    uint8_t signature_length)
{
    const guardian_firmware_trusted_key_record_t *record = NULL;
    guardian_firmware_trusted_key_resolution_t resolution;

    (void)context;

    if ((signed_message == NULL) ||
        (signature == NULL) ||
        (signed_message_length == 0u)) {
        return 0;
    }

    if (signature_algorithm != GUARDIAN_FIRMWARE_SIGNATURE_ED25519) {
        return 0;
    }

    if (signature_length != GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES) {
        return 0;
    }

    resolution = guardian_firmware_trusted_key_provider_resolve_production(
        key_id,
        GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519,
        &record);

    if ((resolution != GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_OK) ||
        (record == NULL)) {
        return 0;
    }

    if ((record->key_id != key_id) ||
        (record->state != GUARDIAN_FIRMWARE_TRUSTED_KEY_STATE_ACTIVE) ||
        (record->algorithm != GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519) ||
        (record->provenance_state !=
         GUARDIAN_FIRMWARE_TRUSTED_KEY_PROVENANCE_PRODUCTION_APPROVED) ||
        (guardian_firmware_provenance_digest_is_zero(record->provenance_digest) != 0)) {
        return 0;
    }

    return guardian_firmware_ed25519_backend_verify(
        signature,
        record->public_key,
        signed_message,
        signed_message_length) == 1 ? 1 : 0;
}
