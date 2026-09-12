#include "guardian_node_link_freshness_persistence_backend.h"

#include "guardian_crypto.h"

#include <limits.h>
#include <string.h>

#define R3E_MAGIC_0 ((uint8_t)0x47U)
#define R3E_MAGIC_1 ((uint8_t)0x50U)
#define R3E_MAGIC_2 ((uint8_t)0x45U)
#define R3E_MAGIC_3 ((uint8_t)0x31U)

#define R3E_HEADER_HASH_MATERIAL_SIZE ((size_t)48U)
#define R3E_HEADER_HASH_OFFSET ((size_t)48U)

#define R3E_PAYLOAD_LENGTH_OFFSET ((size_t)8U)
#define R3E_GENERATION_OFFSET ((size_t)12U)
#define R3E_PAYLOAD_HASH_OFFSET ((size_t)16U)

typedef enum
{
    R3E_SLOT_INVALID = 0,
    R3E_SLOT_ERASED,
    R3E_SLOT_TORN,
    R3E_SLOT_CORRUPTED,
    R3E_SLOT_VALID,
    R3E_SLOT_IO_FAILURE
} r3e_slot_state_t;

typedef struct
{
    r3e_slot_state_t state;
    guardian_node_link_freshness_persisted_record_t record;
} r3e_slot_observation_t;

static void r3e_zero_record(
    guardian_node_link_freshness_persisted_record_t *record)
{
    if (record != NULL)
    {
        (void)memset(record, 0, sizeof(*record));
    }
}

static void r3e_put_u16_be(
    uint8_t *output,
    uint16_t value)
{
    output[0] = (uint8_t)((value >> 8U) & 0xFFU);
    output[1] = (uint8_t)(value & 0xFFU);
}

static uint16_t r3e_get_u16_be(
    const uint8_t *input)
{
    return (uint16_t)(
        ((uint16_t)input[0] << 8U) |
        (uint16_t)input[1]);
}

static void r3e_put_u32_be(
    uint8_t *output,
    uint32_t value)
{
    output[0] = (uint8_t)((value >> 24U) & 0xFFU);
    output[1] = (uint8_t)((value >> 16U) & 0xFFU);
    output[2] = (uint8_t)((value >> 8U) & 0xFFU);
    output[3] = (uint8_t)(value & 0xFFU);
}

static uint32_t r3e_get_u32_be(
    const uint8_t *input)
{
    return
        ((uint32_t)input[0] << 24U) |
        ((uint32_t)input[1] << 16U) |
        ((uint32_t)input[2] << 8U) |
        (uint32_t)input[3];
}

static int r3e_bytes_all_ff(
    const uint8_t *data,
    size_t length)
{
    size_t index;

    if (data == NULL)
    {
        return 0;
    }

    for (index = 0U; index < length; index += 1U)
    {
        if (data[index] != 0xFFU)
        {
            return 0;
        }
    }

    return 1;
}

static int r3e_commit_marker_is_valid(
    const uint8_t *marker)
{
    return
        (marker[0] == (uint8_t)'C') &&
        (marker[1] == (uint8_t)'M') &&
        (marker[2] == (uint8_t)'T') &&
        (marker[3] == (uint8_t)'1');
}

static int r3e_records_equal(
    const guardian_node_link_freshness_persisted_record_t *left,
    const guardian_node_link_freshness_persisted_record_t *right)
{
    uint8_t left_bytes[
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZED_SIZE];

    uint8_t right_bytes[
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZED_SIZE];

    size_t left_written;
    size_t right_written;

    if ((left == NULL) || (right == NULL))
    {
        return 0;
    }

    left_written = 0U;
    right_written = 0U;

    if (guardian_node_link_freshness_persistence_encode(
            left,
            left_bytes,
            sizeof(left_bytes),
            &left_written) !=
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_OK)
    {
        return 0;
    }

    if (guardian_node_link_freshness_persistence_encode(
            right,
            right_bytes,
            sizeof(right_bytes),
            &right_written) !=
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_OK)
    {
        return 0;
    }

    if ((left_written != sizeof(left_bytes)) ||
        (right_written != sizeof(right_bytes)))
    {
        return 0;
    }

    return guardian_crypto_constant_time_equal(
        left_bytes,
        right_bytes,
        sizeof(left_bytes));
}

static int r3e_identity_equal(
    const guardian_node_link_freshness_identity_t *left,
    const guardian_node_link_freshness_identity_t *right)
{
    if ((left == NULL) || (right == NULL))
    {
        return 0;
    }

    if ((left->sender_node_id != right->sender_node_id) ||
        (left->producer_id != right->producer_id) ||
        (left->key_id != right->key_id) ||
        (left->signature_algorithm != right->signature_algorithm))
    {
        return 0;
    }

    if (memcmp(
            left->producer_semantic_profile_id,
            right->producer_semantic_profile_id,
            GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY) != 0)
    {
        return 0;
    }

    if (memcmp(
            left->consumer_semantic_profile_id,
            right->consumer_semantic_profile_id,
            GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY) != 0)
    {
        return 0;
    }

    return memcmp(
        left->compatibility_contract_id,
        right->compatibility_contract_id,
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY) == 0;
}

static int r3e_transition_is_adjacent(
    const guardian_node_link_freshness_persisted_record_t *older,
    const guardian_node_link_freshness_persisted_record_t *newer)
{
    if ((older == NULL) || (newer == NULL))
    {
        return 0;
    }

    if (!r3e_identity_equal(
            &older->identity,
            &newer->identity))
    {
        return 0;
    }

    if (older->accepted_epoch != newer->accepted_epoch)
    {
        return 0;
    }

    if (older->record_generation == UINT32_MAX)
    {
        return 0;
    }

    if (newer->record_generation !=
        (older->record_generation + 1U))
    {
        return 0;
    }

    return newer->accepted_sequence >
        older->accepted_sequence;
}

static guardian_node_link_freshness_persistence_operation_result_t
r3e_media_result_to_operation(
    guardian_node_link_persistence_media_result_t media_result)
{
    switch (media_result)
    {
        case GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_OK:
            return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;

        case GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_UNAVAILABLE:
            return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_UNAVAILABLE;

        case GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE:
            return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_IO_FAILURE;

        case GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_INVALID:
        default:
            return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }
}

static int r3e_validate_body(
    const uint8_t image[
        GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_IMAGE_SIZE],
    guardian_node_link_freshness_persisted_record_t *record)
{
    uint8_t header_digest[GUARDIAN_SHA256_SIZE];
    uint8_t payload_digest[GUARDIAN_SHA256_SIZE];
    guardian_node_link_freshness_persisted_record_t candidate;

    if ((image == NULL) || (record == NULL))
    {
        return 0;
    }

    r3e_zero_record(record);
    r3e_zero_record(&candidate);

    if ((image[0] != R3E_MAGIC_0) ||
        (image[1] != R3E_MAGIC_1) ||
        (image[2] != R3E_MAGIC_2) ||
        (image[3] != R3E_MAGIC_3))
    {
        return 0;
    }

    if (r3e_get_u16_be(&image[4]) !=
        GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_FORMAT_VERSION)
    {
        return 0;
    }

    if (r3e_get_u16_be(&image[6]) !=
        GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_HEADER_SIZE)
    {
        return 0;
    }

    if (r3e_get_u32_be(
            &image[R3E_PAYLOAD_LENGTH_OFFSET]) !=
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZED_SIZE)
    {
        return 0;
    }

    guardian_sha256(
        image,
        R3E_HEADER_HASH_MATERIAL_SIZE,
        header_digest);

    if (!guardian_crypto_constant_time_equal(
            header_digest,
            &image[R3E_HEADER_HASH_OFFSET],
            GUARDIAN_SHA256_SIZE))
    {
        return 0;
    }

    guardian_sha256(
        &image[
            GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_PAYLOAD_OFFSET],
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZED_SIZE,
        payload_digest);

    if (!guardian_crypto_constant_time_equal(
            payload_digest,
            &image[R3E_PAYLOAD_HASH_OFFSET],
            GUARDIAN_SHA256_SIZE))
    {
        return 0;
    }

    if (guardian_node_link_freshness_persistence_decode(
            &image[
                GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_PAYLOAD_OFFSET],
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZED_SIZE,
            &candidate) !=
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_OK)
    {
        return 0;
    }

    if (r3e_get_u32_be(
            &image[R3E_GENERATION_OFFSET]) !=
        candidate.record_generation)
    {
        return 0;
    }

    *record = candidate;

    return 1;
}

static int r3e_build_staged_image(
    const guardian_node_link_freshness_persisted_record_t *record,
    uint8_t image[
        GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_IMAGE_SIZE])
{
    uint8_t digest[GUARDIAN_SHA256_SIZE];
    size_t payload_written;

    if ((record == NULL) || (image == NULL))
    {
        return 0;
    }

    (void)memset(
        image,
        0xFF,
        GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_IMAGE_SIZE);

    image[0] = R3E_MAGIC_0;
    image[1] = R3E_MAGIC_1;
    image[2] = R3E_MAGIC_2;
    image[3] = R3E_MAGIC_3;

    r3e_put_u16_be(
        &image[4],
        GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_FORMAT_VERSION);

    r3e_put_u16_be(
        &image[6],
        (uint16_t)
        GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_HEADER_SIZE);

    r3e_put_u32_be(
        &image[R3E_PAYLOAD_LENGTH_OFFSET],
        (uint32_t)
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZED_SIZE);

    r3e_put_u32_be(
        &image[R3E_GENERATION_OFFSET],
        record->record_generation);

    payload_written = 0U;

    if (guardian_node_link_freshness_persistence_encode(
            record,
            &image[
                GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_PAYLOAD_OFFSET],
            GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZED_SIZE,
            &payload_written) !=
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_OK)
    {
        return 0;
    }

    if (payload_written !=
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZED_SIZE)
    {
        return 0;
    }

    guardian_sha256(
        &image[
            GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_PAYLOAD_OFFSET],
        GUARDIAN_NODE_LINK_PERSISTENCE_CODEC_SERIALIZED_SIZE,
        digest);

    (void)memcpy(
        &image[R3E_PAYLOAD_HASH_OFFSET],
        digest,
        GUARDIAN_SHA256_SIZE);

    guardian_sha256(
        image,
        R3E_HEADER_HASH_MATERIAL_SIZE,
        digest);

    (void)memcpy(
        &image[R3E_HEADER_HASH_OFFSET],
        digest,
        GUARDIAN_SHA256_SIZE);

    return 1;
}

static guardian_node_link_freshness_persistence_operation_result_t
r3e_scan_slot(
    guardian_node_link_freshness_physical_backend_t *backend,
    guardian_node_link_persistence_slot_t slot,
    r3e_slot_observation_t *observation)
{
    uint8_t image[
        GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_IMAGE_SIZE];

    guardian_node_link_persistence_media_result_t media_result;

    if ((backend == NULL) || (observation == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    (void)memset(
        observation,
        0,
        sizeof(*observation));

    observation->state = R3E_SLOT_INVALID;

    media_result =
        backend->media.read(
            backend->media.context,
            slot,
            0U,
            image,
            sizeof(image));

    if (media_result !=
        GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_OK)
    {
        observation->state = R3E_SLOT_IO_FAILURE;

        return r3e_media_result_to_operation(media_result);
    }

    if (r3e_bytes_all_ff(image, sizeof(image)))
    {
        observation->state = R3E_SLOT_ERASED;
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
    }

    if (r3e_bytes_all_ff(
            &image[
                GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_COMMIT_OFFSET],
            GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_COMMIT_SIZE))
    {
        observation->state = R3E_SLOT_TORN;
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
    }

    if (!r3e_commit_marker_is_valid(
            &image[
                GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_COMMIT_OFFSET]))
    {
        observation->state = R3E_SLOT_CORRUPTED;
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
    }

    if (!r3e_validate_body(
            image,
            &observation->record))
    {
        observation->state = R3E_SLOT_CORRUPTED;
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
    }

    observation->state = R3E_SLOT_VALID;

    return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
}

static guardian_node_link_freshness_persistence_operation_result_t
r3e_recover_internal(
    guardian_node_link_freshness_physical_backend_t *backend,
    guardian_node_link_freshness_persistence_status_t *provider_status,
    guardian_node_link_freshness_persisted_record_t *record,
    guardian_node_link_persistence_recovery_diagnostic_t *diagnostic,
    guardian_node_link_persistence_slot_t *selected_slot)
{
    r3e_slot_observation_t a;
    r3e_slot_observation_t b;

    guardian_node_link_freshness_persistence_operation_result_t op_a;
    guardian_node_link_freshness_persistence_operation_result_t op_b;

    const r3e_slot_observation_t *valid;
    const r3e_slot_observation_t *peer;

    guardian_node_link_persistence_slot_t valid_slot;

    if ((backend == NULL) ||
        (provider_status == NULL) ||
        (record == NULL) ||
        (diagnostic == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    *provider_status =
        GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_INVALID;

    *diagnostic =
        GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_INVALID;

    r3e_zero_record(record);

    op_a = r3e_scan_slot(
        backend,
        GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A,
        &a);

    op_b = r3e_scan_slot(
        backend,
        GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_B,
        &b);

    if ((op_a != GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK) ||
        (op_b != GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK))
    {
        *provider_status =
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_UNAVAILABLE_STATE;

        *diagnostic =
            GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_IO_FAILURE;

        if ((op_a ==
             GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_UNAVAILABLE) ||
            (op_b ==
             GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_UNAVAILABLE))
        {
            return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_UNAVAILABLE;
        }

        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_IO_FAILURE;
    }

    if ((a.state == R3E_SLOT_ERASED) &&
        (b.state == R3E_SLOT_ERASED))
    {
        *provider_status =
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_NO_PERSISTED_STATE;

        *diagnostic =
            GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_NO_STATE;

        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
    }

    if ((a.state == R3E_SLOT_VALID) &&
        (b.state == R3E_SLOT_VALID))
    {
        if (a.record.record_generation <
            b.record.record_generation)
        {
            if (!r3e_transition_is_adjacent(
                    &a.record,
                    &b.record))
            {
                *provider_status =
                    GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_UNAVAILABLE_STATE;

                *diagnostic =
                    GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_STATE_UNCERTAIN;

                return
                    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN;
            }

            *record = b.record;

            if (selected_slot != NULL)
            {
                *selected_slot =
                    GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_B;
            }
        }
        else if (b.record.record_generation <
                 a.record.record_generation)
        {
            if (!r3e_transition_is_adjacent(
                    &b.record,
                    &a.record))
            {
                *provider_status =
                    GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_UNAVAILABLE_STATE;

                *diagnostic =
                    GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_STATE_UNCERTAIN;

                return
                    GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN;
            }

            *record = a.record;

            if (selected_slot != NULL)
            {
                *selected_slot =
                    GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A;
            }
        }
        else
        {
            /*
             * Two committed erase domains carrying the same generation
             * violate the alternating-slot invariant, even if payloads happen
             * to be identical. Do not silently choose one.
             */
            *provider_status =
                GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_UNAVAILABLE_STATE;

            *diagnostic =
                GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_STATE_UNCERTAIN;

            return
                GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN;
        }

        *provider_status =
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE;

        *diagnostic =
            GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_VALID;

        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
    }

    if ((a.state == R3E_SLOT_VALID) ||
        (b.state == R3E_SLOT_VALID))
    {
        if (a.state == R3E_SLOT_VALID)
        {
            valid = &a;
            peer = &b;
            valid_slot = GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A;
        }
        else
        {
            valid = &b;
            peer = &a;
            valid_slot = GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_B;
        }

        /*
         * A committed-looking corrupted peer may represent corruption of a
         * newer previously committed generation. Falling back automatically
         * could silently restore older accepted freshness state.
         */
        if (peer->state == R3E_SLOT_CORRUPTED)
        {
            *provider_status =
                GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_UNAVAILABLE_STATE;

            *diagnostic =
                GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_STATE_UNCERTAIN;

            return
                GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN;
        }

        if ((peer->state != R3E_SLOT_ERASED) &&
            (peer->state != R3E_SLOT_TORN))
        {
            *provider_status =
                GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_UNAVAILABLE_STATE;

            *diagnostic =
                GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_STATE_UNCERTAIN;

            return
                GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN;
        }

        *record = valid->record;

        if (selected_slot != NULL)
        {
            *selected_slot = valid_slot;
        }

        *provider_status =
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE;

        if (peer->state == R3E_SLOT_TORN)
        {
            *diagnostic =
                GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_VALID_WITH_TORN_PEER;
        }
        else
        {
            *diagnostic =
                GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_VALID;
        }

        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
    }

    if ((a.state == R3E_SLOT_CORRUPTED) ||
        (b.state == R3E_SLOT_CORRUPTED))
    {
        *provider_status =
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_CORRUPTED_STATE;

        *diagnostic =
            GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_CORRUPTED;

        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
    }

    if ((a.state == R3E_SLOT_TORN) ||
        (b.state == R3E_SLOT_TORN))
    {
        *provider_status =
            GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_TORN_OR_INCOMPLETE_UPDATE;

        *diagnostic =
            GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_TORN_ONLY;

        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
    }

    *provider_status =
        GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_UNAVAILABLE_STATE;

    *diagnostic =
        GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_STATE_UNCERTAIN;

    return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN;
}

guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_freshness_physical_backend_init(
    guardian_node_link_freshness_physical_backend_t *backend,
    const guardian_node_link_persistence_media_t *media)
{
    if ((backend == NULL) ||
        (media == NULL) ||
        (media->read == NULL) ||
        (media->erase == NULL) ||
        (media->program == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    (void)memset(backend, 0, sizeof(*backend));

    backend->media = *media;
    backend->pending_slot =
        GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A;
    backend->pending_valid = 0U;

    return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
}

guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_freshness_physical_backend_make_provider(
    guardian_node_link_freshness_physical_backend_t *backend,
    guardian_node_link_freshness_persistence_provider_t *provider)
{
    if ((backend == NULL) || (provider == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    (void)memset(provider, 0, sizeof(*provider));

    provider->context = backend;
    provider->load =
        guardian_node_link_freshness_physical_backend_load;
    provider->write_candidate =
        guardian_node_link_freshness_physical_backend_write_candidate;
    provider->verify_candidate =
        guardian_node_link_freshness_physical_backend_verify_candidate;
    provider->commit_candidate =
        guardian_node_link_freshness_physical_backend_commit_candidate;

    return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
}

guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_freshness_physical_backend_recover(
    guardian_node_link_freshness_physical_backend_t *backend,
    guardian_node_link_freshness_persistence_status_t *provider_status,
    guardian_node_link_freshness_persisted_record_t *record,
    guardian_node_link_persistence_recovery_diagnostic_t *diagnostic)
{
    return r3e_recover_internal(
        backend,
        provider_status,
        record,
        diagnostic,
        NULL);
}

guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_freshness_physical_backend_load(
    void *context,
    guardian_node_link_freshness_persistence_status_t *provider_status,
    guardian_node_link_freshness_persisted_record_t *record)
{
    guardian_node_link_persistence_recovery_diagnostic_t diagnostic;

    if ((context == NULL) ||
        (provider_status == NULL) ||
        (record == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    diagnostic =
        GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_INVALID;

    return guardian_node_link_freshness_physical_backend_recover(
        (guardian_node_link_freshness_physical_backend_t *)context,
        provider_status,
        record,
        &diagnostic);
}

guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_freshness_physical_backend_write_candidate(
    void *context,
    const guardian_node_link_freshness_persisted_record_t *candidate)
{
    guardian_node_link_freshness_physical_backend_t *backend;

    guardian_node_link_freshness_persistence_status_t status;
    guardian_node_link_freshness_persisted_record_t current;
    guardian_node_link_persistence_recovery_diagnostic_t diagnostic;
    guardian_node_link_persistence_slot_t current_slot;
    guardian_node_link_persistence_slot_t target_slot;

    guardian_node_link_freshness_persistence_operation_result_t operation;
    guardian_node_link_persistence_media_result_t media_result;

    uint8_t image[
        GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_IMAGE_SIZE];

    if ((context == NULL) || (candidate == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    backend =
        (guardian_node_link_freshness_physical_backend_t *)context;

    backend->pending_valid = 0U;

    status = GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_INVALID;
    diagnostic =
        GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_INVALID;

    r3e_zero_record(&current);

    operation =
        r3e_recover_internal(
            backend,
            &status,
            &current,
            &diagnostic,
            &current_slot);

    if (operation !=
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK)
    {
        return operation;
    }

    if (status ==
        GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_NO_PERSISTED_STATE)
    {
        if (candidate->record_generation != 0U)
        {
            return
                GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_VERIFY_FAILURE;
        }

        target_slot =
            GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A;
    }
    else if (status ==
             GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE)
    {
        if (current.record_generation == UINT32_MAX)
        {
            return
                GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_GENERATION_EXHAUSTED;
        }

        if (!r3e_transition_is_adjacent(
                &current,
                candidate))
        {
            return
                GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_VERIFY_FAILURE;
        }

        target_slot =
            (current_slot ==
             GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A)
                ? GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_B
                : GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A;
    }
    else
    {
        return
            GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN;
    }

    if (!r3e_build_staged_image(
            candidate,
            image))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_VERIFY_FAILURE;
    }

    media_result =
        backend->media.erase(
            backend->media.context,
            target_slot);

    if (media_result !=
        GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_OK)
    {
        return r3e_media_result_to_operation(media_result);
    }

    /*
     * Commit marker is intentionally excluded. It is programmed only by the
     * commit callback after independent read-back verification.
     */
    media_result =
        backend->media.program(
            backend->media.context,
            target_slot,
            0U,
            image,
            GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_COMMIT_OFFSET);

    if (media_result !=
        GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_OK)
    {
        return r3e_media_result_to_operation(media_result);
    }

    backend->pending_slot = target_slot;
    backend->pending_valid = 1U;

    return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
}

guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_freshness_physical_backend_verify_candidate(
    void *context,
    const guardian_node_link_freshness_persisted_record_t *candidate)
{
    guardian_node_link_freshness_physical_backend_t *backend;

    uint8_t image[
        GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_IMAGE_SIZE];

    guardian_node_link_freshness_persisted_record_t decoded;

    guardian_node_link_persistence_media_result_t media_result;

    if ((context == NULL) || (candidate == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    backend =
        (guardian_node_link_freshness_physical_backend_t *)context;

    if (backend->pending_valid != 1U)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_VERIFY_FAILURE;
    }

    media_result =
        backend->media.read(
            backend->media.context,
            backend->pending_slot,
            0U,
            image,
            sizeof(image));

    if (media_result !=
        GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_OK)
    {
        return r3e_media_result_to_operation(media_result);
    }

    if (!r3e_bytes_all_ff(
            &image[
                GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_COMMIT_OFFSET],
            GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_COMMIT_SIZE))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_VERIFY_FAILURE;
    }

    r3e_zero_record(&decoded);

    if (!r3e_validate_body(
            image,
            &decoded))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_VERIFY_FAILURE;
    }

    if (!r3e_records_equal(
            &decoded,
            candidate))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_VERIFY_FAILURE;
    }

    return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
}

guardian_node_link_freshness_persistence_operation_result_t
guardian_node_link_freshness_physical_backend_commit_candidate(
    void *context,
    const guardian_node_link_freshness_persisted_record_t *candidate)
{
    static const uint8_t marker[
        GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_COMMIT_SIZE] =
    {
        (uint8_t)'C',
        (uint8_t)'M',
        (uint8_t)'T',
        (uint8_t)'1'
    };

    guardian_node_link_freshness_physical_backend_t *backend;
    guardian_node_link_persistence_media_result_t media_result;

    r3e_slot_observation_t observation;

    if ((context == NULL) || (candidate == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    backend =
        (guardian_node_link_freshness_physical_backend_t *)context;

    if (backend->pending_valid != 1U)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_COMMIT_FAILURE;
    }

    if (guardian_node_link_freshness_physical_backend_verify_candidate(
            backend,
            candidate) !=
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK)
    {
        backend->pending_valid = 0U;

        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_VERIFY_FAILURE;
    }

    media_result =
        backend->media.program(
            backend->media.context,
            backend->pending_slot,
            GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_COMMIT_OFFSET,
            marker,
            sizeof(marker));

    if (media_result !=
        GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_OK)
    {
        backend->pending_valid = 0U;

        /*
         * A failed program operation may have partially changed physical
         * storage. The state after this point is not safely known.
         */
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN;
    }

    (void)memset(
        &observation,
        0,
        sizeof(observation));

    if (r3e_scan_slot(
            backend,
            backend->pending_slot,
            &observation) !=
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK)
    {
        backend->pending_valid = 0U;

        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN;
    }

    if ((observation.state != R3E_SLOT_VALID) ||
        !r3e_records_equal(
            &observation.record,
            candidate))
    {
        backend->pending_valid = 0U;

        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN;
    }

    backend->pending_valid = 0U;

    return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
}