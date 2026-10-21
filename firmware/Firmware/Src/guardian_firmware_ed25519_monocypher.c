#include "guardian_firmware_trusted_signature_adapter.h"
#include "monocypher-ed25519.h"

#include <stddef.h>
#include <stdint.h>

static const uint8_t g_guardian_ed25519_empty_message_sentinel = 0u;

int guardian_firmware_ed25519_backend_verify(
    const uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES],
    const uint8_t public_key[GUARDIAN_FIRMWARE_TRUSTED_KEY_PUBLIC_KEY_BYTES],
    const uint8_t *message,
    size_t message_length)
{
    const uint8_t *safe_message = message;

    if ((signature == NULL) || (public_key == NULL)) {
        return 0;
    }

    if ((message == NULL) && (message_length != 0u)) {
        return 0;
    }

    if (safe_message == NULL) {
        safe_message = &g_guardian_ed25519_empty_message_sentinel;
    }

    return (crypto_ed25519_check(signature, public_key, safe_message, message_length) == 0) ? 1 : 0;
}
