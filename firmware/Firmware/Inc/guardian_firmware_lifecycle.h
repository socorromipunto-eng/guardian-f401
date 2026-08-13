#ifndef GUARDIAN_FIRMWARE_LIFECYCLE_H
#define GUARDIAN_FIRMWARE_LIFECYCLE_H

/* Include portable SHA-256 and constant-time comparison primitives. */
#include "guardian_crypto.h"

/* Include Guardian frame constants and wire result types. */
#include "guardian_protocol.h"

/* Include size_t for bounded platform callbacks. */
#include <stddef.h>

/* Include fixed-width integer types for deterministic metadata fields. */
#include <stdint.h>

/* Define the first firmware-lifecycle schema revision. */
#define GUARDIAN_FIRMWARE_SCHEMA_VERSION ((uint8_t)0x01U)

/* Define the standard Ed25519 production signature identifier. */
#define GUARDIAN_FIRMWARE_SIGNATURE_ED25519 ((uint8_t)0x01U)

/* Reserve one test-only signature identifier for simulator HMAC verification. */
#define GUARDIAN_FIRMWARE_SIGNATURE_DEMO_HMAC_SHA256 ((uint8_t)0xFEU)

/* Bound signatures to the Ed25519 width. */
#define GUARDIAN_FIRMWARE_MAX_SIGNATURE_SIZE ((size_t)64U)

/* Define the exact SHA-256 image digest width. */
#define GUARDIAN_FIRMWARE_DIGEST_SIZE GUARDIAN_SHA256_SIZE

/* Define the exact canonical signed-manifest transcript size. */
#define GUARDIAN_FIRMWARE_SIGNED_MANIFEST_SIZE ((size_t)64U)

/* Define the fixed FIRMWARE_BEGIN prefix before signature bytes. */
#define GUARDIAN_FIRMWARE_MANIFEST_PREFIX_SIZE ((uint16_t)53U)

/* Define the maximum complete FIRMWARE_BEGIN payload size. */
#define GUARDIAN_FIRMWARE_MANIFEST_MAX_SIZE \
    ((uint16_t)(GUARDIAN_FIRMWARE_MANIFEST_PREFIX_SIZE + GUARDIAN_FIRMWARE_MAX_SIGNATURE_SIZE))

/* Define the fixed FIRMWARE_CHUNK prefix before image bytes. */
#define GUARDIAN_FIRMWARE_CHUNK_PREFIX_SIZE ((uint16_t)7U)

/* Keep firmware chunks safely inside one M10 secure request envelope. */
#define GUARDIAN_FIRMWARE_CHUNK_MAX_DATA ((uint16_t)192U)

/* Define the fixed public GET_FIRMWARE_STATUS payload size. */
#define GUARDIAN_FIRMWARE_STATUS_PAYLOAD_SIZE ((uint16_t)34U)

/* Define M12 firmware lifecycle states. */
typedef enum
{
    /* No candidate image is currently staged. */
    GUARDIAN_FIRMWARE_STATE_IDLE = 0,

    /* Candidate bytes are being written sequentially. */
    GUARDIAN_FIRMWARE_STATE_RECEIVING = 1,

    /* Candidate digest and signature have been verified. */
    GUARDIAN_FIRMWARE_STATE_VERIFIED = 2,

    /* Candidate is marked for activation on the next boot. */
    GUARDIAN_FIRMWARE_STATE_PENDING_ACTIVATION = 3,

    /* Candidate boot was confirmed and rollback floor advanced. */
    GUARDIAN_FIRMWARE_STATE_CONFIRMED = 4,

    /* Candidate boot failed and the previous image remained authoritative. */
    GUARDIAN_FIRMWARE_STATE_ROLLED_BACK = 5,

    /* Candidate processing failed. */
    GUARDIAN_FIRMWARE_STATE_FAILED = 6
} guardian_firmware_state_t;

/* Define stable public firmware lifecycle failure identifiers. */
typedef enum
{
    /* No failure is currently recorded. */
    GUARDIAN_FIRMWARE_FAILURE_NONE = 0,

    /* Required platform storage or verification callbacks are missing. */
    GUARDIAN_FIRMWARE_FAILURE_UNCONFIGURED = 1,

    /* Manifest, action or chunk semantics are invalid. */
    GUARDIAN_FIRMWARE_FAILURE_INVALID_PAYLOAD = 2,

    /* Candidate monotonic version violates rollback policy. */
    GUARDIAN_FIRMWARE_FAILURE_ROLLBACK_BLOCKED = 3,

    /* Staging erase or write failed. */
    GUARDIAN_FIRMWARE_FAILURE_STORAGE = 4,

    /* Staged image digest differs from the signed manifest. */
    GUARDIAN_FIRMWARE_FAILURE_HASH_MISMATCH = 5,

    /* Signature algorithm, key or signature verification failed. */
    GUARDIAN_FIRMWARE_FAILURE_SIGNATURE_INVALID = 6,

    /* Pending activation metadata could not be persisted. */
    GUARDIAN_FIRMWARE_FAILURE_ACTIVATION = 7,

    /* Confirmed rollback-floor metadata could not be persisted. */
    GUARDIAN_FIRMWARE_FAILURE_CONFIRMATION = 8,

    /* A chunk arrived at an unexpected offset. */
    GUARDIAN_FIRMWARE_FAILURE_OUT_OF_ORDER = 9
} guardian_firmware_failure_t;

/* Define portable lifecycle operation outcomes. */
typedef enum
{
    /* Indicate successful lifecycle processing. */
    GUARDIAN_FIRMWARE_OK = 0,

    /* Indicate a missing required pointer. */
    GUARDIAN_FIRMWARE_ERROR_NULL_ARGUMENT,

    /* Indicate missing platform lifecycle callbacks. */
    GUARDIAN_FIRMWARE_ERROR_UNCONFIGURED,

    /* Indicate malformed or unsupported payload semantics. */
    GUARDIAN_FIRMWARE_ERROR_INVALID_PAYLOAD,

    /* Indicate an operation attempted in the wrong lifecycle state. */
    GUARDIAN_FIRMWARE_ERROR_INVALID_STATE,

    /* Indicate monotonic anti-rollback rejection. */
    GUARDIAN_FIRMWARE_ERROR_ROLLBACK_BLOCKED,

    /* Indicate staging erase/write/hash backend failure. */
    GUARDIAN_FIRMWARE_ERROR_STORAGE,

    /* Indicate a staged image digest mismatch. */
    GUARDIAN_FIRMWARE_ERROR_HASH_MISMATCH,

    /* Indicate signature verification failure. */
    GUARDIAN_FIRMWARE_ERROR_SIGNATURE_INVALID,

    /* Indicate pending activation persistence failure. */
    GUARDIAN_FIRMWARE_ERROR_ACTIVATION,

    /* Indicate rollback-floor persistence failure. */
    GUARDIAN_FIRMWARE_ERROR_CONFIRMATION,

    /* Indicate an unexpected sequential chunk offset. */
    GUARDIAN_FIRMWARE_ERROR_OUT_OF_ORDER,

    /* Indicate insufficient caller output capacity. */
    GUARDIAN_FIRMWARE_ERROR_OUTPUT_TOO_SMALL
} guardian_firmware_result_t;

/* Store one signed firmware manifest. */
typedef struct
{
    /* Store the selected signature algorithm identifier. */
    uint8_t signature_algorithm;

    /* Store the trusted verifier key slot identifier. */
    uint32_t key_id;

    /* Store the monotonic anti-rollback version counter. */
    uint32_t version_counter;

    /* Store semantic firmware major version. */
    uint16_t firmware_major;

    /* Store semantic firmware minor version. */
    uint16_t firmware_minor;

    /* Store semantic firmware patch version. */
    uint16_t firmware_patch;

    /* Store the exact firmware image byte size. */
    uint32_t image_size;

    /* Store the complete SHA-256 image digest. */
    uint8_t image_sha256[GUARDIAN_FIRMWARE_DIGEST_SIZE];

    /* Store the algorithm-specific signature length. */
    uint8_t signature_length;

    /* Store bounded signature bytes inline. */
    uint8_t signature[GUARDIAN_FIRMWARE_MAX_SIGNATURE_SIZE];
} guardian_firmware_manifest_t;

/* Store one sequential firmware chunk decoded from the wire. */
typedef struct
{
    /* Store the zero-based image byte offset. */
    uint32_t offset;

    /* Store the number of meaningful chunk bytes. */
    uint16_t length;

    /* Store bounded image bytes inline. */
    uint8_t data[GUARDIAN_FIRMWARE_CHUNK_MAX_DATA];
} guardian_firmware_chunk_t;

/* Store public firmware lifecycle diagnostics. */
typedef struct
{
    /* Store current lifecycle state. */
    guardian_firmware_state_t state;

    /* Store current lifecycle failure identifier. */
    guardian_firmware_failure_t failure;

    /* Store candidate signature algorithm or zero. */
    uint8_t signature_algorithm;

    /* Store confirmed active monotonic version. */
    uint32_t active_version_counter;

    /* Store persisted anti-rollback floor. */
    uint32_t rollback_floor;

    /* Store candidate monotonic version or zero. */
    uint32_t candidate_version_counter;

    /* Store sequentially staged image bytes. */
    uint32_t bytes_received;

    /* Store expected candidate image size. */
    uint32_t image_size;

    /* Store candidate verifier key identifier. */
    uint32_t key_id;

    /* Store candidate semantic major version. */
    uint16_t firmware_major;

    /* Store candidate semantic minor version. */
    uint16_t firmware_minor;

    /* Store candidate semantic patch version. */
    uint16_t firmware_patch;
} guardian_firmware_status_t;

/* Erase staging storage for a new candidate image. */
typedef int (*guardian_firmware_erase_fn)(
    void *context,
    uint32_t image_size);

/* Write one bounded sequential candidate image chunk. */
typedef int (*guardian_firmware_write_fn)(
    void *context,
    uint32_t offset,
    const uint8_t *data,
    uint16_t length);

/* Calculate SHA-256 over the exact staged image bytes. */
typedef int (*guardian_firmware_hash_fn)(
    void *context,
    uint32_t image_size,
    uint8_t digest[GUARDIAN_FIRMWARE_DIGEST_SIZE]);

/* Verify one signature over the exact canonical signed-manifest bytes. */
typedef int (*guardian_firmware_verify_signature_fn)(
    void *context,
    uint8_t signature_algorithm,
    uint32_t key_id,
    const uint8_t *signed_message,
    size_t signed_message_length,
    const uint8_t *signature,
    uint8_t signature_length);

/* Persist one verified candidate as pending activation. */
typedef int (*guardian_firmware_mark_pending_fn)(
    void *context,
    uint32_t version_counter,
    uint16_t firmware_major,
    uint16_t firmware_minor,
    uint16_t firmware_patch);

/* Persist the anti-rollback floor after a successful boot confirmation. */
typedef int (*guardian_firmware_persist_floor_fn)(
    void *context,
    uint32_t rollback_floor);

/* Persist completion/clearing of pending-image metadata after one boot outcome. */
typedef int (*guardian_firmware_complete_pending_fn)(
    void *context,
    uint32_t version_counter,
    uint8_t confirmed);

/* Store immutable platform lifecycle configuration. */
typedef struct
{
    /* Store opaque platform backend state. */
    void *context;

    /* Erase the candidate staging area. */
    guardian_firmware_erase_fn erase;

    /* Write candidate image bytes. */
    guardian_firmware_write_fn write;

    /* Hash the exact staged candidate image. */
    guardian_firmware_hash_fn hash;

    /* Verify the candidate manifest signature using trusted key storage. */
    guardian_firmware_verify_signature_fn verify_signature;

    /* Persist pending activation metadata. */
    guardian_firmware_mark_pending_fn mark_pending;

    /* Persist confirmed monotonic rollback floor. */
    guardian_firmware_persist_floor_fn persist_floor;

    /* Clear/finalize pending-image metadata after confirmed or failed boot. */
    guardian_firmware_complete_pending_fn complete_pending;

    /* Bound the largest accepted candidate image. */
    uint32_t max_image_size;

    /* Store the currently confirmed running version counter. */
    uint32_t active_version_counter;

    /* Store the persisted minimum accepted version counter. */
    uint32_t rollback_floor;
} guardian_firmware_config_t;

/* Store the complete transport-independent M12 lifecycle state. */
typedef struct
{
    /* Store copied platform configuration. */
    guardian_firmware_config_t config;

    /* Mark whether required platform callbacks are configured. */
    uint8_t configured;

    /* Store current lifecycle state. */
    guardian_firmware_state_t state;

    /* Store current lifecycle failure identifier. */
    guardian_firmware_failure_t failure;

    /* Store the current candidate signed manifest. */
    guardian_firmware_manifest_t manifest;

    /* Store sequentially accepted candidate bytes. */
    uint32_t bytes_received;
} guardian_firmware_lifecycle_t;

/* Initialize one unconfigured firmware lifecycle context. */
void guardian_firmware_lifecycle_init(
    guardian_firmware_lifecycle_t *lifecycle);

/* Install platform storage, signature and monotonic-version callbacks. */
guardian_firmware_result_t guardian_firmware_lifecycle_configure(
    guardian_firmware_lifecycle_t *lifecycle,
    const guardian_firmware_config_t *config);

/* Decode and validate one FIRMWARE_BEGIN payload. */
guardian_firmware_result_t guardian_firmware_decode_manifest(
    const uint8_t *payload,
    uint16_t payload_length,
    guardian_firmware_manifest_t *manifest);

/* Decode and validate one FIRMWARE_CHUNK payload. */
guardian_firmware_result_t guardian_firmware_decode_chunk(
    const uint8_t *payload,
    uint16_t payload_length,
    guardian_firmware_chunk_t *chunk);

/* Validate one schema-only FINALIZE or ACTIVATE payload. */
guardian_firmware_result_t guardian_firmware_decode_action(
    const uint8_t *payload,
    uint16_t payload_length);

/* Build the exact canonical 64-byte signature transcript. */
guardian_firmware_result_t guardian_firmware_signed_manifest(
    const guardian_firmware_manifest_t *manifest,
    uint8_t output[GUARDIAN_FIRMWARE_SIGNED_MANIFEST_SIZE]);

/* Begin one candidate firmware transfer after rollback-policy checks. */
guardian_firmware_result_t guardian_firmware_begin(
    guardian_firmware_lifecycle_t *lifecycle,
    const guardian_firmware_manifest_t *manifest);

/* Write one exact sequential candidate image chunk. */
guardian_firmware_result_t guardian_firmware_write_chunk(
    guardian_firmware_lifecycle_t *lifecycle,
    const guardian_firmware_chunk_t *chunk);

/* Verify staged image SHA-256 and manifest signature. */
guardian_firmware_result_t guardian_firmware_finalize(
    guardian_firmware_lifecycle_t *lifecycle);

/* Mark one verified candidate pending activation. */
guardian_firmware_result_t guardian_firmware_activate(
    guardian_firmware_lifecycle_t *lifecycle);

/* Confirm one successfully booted pending image and advance rollback floor. */
guardian_firmware_result_t guardian_firmware_confirm_boot(
    guardian_firmware_lifecycle_t *lifecycle,
    uint32_t booted_version_counter);

/* Record a failed pending boot without advancing rollback floor. */
guardian_firmware_result_t guardian_firmware_report_boot_failure(
    guardian_firmware_lifecycle_t *lifecycle,
    uint32_t attempted_version_counter);

/* Return public lifecycle diagnostics by value. */
guardian_firmware_status_t guardian_firmware_status(
    const guardian_firmware_lifecycle_t *lifecycle);

/* Encode one fixed public GET_FIRMWARE_STATUS payload. */
guardian_firmware_result_t guardian_firmware_encode_status_payload(
    const guardian_firmware_status_t *status,
    uint8_t *payload,
    uint16_t payload_capacity,
    uint16_t *payload_length);

#endif
