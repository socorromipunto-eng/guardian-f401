#include "guardian_firmware_trusted_signature_adapter.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static unsigned int g_backend_call_count = 0u;

int guardian_firmware_ed25519_backend_verify(
    const uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES],
    const uint8_t public_key[GUARDIAN_FIRMWARE_TRUSTED_KEY_PUBLIC_KEY_BYTES],
    const uint8_t *message,
    size_t message_length)
{
    ++g_backend_call_count;
    if ((signature == NULL) || (public_key == NULL) ||
        (message == NULL) || (message_length == 0u)) {
        return 0;
    }
    return 1;
}

static void reset_backend(void)
{
    g_backend_call_count = 0u;
}

static int verify_key(
    uint32_t key_id,
    uint8_t algorithm,
    const uint8_t *message,
    size_t message_length,
    const uint8_t *signature,
    uint8_t signature_length)
{
    return guardian_firmware_trusted_signature_verify(
        NULL,
        algorithm,
        key_id,
        message,
        message_length,
        signature,
        signature_length);
}

static void test_generic_key2_remains_inventory_visible(void)
{
    const guardian_firmware_trusted_key_record_t *record = NULL;
    guardian_firmware_trusted_key_resolution_t result;

    result = guardian_firmware_trusted_key_provider_resolve(
        2u,
        GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519,
        &record);

    assert(result == GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_OK);
    assert(record != NULL);
    assert(record->provenance_state ==
           GUARDIAN_FIRMWARE_TRUSTED_KEY_PROVENANCE_LEGACY_UNVERIFIED);
}

static void test_key2_rejected_before_backend(void)
{
    const uint8_t message[] = {0x10u, 0x20u, 0x30u};
    uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES] = {0u};

    reset_backend();
    assert(verify_key(2u,
                      GUARDIAN_FIRMWARE_SIGNATURE_ED25519,
                      message,
                      sizeof(message),
                      signature,
                      GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES) == 0);
    assert(g_backend_call_count == 0u);
    (void)printf("ADAPTER_KEY2_LEGACY_UNVERIFIED=REJECTED_BEFORE_BACKEND\n");
}

static void test_unknown_and_key1_rejected_without_fallback(void)
{
    const uint32_t ids[] = {1u, 3u, 0x12345678u, UINT32_MAX};
    const uint8_t message[] = {0x11u};
    uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES] = {0u};
    size_t i;

    reset_backend();
    for (i = 0u; i < (sizeof(ids) / sizeof(ids[0])); ++i) {
        assert(verify_key(ids[i],
                          GUARDIAN_FIRMWARE_SIGNATURE_ED25519,
                          message,
                          sizeof(message),
                          signature,
                          GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES) == 0);
    }
    assert(g_backend_call_count == 0u);
    (void)printf("ADAPTER_NO_FALLBACK=PASS\n");
}

static void test_algorithm_and_input_guards(void)
{
    const uint8_t message[] = {0x12u};
    uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES] = {0u};

    reset_backend();
    assert(verify_key(2u,
                      GUARDIAN_FIRMWARE_SIGNATURE_DEMO_HMAC_SHA256,
                      message,
                      sizeof(message),
                      signature,
                      GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES) == 0);
    assert(verify_key(2u,
                      GUARDIAN_FIRMWARE_SIGNATURE_ED25519,
                      NULL,
                      sizeof(message),
                      signature,
                      GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES) == 0);
    assert(verify_key(2u,
                      GUARDIAN_FIRMWARE_SIGNATURE_ED25519,
                      message,
                      sizeof(message),
                      NULL,
                      GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES) == 0);
    assert(verify_key(2u,
                      GUARDIAN_FIRMWARE_SIGNATURE_ED25519,
                      message,
                      sizeof(message),
                      signature,
                      (uint8_t)(GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES - 1u)) == 0);
    assert(g_backend_call_count == 0u);
    (void)printf("ADAPTER_ALGORITHM_AND_INPUT_GUARDS=PASS\n");
}

int main(void)
{
    test_generic_key2_remains_inventory_visible();
    test_key2_rejected_before_backend();
    test_unknown_and_key1_rejected_without_fallback();
    test_algorithm_and_input_guards();

    (void)printf("Guardian M17 trusted-signature provenance enforcement host tests: PASS\n");
    return 0;
}
