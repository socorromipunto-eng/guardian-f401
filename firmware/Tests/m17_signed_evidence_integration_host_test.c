#include "guardian_signed_evidence.h"

#include <stdio.h>
#include <string.h>

#define TEST_CHECK(condition) \
    do \
    { \
        if (!(condition)) \
        { \
            return __LINE__; \
        } \
    } \
    while (0)

typedef struct
{
    uint8_t fail_sign;
    uint8_t fail_verify;
} test_provider_context_t;

typedef struct
{
    guardian_multinode_node_t canonical;
    guardian_signed_evidence_trust_t trust;
    uint8_t present;
} test_lookup_context_t;

static guardian_security_witness_provider_result_t test_sign(
    void *context,
    uint8_t signature_algorithm,
    uint32_t key_id,
    const uint8_t *message,
    size_t message_length,
    uint8_t *signature,
    size_t signature_capacity)
{
    test_provider_context_t *test_context = (test_provider_context_t *)context;
    uint8_t digest[GUARDIAN_SHA256_SIZE];
    size_t index;

    if ((test_context == NULL) || (message == NULL) || (signature == NULL))
    {
        return GUARDIAN_SECURITY_WITNESS_PROVIDER_FAILURE;
    }

    if (test_context->fail_sign != 0U)
    {
        return GUARDIAN_SECURITY_WITNESS_PROVIDER_UNAVAILABLE;
    }

    if ((signature_algorithm != GUARDIAN_SECURITY_WITNESS_SIGNATURE_ED25519) ||
        (key_id == 0U) || (message_length == 0U) ||
        (signature_capacity < GUARDIAN_SECURITY_WITNESS_SIGNATURE_SIZE))
    {
        return GUARDIAN_SECURITY_WITNESS_PROVIDER_FAILURE;
    }

    guardian_sha256(message, message_length, digest);

    for (index = 0U; index < GUARDIAN_SHA256_SIZE; index += 1U)
    {
        signature[index] = digest[index];
        signature[index + GUARDIAN_SHA256_SIZE] =
            (uint8_t)(digest[index] ^ (uint8_t)0x5AU);
    }

    guardian_crypto_zero(digest, sizeof(digest));
    return GUARDIAN_SECURITY_WITNESS_PROVIDER_OK;
}

static guardian_security_witness_provider_result_t test_verify(
    void *context,
    uint8_t signature_algorithm,
    uint32_t key_id,
    const uint8_t *message,
    size_t message_length,
    const uint8_t *signature,
    size_t signature_length)
{
    test_provider_context_t *test_context = (test_provider_context_t *)context;
    uint8_t expected[GUARDIAN_SECURITY_WITNESS_SIGNATURE_SIZE];
    guardian_security_witness_provider_result_t result;

    if ((test_context == NULL) || (message == NULL) || (signature == NULL))
    {
        return GUARDIAN_SECURITY_WITNESS_PROVIDER_FAILURE;
    }

    if (test_context->fail_verify != 0U)
    {
        return GUARDIAN_SECURITY_WITNESS_PROVIDER_UNAVAILABLE;
    }

    if (signature_length != GUARDIAN_SECURITY_WITNESS_SIGNATURE_SIZE)
    {
        return GUARDIAN_SECURITY_WITNESS_PROVIDER_SIGNATURE_INVALID;
    }

    result = test_sign(
        context,
        signature_algorithm,
        key_id,
        message,
        message_length,
        expected,
        sizeof(expected));

    if (result != GUARDIAN_SECURITY_WITNESS_PROVIDER_OK)
    {
        guardian_crypto_zero(expected, sizeof(expected));
        return result;
    }

    if (guardian_crypto_constant_time_equal(
            expected,
            signature,
            sizeof(expected)) == 0)
    {
        guardian_crypto_zero(expected, sizeof(expected));
        return GUARDIAN_SECURITY_WITNESS_PROVIDER_SIGNATURE_INVALID;
    }

    guardian_crypto_zero(expected, sizeof(expected));
    return GUARDIAN_SECURITY_WITNESS_PROVIDER_OK;
}

static int test_lookup(
    void *context,
    uint32_t node_id,
    uint32_t producer_id,
    uint32_t key_id,
    guardian_multinode_node_t *resolved_node,
    guardian_signed_evidence_trust_t *trust)
{
    test_lookup_context_t *lookup = (test_lookup_context_t *)context;

    if ((lookup == NULL) || (resolved_node == NULL) || (trust == NULL))
    {
        return 0;
    }

    if ((lookup->present == 0U) ||
        (lookup->canonical.node_id != node_id) ||
        (lookup->canonical.producer_id != producer_id) ||
        (lookup->canonical.key_id != key_id))
    {
        return 0;
    }

    *resolved_node = lookup->canonical;
    *trust = lookup->trust;
    return 1;
}

static guardian_multinode_node_t make_witness_node(void)
{
    guardian_multinode_node_t node;

    node.node_id = 4U;
    node.producer_id = 104U;
    node.key_id = 1004U;
    node.role = GUARDIAN_NODE_ROLE_SECURITY_WITNESS;
    node.authority_class = GUARDIAN_AUTHORITY_CLASS_WITNESS;
    node.capabilities =
        GUARDIAN_CAP_OBSERVATION |
        GUARDIAN_CAP_EVIDENCE_WITNESS |
        GUARDIAN_CAP_SIGNATURE_VERIFY;

    return node;
}

static guardian_multinode_node_t make_high_compute_node(void)
{
    guardian_multinode_node_t node;

    node.node_id = 5U;
    node.producer_id = 105U;
    node.key_id = 1005U;
    node.role = GUARDIAN_NODE_ROLE_HIGH_COMPUTE_PROCESSING;
    node.authority_class = GUARDIAN_AUTHORITY_CLASS_OBSERVE;
    node.capabilities =
        GUARDIAN_CAP_OBSERVATION |
        GUARDIAN_CAP_SIGNAL_PROCESSING;

    return node;
}

static void fill_digest(uint8_t digest[GUARDIAN_SHA256_SIZE], uint8_t seed)
{
    size_t index;

    for (index = 0U; index < GUARDIAN_SHA256_SIZE; index += 1U)
    {
        digest[index] = (uint8_t)(seed + (uint8_t)index);
    }
}

int main(void)
{
    guardian_security_witness_node_t witness;
    guardian_security_witness_node_t replay_witness;
    guardian_security_witness_provider_t provider;
    guardian_multinode_node_t witness_identity = make_witness_node();
    guardian_multinode_node_t producer = make_high_compute_node();
    guardian_multinode_node_t claimed_measurement;
    guardian_signed_evidence_t signer;
    guardian_signed_evidence_t verifier;
    guardian_signed_evidence_t replay_signer;
    guardian_signed_evidence_config_t config;
    guardian_signed_evidence_envelope_t envelope1;
    guardian_signed_evidence_envelope_t envelope2;
    guardian_signed_evidence_envelope_t candidate;
    guardian_signed_evidence_verdict_t verdict;
    test_provider_context_t provider_context;
    test_lookup_context_t lookup_context;
    uint8_t payload_digest[GUARDIAN_SHA256_SIZE];
    uint8_t analysis_digest[GUARDIAN_SHA256_SIZE];
    uint8_t zero_digest[GUARDIAN_SHA256_SIZE];
    uint8_t wrong_previous[GUARDIAN_SHA256_SIZE];
    uint8_t allowed = 1U;

    (void)memset(&provider_context, 0, sizeof(provider_context));
    (void)memset(&lookup_context, 0, sizeof(lookup_context));
    (void)memset(zero_digest, 0, sizeof(zero_digest));
    fill_digest(payload_digest, (uint8_t)0x10U);
    fill_digest(analysis_digest, (uint8_t)0x50U);
    fill_digest(wrong_previous, (uint8_t)0xA0U);

    provider.context = &provider_context;
    provider.sign = test_sign;
    provider.verify = test_verify;
    provider.signature_algorithm = GUARDIAN_SECURITY_WITNESS_SIGNATURE_ED25519;

    guardian_security_witness_init(&witness);
    TEST_CHECK(
        guardian_security_witness_configure(
            &witness,
            &witness_identity,
            &provider) == GUARDIAN_SECURITY_WITNESS_OK);

    guardian_security_witness_init(&replay_witness);
    TEST_CHECK(
        guardian_security_witness_configure(
            &replay_witness,
            &witness_identity,
            &provider) == GUARDIAN_SECURITY_WITNESS_OK);

    lookup_context.canonical = producer;
    lookup_context.trust = GUARDIAN_SIGNED_EVIDENCE_TRUST_TRUSTED;
    lookup_context.present = 1U;

    config.lookup = test_lookup;
    config.lookup_context = &lookup_context;

    guardian_signed_evidence_init(&signer);
    guardian_signed_evidence_init(&verifier);
    guardian_signed_evidence_init(&replay_signer);

    TEST_CHECK(
        guardian_signed_evidence_configure(
            &signer,
            &witness,
            &config) == GUARDIAN_SIGNED_EVIDENCE_OK);

    TEST_CHECK(
        guardian_signed_evidence_configure(
            &verifier,
            &witness,
            &config) == GUARDIAN_SIGNED_EVIDENCE_OK);

    TEST_CHECK(
        guardian_signed_evidence_configure(
            &replay_signer,
            &replay_witness,
            &config) == GUARDIAN_SIGNED_EVIDENCE_OK);

    /* PT01: create one genesis analysis envelope. */
    TEST_CHECK(
        guardian_signed_evidence_create(
            &signer,
            &producer,
            GUARDIAN_SIGNED_EVIDENCE_CLASS_ANALYSIS,
            1U,
            1U,
            payload_digest,
            analysis_digest,
            zero_digest,
            &envelope1) == GUARDIAN_SIGNED_EVIDENCE_OK);


    TEST_CHECK(envelope1.core.actuation_authorized == 0U);

    /* PT02: signature, trust, authorization, chain and freshness are distinct PASS states. */
    TEST_CHECK(
        guardian_signed_evidence_verify_and_accept(
            &verifier,
            &envelope1,
            &verdict) == GUARDIAN_SIGNED_EVIDENCE_OK);

    TEST_CHECK(verdict.signature_valid == 1U);
    TEST_CHECK(verdict.trusted == 1U);
    TEST_CHECK(verdict.authorized == 1U);
    TEST_CHECK(verdict.chain_valid == 1U);
    TEST_CHECK(verdict.fresh == 1U);
    TEST_CHECK(verdict.accepted == 1U);
    TEST_CHECK(verdict.actuation_authorized == 0U);

    /* PT03: second envelope chains to accepted digest. */
    TEST_CHECK(
        guardian_signed_evidence_create(
            &signer,
            &producer,
            GUARDIAN_SIGNED_EVIDENCE_CLASS_ANALYSIS,
            1U,
            2U,
            payload_digest,
            analysis_digest,
            envelope1.evidence_digest,
            &envelope2) == GUARDIAN_SIGNED_EVIDENCE_OK);

    TEST_CHECK(
        guardian_signed_evidence_verify_and_accept(
            &verifier,
            &envelope2,
            &verdict) == GUARDIAN_SIGNED_EVIDENCE_OK);

    /* NT01A: an exact duplicate fails earlier as a chain mismatch. */
    TEST_CHECK(
        guardian_signed_evidence_verify_and_accept(
            &verifier,
            &envelope2,
            &verdict) == GUARDIAN_SIGNED_EVIDENCE_ERROR_CHAIN_MISMATCH);

    /* NT01B: valid chain plus non-advancing sequence reaches replay/regression. */
    TEST_CHECK(
        guardian_signed_evidence_create(
            &replay_signer,
            &producer,
            GUARDIAN_SIGNED_EVIDENCE_CLASS_ANALYSIS,
            1U,
            2U,
            payload_digest,
            analysis_digest,
            envelope2.evidence_digest,
            &candidate) == GUARDIAN_SIGNED_EVIDENCE_OK);

    TEST_CHECK(
        guardian_signed_evidence_verify_and_accept(
            &verifier,
            &candidate,
            &verdict) == GUARDIAN_SIGNED_EVIDENCE_ERROR_REPLAY_OR_REGRESSION);
    TEST_CHECK(verdict.signature_valid == 1U);
    TEST_CHECK(verdict.trusted == 1U);
    TEST_CHECK(verdict.authorized == 1U);
    TEST_CHECK(verdict.chain_valid == 1U);
    TEST_CHECK(verdict.fresh == 0U);
    TEST_CHECK(verdict.accepted == 0U);
    TEST_CHECK(verdict.actuation_authorized == 0U);
    puts("M17_SIGNED_EVIDENCE_REPLAY_REGRESSION_HOSTILE_TEST=PASS");

    /* NT02: tampered core digest invalidates signature binding. */
    candidate = envelope2;
    candidate.core.payload_digest[0] ^= (uint8_t)0x01U;
    TEST_CHECK(
        guardian_signed_evidence_verify_and_accept(
            &verifier,
            &candidate,
            &verdict) == GUARDIAN_SIGNED_EVIDENCE_ERROR_SIGNATURE_INVALID);

    /* NT03: wrong producer identity breaks witness subject binding. */
    candidate = envelope2;
    candidate.core.producer_id += 1U;
    TEST_CHECK(
        guardian_signed_evidence_verify_and_accept(
            &verifier,
            &candidate,
            &verdict) == GUARDIAN_SIGNED_EVIDENCE_ERROR_INVALID_ENVELOPE);

    /* NT04: wrong producer key breaks witness subject binding. */
    candidate = envelope2;
    candidate.core.producer_key_id += 1U;
    TEST_CHECK(
        guardian_signed_evidence_verify_and_accept(
            &verifier,
            &candidate,
            &verdict) == GUARDIAN_SIGNED_EVIDENCE_ERROR_INVALID_ENVELOPE);

    /* NT05: wrong witness identity is rejected before trust. */
    candidate = envelope2;
    candidate.witness_record.witness_node_id += 1U;
    TEST_CHECK(
        guardian_signed_evidence_verify_and_accept(
            &verifier,
            &candidate,
            &verdict) == GUARDIAN_SIGNED_EVIDENCE_ERROR_INVALID_ENVELOPE);

    /* NT06: valid signature does not imply trusted producer. */
    guardian_signed_evidence_init(&verifier);
    lookup_context.trust = GUARDIAN_SIGNED_EVIDENCE_TRUST_UNTRUSTED;
    TEST_CHECK(
        guardian_signed_evidence_configure(
            &verifier,
            &witness,
            &config) == GUARDIAN_SIGNED_EVIDENCE_OK);
    TEST_CHECK(
        guardian_signed_evidence_verify_and_accept(
            &verifier,
            &envelope1,
            &verdict) == GUARDIAN_SIGNED_EVIDENCE_ERROR_UNTRUSTED);
    TEST_CHECK(verdict.signature_valid == 1U);
    TEST_CHECK(verdict.trusted == 0U);

    /* NT07: revoked producer key is distinct from bad signature. */
    lookup_context.trust = GUARDIAN_SIGNED_EVIDENCE_TRUST_REVOKED;
    TEST_CHECK(
        guardian_signed_evidence_verify_and_accept(
            &verifier,
            &envelope1,
            &verdict) == GUARDIAN_SIGNED_EVIDENCE_ERROR_KEY_REVOKED);
    TEST_CHECK(verdict.signature_valid == 1U);

    /* NT08: unknown key is distinct from bad signature. */
    lookup_context.present = 0U;
    TEST_CHECK(
        guardian_signed_evidence_verify_and_accept(
            &verifier,
            &envelope1,
            &verdict) == GUARDIAN_SIGNED_EVIDENCE_ERROR_KEY_UNKNOWN);
    TEST_CHECK(verdict.signature_valid == 1U);

    /* NT09: trusted identity but unauthorized claimed role is rejected after signature/trust. */
    lookup_context.present = 1U;
    lookup_context.trust = GUARDIAN_SIGNED_EVIDENCE_TRUST_TRUSTED;
    guardian_signed_evidence_init(&verifier);
    TEST_CHECK(
        guardian_signed_evidence_configure(
            &verifier,
            &witness,
            &config) == GUARDIAN_SIGNED_EVIDENCE_OK);

    claimed_measurement = producer;
    claimed_measurement.role = GUARDIAN_NODE_ROLE_ST_MEASUREMENT_REFERENCE;
    claimed_measurement.capabilities =
        GUARDIAN_CAP_OBSERVATION |
        GUARDIAN_CAP_SENSOR_ACQUISITION;

    TEST_CHECK(
        guardian_signed_evidence_create(
            &signer,
            &claimed_measurement,
            GUARDIAN_SIGNED_EVIDENCE_CLASS_MEASUREMENT,
            2U,
            1U,
            payload_digest,
            analysis_digest,
            zero_digest,
            &candidate) == GUARDIAN_SIGNED_EVIDENCE_OK);

    TEST_CHECK(
        guardian_signed_evidence_verify_and_accept(
            &verifier,
            &candidate,
            &verdict) == GUARDIAN_SIGNED_EVIDENCE_ERROR_UNAUTHORIZED_ROLE);
    TEST_CHECK(verdict.signature_valid == 1U);
    TEST_CHECK(verdict.trusted == 1U);
    TEST_CHECK(verdict.authorized == 0U);

    /* NT10: chain break is rejected even with valid signature/trust/role. */
    guardian_signed_evidence_init(&verifier);
    TEST_CHECK(
        guardian_signed_evidence_configure(
            &verifier,
            &witness,
            &config) == GUARDIAN_SIGNED_EVIDENCE_OK);
    TEST_CHECK(
        guardian_signed_evidence_create(
            &signer,
            &producer,
            GUARDIAN_SIGNED_EVIDENCE_CLASS_ANALYSIS,
            3U,
            1U,
            payload_digest,
            analysis_digest,
            wrong_previous,
            &candidate) == GUARDIAN_SIGNED_EVIDENCE_OK);
    TEST_CHECK(
        guardian_signed_evidence_verify_and_accept(
            &verifier,
            &candidate,
            &verdict) == GUARDIAN_SIGNED_EVIDENCE_ERROR_CHAIN_MISMATCH);

    /* NT11: actuation escalation in signed core is structurally prohibited. */
    candidate = envelope1;
    candidate.core.actuation_authorized = 1U;
    TEST_CHECK(
        guardian_signed_evidence_verify_and_accept(
            &verifier,
            &candidate,
            &verdict) == GUARDIAN_SIGNED_EVIDENCE_ERROR_INVALID_ENVELOPE);

    /* NT12: signed evidence integration itself can never actuate. */
    allowed = 1U;
    TEST_CHECK(
        guardian_signed_evidence_can_actuate(
            &verifier,
            &allowed) == GUARDIAN_SIGNED_EVIDENCE_ERROR_ACTUATION_PROHIBITED);
    TEST_CHECK(allowed == 0U);

    puts("M17_SIGNED_EVIDENCE_INTEGRATION_HOST_TEST=PASS");
    return 0;
}
