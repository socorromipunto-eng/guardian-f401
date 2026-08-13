/* Include the public M12 firmware lifecycle API. */
#include "guardian_firmware_lifecycle.h"

/* Include memory helpers for bounded metadata copies. */
#include <string.h>

/* Define the exact signature domain label without a terminating null byte. */
static const uint8_t guardian_firmware_signed_domain[] =
    "GF-M12-IMAGE";

/* Write one unsigned 16-bit integer in Guardian big-endian order. */
static void guardian_firmware_write_u16_be(
    uint8_t *output,
    uint16_t value)
{
    /* Write the most-significant byte first. */
    output[0] =
        (uint8_t)((value >> 8U) & 0xFFU);

    /* Write the least-significant byte second. */
    output[1] =
        (uint8_t)(value & 0xFFU);
}

/* Write one unsigned 32-bit integer in Guardian big-endian order. */
static void guardian_firmware_write_u32_be(
    uint8_t *output,
    uint32_t value)
{
    /* Write all four bytes from most significant to least significant. */
    output[0] =
        (uint8_t)((value >> 24U) & 0xFFU);
    output[1] =
        (uint8_t)((value >> 16U) & 0xFFU);
    output[2] =
        (uint8_t)((value >> 8U) & 0xFFU);
    output[3] =
        (uint8_t)(value & 0xFFU);
}

/* Read one unsigned 16-bit integer in Guardian big-endian order. */
static uint16_t guardian_firmware_read_u16_be(
    const uint8_t *input)
{
    /* Combine both wire bytes explicitly. */
    return (uint16_t)(
        ((uint16_t)input[0] << 8U) |
        (uint16_t)input[1]);
}

/* Read one unsigned 32-bit integer in Guardian big-endian order. */
static uint32_t guardian_firmware_read_u32_be(
    const uint8_t *input)
{
    /* Combine all four wire bytes explicitly. */
    return
        ((uint32_t)input[0] << 24U) |
        ((uint32_t)input[1] << 16U) |
        ((uint32_t)input[2] << 8U) |
        (uint32_t)input[3];
}

/* Return whether one published signature algorithm has a valid signature width. */
static int guardian_firmware_signature_valid(
    uint8_t algorithm,
    uint8_t signature_length)
{
    /* Require the standard Ed25519 signature width. */
    if (algorithm ==
        GUARDIAN_FIRMWARE_SIGNATURE_ED25519)
    {
        /* Report exact Ed25519 width validity. */
        return signature_length == 64U;
    }

    /* Permit the simulator/test-only HMAC backend at full SHA-256 width. */
    if (algorithm ==
        GUARDIAN_FIRMWARE_SIGNATURE_DEMO_HMAC_SHA256)
    {
        /* Report exact test-only HMAC width validity. */
        return signature_length == 32U;
    }

    /* Reject unknown signature algorithms. */
    return 0;
}

/* Record one lifecycle failure while preserving the candidate for diagnostics. */
static guardian_firmware_result_t guardian_firmware_fail(
    guardian_firmware_lifecycle_t *lifecycle,
    guardian_firmware_failure_t failure,
    guardian_firmware_result_t result)
{
    /* Publish failure state when lifecycle storage exists. */
    if (lifecycle != NULL)
    {
        /* Record the stable public failure identifier. */
        lifecycle->failure =
            failure;

        /* Move the candidate into FAILED state. */
        lifecycle->state =
            GUARDIAN_FIRMWARE_STATE_FAILED;
    }

    /* Return the caller-visible operation result. */
    return result;
}

/* Return the strict minimum version counter a new candidate must exceed. */
static uint32_t guardian_firmware_minimum_counter(
    const guardian_firmware_lifecycle_t *lifecycle)
{
    /* Start from the confirmed active version. */
    uint32_t minimum =
        lifecycle->config.active_version_counter;

    /* Raise the floor when persisted rollback policy is stricter. */
    if (lifecycle->config.rollback_floor >
        minimum)
    {
        /* Use the persisted anti-rollback floor. */
        minimum =
            lifecycle->config.rollback_floor;
    }

    /* Return the strict lower bound. */
    return minimum;
}

/* Initialize one unconfigured firmware lifecycle context. */
void guardian_firmware_lifecycle_init(
    guardian_firmware_lifecycle_t *lifecycle)
{
    /* Ignore missing caller storage defensively. */
    if (lifecycle == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Clear configuration, candidate and diagnostics deterministically. */
    (void)memset(
        lifecycle,
        0,
        sizeof(*lifecycle));

    /* Publish the initial idle state. */
    lifecycle->state =
        GUARDIAN_FIRMWARE_STATE_IDLE;

    /* Publish no initial failure. */
    lifecycle->failure =
        GUARDIAN_FIRMWARE_FAILURE_NONE;
}

/* Install platform storage, signature and monotonic-version callbacks. */
guardian_firmware_result_t guardian_firmware_lifecycle_configure(
    guardian_firmware_lifecycle_t *lifecycle,
    const guardian_firmware_config_t *config)
{
    /* Reject missing required caller storage. */
    if ((lifecycle == NULL) ||
        (config == NULL))
    {
        /* Report invalid caller state. */
        return GUARDIAN_FIRMWARE_ERROR_NULL_ARGUMENT;
    }

    /* Require every security-critical platform callback. */
    if ((config->erase == NULL) ||
        (config->write == NULL) ||
        (config->hash == NULL) ||
        (config->verify_signature == NULL) ||
        (config->mark_pending == NULL) ||
        (config->persist_floor == NULL) ||
        (config->complete_pending == NULL) ||
        (config->max_image_size == 0U))
    {
        /* Reject incomplete platform lifecycle configuration. */
        return GUARDIAN_FIRMWARE_ERROR_UNCONFIGURED;
    }

    /* Require persisted rollback floor not to exceed the confirmed running version. */
    if (config->rollback_floor >
        config->active_version_counter)
    {
        /* Reject contradictory boot metadata. */
        return GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD;
    }

    /* Reset all prior lifecycle state before installing new platform configuration. */
    guardian_firmware_lifecycle_init(
        lifecycle);

    /* Copy the complete immutable platform configuration by value. */
    lifecycle->config =
        *config;

    /* Mark the lifecycle backend configured. */
    lifecycle->configured =
        1U;

    /* Report successful configuration. */
    return GUARDIAN_FIRMWARE_OK;
}

/* Decode and validate one FIRMWARE_BEGIN payload. */
guardian_firmware_result_t guardian_firmware_decode_manifest(
    const uint8_t *payload,
    uint16_t payload_length,
    guardian_firmware_manifest_t *manifest)
{
    /* Reject missing required storage. */
    if ((payload == NULL) ||
        (manifest == NULL))
    {
        /* Report invalid caller state. */
        return GUARDIAN_FIRMWARE_ERROR_NULL_ARGUMENT;
    }

    /* Require the complete fixed manifest prefix. */
    if (payload_length <
        GUARDIAN_FIRMWARE_MANIFEST_PREFIX_SIZE)
    {
        /* Reject truncated signed metadata. */
        return GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD;
    }

    /* Require schema revision one. */
    if (payload[0] !=
        GUARDIAN_FIRMWARE_SCHEMA_VERSION)
    {
        /* Reject unsupported lifecycle semantics. */
        return GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD;
    }

    /* Read the published signature length. */
    uint8_t signature_length =
        payload[52];

    /* Require an algorithm-specific signature width. */
    if (guardian_firmware_signature_valid(
            payload[1],
            signature_length) == 0)
    {
        /* Reject unknown algorithms or invalid signature widths. */
        return GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD;
    }

    /* Require exact payload length with no trailing ambiguity. */
    if (payload_length !=
        (uint16_t)(
            GUARDIAN_FIRMWARE_MANIFEST_PREFIX_SIZE +
            signature_length))
    {
        /* Reject truncation or trailing bytes. */
        return GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD;
    }

    /* Clear caller output before populating it. */
    (void)memset(
        manifest,
        0,
        sizeof(*manifest));

    /* Decode signature algorithm. */
    manifest->signature_algorithm =
        payload[1];

    /* Decode trusted key slot identifier. */
    manifest->key_id =
        guardian_firmware_read_u32_be(
            &payload[2]);

    /* Decode monotonic version counter. */
    manifest->version_counter =
        guardian_firmware_read_u32_be(
            &payload[6]);

    /* Reject zero because status uses zero to mean no candidate. */
    if (manifest->version_counter == 0U)
    {
        /* Reject ambiguous candidate identity. */
        return GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD;
    }

    /* Decode semantic firmware major version. */
    manifest->firmware_major =
        guardian_firmware_read_u16_be(
            &payload[10]);

    /* Decode semantic firmware minor version. */
    manifest->firmware_minor =
        guardian_firmware_read_u16_be(
            &payload[12]);

    /* Decode semantic firmware patch version. */
    manifest->firmware_patch =
        guardian_firmware_read_u16_be(
            &payload[14]);

    /* Decode exact image size. */
    manifest->image_size =
        guardian_firmware_read_u32_be(
            &payload[16]);

    /* Reject empty image metadata. */
    if (manifest->image_size == 0U)
    {
        /* Require one real firmware image. */
        return GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD;
    }

    /* Copy the complete SHA-256 image digest. */
    (void)memcpy(
        manifest->image_sha256,
        &payload[20],
        GUARDIAN_FIRMWARE_DIGEST_SIZE);

    /* Store validated signature length. */
    manifest->signature_length =
        signature_length;

    /* Copy exact signature bytes. */
    (void)memcpy(
        manifest->signature,
        &payload[53],
        signature_length);

    /* Report successful manifest decoding. */
    return GUARDIAN_FIRMWARE_OK;
}

/* Decode and validate one FIRMWARE_CHUNK payload. */
guardian_firmware_result_t guardian_firmware_decode_chunk(
    const uint8_t *payload,
    uint16_t payload_length,
    guardian_firmware_chunk_t *chunk)
{
    /* Reject missing required storage. */
    if ((payload == NULL) ||
        (chunk == NULL))
    {
        /* Report invalid caller state. */
        return GUARDIAN_FIRMWARE_ERROR_NULL_ARGUMENT;
    }

    /* Require the fixed prefix plus at least one image byte. */
    if (payload_length <=
        GUARDIAN_FIRMWARE_CHUNK_PREFIX_SIZE)
    {
        /* Reject empty or truncated chunks. */
        return GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD;
    }

    /* Require schema revision one. */
    if (payload[0] !=
        GUARDIAN_FIRMWARE_SCHEMA_VERSION)
    {
        /* Reject unknown chunk semantics. */
        return GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD;
    }

    /* Decode exact chunk byte length. */
    uint16_t chunk_length =
        guardian_firmware_read_u16_be(
            &payload[5]);

    /* Require a non-empty bounded chunk. */
    if ((chunk_length == 0U) ||
        (chunk_length >
         GUARDIAN_FIRMWARE_CHUNK_MAX_DATA))
    {
        /* Reject unsupported chunk sizes. */
        return GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD;
    }

    /* Require exact payload size. */
    if (payload_length !=
        (uint16_t)(
            GUARDIAN_FIRMWARE_CHUNK_PREFIX_SIZE +
            chunk_length))
    {
        /* Reject truncation or trailing ambiguity. */
        return GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD;
    }

    /* Clear caller output before populating it. */
    (void)memset(
        chunk,
        0,
        sizeof(*chunk));

    /* Decode zero-based image offset. */
    chunk->offset =
        guardian_firmware_read_u32_be(
            &payload[1]);

    /* Publish validated chunk length. */
    chunk->length =
        chunk_length;

    /* Copy exact image bytes. */
    (void)memcpy(
        chunk->data,
        &payload[7],
        chunk_length);

    /* Report successful chunk decoding. */
    return GUARDIAN_FIRMWARE_OK;
}

/* Validate one schema-only FINALIZE or ACTIVATE payload. */
guardian_firmware_result_t guardian_firmware_decode_action(
    const uint8_t *payload,
    uint16_t payload_length)
{
    /* Reject missing payload pointer for the required schema byte. */
    if (payload == NULL)
    {
        /* Report invalid caller state. */
        return GUARDIAN_FIRMWARE_ERROR_NULL_ARGUMENT;
    }

    /* Require exactly one schema byte. */
    if (payload_length != 1U)
    {
        /* Reject undefined action bytes. */
        return GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD;
    }

    /* Require schema revision one. */
    if (payload[0] !=
        GUARDIAN_FIRMWARE_SCHEMA_VERSION)
    {
        /* Reject unknown action semantics. */
        return GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD;
    }

    /* Report successful action decoding. */
    return GUARDIAN_FIRMWARE_OK;
}

/* Build the exact canonical 64-byte signature transcript. */
guardian_firmware_result_t guardian_firmware_signed_manifest(
    const guardian_firmware_manifest_t *manifest,
    uint8_t output[GUARDIAN_FIRMWARE_SIGNED_MANIFEST_SIZE])
{
    /* Reject missing required storage. */
    if ((manifest == NULL) ||
        (output == NULL))
    {
        /* Report invalid caller state. */
        return GUARDIAN_FIRMWARE_ERROR_NULL_ARGUMENT;
    }

    /* Require a supported algorithm-specific signature width. */
    if (guardian_firmware_signature_valid(
            manifest->signature_algorithm,
            manifest->signature_length) == 0)
    {
        /* Reject invalid signed metadata. */
        return GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD;
    }

    /* Copy the exact twelve-byte domain label. */
    (void)memcpy(
        &output[0],
        guardian_firmware_signed_domain,
        sizeof(guardian_firmware_signed_domain) - 1U);

    /* Bind schema revision. */
    output[12] =
        GUARDIAN_FIRMWARE_SCHEMA_VERSION;

    /* Bind signature algorithm. */
    output[13] =
        manifest->signature_algorithm;

    /* Bind trusted key slot identifier. */
    guardian_firmware_write_u32_be(
        &output[14],
        manifest->key_id);

    /* Bind monotonic anti-rollback version counter. */
    guardian_firmware_write_u32_be(
        &output[18],
        manifest->version_counter);

    /* Bind semantic major version. */
    guardian_firmware_write_u16_be(
        &output[22],
        manifest->firmware_major);

    /* Bind semantic minor version. */
    guardian_firmware_write_u16_be(
        &output[24],
        manifest->firmware_minor);

    /* Bind semantic patch version. */
    guardian_firmware_write_u16_be(
        &output[26],
        manifest->firmware_patch);

    /* Bind exact image size. */
    guardian_firmware_write_u32_be(
        &output[28],
        manifest->image_size);

    /* Bind the complete image SHA-256 digest. */
    (void)memcpy(
        &output[32],
        manifest->image_sha256,
        GUARDIAN_FIRMWARE_DIGEST_SIZE);

    /* Report successful canonical transcript construction. */
    return GUARDIAN_FIRMWARE_OK;
}

/* Begin one candidate firmware transfer after rollback-policy checks. */
guardian_firmware_result_t guardian_firmware_begin(
    guardian_firmware_lifecycle_t *lifecycle,
    const guardian_firmware_manifest_t *manifest)
{
    /* Reject missing required storage. */
    if ((lifecycle == NULL) ||
        (manifest == NULL))
    {
        /* Report invalid caller state. */
        return GUARDIAN_FIRMWARE_ERROR_NULL_ARGUMENT;
    }

    /* Require configured platform callbacks. */
    if (lifecycle->configured == 0U)
    {
        /* Record fail-closed platform configuration failure. */
        return guardian_firmware_fail(
            lifecycle,
            GUARDIAN_FIRMWARE_FAILURE_UNCONFIGURED,
            GUARDIAN_FIRMWARE_ERROR_UNCONFIGURED);
    }

    /* Require a supported signature algorithm and exact signature width. */
    if (guardian_firmware_signature_valid(
            manifest->signature_algorithm,
            manifest->signature_length) == 0)
    {
        /* Record invalid signed metadata. */
        return guardian_firmware_fail(
            lifecycle,
            GUARDIAN_FIRMWARE_FAILURE_INVALID_PAYLOAD,
            GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD);
    }

    /* Require a non-empty bounded image size. */
    if ((manifest->image_size == 0U) ||
        (manifest->image_size >
         lifecycle->config.max_image_size))
    {
        /* Reject candidate outside the platform staging bound. */
        return guardian_firmware_fail(
            lifecycle,
            GUARDIAN_FIRMWARE_FAILURE_INVALID_PAYLOAD,
            GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD);
    }

    /* Require strict monotonic advancement beyond both running and persisted floors. */
    if (manifest->version_counter <=
        guardian_firmware_minimum_counter(
            lifecycle))
    {
        /* Record anti-rollback policy rejection. */
        return guardian_firmware_fail(
            lifecycle,
            GUARDIAN_FIRMWARE_FAILURE_ROLLBACK_BLOCKED,
            GUARDIAN_FIRMWARE_ERROR_ROLLBACK_BLOCKED);
    }

    /* Erase candidate staging storage before accepting any image bytes. */
    if (lifecycle->config.erase(
            lifecycle->config.context,
            manifest->image_size) == 0)
    {
        /* Record platform storage failure. */
        return guardian_firmware_fail(
            lifecycle,
            GUARDIAN_FIRMWARE_FAILURE_STORAGE,
            GUARDIAN_FIRMWARE_ERROR_STORAGE);
    }

    /* Copy the complete candidate signed metadata by value. */
    lifecycle->manifest =
        *manifest;

    /* Start sequential byte ownership at offset zero. */
    lifecycle->bytes_received =
        0U;

    /* Clear any previous failure. */
    lifecycle->failure =
        GUARDIAN_FIRMWARE_FAILURE_NONE;

    /* Enter candidate receiving state. */
    lifecycle->state =
        GUARDIAN_FIRMWARE_STATE_RECEIVING;

    /* Report successful transfer initialization. */
    return GUARDIAN_FIRMWARE_OK;
}

/* Write one exact sequential candidate image chunk. */
guardian_firmware_result_t guardian_firmware_write_chunk(
    guardian_firmware_lifecycle_t *lifecycle,
    const guardian_firmware_chunk_t *chunk)
{
    /* Reject missing required storage. */
    if ((lifecycle == NULL) ||
        (chunk == NULL))
    {
        /* Report invalid caller state. */
        return GUARDIAN_FIRMWARE_ERROR_NULL_ARGUMENT;
    }

    /* Require an active candidate transfer. */
    if (lifecycle->state !=
        GUARDIAN_FIRMWARE_STATE_RECEIVING)
    {
        /* Reject writes outside the receiving state. */
        return GUARDIAN_FIRMWARE_ERROR_INVALID_STATE;
    }

    /* Require a non-empty bounded chunk. */
    if ((chunk->length == 0U) ||
        (chunk->length >
         GUARDIAN_FIRMWARE_CHUNK_MAX_DATA))
    {
        /* Record malformed chunk semantics. */
        return guardian_firmware_fail(
            lifecycle,
            GUARDIAN_FIRMWARE_FAILURE_INVALID_PAYLOAD,
            GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD);
    }

    /* Require strict sequential offsets with no overlap or gaps. */
    if (chunk->offset !=
        lifecycle->bytes_received)
    {
        /* Record out-of-order transfer failure. */
        return guardian_firmware_fail(
            lifecycle,
            GUARDIAN_FIRMWARE_FAILURE_OUT_OF_ORDER,
            GUARDIAN_FIRMWARE_ERROR_OUT_OF_ORDER);
    }

    /* Require the chunk to remain within the signed image size. */
    if ((uint64_t)chunk->offset +
        (uint64_t)chunk->length >
        (uint64_t)lifecycle->manifest.image_size)
    {
        /* Record image-bound violation. */
        return guardian_firmware_fail(
            lifecycle,
            GUARDIAN_FIRMWARE_FAILURE_INVALID_PAYLOAD,
            GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD);
    }

    /* Persist exact chunk bytes through the platform staging backend. */
    if (lifecycle->config.write(
            lifecycle->config.context,
            chunk->offset,
            chunk->data,
            chunk->length) == 0)
    {
        /* Record storage write failure. */
        return guardian_firmware_fail(
            lifecycle,
            GUARDIAN_FIRMWARE_FAILURE_STORAGE,
            GUARDIAN_FIRMWARE_ERROR_STORAGE);
    }

    /* Advance sequential transfer ownership only after a successful write. */
    lifecycle->bytes_received +=
        (uint32_t)chunk->length;

    /* Report successful chunk staging. */
    return GUARDIAN_FIRMWARE_OK;
}

/* Verify staged image SHA-256 and manifest signature. */
guardian_firmware_result_t guardian_firmware_finalize(
    guardian_firmware_lifecycle_t *lifecycle)
{
    /* Reject missing lifecycle storage. */
    if (lifecycle == NULL)
    {
        /* Report invalid caller state. */
        return GUARDIAN_FIRMWARE_ERROR_NULL_ARGUMENT;
    }

    /* Require an active candidate transfer. */
    if (lifecycle->state !=
        GUARDIAN_FIRMWARE_STATE_RECEIVING)
    {
        /* Reject finalize outside receiving state. */
        return GUARDIAN_FIRMWARE_ERROR_INVALID_STATE;
    }

    /* Require every signed image byte before verification. */
    if (lifecycle->bytes_received !=
        lifecycle->manifest.image_size)
    {
        /* Record incomplete candidate image. */
        return guardian_firmware_fail(
            lifecycle,
            GUARDIAN_FIRMWARE_FAILURE_INVALID_PAYLOAD,
            GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD);
    }

    /* Store the platform-calculated staged image digest. */
    uint8_t actual_digest[
        GUARDIAN_FIRMWARE_DIGEST_SIZE] = {0};

    /* Calculate SHA-256 across the exact staged image. */
    if (lifecycle->config.hash(
            lifecycle->config.context,
            lifecycle->manifest.image_size,
            actual_digest) == 0)
    {
        /* Record storage/hash backend failure. */
        return guardian_firmware_fail(
            lifecycle,
            GUARDIAN_FIRMWARE_FAILURE_STORAGE,
            GUARDIAN_FIRMWARE_ERROR_STORAGE);
    }

    /* Compare the signed digest without data-dependent early exit. */
    if (guardian_crypto_constant_time_equal(
            actual_digest,
            lifecycle->manifest.image_sha256,
            GUARDIAN_FIRMWARE_DIGEST_SIZE) == 0)
    {
        /* Erase temporary digest bytes. */
        guardian_crypto_zero(
            actual_digest,
            sizeof(actual_digest));

        /* Record image-integrity failure. */
        return guardian_firmware_fail(
            lifecycle,
            GUARDIAN_FIRMWARE_FAILURE_HASH_MISMATCH,
            GUARDIAN_FIRMWARE_ERROR_HASH_MISMATCH);
    }

    /* Erase the temporary digest after successful comparison. */
    guardian_crypto_zero(
        actual_digest,
        sizeof(actual_digest));

    /* Store the exact canonical signed-manifest transcript. */
    uint8_t signed_message[
        GUARDIAN_FIRMWARE_SIGNED_MANIFEST_SIZE] = {0};

    /* Build the domain-separated signature transcript. */
    guardian_firmware_result_t signed_result =
        guardian_firmware_signed_manifest(
            &lifecycle->manifest,
            signed_message);

    /* Treat impossible candidate metadata failure as invalid payload. */
    if (signed_result !=
        GUARDIAN_FIRMWARE_OK)
    {
        /* Erase temporary transcript. */
        guardian_crypto_zero(
            signed_message,
            sizeof(signed_message));

        /* Record invalid signed metadata. */
        return guardian_firmware_fail(
            lifecycle,
            GUARDIAN_FIRMWARE_FAILURE_INVALID_PAYLOAD,
            signed_result);
    }

    /* Verify signature using platform-controlled trusted key storage/backend. */
    int verified =
        lifecycle->config.verify_signature(
            lifecycle->config.context,
            lifecycle->manifest.signature_algorithm,
            lifecycle->manifest.key_id,
            signed_message,
            sizeof(signed_message),
            lifecycle->manifest.signature,
            lifecycle->manifest.signature_length);

    /* Erase the canonical transcript after verification. */
    guardian_crypto_zero(
        signed_message,
        sizeof(signed_message));

    /* Fail closed when signature verification fails. */
    if (verified == 0)
    {
        /* Record image authenticity failure. */
        return guardian_firmware_fail(
            lifecycle,
            GUARDIAN_FIRMWARE_FAILURE_SIGNATURE_INVALID,
            GUARDIAN_FIRMWARE_ERROR_SIGNATURE_INVALID);
    }

    /* Re-check rollback policy at the verification boundary. */
    if (lifecycle->manifest.version_counter <=
        guardian_firmware_minimum_counter(
            lifecycle))
    {
        /* Record rollback policy rejection. */
        return guardian_firmware_fail(
            lifecycle,
            GUARDIAN_FIRMWARE_FAILURE_ROLLBACK_BLOCKED,
            GUARDIAN_FIRMWARE_ERROR_ROLLBACK_BLOCKED);
    }

    /* Publish verified candidate state. */
    lifecycle->state =
        GUARDIAN_FIRMWARE_STATE_VERIFIED;

    /* Clear failure diagnostics. */
    lifecycle->failure =
        GUARDIAN_FIRMWARE_FAILURE_NONE;

    /* Report successful image verification. */
    return GUARDIAN_FIRMWARE_OK;
}

/* Mark one verified candidate pending activation. */
guardian_firmware_result_t guardian_firmware_activate(
    guardian_firmware_lifecycle_t *lifecycle)
{
    /* Reject missing lifecycle storage. */
    if (lifecycle == NULL)
    {
        /* Report invalid caller state. */
        return GUARDIAN_FIRMWARE_ERROR_NULL_ARGUMENT;
    }

    /* Require an authenticated verified candidate. */
    if (lifecycle->state !=
        GUARDIAN_FIRMWARE_STATE_VERIFIED)
    {
        /* Reject activation before verification. */
        return GUARDIAN_FIRMWARE_ERROR_INVALID_STATE;
    }

    /* Persist pending boot metadata through the platform backend. */
    if (lifecycle->config.mark_pending(
            lifecycle->config.context,
            lifecycle->manifest.version_counter,
            lifecycle->manifest.firmware_major,
            lifecycle->manifest.firmware_minor,
            lifecycle->manifest.firmware_patch) == 0)
    {
        /* Record pending-metadata persistence failure. */
        return guardian_firmware_fail(
            lifecycle,
            GUARDIAN_FIRMWARE_FAILURE_ACTIVATION,
            GUARDIAN_FIRMWARE_ERROR_ACTIVATION);
    }

    /* Publish pending activation state. */
    lifecycle->state =
        GUARDIAN_FIRMWARE_STATE_PENDING_ACTIVATION;

    /* Clear failure diagnostics. */
    lifecycle->failure =
        GUARDIAN_FIRMWARE_FAILURE_NONE;

    /* Report successful pending activation. */
    return GUARDIAN_FIRMWARE_OK;
}

/* Confirm one successfully booted pending image and advance rollback floor. */
guardian_firmware_result_t guardian_firmware_confirm_boot(
    guardian_firmware_lifecycle_t *lifecycle,
    uint32_t booted_version_counter)
{
    /* Reject missing lifecycle storage. */
    if (lifecycle == NULL)
    {
        /* Report invalid caller state. */
        return GUARDIAN_FIRMWARE_ERROR_NULL_ARGUMENT;
    }

    /* Require a pending candidate. */
    if (lifecycle->state !=
        GUARDIAN_FIRMWARE_STATE_PENDING_ACTIVATION)
    {
        /* Reject confirmation outside pending state. */
        return GUARDIAN_FIRMWARE_ERROR_INVALID_STATE;
    }

    /* Require the running image to match the pending candidate exactly. */
    if (booted_version_counter !=
        lifecycle->manifest.version_counter)
    {
        /* Record contradictory boot metadata. */
        return guardian_firmware_fail(
            lifecycle,
            GUARDIAN_FIRMWARE_FAILURE_CONFIRMATION,
            GUARDIAN_FIRMWARE_ERROR_CONFIRMATION);
    }

    /* Persist the new monotonic anti-rollback floor only after successful boot. */
    if (lifecycle->config.persist_floor(
            lifecycle->config.context,
            booted_version_counter) == 0)
    {
        /* Record persistence failure. */
        return guardian_firmware_fail(
            lifecycle,
            GUARDIAN_FIRMWARE_FAILURE_CONFIRMATION,
            GUARDIAN_FIRMWARE_ERROR_CONFIRMATION);
    }

    /* Reflect the already-persisted rollback floor in runtime state. */
    lifecycle->config.rollback_floor =
        booted_version_counter;

    /* Publish the running candidate as the active version. */
    lifecycle->config.active_version_counter =
        booted_version_counter;

    /* Clear/finalize the persistent pending marker only after rollback floor persistence. */
    if (lifecycle->config.complete_pending(
            lifecycle->config.context,
            booted_version_counter,
            1U) == 0)
    {
        /* Preserve the advanced rollback floor but report incomplete boot metadata finalization. */
        return guardian_firmware_fail(
            lifecycle,
            GUARDIAN_FIRMWARE_FAILURE_CONFIRMATION,
            GUARDIAN_FIRMWARE_ERROR_CONFIRMATION);
    }

    /* Publish successful confirmation state. */
    lifecycle->state =
        GUARDIAN_FIRMWARE_STATE_CONFIRMED;

    /* Clear failure diagnostics. */
    lifecycle->failure =
        GUARDIAN_FIRMWARE_FAILURE_NONE;

    /* Report successful boot confirmation. */
    return GUARDIAN_FIRMWARE_OK;
}

/* Record a failed pending boot without advancing rollback floor. */
guardian_firmware_result_t guardian_firmware_report_boot_failure(
    guardian_firmware_lifecycle_t *lifecycle,
    uint32_t attempted_version_counter)
{
    /* Reject missing lifecycle storage. */
    if (lifecycle == NULL)
    {
        /* Report invalid caller state. */
        return GUARDIAN_FIRMWARE_ERROR_NULL_ARGUMENT;
    }

    /* Require one pending candidate. */
    if (lifecycle->state !=
        GUARDIAN_FIRMWARE_STATE_PENDING_ACTIVATION)
    {
        /* Reject failure reporting outside pending state. */
        return GUARDIAN_FIRMWARE_ERROR_INVALID_STATE;
    }

    /* Require the attempted boot to match the pending candidate. */
    if (attempted_version_counter !=
        lifecycle->manifest.version_counter)
    {
        /* Reject contradictory boot metadata. */
        return GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD;
    }

    /* Persist/clear the failed pending-image marker before declaring rollback complete. */
    if (lifecycle->config.complete_pending(
            lifecycle->config.context,
            attempted_version_counter,
            0U) == 0)
    {
        /* Keep the rollback floor unchanged but report persistent boot-outcome failure. */
        return guardian_firmware_fail(
            lifecycle,
            GUARDIAN_FIRMWARE_FAILURE_CONFIRMATION,
            GUARDIAN_FIRMWARE_ERROR_CONFIRMATION);
    }

    /* Preserve the previous active version and rollback floor unchanged. */
    lifecycle->state =
        GUARDIAN_FIRMWARE_STATE_ROLLED_BACK;

    /* Clear failure because rollback is an expected safe lifecycle outcome. */
    lifecycle->failure =
        GUARDIAN_FIRMWARE_FAILURE_NONE;

    /* Report successful rollback-state recording. */
    return GUARDIAN_FIRMWARE_OK;
}

/* Return public lifecycle diagnostics by value. */
guardian_firmware_status_t guardian_firmware_status(
    const guardian_firmware_lifecycle_t *lifecycle)
{
    /* Create deterministic zero status for null callers. */
    guardian_firmware_status_t status = {0};

    /* Return empty status when lifecycle storage is unavailable. */
    if (lifecycle == NULL)
    {
        /* Return deterministic zero diagnostics. */
        return status;
    }

    /* Publish current lifecycle state. */
    status.state =
        lifecycle->state;

    /* Publish current failure identifier. */
    status.failure =
        lifecycle->failure;

    /* Publish confirmed active version counter. */
    status.active_version_counter =
        lifecycle->config.active_version_counter;

    /* Publish persisted rollback floor. */
    status.rollback_floor =
        lifecycle->config.rollback_floor;

    /* Publish candidate fields only when candidate metadata exists. */
    if (lifecycle->manifest.version_counter != 0U)
    {
        /* Publish candidate signature algorithm. */
        status.signature_algorithm =
            lifecycle->manifest.signature_algorithm;

        /* Publish candidate monotonic version. */
        status.candidate_version_counter =
            lifecycle->manifest.version_counter;

        /* Publish accepted candidate byte count. */
        status.bytes_received =
            lifecycle->bytes_received;

        /* Publish signed image size. */
        status.image_size =
            lifecycle->manifest.image_size;

        /* Publish trusted verifier key slot identifier. */
        status.key_id =
            lifecycle->manifest.key_id;

        /* Publish semantic candidate major version. */
        status.firmware_major =
            lifecycle->manifest.firmware_major;

        /* Publish semantic candidate minor version. */
        status.firmware_minor =
            lifecycle->manifest.firmware_minor;

        /* Publish semantic candidate patch version. */
        status.firmware_patch =
            lifecycle->manifest.firmware_patch;
    }

    /* Return immutable public diagnostics. */
    return status;
}

/* Encode one fixed public GET_FIRMWARE_STATUS payload. */
guardian_firmware_result_t guardian_firmware_encode_status_payload(
    const guardian_firmware_status_t *status,
    uint8_t *payload,
    uint16_t payload_capacity,
    uint16_t *payload_length)
{
    /* Reject missing required storage. */
    if ((status == NULL) ||
        (payload == NULL) ||
        (payload_length == NULL))
    {
        /* Report invalid caller state. */
        return GUARDIAN_FIRMWARE_ERROR_NULL_ARGUMENT;
    }

    /* Require the complete fixed status output capacity. */
    if (payload_capacity <
        GUARDIAN_FIRMWARE_STATUS_PAYLOAD_SIZE)
    {
        /* Report bounded output failure. */
        return GUARDIAN_FIRMWARE_ERROR_OUTPUT_TOO_SMALL;
    }

    /* Publish schema revision one. */
    payload[0] =
        GUARDIAN_FIRMWARE_SCHEMA_VERSION;

    /* Publish current lifecycle state. */
    payload[1] =
        (uint8_t)status->state;

    /* Publish current failure code. */
    payload[2] =
        (uint8_t)status->failure;

    /* Publish candidate signature algorithm. */
    payload[3] =
        status->signature_algorithm;

    /* Publish active monotonic version. */
    guardian_firmware_write_u32_be(
        &payload[4],
        status->active_version_counter);

    /* Publish persisted rollback floor. */
    guardian_firmware_write_u32_be(
        &payload[8],
        status->rollback_floor);

    /* Publish candidate monotonic version. */
    guardian_firmware_write_u32_be(
        &payload[12],
        status->candidate_version_counter);

    /* Publish sequentially staged bytes. */
    guardian_firmware_write_u32_be(
        &payload[16],
        status->bytes_received);

    /* Publish signed candidate image size. */
    guardian_firmware_write_u32_be(
        &payload[20],
        status->image_size);

    /* Publish trusted verifier key slot identifier. */
    guardian_firmware_write_u32_be(
        &payload[24],
        status->key_id);

    /* Publish semantic candidate major version. */
    guardian_firmware_write_u16_be(
        &payload[28],
        status->firmware_major);

    /* Publish semantic candidate minor version. */
    guardian_firmware_write_u16_be(
        &payload[30],
        status->firmware_minor);

    /* Publish semantic candidate patch version. */
    guardian_firmware_write_u16_be(
        &payload[32],
        status->firmware_patch);

    /* Publish exact fixed payload size. */
    *payload_length =
        GUARDIAN_FIRMWARE_STATUS_PAYLOAD_SIZE;

    /* Report successful serialization. */
    return GUARDIAN_FIRMWARE_OK;
}
