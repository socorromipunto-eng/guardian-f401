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

static const guardian_firmware_trusted_key_record_t *resolve_key2_generic(void)
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

static void test_generic_key2_record_is_preserved(void)
{
    const guardian_firmware_trusted_key_record_t *record = resolve_key2_generic();
    assert(record->key_id == 2u);
    assert(record->state == GUARDIAN_FIRMWARE_TRUSTED_KEY_STATE_ACTIVE);
    assert(record->algorithm == GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519);
    assert(memcmp(record->public_key,
                  expected_key2_public_key,
                  GUARDIAN_FIRMWARE_TRUSTED_KEY_PUBLIC_KEY_BYTES) == 0);
}

static void test_key2_is_legacy_unverified(void)
{
    const guardian_firmware_trusted_key_record_t *record = resolve_key2_generic();
    uint8_t zero_digest[GUARDIAN_FIRMWARE_TRUSTED_KEY_PROVENANCE_DIGEST_BYTES] = {0u};

    assert(record->provenance_state ==
           GUARDIAN_FIRMWARE_TRUSTED_KEY_PROVENANCE_LEGACY_UNVERIFIED);
    assert(memcmp(record->provenance_digest,
                  zero_digest,
                  sizeof(zero_digest)) == 0);
}

static void test_production_resolve_rejects_key2(void)
{
    const guardian_firmware_trusted_key_record_t *record =
        (const guardian_firmware_trusted_key_record_t *)(uintptr_t)1u;
    guardian_firmware_trusted_key_resolution_t result;

    result = guardian_firmware_trusted_key_provider_resolve_production(
        2u,
        GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519,
        &record);

    assert(result == GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_UNVERIFIED_PROVENANCE);
    assert(record == NULL);
}

static void test_unknown_key_fails_closed(void)
{
    const guardian_firmware_trusted_key_record_t *record = NULL;
    guardian_firmware_trusted_key_resolution_t result;

    result = guardian_firmware_trusted_key_provider_resolve_production(
        999u,
        GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519,
        &record);

    assert(result == GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_UNKNOWN_KEY);
    assert(record == NULL);
}

static void test_algorithm_mismatch_precedes_provenance(void)
{
    const guardian_firmware_trusted_key_record_t *record = NULL;
    guardian_firmware_trusted_key_resolution_t result;

    result = guardian_firmware_trusted_key_provider_resolve_production(
        2u,
        GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_UNKNOWN,
        &record);

    assert(result == GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_ALGORITHM_MISMATCH);
    assert(record == NULL);
}

static void test_null_output_is_rejected(void)
{
    assert(guardian_firmware_trusted_key_provider_resolve_production(
               2u,
               GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519,
               NULL) == GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_INVALID_ARGUMENT);
}

static void test_no_fallback_for_boundary_ids(void)
{
    const uint32_t ids[] = {0u, 1u, UINT32_MAX};
    size_t i;

    for (i = 0u; i < (sizeof(ids) / sizeof(ids[0])); ++i) {
        const guardian_firmware_trusted_key_record_t *record = NULL;
        guardian_firmware_trusted_key_resolution_t result;

        result = guardian_firmware_trusted_key_provider_resolve_production(
            ids[i],
            GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519,
            &record);

        assert(result == GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_UNKNOWN_KEY);
        assert(record == NULL);
    }
}

int main(void)
{
    test_generic_key2_record_is_preserved();
    test_key2_is_legacy_unverified();
    test_production_resolve_rejects_key2();
    test_unknown_key_fails_closed();
    test_algorithm_mismatch_precedes_provenance();
    test_null_output_is_rejected();
    test_no_fallback_for_boundary_ids();

    (void)printf("Guardian M17 production key provenance gate host tests: PASS\n");
    (void)printf("KEY2_GENERIC_LOOKUP=PRESENT\n");
    (void)printf("KEY2_PROVENANCE=LEGACY_UNVERIFIED\n");
    (void)printf("PRODUCTION_RESOLVE_KEY2=REJECTED_UNVERIFIED_PROVENANCE\n");
    return 0;
}
