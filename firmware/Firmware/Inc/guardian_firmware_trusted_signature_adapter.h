#ifndef GUARDIAN_FIRMWARE_TRUSTED_SIGNATURE_ADAPTER_H
#define GUARDIAN_FIRMWARE_TRUSTED_SIGNATURE_ADAPTER_H

#include "guardian_firmware_lifecycle.h"
#include "guardian_firmware_trusted_key_provider.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES 64u

/*
 * B01 backend contract. The concrete Ed25519 primitive is supplied outside
 * the lifecycle and trusted-key provider. Return 1 only for a valid signature;
 * return 0 for invalid input, invalid signature, or backend failure.
 */
int guardian_firmware_ed25519_backend_verify(
    const uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES],
    const uint8_t public_key[GUARDIAN_FIRMWARE_TRUSTED_KEY_PUBLIC_KEY_BYTES],
    const uint8_t *message,
    size_t message_length);

/*
 * Source-compatible guardian_firmware_verify_signature_fn adapter.
 * The lifecycle context is intentionally not interpreted by this adapter.
 */
int guardian_firmware_trusted_signature_verify(
    void *context,
    uint8_t signature_algorithm,
    uint32_t key_id,
    const uint8_t *signed_message,
    size_t signed_message_length,
    const uint8_t *signature,
    uint8_t signature_length);

#ifdef __cplusplus
}
#endif

#endif /* GUARDIAN_FIRMWARE_TRUSTED_SIGNATURE_ADAPTER_H */
