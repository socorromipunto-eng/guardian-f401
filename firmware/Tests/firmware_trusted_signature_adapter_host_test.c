#include "guardian_firmware_trusted_signature_adapter.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const uint8_t expected_key2_public_key[GUARDIAN_FIRMWARE_TRUSTED_KEY_PUBLIC_KEY_BYTES] = {
    0x33u, 0x0Eu, 0x6Du, 0xB6u, 0xBBu, 0x72u, 0x3Fu, 0xFEu,
    0x2Bu, 0x98u, 0x7Au, 0xD6u, 0x0Eu, 0x07u, 0x73u, 0x97u,
    0xCCu, 0x33u, 0x3Fu, 0x04u, 0x7Bu, 0x4Bu, 0x77u, 0x06u,
    0x68u, 0x9Fu, 0xA2u, 0x49u, 0x69u, 0x26u, 0xCBu, 0xC4u
};

static unsigned int backend_call_count;
static int backend_available;
static int backend_result;
static const uint8_t *backend_signature_ptr;
static const uint8_t *backend_public_key_ptr;
static const uint8_t *backend_message_ptr;
static size_t backend_message_length;
static uint8_t backend_public_key_copy[GUARDIAN_FIRMWARE_TRUSTED_KEY_PUBLIC_KEY_BYTES];

static void reset_backend(void)
{
    backend_call_count = 0u;
    backend_available = 1;
    backend_result = 1;
    backend_signature_ptr = NULL;
    backend_public_key_ptr = NULL;
    backend_message_ptr = NULL;
    backend_message_length = 0u;
    (void)memset(backend_public_key_copy, 0, sizeof(backend_public_key_copy));
}

int guardian_firmware_ed25519_backend_verify(
    const uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES],
    const uint8_t public_key[GUARDIAN_FIRMWARE_TRUSTED_KEY_PUBLIC_KEY_BYTES],
    const uint8_t *message,
    size_t message_length)
{
    ++backend_call_count;
    backend_signature_ptr = signature;
    backend_public_key_ptr = public_key;
    backend_message_ptr = message;
    backend_message_length = message_length;

    if (public_key != NULL) {
        (void)memcpy(backend_public_key_copy,
                     public_key,
                     GUARDIAN_FIRMWARE_TRUSTED_KEY_PUBLIC_KEY_BYTES);
    }

    if (backend_available == 0) {
        return 0;
    }

    return backend_result == 1 ? 1 : 0;
}

static int call_valid_key2(
    const uint8_t *message,
    size_t message_length,
    const uint8_t *signature)
{
    return guardian_firmware_trusted_signature_verify(
        NULL,
        GUARDIAN_FIRMWARE_SIGNATURE_ED25519,
        2u,
        message,
        message_length,
        signature,
        GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES);
}

/* PT01: ACTIVE key_id=2 ED25519 provider resolution reaches the backend. */
static void test_pt01_active_key2_reaches_backend(void)
{
    const uint8_t message[] = {0x10u, 0x20u, 0x30u};
    uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES] = {0u};

    reset_backend();
    assert(call_valid_key2(message, sizeof(message), signature) == 1);
    assert(backend_call_count == 1u);
}

/* PT02: backend receives the exact 32-byte key_id=2 public key. */
static void test_pt02_backend_receives_exact_key2_public_key(void)
{
    const uint8_t message[] = {0x41u};
    uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES] = {0u};

    reset_backend();
    assert(call_valid_key2(message, sizeof(message), signature) == 1);
    assert(backend_public_key_ptr != NULL);
    assert(memcmp(backend_public_key_copy,
                  expected_key2_public_key,
                  GUARDIAN_FIRMWARE_TRUSTED_KEY_PUBLIC_KEY_BYTES) == 0);
}

/* PT03: backend receives original message/signature pointers and message length. */
static void test_pt03_backend_receives_original_inputs(void)
{
    const uint8_t message[] = {0x01u, 0x02u, 0x03u, 0x04u};
    uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES] = {0u};

    reset_backend();
    assert(call_valid_key2(message, sizeof(message), signature) == 1);
    assert(backend_message_ptr == message);
    assert(backend_signature_ptr == signature);
    assert(backend_message_length == sizeof(message));
}

/* PT04: backend success maps to lifecycle-compatible callback success. */
static void test_pt04_backend_success_maps_to_success(void)
{
    const uint8_t message[] = {0x55u};
    uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES] = {0u};

    reset_backend();
    backend_result = 1;
    assert(call_valid_key2(message, sizeof(message), signature) == 1);
}

/* NT01: unknown key id fails closed without a backend call. */
static void test_nt01_unknown_key_fails_closed(void)
{
    const uint8_t message[] = {0x11u};
    uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES] = {0u};

    reset_backend();
    assert(guardian_firmware_trusted_signature_verify(
               NULL, GUARDIAN_FIRMWARE_SIGNATURE_ED25519, 3u,
               message, sizeof(message), signature,
               GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES) == 0);
    assert(backend_call_count == 0u);
}

/* NT02: key_id=1 fails closed and is never reused for production verification. */
static void test_nt02_key1_fails_closed(void)
{
    const uint8_t message[] = {0x12u};
    uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES] = {0u};

    reset_backend();
    assert(guardian_firmware_trusted_signature_verify(
               NULL, GUARDIAN_FIRMWARE_SIGNATURE_ED25519, 1u,
               message, sizeof(message), signature,
               GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES) == 0);
    assert(backend_call_count == 0u);
}

/* NT03: signature algorithm mismatch fails closed without a backend call. */
static void test_nt03_algorithm_mismatch_fails_closed(void)
{
    const uint8_t message[] = {0x13u};
    uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES] = {0u};

    reset_backend();
    assert(guardian_firmware_trusted_signature_verify(
               NULL, 0xFFu, 2u,
               message, sizeof(message), signature,
               GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES) == 0);
    assert(backend_call_count == 0u);
}

/* NT04: NULL message fails closed. */
static void test_nt04_null_message_fails_closed(void)
{
    uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES] = {0u};

    reset_backend();
    assert(guardian_firmware_trusted_signature_verify(
               NULL, GUARDIAN_FIRMWARE_SIGNATURE_ED25519, 2u,
               NULL, 1u, signature,
               GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES) == 0);
    assert(backend_call_count == 0u);
}

/* NT05: NULL signature fails closed. */
static void test_nt05_null_signature_fails_closed(void)
{
    const uint8_t message[] = {0x15u};

    reset_backend();
    assert(guardian_firmware_trusted_signature_verify(
               NULL, GUARDIAN_FIRMWARE_SIGNATURE_ED25519, 2u,
               message, sizeof(message), NULL,
               GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES) == 0);
    assert(backend_call_count == 0u);
}

/* NT06: invalid signature length fails closed. */
static void test_nt06_invalid_signature_length_fails_closed(void)
{
    const uint8_t message[] = {0x16u};
    uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES] = {0u};

    reset_backend();
    assert(guardian_firmware_trusted_signature_verify(
               NULL, GUARDIAN_FIRMWARE_SIGNATURE_ED25519, 2u,
               message, sizeof(message), signature,
               (uint8_t)(GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES - 1u)) == 0);
    assert(backend_call_count == 0u);
}

/* NT07: controlled unavailable-backend double fails closed. */
static void test_nt07_unavailable_backend_fails_closed(void)
{
    const uint8_t message[] = {0x17u};
    uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES] = {0u};

    reset_backend();
    backend_available = 0;
    assert(call_valid_key2(message, sizeof(message), signature) == 0);
    assert(backend_call_count == 1u);
}

/* NT08: backend verification failure maps to callback failure. */
static void test_nt08_backend_failure_maps_to_failure(void)
{
    const uint8_t message[] = {0x18u};
    uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES] = {0u};

    reset_backend();
    backend_result = 0;
    assert(call_valid_key2(message, sizeof(message), signature) == 0);
    assert(backend_call_count == 1u);
}

/* HT01: provider rejection produces exactly zero backend calls. */
static void test_ht01_provider_rejection_zero_backend_calls(void)
{
    const uint8_t message[] = {0x21u};
    uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES] = {0u};

    reset_backend();
    assert(guardian_firmware_trusted_signature_verify(
               NULL, GUARDIAN_FIRMWARE_SIGNATURE_ED25519, 0x12345678u,
               message, sizeof(message), signature,
               GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES) == 0);
    assert(backend_call_count == 0u);
}

/* HT02: unknown and key_id=1 inputs never fall back to key_id=2. */
static void test_ht02_no_fallback_key(void)
{
    const uint32_t ids[] = {1u, 3u, 0x12345678u};
    const uint8_t message[] = {0x22u};
    uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES] = {0u};
    size_t i;

    reset_backend();
    for (i = 0u; i < (sizeof(ids) / sizeof(ids[0])); ++i) {
        assert(guardian_firmware_trusted_signature_verify(
                   NULL, GUARDIAN_FIRMWARE_SIGNATURE_ED25519, ids[i],
                   message, sizeof(message), signature,
                   GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES) == 0);
    }
    assert(backend_call_count == 0u);
}

/* HT03: repeated unknown keys never leak or return the key_id=2 path. */
static void test_ht03_repeated_unknown_keys_fail_closed(void)
{
    const uint8_t message[] = {0x23u};
    uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES] = {0u};
    uint32_t i;

    reset_backend();
    for (i = 0u; i < 64u; ++i) {
        assert(guardian_firmware_trusted_signature_verify(
                   NULL, GUARDIAN_FIRMWARE_SIGNATURE_ED25519, 1000u + i,
                   message, sizeof(message), signature,
                   GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES) == 0);
    }
    assert(backend_call_count == 0u);
    assert(backend_public_key_ptr == NULL);
}

/* HT04: adapter does not mutate caller message or signature buffers. */
static void test_ht04_input_buffers_not_mutated(void)
{
    uint8_t message[] = {0x31u, 0x32u, 0x33u, 0x34u};
    uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES] = {0u};
    uint8_t message_before[sizeof(message)];
    uint8_t signature_before[sizeof(signature)];
    size_t i;

    for (i = 0u; i < sizeof(signature); ++i) {
        signature[i] = (uint8_t)i;
    }
    (void)memcpy(message_before, message, sizeof(message));
    (void)memcpy(signature_before, signature, sizeof(signature));

    reset_backend();
    assert(call_valid_key2(message, sizeof(message), signature) == 1);
    assert(memcmp(message, message_before, sizeof(message)) == 0);
    assert(memcmp(signature, signature_before, sizeof(signature)) == 0);
}

/* HT05: key-id boundary values zero and UINT32_MAX fail closed. */
static void test_ht05_key_id_boundaries_fail_closed(void)
{
    const uint32_t ids[] = {0u, UINT32_MAX};
    const uint8_t message[] = {0x35u};
    uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES] = {0u};
    size_t i;

    reset_backend();
    for (i = 0u; i < (sizeof(ids) / sizeof(ids[0])); ++i) {
        assert(guardian_firmware_trusted_signature_verify(
                   NULL, GUARDIAN_FIRMWARE_SIGNATURE_ED25519, ids[i],
                   message, sizeof(message), signature,
                   GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES) == 0);
    }
    assert(backend_call_count == 0u);
}

/* HT06: unknown signature-algorithm boundary fails closed. */
static void test_ht06_unknown_algorithm_fails_closed(void)
{
    const uint8_t message[] = {0x36u};
    uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES] = {0u};

    reset_backend();
    assert(guardian_firmware_trusted_signature_verify(
               NULL, 0u, 2u,
               message, sizeof(message), signature,
               GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES) == 0);
    assert(backend_call_count == 0u);
}

int main(void)
{
    test_pt01_active_key2_reaches_backend();
    test_pt02_backend_receives_exact_key2_public_key();
    test_pt03_backend_receives_original_inputs();
    test_pt04_backend_success_maps_to_success();

    test_nt01_unknown_key_fails_closed();
    test_nt02_key1_fails_closed();
    test_nt03_algorithm_mismatch_fails_closed();
    test_nt04_null_message_fails_closed();
    test_nt05_null_signature_fails_closed();
    test_nt06_invalid_signature_length_fails_closed();
    test_nt07_unavailable_backend_fails_closed();
    test_nt08_backend_failure_maps_to_failure();

    test_ht01_provider_rejection_zero_backend_calls();
    test_ht02_no_fallback_key();
    test_ht03_repeated_unknown_keys_fail_closed();
    test_ht04_input_buffers_not_mutated();
    test_ht05_key_id_boundaries_fail_closed();
    test_ht06_unknown_algorithm_fails_closed();

    (void)printf("Guardian B03 trusted-signature adapter host tests: PASS\n");
    return 0;
}
