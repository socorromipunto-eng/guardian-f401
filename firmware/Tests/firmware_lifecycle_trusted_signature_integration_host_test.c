#include "guardian_firmware_lifecycle.h"
#include "guardian_firmware_trusted_signature_adapter.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TEST_STORAGE_CAPACITY ((size_t)1024U)

typedef struct
{
    uint8_t storage[TEST_STORAGE_CAPACITY];
    uint32_t image_size;
} integration_storage_t;

static unsigned int g_backend_call_count = 0U;
static int g_backend_result = 1;
static uint8_t g_last_public_key[GUARDIAN_FIRMWARE_TRUSTED_KEY_PUBLIC_KEY_BYTES];
static uint8_t g_last_signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES];
static size_t g_last_message_length = 0U;

static const uint8_t g_expected_key2_public_key[GUARDIAN_FIRMWARE_TRUSTED_KEY_PUBLIC_KEY_BYTES] = {
    0x33u, 0x0Eu, 0x6Du, 0xB6u, 0xBBu, 0x72u, 0x3Fu, 0xFEu,
    0x2Bu, 0x98u, 0x7Au, 0xD6u, 0x0Eu, 0x07u, 0x73u, 0x97u,
    0xCCu, 0x33u, 0x3Fu, 0x04u, 0x7Bu, 0x4Bu, 0x77u, 0x06u,
    0x68u, 0x9Fu, 0xA2u, 0x49u, 0x69u, 0x26u, 0xCBu, 0xC4u
};

int guardian_firmware_ed25519_backend_verify(
    const uint8_t signature[GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES],
    const uint8_t public_key[GUARDIAN_FIRMWARE_TRUSTED_KEY_PUBLIC_KEY_BYTES],
    const uint8_t *message,
    size_t message_length)
{
    if ((signature == NULL) ||
        (public_key == NULL) ||
        (message == NULL) ||
        (message_length == 0U))
    {
        return 0;
    }

    ++g_backend_call_count;
    (void)memcpy(g_last_public_key, public_key, sizeof(g_last_public_key));
    (void)memcpy(g_last_signature, signature, sizeof(g_last_signature));
    g_last_message_length = message_length;
    return g_backend_result;
}

static void reset_backend(int result)
{
    g_backend_call_count = 0U;
    g_backend_result = result;
    (void)memset(g_last_public_key, 0, sizeof(g_last_public_key));
    (void)memset(g_last_signature, 0, sizeof(g_last_signature));
    g_last_message_length = 0U;
}

static int integration_erase(void *context, uint32_t image_size)
{
    integration_storage_t *storage = (integration_storage_t *)context;
    if ((storage == NULL) || (image_size > TEST_STORAGE_CAPACITY)) {
        return 0;
    }
    (void)memset(storage->storage, 0, sizeof(storage->storage));
    storage->image_size = image_size;
    return 1;
}

static int integration_write(
    void *context,
    uint32_t offset,
    const uint8_t *data,
    uint16_t length)
{
    integration_storage_t *storage = (integration_storage_t *)context;
    if ((storage == NULL) ||
        ((data == NULL) && (length != 0U)) ||
        ((uint64_t)offset + (uint64_t)length > (uint64_t)storage->image_size))
    {
        return 0;
    }
    (void)memcpy(&storage->storage[offset], data, length);
    return 1;
}

static int integration_hash(
    void *context,
    uint32_t image_size,
    uint8_t digest[GUARDIAN_FIRMWARE_DIGEST_SIZE])
{
    integration_storage_t *storage = (integration_storage_t *)context;
    if ((storage == NULL) ||
        (digest == NULL) ||
        (image_size != storage->image_size))
    {
        return 0;
    }
    guardian_sha256(storage->storage, image_size, digest);
    return 1;
}

static int integration_mark_pending(
    void *context,
    uint32_t version_counter,
    uint16_t firmware_major,
    uint16_t firmware_minor,
    uint16_t firmware_patch)
{
    (void)context;
    (void)version_counter;
    (void)firmware_major;
    (void)firmware_minor;
    (void)firmware_patch;
    return 1;
}

static int integration_persist_floor(void *context, uint32_t rollback_floor)
{
    (void)context;
    (void)rollback_floor;
    return 1;
}

static int integration_complete_pending(
    void *context,
    uint32_t version_counter,
    uint8_t confirmed)
{
    (void)context;
    (void)version_counter;
    (void)confirmed;
    return 1;
}

static void configure_lifecycle(
    guardian_firmware_lifecycle_t *lifecycle,
    integration_storage_t *storage,
    uint8_t required_algorithm)
{
    guardian_firmware_config_t config = {0};

    guardian_firmware_lifecycle_init(lifecycle);
    config.context = storage;
    config.erase = integration_erase;
    config.write = integration_write;
    config.hash = integration_hash;
    config.verify_signature = guardian_firmware_trusted_signature_verify;
    config.required_signature_algorithm = required_algorithm;
    config.mark_pending = integration_mark_pending;
    config.persist_floor = integration_persist_floor;
    config.complete_pending = integration_complete_pending;
    config.max_image_size = (uint32_t)TEST_STORAGE_CAPACITY;
    config.active_version_counter = 10U;
    config.rollback_floor = 10U;

    assert(guardian_firmware_lifecycle_configure(lifecycle, &config) == GUARDIAN_FIRMWARE_OK);
}

static guardian_firmware_manifest_t build_manifest(
    const uint8_t *image,
    uint32_t image_size,
    uint32_t key_id,
    uint8_t algorithm)
{
    guardian_firmware_manifest_t manifest = {0};
    size_t i;

    manifest.signature_algorithm = algorithm;
    manifest.key_id = key_id;
    manifest.version_counter = 11U;
    manifest.firmware_major = 0U;
    manifest.firmware_minor = 16U;
    manifest.firmware_patch = 0U;
    manifest.image_size = image_size;
    guardian_sha256(image, image_size, manifest.image_sha256);
    manifest.signature_length = (uint8_t)GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES;

    for (i = 0U; i < sizeof(manifest.signature); ++i) {
        manifest.signature[i] = (uint8_t)(0xA0U ^ (uint8_t)i);
    }

    return manifest;
}

static void stage_image(
    guardian_firmware_lifecycle_t *lifecycle,
    const uint8_t *image,
    uint32_t image_size)
{
    guardian_firmware_chunk_t chunk = {0};
    assert(image_size <= GUARDIAN_FIRMWARE_CHUNK_MAX_DATA);
    chunk.offset = 0U;
    chunk.length = (uint16_t)image_size;
    (void)memcpy(chunk.data, image, image_size);
    assert(guardian_firmware_write_chunk(lifecycle, &chunk) == GUARDIAN_FIRMWARE_OK);
}

static guardian_firmware_result_t begin_stage_finalize(
    guardian_firmware_lifecycle_t *lifecycle,
    const guardian_firmware_manifest_t *manifest,
    const uint8_t *image,
    uint32_t image_size)
{
    guardian_firmware_result_t begin_result = guardian_firmware_begin(lifecycle, manifest);
    if (begin_result != GUARDIAN_FIRMWARE_OK) {
        return begin_result;
    }
    stage_image(lifecycle, image, image_size);
    return guardian_firmware_finalize(lifecycle);
}

/* IT01 */
static void test_it01_lifecycle_binds_adapter(void)
{
    guardian_firmware_lifecycle_t lifecycle = {0};
    integration_storage_t storage = {0};
    configure_lifecycle(&lifecycle, &storage, GUARDIAN_FIRMWARE_SIGNATURE_ED25519);
    assert(lifecycle.config.verify_signature == guardian_firmware_trusted_signature_verify);
}

/* IT02 */
static void test_it02_valid_key2_reaches_provider_and_backend(void)
{
    guardian_firmware_lifecycle_t lifecycle = {0};
    integration_storage_t storage = {0};
    static const uint8_t image[] = "Guardian B03 E2E valid key2";
    guardian_firmware_manifest_t manifest;

    configure_lifecycle(&lifecycle, &storage, GUARDIAN_FIRMWARE_SIGNATURE_ED25519);
    manifest = build_manifest(image, (uint32_t)sizeof(image), 2U, GUARDIAN_FIRMWARE_SIGNATURE_ED25519);
    reset_backend(1);

    assert(begin_stage_finalize(&lifecycle, &manifest, image, (uint32_t)sizeof(image)) == GUARDIAN_FIRMWARE_OK);
    assert(lifecycle.state == GUARDIAN_FIRMWARE_STATE_VERIFIED);
    assert(g_backend_call_count == 1U);
    assert(g_last_message_length == GUARDIAN_FIRMWARE_SIGNED_MANIFEST_SIZE);
    assert(memcmp(g_last_public_key, g_expected_key2_public_key, sizeof(g_last_public_key)) == 0);
    assert(memcmp(g_last_signature, manifest.signature, sizeof(g_last_signature)) == 0);
}

/* IT03 */
static void test_it03_unknown_key_fails_closed(void)
{
    guardian_firmware_lifecycle_t lifecycle = {0};
    integration_storage_t storage = {0};
    static const uint8_t image[] = "Guardian B03 E2E unknown key";
    guardian_firmware_manifest_t manifest;

    configure_lifecycle(&lifecycle, &storage, GUARDIAN_FIRMWARE_SIGNATURE_ED25519);
    manifest = build_manifest(image, (uint32_t)sizeof(image), 999U, GUARDIAN_FIRMWARE_SIGNATURE_ED25519);
    reset_backend(1);

    assert(begin_stage_finalize(&lifecycle, &manifest, image, (uint32_t)sizeof(image)) == GUARDIAN_FIRMWARE_ERROR_SIGNATURE_INVALID);
    assert(g_backend_call_count == 0U);
}

/* IT04 */
static void test_it04_key1_fails_closed_no_production_reuse(void)
{
    guardian_firmware_lifecycle_t lifecycle = {0};
    integration_storage_t storage = {0};
    static const uint8_t image[] = "Guardian B03 E2E key1 rejected";
    guardian_firmware_manifest_t manifest;

    configure_lifecycle(&lifecycle, &storage, GUARDIAN_FIRMWARE_SIGNATURE_ED25519);
    manifest = build_manifest(image, (uint32_t)sizeof(image), 1U, GUARDIAN_FIRMWARE_SIGNATURE_ED25519);
    reset_backend(1);

    assert(begin_stage_finalize(&lifecycle, &manifest, image, (uint32_t)sizeof(image)) == GUARDIAN_FIRMWARE_ERROR_SIGNATURE_INVALID);
    assert(g_backend_call_count == 0U);
}

/* IT05 */
static void test_it05_algorithm_mismatch_fails_closed(void)
{
    guardian_firmware_lifecycle_t lifecycle = {0};
    integration_storage_t storage = {0};
    static const uint8_t image[] = "Guardian B03 E2E algorithm mismatch";
    guardian_firmware_manifest_t manifest;

    configure_lifecycle(&lifecycle, &storage, GUARDIAN_FIRMWARE_SIGNATURE_ED25519);
    manifest = build_manifest(image, (uint32_t)sizeof(image), 2U, GUARDIAN_FIRMWARE_SIGNATURE_DEMO_HMAC_SHA256);
    reset_backend(1);

    assert(guardian_firmware_begin(&lifecycle, &manifest) == GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD);
    assert(g_backend_call_count == 0U);
}

/* IT06 */
static void test_it06_backend_failure_propagates(void)
{
    guardian_firmware_lifecycle_t lifecycle = {0};
    integration_storage_t storage = {0};
    static const uint8_t image[] = "Guardian B03 E2E backend failure";
    guardian_firmware_manifest_t manifest;

    configure_lifecycle(&lifecycle, &storage, GUARDIAN_FIRMWARE_SIGNATURE_ED25519);
    manifest = build_manifest(image, (uint32_t)sizeof(image), 2U, GUARDIAN_FIRMWARE_SIGNATURE_ED25519);
    reset_backend(0);

    assert(begin_stage_finalize(&lifecycle, &manifest, image, (uint32_t)sizeof(image)) == GUARDIAN_FIRMWARE_ERROR_SIGNATURE_INVALID);
    assert(g_backend_call_count == 1U);
    assert(lifecycle.failure == GUARDIAN_FIRMWARE_FAILURE_SIGNATURE_INVALID);
}

/* IT07 */
static void test_it07_no_fallback_key_path(void)
{
    guardian_firmware_lifecycle_t lifecycle = {0};
    integration_storage_t storage = {0};
    static const uint8_t image[] = "Guardian B03 E2E no fallback";
    guardian_firmware_manifest_t manifest;

    configure_lifecycle(&lifecycle, &storage, GUARDIAN_FIRMWARE_SIGNATURE_ED25519);
    manifest = build_manifest(image, (uint32_t)sizeof(image), 0U, GUARDIAN_FIRMWARE_SIGNATURE_ED25519);
    reset_backend(1);

    assert(begin_stage_finalize(&lifecycle, &manifest, image, (uint32_t)sizeof(image)) == GUARDIAN_FIRMWARE_ERROR_SIGNATURE_INVALID);
    assert(g_backend_call_count == 0U);
}

/* IT08 */
static void test_it08_lifecycle_does_not_bypass_adapter_or_provider(void)
{
    guardian_firmware_lifecycle_t lifecycle = {0};
    integration_storage_t storage = {0};
    static const uint8_t image[] = "Guardian B03 E2E no bypass";
    guardian_firmware_manifest_t manifest;

    configure_lifecycle(&lifecycle, &storage, GUARDIAN_FIRMWARE_SIGNATURE_ED25519);
    manifest = build_manifest(image, (uint32_t)sizeof(image), 2U, GUARDIAN_FIRMWARE_SIGNATURE_ED25519);
    reset_backend(1);

    assert(lifecycle.config.verify_signature == guardian_firmware_trusted_signature_verify);
    assert(begin_stage_finalize(&lifecycle, &manifest, image, (uint32_t)sizeof(image)) == GUARDIAN_FIRMWARE_OK);
    assert(g_backend_call_count == 1U);
    assert(memcmp(g_last_public_key, g_expected_key2_public_key, sizeof(g_last_public_key)) == 0);
}

int main(void)
{
    test_it01_lifecycle_binds_adapter();
    test_it02_valid_key2_reaches_provider_and_backend();
    test_it03_unknown_key_fails_closed();
    test_it04_key1_fails_closed_no_production_reuse();
    test_it05_algorithm_mismatch_fails_closed();
    test_it06_backend_failure_propagates();
    test_it07_no_fallback_key_path();
    test_it08_lifecycle_does_not_bypass_adapter_or_provider();

    (void)printf("Guardian B03 lifecycle end-to-end integration host tests: PASS\n");
    return 0;
}
