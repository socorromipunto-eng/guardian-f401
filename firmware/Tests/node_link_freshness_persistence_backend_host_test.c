#include "guardian_node_link_freshness_persistence_backend.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
    uint8_t slot_a[
        GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A_CAPACITY];

    uint8_t slot_b[
        GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_B_CAPACITY];

    int fail_next_read;
    int fail_next_erase;
    int fail_next_program;

    size_t partial_program_bytes;
} test_media_t;

static int td_total = 0;
static int td_pass = 0;

static void td(
    const char *name,
    int condition)
{
    td_total += 1;

    if (condition)
    {
        td_pass += 1;
        (void)printf("TD-%02d %s=PASS\n", td_total, name);
    }
    else
    {
        (void)printf("TD-%02d %s=FAIL\n", td_total, name);
    }
}

static uint8_t *slot_bytes(
    test_media_t *media,
    guardian_node_link_persistence_slot_t slot)
{
    if (slot == GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A)
    {
        return media->slot_a;
    }

    if (slot == GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_B)
    {
        return media->slot_b;
    }

    return NULL;
}

static size_t slot_capacity(
    guardian_node_link_persistence_slot_t slot)
{
    if (slot == GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A_CAPACITY;
    }

    if (slot == GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_B)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_B_CAPACITY;
    }

    return 0U;
}

static guardian_node_link_persistence_media_result_t
test_read(
    void *context,
    guardian_node_link_persistence_slot_t slot,
    size_t offset,
    uint8_t *output,
    size_t length)
{
    test_media_t *media;
    uint8_t *bytes;
    size_t capacity;

    if ((context == NULL) || (output == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_INVALID;
    }

    media = (test_media_t *)context;

    if (media->fail_next_read)
    {
        media->fail_next_read = 0;
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE;
    }

    bytes = slot_bytes(media, slot);
    capacity = slot_capacity(slot);

    if ((bytes == NULL) ||
        (offset > capacity) ||
        (length > (capacity - offset)))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE;
    }

    (void)memcpy(
        output,
        &bytes[offset],
        length);

    return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_OK;
}

static guardian_node_link_persistence_media_result_t
test_erase(
    void *context,
    guardian_node_link_persistence_slot_t slot)
{
    test_media_t *media;
    uint8_t *bytes;
    size_t capacity;

    if (context == NULL)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_INVALID;
    }

    media = (test_media_t *)context;

    if (media->fail_next_erase)
    {
        media->fail_next_erase = 0;
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE;
    }

    bytes = slot_bytes(media, slot);
    capacity = slot_capacity(slot);

    if ((bytes == NULL) || (capacity == 0U))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE;
    }

    (void)memset(bytes, 0xFF, capacity);

    return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_OK;
}

static guardian_node_link_persistence_media_result_t
test_program(
    void *context,
    guardian_node_link_persistence_slot_t slot,
    size_t offset,
    const uint8_t *data,
    size_t length)
{
    test_media_t *media;
    uint8_t *bytes;
    size_t capacity;
    size_t index;
    size_t actual_length;

    if ((context == NULL) || (data == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_INVALID;
    }

    media = (test_media_t *)context;

    bytes = slot_bytes(media, slot);
    capacity = slot_capacity(slot);

    if ((bytes == NULL) ||
        (offset > capacity) ||
        (length > (capacity - offset)))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE;
    }

    if (media->fail_next_program)
    {
        media->fail_next_program = 0;
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE;
    }

    actual_length = length;

    if ((media->partial_program_bytes > 0U) &&
        (media->partial_program_bytes < length))
    {
        actual_length = media->partial_program_bytes;
        media->partial_program_bytes = 0U;
    }

    for (index = 0U; index < actual_length; index += 1U)
    {
        /*
         * Model NOR FLASH programming: programmed bits may move only 1 -> 0.
         */
        if ((bytes[offset + index] & data[index]) != data[index])
        {
            return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE;
        }

        bytes[offset + index] =
            (uint8_t)(bytes[offset + index] & data[index]);
    }

    if (actual_length != length)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE;
    }

    return GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_OK;
}

static void media_reset(
    test_media_t *media)
{
    (void)memset(media, 0, sizeof(*media));

    (void)memset(
        media->slot_a,
        0xFF,
        sizeof(media->slot_a));

    (void)memset(
        media->slot_b,
        0xFF,
        sizeof(media->slot_b));
}

static void set_ref(
    char output[GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY],
    const char *value)
{
    size_t length;

    (void)memset(
        output,
        0,
        GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY);

    length = strlen(value);

    if (length >= GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY)
    {
        length =
            GUARDIAN_NODE_LINK_SEMANTIC_REF_CAPACITY - 1U;
    }

    (void)memcpy(output, value, length);
}

static guardian_node_link_freshness_persisted_record_t
make_record(
    uint32_t sequence,
    uint32_t generation)
{
    guardian_node_link_freshness_persisted_record_t record;

    (void)memset(&record, 0, sizeof(record));

    record.schema_version =
        GUARDIAN_NODE_LINK_FRESHNESS_PERSISTENCE_SCHEMA_VERSION;

    record.identity.sender_node_id = 0x1001U;
    record.identity.producer_id = 0x2001U;
    record.identity.key_id = 0x3001U;
    record.identity.signature_algorithm = 0x01U;

    set_ref(
        record.identity.producer_semantic_profile_id,
        "guardian:test:producer:v1");

    set_ref(
        record.identity.consumer_semantic_profile_id,
        "guardian:test:consumer:v1");

    set_ref(
        record.identity.compatibility_contract_id,
        "guardian:test:compat:v1");

    record.accepted_epoch = 7U;
    record.accepted_sequence = sequence;
    record.record_generation = generation;

    return record;
}

static int setup_backend(
    test_media_t *media,
    guardian_node_link_freshness_physical_backend_t *backend,
    guardian_node_link_freshness_persistence_provider_t *provider)
{
    guardian_node_link_persistence_media_t adapter;

    (void)memset(&adapter, 0, sizeof(adapter));

    adapter.context = media;
    adapter.read = test_read;
    adapter.erase = test_erase;
    adapter.program = test_program;

    if (guardian_node_link_freshness_physical_backend_init(
            backend,
            &adapter) !=
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK)
    {
        return 0;
    }

    return
        guardian_node_link_freshness_physical_backend_make_provider(
            backend,
            provider) ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
}

static int commit_record(
    guardian_node_link_freshness_persistence_provider_t *provider,
    const guardian_node_link_freshness_persisted_record_t *previous,
    const guardian_node_link_freshness_persisted_record_t *candidate)
{
    guardian_node_link_freshness_persistence_transaction_outcome_t outcome;

    outcome =
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_INVALID;

    if (guardian_node_link_freshness_persistence_transact_commit(
            provider,
            previous,
            &candidate->identity,
            candidate,
            &outcome) !=
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK)
    {
        return 0;
    }

    return
        outcome ==
        GUARDIAN_NODE_LINK_PERSISTENCE_TRANSACTION_COMMITTED;
}

int main(void)
{
    static test_media_t media;

    guardian_node_link_freshness_physical_backend_t backend;
    guardian_node_link_freshness_physical_backend_t rebooted;

    guardian_node_link_freshness_persistence_provider_t provider;
    guardian_node_link_freshness_persistence_provider_t rebooted_provider;

    guardian_node_link_freshness_persistence_status_t status;
    guardian_node_link_freshness_persisted_record_t loaded;
    guardian_node_link_persistence_recovery_diagnostic_t diagnostic;

    guardian_node_link_freshness_persisted_record_t r0;
    guardian_node_link_freshness_persisted_record_t r1;
    guardian_node_link_freshness_persisted_record_t r2;

    guardian_node_link_freshness_persistence_operation_result_t operation;

    uint8_t saved_generation_zero[
        GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_IMAGE_SIZE];

    int ok;

    media_reset(&media);

    ok = setup_backend(
        &media,
        &backend,
        &provider);

    td("backend_init", ok);

    status = GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_INVALID;
    diagnostic = GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_INVALID;
    (void)memset(&loaded, 0xA5, sizeof(loaded));

    operation =
        guardian_node_link_freshness_physical_backend_recover(
            &backend,
            &status,
            &loaded,
            &diagnostic);

    td(
        "erased_media_is_no_state",
        (operation ==
         GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK) &&
        (status ==
         GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_NO_PERSISTED_STATE) &&
        (diagnostic ==
         GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_NO_STATE));

    r0 = make_record(11U, 0U);

    td(
        "bootstrap_commit_generation_zero",
        commit_record(
            &provider,
            NULL,
            &r0));

    (void)memcpy(
        saved_generation_zero,
        media.slot_a,
        sizeof(saved_generation_zero));

    status = GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_INVALID;
    diagnostic = GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_INVALID;
    (void)memset(&loaded, 0, sizeof(loaded));

    operation =
        guardian_node_link_freshness_physical_backend_recover(
            &backend,
            &status,
            &loaded,
            &diagnostic);

    td(
        "bootstrap_load_valid",
        (operation ==
         GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK) &&
        (status ==
         GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE) &&
        (loaded.record_generation == 0U) &&
        (loaded.accepted_sequence == 11U));

    r1 = make_record(12U, 1U);

    td(
        "continuation_commit_generation_one",
        commit_record(
            &provider,
            &r0,
            &r1));

    status = GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_INVALID;
    diagnostic = GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_INVALID;
    (void)memset(&loaded, 0, sizeof(loaded));

    operation =
        guardian_node_link_freshness_physical_backend_recover(
            &backend,
            &status,
            &loaded,
            &diagnostic);

    td(
        "two_valid_adjacent_select_higher",
        (operation ==
         GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK) &&
        (loaded.record_generation == 1U) &&
        (loaded.accepted_sequence == 12U));

    /*
     * Simulate reboot: backend RAM pending state disappears, physical bytes
     * remain.
     */
    ok = setup_backend(
        &media,
        &rebooted,
        &rebooted_provider);

    td("reboot_backend_init", ok);

    status = GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_INVALID;
    diagnostic = GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_INVALID;
    (void)memset(&loaded, 0, sizeof(loaded));

    operation =
        rebooted_provider.load(
            rebooted_provider.context,
            &status,
            &loaded);

    td(
        "cross_reboot_restore_latest",
        (operation ==
         GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK) &&
        (status ==
         GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE) &&
        (loaded.record_generation == 1U));

    r2 = make_record(13U, 2U);

    /*
     * Full staged candidate, but no commit marker.
     */
    operation =
        rebooted_provider.write_candidate(
            rebooted_provider.context,
            &r2);

    td(
        "stage_candidate_without_commit",
        operation ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK);

    ok = setup_backend(
        &media,
        &rebooted,
        &rebooted_provider);

    td("reboot_after_uncommitted_stage", ok);

    status = GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_INVALID;
    diagnostic = GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_INVALID;
    (void)memset(&loaded, 0, sizeof(loaded));

    operation =
        guardian_node_link_freshness_physical_backend_recover(
            &rebooted,
            &status,
            &loaded,
            &diagnostic);

    td(
        "torn_candidate_preserves_previous_commit",
        (operation ==
         GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK) &&
        (status ==
         GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE) &&
        (diagnostic ==
         GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_VALID_WITH_TORN_PEER) &&
        (loaded.record_generation == 1U));

    /*
     * Erase the torn inactive slot before next scenario.
     */
    (void)test_erase(
        &media,
        GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A);

    /*
     * Partial payload/header program after erase.
     */
    media.partial_program_bytes = 31U;

    operation =
        rebooted_provider.write_candidate(
            rebooted_provider.context,
            &r2);

    td(
        "partial_stage_reports_failure",
        operation ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_IO_FAILURE);

    ok = setup_backend(
        &media,
        &rebooted,
        &rebooted_provider);

    td("reboot_after_partial_stage", ok);

    status = GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_INVALID;
    diagnostic = GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_INVALID;
    (void)memset(&loaded, 0, sizeof(loaded));

    operation =
        guardian_node_link_freshness_physical_backend_recover(
            &rebooted,
            &status,
            &loaded,
            &diagnostic);

    td(
        "partial_stage_preserves_previous_commit",
        (operation ==
         GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK) &&
        (diagnostic ==
         GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_VALID_WITH_TORN_PEER) &&
        (loaded.record_generation == 1U));

    /*
     * Clean torn slot and stage again.
     */
    (void)test_erase(
        &media,
        GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A);

    operation =
        rebooted_provider.write_candidate(
            rebooted_provider.context,
            &r2);

    td(
        "stage_before_partial_commit",
        operation ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK);

    operation =
        rebooted_provider.verify_candidate(
            rebooted_provider.context,
            &r2);

    td(
        "readback_verify_before_commit",
        operation ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK);

    /*
     * Simulate power loss while commit marker is being programmed.
     */
    media.partial_program_bytes = 2U;

    operation =
        rebooted_provider.commit_candidate(
            rebooted_provider.context,
            &r2);

    td(
        "partial_commit_marker_is_uncertain",
        operation ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN);

    ok = setup_backend(
        &media,
        &rebooted,
        &rebooted_provider);

    td("reboot_after_partial_commit_marker", ok);

    status = GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_INVALID;
    diagnostic = GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_INVALID;
    (void)memset(&loaded, 0, sizeof(loaded));

    operation =
        guardian_node_link_freshness_physical_backend_recover(
            &rebooted,
            &status,
            &loaded,
            &diagnostic);

    td(
        "partial_commit_marker_fails_closed",
        (operation ==
         GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN) &&
        (status ==
         GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_UNAVAILABLE_STATE) &&
        (diagnostic ==
         GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_STATE_UNCERTAIN));

    /*
     * Restore a clean known state: generation 0 in A, erased B.
     */
    media_reset(&media);

    (void)memcpy(
        media.slot_a,
        saved_generation_zero,
        sizeof(saved_generation_zero));

    ok = setup_backend(
        &media,
        &backend,
        &provider);

    td("known_state_restore", ok);

    r1 = make_record(12U, 1U);

    td(
        "recommit_generation_one",
        commit_record(
            &provider,
            &r0,
            &r1));

    /*
     * Corrupt a committed newer payload while the older slot is still valid.
     * Automatic fallback would permit silent rollback, so recovery must stop.
     */
    media.slot_b[
        GUARDIAN_NODE_LINK_PERSISTENCE_PHYSICAL_PAYLOAD_OFFSET + 7U] ^=
        0x01U;

    status = GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_INVALID;
    diagnostic = GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_INVALID;
    (void)memset(&loaded, 0, sizeof(loaded));

    operation =
        guardian_node_link_freshness_physical_backend_recover(
            &backend,
            &status,
            &loaded,
            &diagnostic);

    td(
        "committed_newer_corruption_no_old_fallback",
        (operation ==
         GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN) &&
        (status ==
         GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_UNAVAILABLE_STATE));

    /*
     * Both erased remains legitimate no-state; it is not inferred as a valid
     * previously-established freshness state.
     */
    media_reset(&media);

    ok = setup_backend(
        &media,
        &backend,
        &provider);

    td("reset_media_backend", ok);

    media.fail_next_read = 1;

    status = GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_INVALID;
    diagnostic = GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_INVALID;
    (void)memset(&loaded, 0, sizeof(loaded));

    operation =
        guardian_node_link_freshness_physical_backend_recover(
            &backend,
            &status,
            &loaded,
            &diagnostic);

    td(
        "read_failure_is_unavailable",
        (operation ==
         GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_IO_FAILURE) &&
        (status ==
         GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_UNAVAILABLE_STATE) &&
        (diagnostic ==
         GUARDIAN_NODE_LINK_PERSISTENCE_RECOVERY_IO_FAILURE));

    /*
     * Erase failure during bootstrap must not report committed.
     */
    media_reset(&media);

    ok = setup_backend(
        &media,
        &backend,
        &provider);

    td("erase_failure_backend_init", ok);

    media.fail_next_erase = 1;

    operation =
        provider.write_candidate(
            provider.context,
            &r0);

    td(
        "erase_failure_fails_transaction_stage",
        operation ==
        GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_IO_FAILURE);

    /*
     * Raw NOR rule: programming cannot move a bit 0 -> 1 without erase.
     */
    media_reset(&media);

    media.slot_a[0] = 0x00U;

    {
        const uint8_t value = 0xFFU;

        td(
            "nor_zero_to_one_rejected",
            test_program(
                &media,
                GUARDIAN_NODE_LINK_PERSISTENCE_SLOT_A,
                0U,
                &value,
                1U) ==
            GUARDIAN_NODE_LINK_PERSISTENCE_MEDIA_IO_FAILURE);
    }

    (void)printf("TD_TOTAL=%d\n", td_total);
    (void)printf("TD_PASS_COUNT=%d\n", td_pass);
    (void)printf(
        "TD_FAIL_COUNT=%d\n",
        td_total - td_pass);

    if (td_total != td_pass)
    {
        (void)printf(
            "C5_R3C_E_C1_C2_HOST_TECHNICAL_DESTRUCTION=FAIL\n");

        return 1;
    }

    (void)printf(
        "C5_R3C_E_C1_C2_HOST_TECHNICAL_DESTRUCTION=PASS\n");

    (void)printf(
        "PHYSICAL_BACKEND_PORTABLE_IMPLEMENTED=YES\n");

    (void)printf(
        "HOST_POWER_LOSS_FAULT_INJECTION_VALIDATED=YES\n");

    (void)printf(
        "STM32_HAL_FLASH_ADAPTER_IMPLEMENTED=NO\n");

    (void)printf(
        "STM32_LINKER_RESERVATION_DEMONSTRATED=NO\n");

    (void)printf(
        "HARDWARE_POWER_LOSS_VALIDATED=NO\n");

    (void)printf(
        "ROLLBACK_ANCHOR_DEMONSTRATED=NO\n");

    (void)printf(
        "PERSISTENT_ANTI_REPLAY_DEMONSTRATED=NO\n");

    return 0;
}