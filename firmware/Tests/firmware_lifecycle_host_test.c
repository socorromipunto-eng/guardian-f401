/* Include the portable M12 lifecycle under test. */
#include "guardian_firmware_lifecycle.h"

/* Include assertion support for deterministic host verification. */
#include <assert.h>

/* Include standard output for one concise CI success line. */
#include <stdio.h>

/* Include memory helpers for fake staging storage. */
#include <string.h>

/* Define bounded fake staging capacity. */
#define TEST_STORAGE_CAPACITY ((size_t)4096U)

/* Store fake staging and persistence state. */
typedef struct
{
    /* Store candidate image bytes. */
    uint8_t storage[TEST_STORAGE_CAPACITY];

    /* Store the currently allocated candidate image size. */
    uint32_t image_size;

    /* Store one simulator-only HMAC verification key. */
    uint8_t signing_key[32];

    /* Store pending monotonic version metadata. */
    uint32_t pending_version_counter;

    /* Store persisted rollback floor metadata. */
    uint32_t persisted_floor;

    /* Store the last pending candidate whose boot outcome was finalized. */
    uint32_t completed_version_counter;

    /* Store whether the finalized pending candidate was confirmed. */
    uint8_t completed_confirmed;
} test_backend_t;

/* Erase fake candidate staging storage. */
static int test_erase(
    void *context,
    uint32_t image_size)
{
    /* Recover fake backend state. */
    test_backend_t *backend =
        (test_backend_t *)context;

    /* Reject missing state or oversized candidate allocation. */
    if ((backend == NULL) ||
        (image_size >
         TEST_STORAGE_CAPACITY))
    {
        /* Report staging failure. */
        return 0;
    }

    /* Clear complete fake storage. */
    (void)memset(
        backend->storage,
        0,
        sizeof(backend->storage));

    /* Publish exact allocated image size. */
    backend->image_size =
        image_size;

    /* Report successful erase. */
    return 1;
}

/* Write one exact sequential fake storage chunk. */
static int test_write(
    void *context,
    uint32_t offset,
    const uint8_t *data,
    uint16_t length)
{
    /* Recover fake backend state. */
    test_backend_t *backend =
        (test_backend_t *)context;

    /* Reject missing state or invalid source data. */
    if ((backend == NULL) ||
        ((data == NULL) &&
         (length != 0U)))
    {
        /* Report storage failure. */
        return 0;
    }

    /* Reject writes beyond allocated candidate bytes. */
    if ((uint64_t)offset +
        (uint64_t)length >
        (uint64_t)backend->image_size)
    {
        /* Report storage failure. */
        return 0;
    }

    /* Copy exact candidate bytes. */
    (void)memcpy(
        &backend->storage[offset],
        data,
        length);

    /* Report successful write. */
    return 1;
}

/* Calculate SHA-256 across exact fake staged bytes. */
static int test_hash(
    void *context,
    uint32_t image_size,
    uint8_t digest[GUARDIAN_FIRMWARE_DIGEST_SIZE])
{
    /* Recover fake backend state. */
    test_backend_t *backend =
        (test_backend_t *)context;

    /* Reject missing state or inconsistent image size. */
    if ((backend == NULL) ||
        (digest == NULL) ||
        (image_size !=
         backend->image_size))
    {
        /* Report backend failure. */
        return 0;
    }

    /* Calculate exact staged image SHA-256. */
    guardian_sha256(
        backend->storage,
        image_size,
        digest);

    /* Report successful hash calculation. */
    return 1;
}

/* Verify the simulator/test-only HMAC signature over canonical manifest bytes. */
static int test_verify_signature(
    void *context,
    uint8_t signature_algorithm,
    uint32_t key_id,
    const uint8_t *signed_message,
    size_t signed_message_length,
    const uint8_t *signature,
    uint8_t signature_length)
{
    /* Recover fake backend state. */
    test_backend_t *backend =
        (test_backend_t *)context;

    /* Store the complete expected HMAC. */
    uint8_t expected[GUARDIAN_SHA256_SIZE] = {0};

    /* Require the explicitly test-only algorithm and key slot. */
    if ((backend == NULL) ||
        (signature_algorithm !=
         GUARDIAN_FIRMWARE_SIGNATURE_DEMO_HMAC_SHA256) ||
        (key_id != 1U) ||
        (signed_message == NULL) ||
        (signed_message_length !=
         GUARDIAN_FIRMWARE_SIGNED_MANIFEST_SIZE) ||
        (signature == NULL) ||
        (signature_length !=
         GUARDIAN_SHA256_SIZE))
    {
        /* Reject unsupported verification semantics. */
        return 0;
    }

    /* Calculate expected HMAC-SHA-256. */
    guardian_hmac_sha256(
        backend->signing_key,
        sizeof(backend->signing_key),
        signed_message,
        signed_message_length,
        expected);

    /* Compare signatures without data-dependent early exit. */
    int valid =
        guardian_crypto_constant_time_equal(
            expected,
            signature,
            GUARDIAN_SHA256_SIZE);

    /* Erase temporary expected signature. */
    guardian_crypto_zero(
        expected,
        sizeof(expected));

    /* Return verification result. */
    return valid;
}

/* Persist fake pending activation metadata. */
static int test_mark_pending(
    void *context,
    uint32_t version_counter,
    uint16_t firmware_major,
    uint16_t firmware_minor,
    uint16_t firmware_patch)
{
    /* Recover fake backend state. */
    test_backend_t *backend =
        (test_backend_t *)context;

    /* Preserve semantic fields as intentionally unused test inputs. */
    (void)firmware_major;
    (void)firmware_minor;
    (void)firmware_patch;

    /* Reject missing backend state. */
    if (backend == NULL)
    {
        /* Report persistence failure. */
        return 0;
    }

    /* Publish pending monotonic version. */
    backend->pending_version_counter =
        version_counter;

    /* Report successful persistence. */
    return 1;
}

/* Persist fake rollback floor metadata. */
static int test_persist_floor(
    void *context,
    uint32_t rollback_floor)
{
    /* Recover fake backend state. */
    test_backend_t *backend =
        (test_backend_t *)context;

    /* Reject missing backend state. */
    if (backend == NULL)
    {
        /* Report persistence failure. */
        return 0;
    }

    /* Publish persisted floor. */
    backend->persisted_floor =
        rollback_floor;

    /* Report successful persistence. */
    return 1;
}

/* Persist completion or clearing of one pending-image boot outcome. */
static int test_complete_pending(
    void *context,
    uint32_t version_counter,
    uint8_t confirmed)
{
    /* Recover fake backend state. */
    test_backend_t *backend =
        (test_backend_t *)context;

    /* Reject missing backend state or non-canonical boolean values. */
    if ((backend == NULL) ||
        (confirmed > 1U))
    {
        /* Report persistence failure. */
        return 0;
    }

    /* Publish the completed pending version. */
    backend->completed_version_counter =
        version_counter;

    /* Publish the confirmed/rollback outcome. */
    backend->completed_confirmed =
        confirmed;

    /* Clear the persistent pending marker. */
    backend->pending_version_counter =
        0U;

    /* Report successful boot-outcome persistence. */
    return 1;
}

/* Build one signed candidate manifest over exact test image bytes. */
static guardian_firmware_manifest_t test_manifest(
    test_backend_t *backend,
    const uint8_t *image,
    uint32_t image_size,
    uint32_t version_counter)
{
    /* Create deterministic candidate metadata. */
    guardian_firmware_manifest_t manifest = {0};

    /* Store canonical signing transcript. */
    uint8_t signed_message[
        GUARDIAN_FIRMWARE_SIGNED_MANIFEST_SIZE] = {0};

    /* Publish simulator/test-only signature algorithm. */
    manifest.signature_algorithm =
        GUARDIAN_FIRMWARE_SIGNATURE_DEMO_HMAC_SHA256;

    /* Select trusted test key slot one. */
    manifest.key_id =
        1U;

    /* Publish monotonic candidate version. */
    manifest.version_counter =
        version_counter;

    /* Publish semantic version 0.12.0. */
    manifest.firmware_major =
        0U;
    manifest.firmware_minor =
        12U;
    manifest.firmware_patch =
        0U;

    /* Publish exact candidate image size. */
    manifest.image_size =
        image_size;

    /* Calculate signed image digest. */
    guardian_sha256(
        image,
        image_size,
        manifest.image_sha256);

    /* Publish full HMAC signature width. */
    manifest.signature_length =
        32U;

    /* Build the exact canonical signature transcript. */
    assert(
        guardian_firmware_signed_manifest(
            &manifest,
            signed_message) ==
        GUARDIAN_FIRMWARE_OK);

    /* Calculate the simulator/test-only manifest signature. */
    guardian_hmac_sha256(
        backend->signing_key,
        sizeof(backend->signing_key),
        signed_message,
        sizeof(signed_message),
        manifest.signature);

    /* Erase temporary canonical transcript. */
    guardian_crypto_zero(
        signed_message,
        sizeof(signed_message));

    /* Return complete signed manifest by value. */
    return manifest;
}

/* Configure one fresh lifecycle and fake backend. */
static void test_configure(
    guardian_firmware_lifecycle_t *lifecycle,
    test_backend_t *backend,
    uint32_t active_version)
{
    /* Create one complete platform configuration. */
    guardian_firmware_config_t config = {0};

    /* Initialize lifecycle state. */
    guardian_firmware_lifecycle_init(
        lifecycle);

    /* Fill deterministic test signing key bytes. */
    size_t index = 0U;

    /* Publish all thirty-two test key bytes. */
    for (index = 0U;
         index < sizeof(backend->signing_key);
         ++index)
    {
        /* Store one deterministic non-secret test byte. */
        backend->signing_key[index] =
            (uint8_t)(
                index ^
                0xA5U);
    }

    /* Connect fake backend context. */
    config.context =
        backend;

    /* Connect fake staging erase. */
    config.erase =
        test_erase;

    /* Connect fake staging write. */
    config.write =
        test_write;

    /* Connect fake staged-image hash. */
    config.hash =
        test_hash;

    /* Connect fake test signature verifier. */
    config.verify_signature =
        test_verify_signature;

    /* Connect fake pending metadata persistence. */
    config.mark_pending =
        test_mark_pending;

    /* Connect fake rollback-floor persistence. */
    config.persist_floor =
        test_persist_floor;

    /* Connect fake persistent pending-image completion. */
    config.complete_pending =
        test_complete_pending;

    /* Publish bounded candidate capacity. */
    config.max_image_size =
        TEST_STORAGE_CAPACITY;

    /* Publish confirmed active version. */
    config.active_version_counter =
        active_version;

    /* Start rollback floor at confirmed active version. */
    config.rollback_floor =
        active_version;

    /* Install lifecycle configuration. */
    assert(
        guardian_firmware_lifecycle_configure(
            lifecycle,
            &config) ==
        GUARDIAN_FIRMWARE_OK);
}

/* Stage one image through bounded sequential chunks. */
static void test_stage_image(
    guardian_firmware_lifecycle_t *lifecycle,
    const uint8_t *image,
    uint32_t image_size)
{
    /* Start at image offset zero. */
    uint32_t offset =
        0U;

    /* Stage every image byte sequentially. */
    while (offset <
           image_size)
    {
        /* Create one bounded chunk. */
        guardian_firmware_chunk_t chunk = {0};

        /* Publish exact next offset. */
        chunk.offset =
            offset;

        /* Calculate remaining image bytes. */
        uint32_t remaining =
            image_size -
            offset;

        /* Select the shared M12 chunk bound. */
        chunk.length =
            (uint16_t)(
                (remaining >
                 GUARDIAN_FIRMWARE_CHUNK_MAX_DATA)
                ? GUARDIAN_FIRMWARE_CHUNK_MAX_DATA
                : remaining);

        /* Copy exact test image bytes. */
        (void)memcpy(
            chunk.data,
            &image[offset],
            chunk.length);

        /* Stage the chunk successfully. */
        assert(
            guardian_firmware_write_chunk(
                lifecycle,
                &chunk) ==
            GUARDIAN_FIRMWARE_OK);

        /* Advance by exact staged bytes. */
        offset +=
            (uint32_t)chunk.length;
    }
}

/* Verify signed image confirmation advances monotonic rollback floor. */
static void test_confirmed_candidate(void)
{
    /* Create lifecycle and backend storage. */
    guardian_firmware_lifecycle_t lifecycle = {0};
    test_backend_t backend = {0};

    /* Build one deterministic candidate image. */
    uint8_t image[513] = {0};

    /* Fill deterministic image bytes. */
    size_t index = 0U;

    /* Publish every test image byte. */
    for (index = 0U;
         index < sizeof(image);
         ++index)
    {
        /* Store one repeatable image byte. */
        image[index] =
            (uint8_t)index;
    }

    /* Configure current version eleven. */
    test_configure(
        &lifecycle,
        &backend,
        11U);

    /* Build signed version-twelve metadata. */
    guardian_firmware_manifest_t manifest =
        test_manifest(
            &backend,
            image,
            sizeof(image),
            12U);

    /* Begin candidate staging. */
    assert(
        guardian_firmware_begin(
            &lifecycle,
            &manifest) ==
        GUARDIAN_FIRMWARE_OK);

    /* Stage every exact image byte. */
    test_stage_image(
        &lifecycle,
        image,
        sizeof(image));

    /* Verify digest and signature. */
    assert(
        guardian_firmware_finalize(
            &lifecycle) ==
        GUARDIAN_FIRMWARE_OK);

    /* Require verified state. */
    assert(
        lifecycle.state ==
        GUARDIAN_FIRMWARE_STATE_VERIFIED);

    /* Mark candidate pending activation. */
    assert(
        guardian_firmware_activate(
            &lifecycle) ==
        GUARDIAN_FIRMWARE_OK);

    /* Require fake pending metadata. */
    assert(
        backend.pending_version_counter ==
        12U);

    /* Require rollback floor still eleven before boot confirmation. */
    assert(
        lifecycle.config.rollback_floor ==
        11U);

    /* Confirm successful boot. */
    assert(
        guardian_firmware_confirm_boot(
            &lifecycle,
            12U) ==
        GUARDIAN_FIRMWARE_OK);

    /* Require confirmed active version. */
    assert(
        lifecycle.config.active_version_counter ==
        12U);

    /* Require rollback floor advancement only after confirmation. */
    assert(
        lifecycle.config.rollback_floor ==
        12U);

    /* Require persisted floor callback. */
    assert(
        backend.persisted_floor ==
        12U);

    /* Require the persistent pending marker to be cleared. */
    assert(
        backend.pending_version_counter ==
        0U);

    /* Require the confirmed boot outcome to be persisted. */
    assert(
        backend.completed_version_counter ==
        12U);

    /* Require the successful outcome flag. */
    assert(
        backend.completed_confirmed ==
        1U);

    /* Require same-version candidate to be blocked afterward. */
    assert(
        guardian_firmware_begin(
            &lifecycle,
            &manifest) ==
        GUARDIAN_FIRMWARE_ERROR_ROLLBACK_BLOCKED);
}

/* Verify a forged signature is rejected after exact image hashing. */
static void test_invalid_signature(void)
{
    /* Create lifecycle and backend storage. */
    guardian_firmware_lifecycle_t lifecycle = {0};
    test_backend_t backend = {0};

    /* Define one deterministic image. */
    static const uint8_t image[] =
        "Guardian M12 signed image test";

    /* Configure current version eleven. */
    test_configure(
        &lifecycle,
        &backend,
        11U);

    /* Build one valid signed manifest. */
    guardian_firmware_manifest_t manifest =
        test_manifest(
            &backend,
            image,
            sizeof(image),
            12U);

    /* Corrupt one signature byte. */
    manifest.signature[0] ^=
        0x01U;

    /* Begin candidate staging. */
    assert(
        guardian_firmware_begin(
            &lifecycle,
            &manifest) ==
        GUARDIAN_FIRMWARE_OK);

    /* Stage exact image bytes. */
    test_stage_image(
        &lifecycle,
        image,
        sizeof(image));

    /* Require signature verification failure. */
    assert(
        guardian_firmware_finalize(
            &lifecycle) ==
        GUARDIAN_FIRMWARE_ERROR_SIGNATURE_INVALID);

    /* Require stable public failure state. */
    assert(
        lifecycle.state ==
        GUARDIAN_FIRMWARE_STATE_FAILED);

    /* Require signature-specific diagnostics. */
    assert(
        lifecycle.failure ==
        GUARDIAN_FIRMWARE_FAILURE_SIGNATURE_INVALID);
}

/* Verify failed pending boot never advances rollback floor. */
static void test_failed_boot_preserves_floor(void)
{
    /* Create lifecycle and backend storage. */
    guardian_firmware_lifecycle_t lifecycle = {0};
    test_backend_t backend = {0};

    /* Define one deterministic candidate image. */
    static const uint8_t image[] =
        "Guardian M12 rollback test image";

    /* Configure current version eleven. */
    test_configure(
        &lifecycle,
        &backend,
        11U);

    /* Build signed version-twelve metadata. */
    guardian_firmware_manifest_t manifest =
        test_manifest(
            &backend,
            image,
            sizeof(image),
            12U);

    /* Begin and stage the complete image. */
    assert(
        guardian_firmware_begin(
            &lifecycle,
            &manifest) ==
        GUARDIAN_FIRMWARE_OK);

    /* Stage exact candidate bytes. */
    test_stage_image(
        &lifecycle,
        image,
        sizeof(image));

    /* Verify candidate. */
    assert(
        guardian_firmware_finalize(
            &lifecycle) ==
        GUARDIAN_FIRMWARE_OK);

    /* Mark candidate pending. */
    assert(
        guardian_firmware_activate(
            &lifecycle) ==
        GUARDIAN_FIRMWARE_OK);

    /* Record one failed candidate boot. */
    assert(
        guardian_firmware_report_boot_failure(
            &lifecycle,
            12U) ==
        GUARDIAN_FIRMWARE_OK);

    /* Require safe rollback state. */
    assert(
        lifecycle.state ==
        GUARDIAN_FIRMWARE_STATE_ROLLED_BACK);

    /* Require previous active version remains authoritative. */
    assert(
        lifecycle.config.active_version_counter ==
        11U);

    /* Require rollback floor remains unchanged. */
    assert(
        lifecycle.config.rollback_floor ==
        11U);

    /* Require no floor persistence callback occurred. */
    assert(
        backend.persisted_floor ==
        0U);

    /* Require the failed pending marker to be cleared persistently. */
    assert(
        backend.pending_version_counter ==
        0U);

    /* Require the failed candidate version to be recorded as completed. */
    assert(
        backend.completed_version_counter ==
        12U);

    /* Require the safe rollback outcome flag. */
    assert(
        backend.completed_confirmed ==
        0U);
}

/* Execute every portable M12 lifecycle test. */
int main(void)
{
    /* Verify confirmation and anti-rollback advancement. */
    test_confirmed_candidate();

    /* Verify forged image metadata fails closed. */
    test_invalid_signature();

    /* Verify failed boot preserves previous trusted version. */
    test_failed_boot_preserves_floor();

    /* Print one concise success line for local and CI logs. */
    (void)printf(
        "Guardian M12 firmware lifecycle host tests: PASS\n");

    /* Return conventional success. */
    return 0;
}
