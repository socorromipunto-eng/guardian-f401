#include "guardian_firmware_lifecycle.h"
#include "guardian_firmware_trusted_signature_adapter.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TEST_STORAGE_CAPACITY ((size_t)1024u)

typedef struct {
    uint8_t storage[TEST_STORAGE_CAPACITY];
    uint32_t image_size;
} integration_storage_t;

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
        ((data == NULL) && (length != 0u)) ||
        ((uint64_t)offset + (uint64_t)length > (uint64_t)storage->image_size)) {
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
    if ((storage == NULL) || (digest == NULL) ||
        (image_size != storage->image_size)) {
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
    integration_storage_t *storage)
{
    guardian_firmware_config_t config = {0};

    guardian_firmware_lifecycle_init(lifecycle);
    config.context = storage;
    config.erase = integration_erase;
    config.write = integration_write;
    config.hash = integration_hash;
    config.verify_signature = guardian_firmware_trusted_signature_verify;
    config.required_signature_algorithm = GUARDIAN_FIRMWARE_SIGNATURE_ED25519;
    config.mark_pending = integration_mark_pending;
    config.persist_floor = integration_persist_floor;
    config.complete_pending = integration_complete_pending;
    config.max_image_size = (uint32_t)TEST_STORAGE_CAPACITY;
    config.active_version_counter = 10u;
    config.rollback_floor = 10u;

    assert(guardian_firmware_lifecycle_configure(lifecycle, &config) ==
           GUARDIAN_FIRMWARE_OK);
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
    manifest.version_counter = 11u;
    manifest.firmware_major = 0u;
    manifest.firmware_minor = 17u;
    manifest.firmware_patch = 0u;
    manifest.image_size = image_size;
    guardian_sha256(image, image_size, manifest.image_sha256);
    manifest.signature_length = (uint8_t)GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES;
    for (i = 0u; i < sizeof(manifest.signature); ++i) {
        manifest.signature[i] = (uint8_t)(0xA0u ^ (uint8_t)i);
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
    chunk.offset = 0u;
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
    guardian_firmware_result_t result;

    result = guardian_firmware_begin(lifecycle, manifest);
    if (result != GUARDIAN_FIRMWARE_OK) {
        return result;
    }
    stage_image(lifecycle, image, image_size);
    return guardian_firmware_finalize(lifecycle);
}

static void assert_signature_rejected_without_backend(uint32_t key_id)
{
    guardian_firmware_lifecycle_t lifecycle = {0};
    integration_storage_t storage = {0};
    static const uint8_t image[] = "Guardian M17 provenance enforcement";
    guardian_firmware_manifest_t manifest;

    configure_lifecycle(&lifecycle, &storage);
    manifest = build_manifest(image,
                              (uint32_t)sizeof(image),
                              key_id,
                              GUARDIAN_FIRMWARE_SIGNATURE_ED25519);
    reset_backend();

    assert(begin_stage_finalize(&lifecycle,
                                &manifest,
                                image,
                                (uint32_t)sizeof(image)) ==
           GUARDIAN_FIRMWARE_ERROR_SIGNATURE_INVALID);
    assert(lifecycle.failure == GUARDIAN_FIRMWARE_FAILURE_SIGNATURE_INVALID);
    assert(g_backend_call_count == 0u);
}

static void test_lifecycle_binds_adapter(void)
{
    guardian_firmware_lifecycle_t lifecycle = {0};
    integration_storage_t storage = {0};

    configure_lifecycle(&lifecycle, &storage);
    assert(lifecycle.config.verify_signature == guardian_firmware_trusted_signature_verify);
}

static void test_key2_legacy_unverified_rejected(void)
{
    assert_signature_rejected_without_backend(2u);
    (void)printf("LIFECYCLE_KEY2_LEGACY_UNVERIFIED=REJECTED_BEFORE_BACKEND\n");
}

static void test_unknown_key_rejected(void)
{
    assert_signature_rejected_without_backend(999u);
    (void)printf("LIFECYCLE_UNKNOWN_KEY=REJECTED\n");
}

static void test_no_fallback_key_rejected(void)
{
    assert_signature_rejected_without_backend(0u);
    assert_signature_rejected_without_backend(1u);
    (void)printf("LIFECYCLE_NO_FALLBACK=PASS\n");
}

static void test_algorithm_confusion_rejected_before_transfer(void)
{
    guardian_firmware_lifecycle_t lifecycle = {0};
    integration_storage_t storage = {0};
    static const uint8_t image[] = "Guardian M17 algorithm confusion";
    guardian_firmware_manifest_t manifest;

    configure_lifecycle(&lifecycle, &storage);
    manifest = build_manifest(image,
                              (uint32_t)sizeof(image),
                              2u,
                              GUARDIAN_FIRMWARE_SIGNATURE_DEMO_HMAC_SHA256);
    reset_backend();

    assert(guardian_firmware_begin(&lifecycle, &manifest) ==
           GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD);
    assert(g_backend_call_count == 0u);
    (void)printf("LIFECYCLE_ALGORITHM_CONFUSION=REJECTED\n");
}

int main(void)
{
    test_lifecycle_binds_adapter();
    test_key2_legacy_unverified_rejected();
    test_unknown_key_rejected();
    test_no_fallback_key_rejected();
    test_algorithm_confusion_rejected_before_transfer();

    (void)printf("Guardian M17 lifecycle provenance enforcement host tests: PASS\n");
    return 0;
}
