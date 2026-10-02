#ifndef GUARDIAN_FIRMWARE_TRUSTED_KEY_PROVIDER_H
#define GUARDIAN_FIRMWARE_TRUSTED_KEY_PROVIDER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GUARDIAN_FIRMWARE_TRUSTED_KEY_PUBLIC_KEY_BYTES 32u

typedef enum {
    GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_UNKNOWN = 0,
    GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519 = 1
} guardian_firmware_trusted_key_algorithm_t;

typedef enum {
    GUARDIAN_FIRMWARE_TRUSTED_KEY_STATE_UNKNOWN = 0,
    GUARDIAN_FIRMWARE_TRUSTED_KEY_STATE_ACTIVE = 1,
    GUARDIAN_FIRMWARE_TRUSTED_KEY_STATE_RETIRED = 2,
    GUARDIAN_FIRMWARE_TRUSTED_KEY_STATE_REVOKED = 3
} guardian_firmware_trusted_key_state_t;

typedef struct {
    uint32_t key_id;
    guardian_firmware_trusted_key_algorithm_t algorithm;
    guardian_firmware_trusted_key_state_t state;
    uint8_t public_key[GUARDIAN_FIRMWARE_TRUSTED_KEY_PUBLIC_KEY_BYTES];
} guardian_firmware_trusted_key_record_t;

typedef enum {
    GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_OK = 0,
    GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_UNKNOWN_KEY = 1,
    GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_AMBIGUOUS_KEY = 2,
    GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_NON_ACTIVE_KEY = 3,
    GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_ALGORITHM_MISMATCH = 4,
    GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_INVALID_ARGUMENT = 5
} guardian_firmware_trusted_key_resolution_t;

guardian_firmware_trusted_key_resolution_t
 guardian_firmware_trusted_key_provider_resolve(
    uint32_t key_id,
    guardian_firmware_trusted_key_algorithm_t required_algorithm,
    const guardian_firmware_trusted_key_record_t **out_record);

#ifdef __cplusplus
}
#endif

#endif /* GUARDIAN_FIRMWARE_TRUSTED_KEY_PROVIDER_H */
