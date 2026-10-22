#include "guardian_security_witness.h"

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

static guardian_security_witness_provider_result_t
test_sign(
    void *context,
    uint8_t signature_algorithm,
    uint32_t key_id,
    const uint8_t *message,
    size_t message_length,
    uint8_t *signature,
    size_t signature_capacity)
{
    test_provider_context_t *test_context =
        (test_provider_context_t *)context;

    uint8_t digest[
        GUARDIAN_SHA256_SIZE];

    size_t index;

    if ((test_context == NULL) ||
        (message == NULL) ||
        (signature == NULL))
    {
        return
            GUARDIAN_SECURITY_WITNESS_PROVIDER_FAILURE;
    }

    if (test_context->fail_sign != 0U)
    {
        return
            GUARDIAN_SECURITY_WITNESS_PROVIDER_UNAVAILABLE;
    }

    if ((signature_algorithm !=
            GUARDIAN_SECURITY_WITNESS_SIGNATURE_ED25519) ||
        (key_id == 0U) ||
        (message_length == 0U) ||
        (signature_capacity <
            GUARDIAN_SECURITY_WITNESS_SIGNATURE_SIZE))
    {
        return
            GUARDIAN_SECURITY_WITNESS_PROVIDER_FAILURE;
    }

    guardian_sha256(
        message,
        message_length,
        digest);

    for (index = 0U;
         index < GUARDIAN_SHA256_SIZE;
         index += 1U)
    {
        signature[index] =
            digest[index];

        signature[
            index +
            GUARDIAN_SHA256_SIZE
        ] =
            (uint8_t)(
                digest[index] ^
                (uint8_t)0xA5U);
    }

    guardian_crypto_zero(
        digest,
        sizeof(digest));

    return
        GUARDIAN_SECURITY_WITNESS_PROVIDER_OK;
}

static guardian_security_witness_provider_result_t
test_verify(
    void *context,
    uint8_t signature_algorithm,
    uint32_t key_id,
    const uint8_t *message,
    size_t message_length,
    const uint8_t *signature,
    size_t signature_length)
{
    test_provider_context_t *test_context =
        (test_provider_context_t *)context;

    uint8_t expected[
        GUARDIAN_SECURITY_WITNESS_SIGNATURE_SIZE];

    guardian_security_witness_provider_result_t
        result;

    if ((test_context == NULL) ||
        (message == NULL) ||
        (signature == NULL))
    {
        return
            GUARDIAN_SECURITY_WITNESS_PROVIDER_FAILURE;
    }

    if (test_context->fail_verify != 0U)
    {
        return
            GUARDIAN_SECURITY_WITNESS_PROVIDER_UNAVAILABLE;
    }

    if (signature_length !=
        GUARDIAN_SECURITY_WITNESS_SIGNATURE_SIZE)
    {
        return
            GUARDIAN_SECURITY_WITNESS_PROVIDER_SIGNATURE_INVALID;
    }

    result =
        test_sign(
            context,
            signature_algorithm,
            key_id,
            message,
            message_length,
            expected,
            sizeof(expected));

    if (result !=
        GUARDIAN_SECURITY_WITNESS_PROVIDER_OK)
    {
        guardian_crypto_zero(
            expected,
            sizeof(expected));

        return result;
    }

    if (guardian_crypto_constant_time_equal(
            expected,
            signature,
            sizeof(expected)) == 0)
    {
        guardian_crypto_zero(
            expected,
            sizeof(expected));

        return
            GUARDIAN_SECURITY_WITNESS_PROVIDER_SIGNATURE_INVALID;
    }

    guardian_crypto_zero(
        expected,
        sizeof(expected));

    return
        GUARDIAN_SECURITY_WITNESS_PROVIDER_OK;
}

static guardian_multinode_node_t
make_witness_node(void)
{
    guardian_multinode_node_t node;

    node.node_id =
        4U;

    node.producer_id =
        104U;

    node.key_id =
        1004U;

    node.role =
        GUARDIAN_NODE_ROLE_SECURITY_WITNESS;

    node.authority_class =
        GUARDIAN_AUTHORITY_CLASS_WITNESS;

    node.capabilities =
        GUARDIAN_CAP_OBSERVATION |
        GUARDIAN_CAP_EVIDENCE_WITNESS |
        GUARDIAN_CAP_SIGNATURE_VERIFY;

    return node;
}

static guardian_security_witness_input_t
make_input(
    uint32_t sequence,
    const uint8_t *payload,
    size_t payload_length)
{
    guardian_security_witness_input_t input;

    input.subject_node_id =
        3U;

    input.subject_producer_id =
        103U;

    input.subject_key_id =
        1003U;

    input.epoch =
        1U;

    input.sequence =
        sequence;

    input.evidence_type =
        GUARDIAN_SECURITY_WITNESS_EVIDENCE_OBSERVATION;

    input.payload =
        payload;

    input.payload_length =
        payload_length;

    return input;
}

int main(void)
{
    static const uint8_t payload[] =
    {
        0x47U,
        0x55U,
        0x41U,
        0x52U,
        0x44U
    };

    guardian_security_witness_node_t node;

    guardian_multinode_node_t identity =
        make_witness_node();

    guardian_security_witness_provider_t
        provider;

    test_provider_context_t
        provider_context;

    guardian_security_witness_input_t
        input;

    guardian_security_witness_record_t
        record;

    guardian_security_witness_record_t
        tampered;

    uint8_t allowed =
        1U;

    (void)memset(
        &provider_context,
        0,
        sizeof(provider_context));

    provider.context =
        &provider_context;

    provider.sign =
        test_sign;

    provider.verify =
        test_verify;

    provider.signature_algorithm =
        GUARDIAN_SECURITY_WITNESS_SIGNATURE_ED25519;

    guardian_security_witness_init(
        &node);

    TEST_CHECK(
        node.initialized ==
        0U);

    TEST_CHECK(
        guardian_security_witness_configure(
            &node,
            &identity,
            &provider) ==
        GUARDIAN_SECURITY_WITNESS_OK);

    input =
        make_input(
            1U,
            payload,
            sizeof(payload));

    TEST_CHECK(
        guardian_security_witness_create(
            &node,
            &input,
            &record) ==
        GUARDIAN_SECURITY_WITNESS_OK);

    TEST_CHECK(
        record.witness_node_id ==
        4U);

    TEST_CHECK(
        record.subject_node_id ==
        3U);

    TEST_CHECK(
        record.authority_flags ==
        GUARDIAN_SECURITY_WITNESS_AUTHORITY_NONE);

    TEST_CHECK(
        guardian_security_witness_verify(
            &node,
            &record) ==
        GUARDIAN_SECURITY_WITNESS_OK);

    TEST_CHECK(
        guardian_security_witness_create(
            &node,
            &input,
            &record) ==
        GUARDIAN_SECURITY_WITNESS_ERROR_REPLAY_OR_REGRESSION);

    input.sequence =
        2U;

    TEST_CHECK(
        guardian_security_witness_create(
            &node,
            &input,
            &record) ==
        GUARDIAN_SECURITY_WITNESS_OK);

    tampered =
        record;

    tampered.payload_digest[0] ^=
        (uint8_t)0x01U;

    TEST_CHECK(
        guardian_security_witness_verify(
            &node,
            &tampered) ==
        GUARDIAN_SECURITY_WITNESS_ERROR_SIGNATURE_INVALID);

    tampered =
        record;

    tampered.authority_flags =
        1U;

    TEST_CHECK(
        guardian_security_witness_verify(
            &node,
            &tampered) ==
        GUARDIAN_SECURITY_WITNESS_ERROR_INVALID_EVIDENCE);

    tampered =
        record;

    tampered.witness_node_id =
        99U;

    TEST_CHECK(
        guardian_security_witness_verify(
            &node,
            &tampered) ==
        GUARDIAN_SECURITY_WITNESS_ERROR_INVALID_EVIDENCE);

    provider_context.fail_sign =
        1U;

    input.sequence =
        3U;

    TEST_CHECK(
        guardian_security_witness_create(
            &node,
            &input,
            &record) ==
        GUARDIAN_SECURITY_WITNESS_ERROR_PROVIDER_UNAVAILABLE);

    provider_context.fail_sign =
        0U;

    TEST_CHECK(
        guardian_security_witness_create(
            &node,
            &input,
            &record) ==
        GUARDIAN_SECURITY_WITNESS_OK);

    provider_context.fail_verify =
        1U;

    TEST_CHECK(
        guardian_security_witness_verify(
            &node,
            &record) ==
        GUARDIAN_SECURITY_WITNESS_ERROR_PROVIDER_UNAVAILABLE);

    provider_context.fail_verify =
        0U;

    allowed =
        1U;

    TEST_CHECK(
        guardian_security_witness_can_actuate(
            &node,
            &allowed) ==
        GUARDIAN_SECURITY_WITNESS_ERROR_ACTUATION_PROHIBITED);

    TEST_CHECK(
        allowed ==
        0U);

    identity.capabilities |=
        GUARDIAN_CAP_ACTUATION_GATE;

    TEST_CHECK(
        guardian_security_witness_configure(
            &node,
            &identity,
            &provider) ==
        GUARDIAN_SECURITY_WITNESS_ERROR_INVALID_NODE);

    puts(
        "M17_SECURITY_WITNESS_HOST_TEST=PASS");

    return 0;
}
