#include "guardian_firmware_trusted_key_provider.h"

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

static const guardian_firmware_trusted_key_record_t *resolve_key2(void)
{
    const guardian_firmware_trusted_key_record_t *record = NULL;
    guardian_firmware_trusted_key_resolution_t result;

    result = guardian_firmware_trusted_key_provider_resolve(
        2u,
        GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519,
        &record);

    assert(result == GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_OK);
    assert(record != NULL);
    return record;
}

/* PT01: key_id=2 ACTIVE ED25519 resolves an exact 32-byte public key. */
static void test_pt01_key2_resolves(void)
{
    const guardian_firmware_trusted_key_record_t *record = resolve_key2();
    assert(record->key_id == 2u);
    assert(sizeof(record->public_key) == GUARDIAN_FIRMWARE_TRUSTED_KEY_PUBLIC_KEY_BYTES);
}

/* PT02: resolved bytes match the independently provisioned public key. */
static void test_pt02_public_key_bytes_match(void)
{
    const guardian_firmware_trusted_key_record_t *record = resolve_key2();
    assert(memcmp(record->public_key,
                  expected_key2_public_key,
                  GUARDIAN_FIRMWARE_TRUSTED_KEY_PUBLIC_KEY_BYTES) == 0);
}

/* PT03: key_id=2 record reports ACTIVE state. */
static void test_pt03_active_state(void)
{
    const guardian_firmware_trusted_key_record_t *record = resolve_key2();
    assert(record->state == GUARDIAN_FIRMWARE_TRUSTED_KEY_STATE_ACTIVE);
}

/* PT04: key_id=2 record reports ED25519 algorithm. */
static void test_pt04_ed25519_algorithm(void)
{
    const guardian_firmware_trusted_key_record_t *record = resolve_key2();
    assert(record->algorithm == GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519);
}

/* NT01: an unknown key id fails closed and clears the output pointer. */
static void test_nt01_unknown_key_fails_closed(void)
{
    const guardian_firmware_trusted_key_record_t *record =
        (const guardian_firmware_trusted_key_record_t *)(uintptr_t)1u;
    guardian_firmware_trusted_key_resolution_t result;

    result = guardian_firmware_trusted_key_provider_resolve(
        3u,
        GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519,
        &record);

    assert(result == GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_UNKNOWN_KEY);
    assert(record == NULL);
}

/* NT02: compromised/non-promotable key_id=1 never resolves for production use. */
static void test_nt02_key1_does_not_resolve(void)
{
    const guardian_firmware_trusted_key_record_t *record = NULL;
    guardian_firmware_trusted_key_resolution_t result;

    result = guardian_firmware_trusted_key_provider_resolve(
        1u,
        GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519,
        &record);

    assert(result == GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_UNKNOWN_KEY);
    assert(record == NULL);
}

/* NT03: algorithm mismatch fails closed without returning key material. */
static void test_nt03_algorithm_mismatch_fails_closed(void)
{
    const guardian_firmware_trusted_key_record_t *record = NULL;
    guardian_firmware_trusted_key_resolution_t result;

    result = guardian_firmware_trusted_key_provider_resolve(
        2u,
        GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_UNKNOWN,
        &record);

    assert(result == GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_ALGORITHM_MISMATCH);
    assert(record == NULL);
}

/* NT04: NULL output argument is rejected deterministically. */
static void test_nt04_null_output_rejected(void)
{
    guardian_firmware_trusted_key_resolution_t result;

    result = guardian_firmware_trusted_key_provider_resolve(
        2u,
        GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519,
        NULL);

    assert(result == GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_INVALID_ARGUMENT);
}

/* NT05: unknown lookups never fall back to the production key. */
static void test_nt05_no_fallback_key(void)
{
    const guardian_firmware_trusted_key_record_t *record = NULL;
    guardian_firmware_trusted_key_resolution_t result;

    result = guardian_firmware_trusted_key_provider_resolve(
        0x12345678u,
        GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519,
        &record);

    assert(result == GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_UNKNOWN_KEY);
    assert(record == NULL);
}

/* HT01: repeated unknown lookups never disclose key_id=2. */
static void test_ht01_repeated_unknown_lookups(void)
{
    uint32_t i;

    for (i = 0u; i < 64u; ++i) {
        const guardian_firmware_trusted_key_record_t *record = NULL;
        guardian_firmware_trusted_key_resolution_t result;

        result = guardian_firmware_trusted_key_provider_resolve(
            1000u + i,
            GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519,
            &record);

        assert(result == GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_UNKNOWN_KEY);
        assert(record == NULL);
    }
}

/* HT02: key-id boundary values fail closed unless explicitly provisioned. */
static void test_ht02_key_id_boundaries_fail_closed(void)
{
    const uint32_t ids[] = {0u, UINT32_MAX};
    size_t i;

    for (i = 0u; i < (sizeof(ids) / sizeof(ids[0])); ++i) {
        const guardian_firmware_trusted_key_record_t *record = NULL;
        guardian_firmware_trusted_key_resolution_t result;

        result = guardian_firmware_trusted_key_provider_resolve(
            ids[i],
            GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519,
            &record);

        assert(result == GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_UNKNOWN_KEY);
        assert(record == NULL);
    }
}

/* HT03: key_id=1 cannot be confused with key_id=2 public-key bytes. */
static void test_ht03_no_key1_key2_confusion(void)
{
    const guardian_firmware_trusted_key_record_t *key1_record = NULL;
    const guardian_firmware_trusted_key_record_t *key2_record = resolve_key2();
    guardian_firmware_trusted_key_resolution_t key1_result;

    key1_result = guardian_firmware_trusted_key_provider_resolve(
        1u,
        GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519,
        &key1_record);

    assert(key1_result == GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_UNKNOWN_KEY);
    assert(key1_record == NULL);
    assert(memcmp(key2_record->public_key,
                  expected_key2_public_key,
                  GUARDIAN_FIRMWARE_TRUSTED_KEY_PUBLIC_KEY_BYTES) == 0);
}

/* HT04: provider lookup cannot mutate caller input variables passed by value. */
static void test_ht04_caller_inputs_unchanged(void)
{
    uint32_t key_id = 2u;
    guardian_firmware_trusted_key_algorithm_t algorithm =
        GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519;
    const guardian_firmware_trusted_key_record_t *record = NULL;
    guardian_firmware_trusted_key_resolution_t result;

    result = guardian_firmware_trusted_key_provider_resolve(
        key_id,
        algorithm,
        &record);

    assert(result == GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_OK);
    assert(record != NULL);
    assert(key_id == 2u);
    assert(algorithm == GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519);
}

int main(void)
{
    test_pt01_key2_resolves();
    test_pt02_public_key_bytes_match();
    test_pt03_active_state();
    test_pt04_ed25519_algorithm();

    test_nt01_unknown_key_fails_closed();
    test_nt02_key1_does_not_resolve();
    test_nt03_algorithm_mismatch_fails_closed();
    test_nt04_null_output_rejected();
    test_nt05_no_fallback_key();

    test_ht01_repeated_unknown_lookups();
    test_ht02_key_id_boundaries_fail_closed();
    test_ht03_no_key1_key2_confusion();
    test_ht04_caller_inputs_unchanged();

    (void)printf("Guardian B03 trusted-key provider host tests: PASS\n");
    return 0;
}
