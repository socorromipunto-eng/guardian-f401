#include "guardian_node_link_persistent_anti_replay.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
    guardian_node_link_freshness_persisted_record_t committed;
    guardian_node_link_freshness_persisted_record_t candidate;
    guardian_node_link_freshness_persistence_status_t status;
    uint8_t has_committed;
    uint8_t has_candidate;
    uint8_t fail_write;
    uint8_t fail_verify;
    uint8_t fail_commit;
} test_persistence_store_t;

typedef struct
{
    guardian_node_link_rollback_anchor_record_t record;
    guardian_node_link_rollback_anchor_status_t status;
    uint8_t fail_read;
    uint8_t fail_advance;
    uint8_t corrupt_after_advance;
} test_anchor_store_t;

static void copy_ref(char *dst, size_t cap, const char *src)
{
    size_t i;

    assert(dst != NULL);
    assert(src != NULL);
    assert(cap > 0U);

    for (i = 0U; (i + 1U) < cap && src[i] != '\0'; i += 1U)
    {
        dst[i] = src[i];
    }

    dst[i] = '\0';
}

static guardian_node_link_freshness_identity_t make_identity(void)
{
    guardian_node_link_freshness_identity_t identity;

    (void)memset(&identity, 0, sizeof(identity));
    identity.sender_node_id = 3U;
    identity.producer_id = 3003U;
    identity.key_id = 23U;
    identity.signature_algorithm = GUARDIAN_NODE_LINK_SIGNATURE_ED25519;

    copy_ref(
        identity.producer_semantic_profile_id,
        sizeof(identity.producer_semantic_profile_id),
        "M17-ST-MEASUREMENT-V1");

    copy_ref(
        identity.consumer_semantic_profile_id,
        sizeof(identity.consumer_semantic_profile_id),
        "M17-F401-CONTROL-V1");

    copy_ref(
        identity.compatibility_contract_id,
        sizeof(identity.compatibility_contract_id),
        "M17-NODELINK-COMPAT-V1");

    return identity;
}

static guardian_node_link_freshness_persisted_record_t make_record(
    const guardian_node_link_freshness_identity_t *identity,
    uint32_t epoch,
    uint32_t sequence,
    uint32_t generation)
{
    guardian_node_link_freshness_persisted_record_t record;

    assert(identity != NULL);

    (void)memset(&record, 0, sizeof(record));
    record.schema_version = GUARDIAN_NODE_LINK_FRESHNESS_PERSISTENCE_SCHEMA_VERSION;
    record.identity = *identity;
    record.accepted_epoch = epoch;
    record.accepted_sequence = sequence;
    record.record_generation = generation;

    return record;
}

static guardian_node_link_freshness_persistence_operation_result_t p_load(
    void *context,
    guardian_node_link_freshness_persistence_status_t *provider_status,
    guardian_node_link_freshness_persisted_record_t *record)
{
    test_persistence_store_t *store = (test_persistence_store_t *)context;

    if ((store == NULL) || (provider_status == NULL) || (record == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    *provider_status = store->status;
    (void)memset(record, 0, sizeof(*record));

    if ((store->status == GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE) &&
        (store->has_committed != 0U))
    {
        *record = store->committed;
    }

    return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
}

static guardian_node_link_freshness_persistence_operation_result_t p_write(
    void *context,
    const guardian_node_link_freshness_persisted_record_t *candidate)
{
    test_persistence_store_t *store = (test_persistence_store_t *)context;

    if ((store == NULL) || (candidate == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    if (store->fail_write != 0U)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_IO_FAILURE;
    }

    store->candidate = *candidate;
    store->has_candidate = 1U;
    return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
}

static guardian_node_link_freshness_persistence_operation_result_t p_verify(
    void *context,
    const guardian_node_link_freshness_persisted_record_t *candidate)
{
    test_persistence_store_t *store = (test_persistence_store_t *)context;

    if ((store == NULL) || (candidate == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    if (store->fail_verify != 0U)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_VERIFY_FAILURE;
    }

    if ((store->has_candidate == 0U) ||
        (memcmp(&store->candidate, candidate, sizeof(*candidate)) != 0))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_VERIFY_FAILURE;
    }

    return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
}

static guardian_node_link_freshness_persistence_operation_result_t p_commit(
    void *context,
    const guardian_node_link_freshness_persisted_record_t *candidate)
{
    test_persistence_store_t *store = (test_persistence_store_t *)context;

    if ((store == NULL) || (candidate == NULL))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_INVALID;
    }

    if (store->fail_commit != 0U)
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_COMMIT_FAILURE;
    }

    if ((store->has_candidate == 0U) ||
        (memcmp(&store->candidate, candidate, sizeof(*candidate)) != 0))
    {
        return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_STATE_UNCERTAIN;
    }

    store->committed = *candidate;
    store->has_committed = 1U;
    store->has_candidate = 0U;
    store->status = GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE;

    return GUARDIAN_NODE_LINK_PERSISTENCE_OPERATION_OK;
}

static guardian_node_link_rollback_anchor_provider_result_t a_status(
    void *context,
    const guardian_node_link_freshness_identity_t *identity,
    guardian_node_link_rollback_anchor_status_t *status)
{
    test_anchor_store_t *store = (test_anchor_store_t *)context;
    (void)identity;

    if ((store == NULL) || (status == NULL))
    {
        return GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT;
    }

    *status = store->status;
    return GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK;
}

static guardian_node_link_rollback_anchor_provider_result_t a_read(
    void *context,
    const guardian_node_link_freshness_identity_t *identity,
    guardian_node_link_rollback_anchor_record_t *record,
    guardian_node_link_rollback_anchor_status_t *status)
{
    test_anchor_store_t *store = (test_anchor_store_t *)context;
    (void)identity;

    if ((store == NULL) || (record == NULL) || (status == NULL))
    {
        return GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT;
    }

    if (store->fail_read != 0U)
    {
        *status = GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_UNAVAILABLE;
        return GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_UNAVAILABLE;
    }

    *record = store->record;
    *status = store->status;
    return GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK;
}

static guardian_node_link_rollback_anchor_provider_result_t a_advance(
    void *context,
    const guardian_node_link_freshness_identity_t *identity,
    uint32_t expected_generation,
    uint32_t next_generation,
    const guardian_node_link_rollback_anchor_commitment_t *next_commitment)
{
    test_anchor_store_t *store = (test_anchor_store_t *)context;

    if ((store == NULL) || (identity == NULL) || (next_commitment == NULL))
    {
        return GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_INVALID_ARGUMENT;
    }

    if (store->fail_advance != 0U)
    {
        return GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_IO_ERROR;
    }

    if ((store->status != GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_VALID) ||
        (store->record.generation != expected_generation))
    {
        return GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_CONFLICT;
    }

    store->record.identity = *identity;
    store->record.generation = next_generation;
    store->record.commitment = *next_commitment;

    if (store->corrupt_after_advance != 0U)
    {
        store->record.commitment.bytes[0] ^= 0x01U;
    }

    return GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK;
}

static guardian_node_link_freshness_persistence_provider_t make_p_provider(
    test_persistence_store_t *store)
{
    guardian_node_link_freshness_persistence_provider_t provider;

    (void)memset(&provider, 0, sizeof(provider));
    provider.context = store;
    provider.load = p_load;
    provider.write_candidate = p_write;
    provider.verify_candidate = p_verify;
    provider.commit_candidate = p_commit;

    return provider;
}

static guardian_node_link_rollback_anchor_provider_t make_a_provider(
    test_anchor_store_t *store)
{
    guardian_node_link_rollback_anchor_provider_t provider;

    (void)memset(&provider, 0, sizeof(provider));
    provider.context = store;
    provider.status = a_status;
    provider.read = a_read;
    provider.advance = a_advance;

    return provider;
}

static void initialize_stores(
    test_persistence_store_t *pstore,
    test_anchor_store_t *astore,
    const guardian_node_link_freshness_persisted_record_t *record)
{
    guardian_node_link_rollback_anchor_provider_result_t result;

    assert(pstore != NULL);
    assert(astore != NULL);
    assert(record != NULL);

    (void)memset(pstore, 0, sizeof(*pstore));
    (void)memset(astore, 0, sizeof(*astore));

    pstore->committed = *record;
    pstore->has_committed = 1U;
    pstore->status = GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_VALID_PERSISTED_STATE;

    astore->record.identity = record->identity;
    astore->record.generation = record->record_generation;
    astore->status = GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_VALID;

    result = guardian_node_link_rollback_anchor_commitment_from_record(
        record,
        &astore->record.commitment);

    assert(result == GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_PROVIDER_OK);
}

static void test_recover_matching_state(void)
{
    guardian_node_link_freshness_identity_t identity = make_identity();
    guardian_node_link_freshness_persisted_record_t record = make_record(&identity, 9U, 10U, 0U);
    test_persistence_store_t pstore;
    test_anchor_store_t astore;
    guardian_node_link_freshness_persistence_provider_t p;
    guardian_node_link_rollback_anchor_provider_t a;
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_persisted_record_t restored;
    guardian_node_link_persistent_anti_replay_evidence_t evidence;

    initialize_stores(&pstore, &astore, &record);
    p = make_p_provider(&pstore);
    a = make_a_provider(&astore);

    assert(guardian_node_link_persistent_anti_replay_recover(
               &p, &a, &identity, &runtime, &restored, &evidence) ==
           GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_OK);
    assert(runtime.condition == GUARDIAN_NODE_LINK_FRESHNESS_ACTIVE);
    assert(runtime.accepted_epoch == 9U);
    assert(runtime.accepted_sequence == 10U);
    assert(restored.record_generation == 0U);
    assert(evidence.runtime_restored == 1U);
    assert(evidence.persistent_anti_replay_valid == 1U);
    assert(evidence.actuation_authorized == 0U);
}

static void test_bootstrap_requires_separate_provisioning(void)
{
    guardian_node_link_freshness_identity_t identity = make_identity();
    test_persistence_store_t pstore;
    test_anchor_store_t astore;
    guardian_node_link_freshness_persistence_provider_t p;
    guardian_node_link_rollback_anchor_provider_t a;
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_persisted_record_t restored;
    guardian_node_link_persistent_anti_replay_evidence_t evidence;

    (void)memset(&pstore, 0, sizeof(pstore));
    (void)memset(&astore, 0, sizeof(astore));
    pstore.status = GUARDIAN_NODE_LINK_PERSISTENCE_STATUS_NO_PERSISTED_STATE;
    astore.status = GUARDIAN_NODE_LINK_ROLLBACK_ANCHOR_STATUS_UNPROVISIONED;
    p = make_p_provider(&pstore);
    a = make_a_provider(&astore);

    assert(guardian_node_link_persistent_anti_replay_recover(
               &p, &a, &identity, &runtime, &restored, &evidence) ==
           GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_BOOTSTRAP_REQUIRED);
    assert(runtime.condition == GUARDIAN_NODE_LINK_FRESHNESS_UNINITIALIZED);
}

static void test_rollback_and_mismatch_fail_closed(void)
{
    guardian_node_link_freshness_identity_t identity = make_identity();
    guardian_node_link_freshness_persisted_record_t record = make_record(&identity, 9U, 10U, 0U);
    test_persistence_store_t pstore;
    test_anchor_store_t astore;
    guardian_node_link_freshness_persistence_provider_t p;
    guardian_node_link_rollback_anchor_provider_t a;
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_persisted_record_t restored;
    guardian_node_link_persistent_anti_replay_evidence_t evidence;

    initialize_stores(&pstore, &astore, &record);
    astore.record.generation = 1U;
    p = make_p_provider(&pstore);
    a = make_a_provider(&astore);

    assert(guardian_node_link_persistent_anti_replay_recover(
               &p, &a, &identity, &runtime, &restored, &evidence) ==
           GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_ROLLBACK_SUSPECTED);
    assert(runtime.condition == GUARDIAN_NODE_LINK_FRESHNESS_UNINITIALIZED);

    initialize_stores(&pstore, &astore, &record);
    astore.record.commitment.bytes[0] ^= 0x80U;
    p = make_p_provider(&pstore);
    a = make_a_provider(&astore);

    assert(guardian_node_link_persistent_anti_replay_recover(
               &p, &a, &identity, &runtime, &restored, &evidence) ==
           GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_STATE_MISMATCH);
    assert(runtime.condition == GUARDIAN_NODE_LINK_FRESHNESS_UNINITIALIZED);
}

static void test_persisted_ahead_of_anchor_is_uncertain(void)
{
    guardian_node_link_freshness_identity_t identity = make_identity();
    guardian_node_link_freshness_persisted_record_t record0 = make_record(&identity, 9U, 10U, 0U);
    guardian_node_link_freshness_persisted_record_t record1 = make_record(&identity, 9U, 11U, 1U);
    test_persistence_store_t pstore;
    test_anchor_store_t astore;
    guardian_node_link_freshness_persistence_provider_t p;
    guardian_node_link_rollback_anchor_provider_t a;
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_persisted_record_t restored;
    guardian_node_link_persistent_anti_replay_evidence_t evidence;

    initialize_stores(&pstore, &astore, &record0);
    pstore.committed = record1;
    p = make_p_provider(&pstore);
    a = make_a_provider(&astore);

    assert(guardian_node_link_persistent_anti_replay_recover(
               &p, &a, &identity, &runtime, &restored, &evidence) ==
           GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_STATE_UNCERTAIN);
    assert(runtime.condition == GUARDIAN_NODE_LINK_FRESHNESS_UNINITIALIZED);
}

static void test_valid_continuation_updates_only_after_anchor_readback(void)
{
    guardian_node_link_freshness_identity_t identity = make_identity();
    guardian_node_link_freshness_persisted_record_t record = make_record(&identity, 9U, 10U, 0U);
    test_persistence_store_t pstore;
    test_anchor_store_t astore;
    guardian_node_link_freshness_persistence_provider_t p;
    guardian_node_link_rollback_anchor_provider_t a;
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_persisted_record_t committed;
    guardian_node_link_persistent_anti_replay_evidence_t evidence;

    initialize_stores(&pstore, &astore, &record);
    p = make_p_provider(&pstore);
    a = make_a_provider(&astore);
    {
        guardian_node_link_freshness_persisted_record_t restored;
        guardian_node_link_persistent_anti_replay_evidence_t recovery_evidence;
        assert(guardian_node_link_persistent_anti_replay_recover(
                   &p, &a, &identity, &runtime, &restored, &recovery_evidence) ==
               GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_OK);
    }

    assert(guardian_node_link_persistent_anti_replay_commit_continuation(
               &p, &a, &identity, &record, 9U, 11U,
               &runtime, &committed, &evidence) ==
           GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_OK);
    assert(committed.record_generation == 1U);
    assert(committed.accepted_sequence == 11U);
    assert(runtime.condition == GUARDIAN_NODE_LINK_FRESHNESS_ACTIVE);
    assert(runtime.accepted_sequence == 11U);
    assert(evidence.persistence_committed == 1U);
    assert(evidence.anchor_advanced == 1U);
    assert(evidence.anchor_readback_verified == 1U);
    assert(evidence.persistent_anti_replay_valid == 1U);
    assert(evidence.actuation_authorized == 0U);
}

static void test_replay_epoch_and_exhaustion_rejected(void)
{
    guardian_node_link_freshness_identity_t identity = make_identity();
    guardian_node_link_freshness_persisted_record_t record = make_record(&identity, 9U, 10U, 0U);
    test_persistence_store_t pstore;
    test_anchor_store_t astore;
    guardian_node_link_freshness_persistence_provider_t p;
    guardian_node_link_rollback_anchor_provider_t a;
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_persisted_record_t committed;
    guardian_node_link_persistent_anti_replay_evidence_t evidence;

    initialize_stores(&pstore, &astore, &record);
    p = make_p_provider(&pstore);
    a = make_a_provider(&astore);
    {
        guardian_node_link_freshness_persisted_record_t restored;
        guardian_node_link_persistent_anti_replay_evidence_t recovery_evidence;
        assert(guardian_node_link_persistent_anti_replay_recover(
                   &p, &a, &identity, &runtime, &restored, &recovery_evidence) ==
               GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_OK);
    }

    assert(guardian_node_link_persistent_anti_replay_commit_continuation(
               &p, &a, &identity, &record, 9U, 10U,
               &runtime, &committed, &evidence) ==
           GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_REPLAY_OR_REGRESSION);

    assert(guardian_node_link_persistent_anti_replay_commit_continuation(
               &p, &a, &identity, &record, 10U, 11U,
               &runtime, &committed, &evidence) ==
           GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_EPOCH_TRANSITION_REQUIRED);

    record.record_generation = UINT32_MAX - 1U;
    initialize_stores(&pstore, &astore, &record);
    p = make_p_provider(&pstore);
    a = make_a_provider(&astore);
    {
        guardian_node_link_freshness_persisted_record_t restored;
        guardian_node_link_persistent_anti_replay_evidence_t recovery_evidence;
        assert(guardian_node_link_persistent_anti_replay_recover(
                   &p, &a, &identity, &runtime, &restored, &recovery_evidence) ==
               GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_OK);
    }

    assert(guardian_node_link_persistent_anti_replay_commit_continuation(
               &p, &a, &identity, &record, 9U, 11U,
               &runtime, &committed, &evidence) ==
           GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_GENERATION_EXHAUSTED);
}

static void test_partial_failures_never_promote_runtime(void)
{
    guardian_node_link_freshness_identity_t identity = make_identity();
    guardian_node_link_freshness_persisted_record_t record = make_record(&identity, 9U, 10U, 0U);
    test_persistence_store_t pstore;
    test_anchor_store_t astore;
    guardian_node_link_freshness_persistence_provider_t p;
    guardian_node_link_rollback_anchor_provider_t a;
    guardian_node_link_freshness_runtime_t runtime;
    guardian_node_link_freshness_persisted_record_t committed;
    guardian_node_link_persistent_anti_replay_evidence_t evidence;

    initialize_stores(&pstore, &astore, &record);
    pstore.fail_commit = 1U;
    p = make_p_provider(&pstore);
    a = make_a_provider(&astore);
    {
        guardian_node_link_freshness_persisted_record_t restored;
        guardian_node_link_persistent_anti_replay_evidence_t recovery_evidence;
        assert(guardian_node_link_persistent_anti_replay_recover(
                   &p, &a, &identity, &runtime, &restored, &recovery_evidence) ==
               GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_OK);
    }

    assert(guardian_node_link_persistent_anti_replay_commit_continuation(
               &p, &a, &identity, &record, 9U, 11U,
               &runtime, &committed, &evidence) ==
           GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_STATE_UNCERTAIN);
    assert(runtime.condition == GUARDIAN_NODE_LINK_FRESHNESS_ACTIVE);
    assert(runtime.accepted_sequence == 10U);

    initialize_stores(&pstore, &astore, &record);
    astore.fail_advance = 1U;
    p = make_p_provider(&pstore);
    a = make_a_provider(&astore);
    {
        guardian_node_link_freshness_persisted_record_t restored;
        guardian_node_link_persistent_anti_replay_evidence_t recovery_evidence;
        assert(guardian_node_link_persistent_anti_replay_recover(
                   &p, &a, &identity, &runtime, &restored, &recovery_evidence) ==
               GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_OK);
    }

    assert(guardian_node_link_persistent_anti_replay_commit_continuation(
               &p, &a, &identity, &record, 9U, 11U,
               &runtime, &committed, &evidence) ==
           GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_STATE_UNCERTAIN);
    assert(runtime.condition == GUARDIAN_NODE_LINK_FRESHNESS_ACTIVE);
    assert(runtime.accepted_sequence == 10U);
    assert(evidence.persistence_committed == 1U);
    assert(evidence.anchor_advanced == 0U);

    initialize_stores(&pstore, &astore, &record);
    astore.corrupt_after_advance = 1U;
    p = make_p_provider(&pstore);
    a = make_a_provider(&astore);
    {
        guardian_node_link_freshness_persisted_record_t restored;
        guardian_node_link_persistent_anti_replay_evidence_t recovery_evidence;
        assert(guardian_node_link_persistent_anti_replay_recover(
                   &p, &a, &identity, &runtime, &restored, &recovery_evidence) ==
               GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_OK);
    }

    assert(guardian_node_link_persistent_anti_replay_commit_continuation(
               &p, &a, &identity, &record, 9U, 11U,
               &runtime, &committed, &evidence) ==
           GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_STATE_UNCERTAIN);
    assert(runtime.condition == GUARDIAN_NODE_LINK_FRESHNESS_ACTIVE);
    assert(runtime.accepted_sequence == 10U);
}

static void test_actuation_is_never_granted(void)
{
    uint8_t allowed = 1U;

    assert(guardian_node_link_persistent_anti_replay_can_actuate(&allowed) ==
           GUARDIAN_NODE_LINK_PERSISTENT_ANTI_REPLAY_OK);
    assert(allowed == 0U);
}

int main(void)
{
    test_recover_matching_state();
    test_bootstrap_requires_separate_provisioning();
    test_rollback_and_mismatch_fail_closed();
    test_persisted_ahead_of_anchor_is_uncertain();
    test_valid_continuation_updates_only_after_anchor_readback();
    test_replay_epoch_and_exhaustion_rejected();
    test_partial_failures_never_promote_runtime();
    test_actuation_is_never_granted();

    (void)printf("M17_PERSISTENT_ANTI_REPLAY_CONTINUATION_HOST_TEST=PASS\n");
    (void)printf("PERSISTENT_ANTI_REPLAY_INITIAL_PROVISIONING=NOT_DEMONSTRATED\n");
    (void)printf("PHYSICAL_ROLLBACK_ANCHOR_BACKEND=NOT_DEMONSTRATED\n");
    (void)printf("HARDWARE_POWER_LOSS_VALIDATION=NOT_DEMONSTRATED\n");
    (void)printf("PERSISTENT_ANTI_REPLAY_ACTUATION_AUTHORITY=NONE\n");

    return 0;
}