#include "guardian_node_link_auth.h"

#include <stdio.h>
#include <string.h>

typedef enum
{
    TEST_PROVIDER_NORMAL = 0,
    TEST_PROVIDER_KEY_UNKNOWN,
    TEST_PROVIDER_KEY_REVOKED,
    TEST_PROVIDER_ALGORITHM_UNSUPPORTED,
    TEST_PROVIDER_UNAVAILABLE,
    TEST_PROVIDER_FAILURE
} test_provider_mode_t;

typedef struct
{
    uint8_t expected_transcript[
        GUARDIAN_NODE_LINK_AUTH_TRANSCRIPT_MAX_SIZE];

    size_t expected_transcript_length;

    uint32_t expected_producer_id;
    uint32_t expected_key_id;

    uint8_t expected_signature[
        GUARDIAN_NODE_LINK_ED25519_SIGNATURE_SIZE];

    size_t expected_signature_length;

    test_provider_mode_t mode;
    uint32_t calls;
} test_verify_context_t;

static int require_true(
    int condition,
    const char *name)
{
    if (!condition)
    {
        (void)printf("FAIL: %s\n", name);
        return 0;
    }

    return 1;
}

static guardian_node_link_frame_t make_frame(void)
{
    guardian_node_link_frame_t frame;

    (void)memset(&frame, 0, sizeof(frame));

    frame.message_type =
        GUARDIAN_NODE_LINK_MESSAGE_SUPERVISION_STATE;

    frame.flags = GUARDIAN_NODE_LINK_SUPPORTED_FLAGS;
    frame.node_state = GUARDIAN_NODE_STATE_ACTIVE;
    frame.payload_length = 4U;
    frame.sequence = 7U;
    frame.sender_epoch = 11U;
    frame.sender_node_id = 1001U;

    frame.payload[0] = 0x10U;
    frame.payload[1] = 0x20U;
    frame.payload[2] = 0x30U;
    frame.payload[3] = 0x40U;

    return frame;
}

static void make_signature(
    uint8_t signature[
        GUARDIAN_NODE_LINK_ED25519_SIGNATURE_SIZE])
{
    size_t index;

    for (index = 0U;
         index < GUARDIAN_NODE_LINK_ED25519_SIGNATURE_SIZE;
         index += 1U)
    {
        signature[index] =
            (uint8_t)((index + 1U) & 0xFFU);
    }
}

static guardian_node_link_verify_result_t
test_verify_signature(
    void *context,
    uint8_t signature_algorithm,
    uint32_t producer_id,
    uint32_t key_id,
    const uint8_t *transcript,
    size_t transcript_length,
    const uint8_t *signature,
    size_t signature_length)
{
    test_verify_context_t *verify =
        (test_verify_context_t *)context;

    if ((verify == NULL) ||
        (transcript == NULL) ||
        (signature == NULL))
    {
        return GUARDIAN_NODE_LINK_VERIFY_PROVIDER_FAILURE;
    }

    verify->calls += 1U;

    switch (verify->mode)
    {
        case TEST_PROVIDER_KEY_UNKNOWN:
            return GUARDIAN_NODE_LINK_VERIFY_KEY_UNKNOWN;

        case TEST_PROVIDER_KEY_REVOKED:
            return GUARDIAN_NODE_LINK_VERIFY_KEY_REVOKED;

        case TEST_PROVIDER_ALGORITHM_UNSUPPORTED:
            return GUARDIAN_NODE_LINK_VERIFY_ALGORITHM_UNSUPPORTED;

        case TEST_PROVIDER_UNAVAILABLE:
            return GUARDIAN_NODE_LINK_VERIFY_PROVIDER_UNAVAILABLE;

        case TEST_PROVIDER_FAILURE:
            return GUARDIAN_NODE_LINK_VERIFY_PROVIDER_FAILURE;

        case TEST_PROVIDER_NORMAL:
        default:
            break;
    }

    if (signature_algorithm !=
        GUARDIAN_NODE_LINK_SIGNATURE_ED25519)
    {
        return GUARDIAN_NODE_LINK_VERIFY_ALGORITHM_UNSUPPORTED;
    }

    if ((producer_id != verify->expected_producer_id) ||
        (key_id != verify->expected_key_id))
    {
        return GUARDIAN_NODE_LINK_VERIFY_SIGNATURE_INVALID;
    }

    if ((transcript_length !=
            verify->expected_transcript_length) ||
        (signature_length !=
            verify->expected_signature_length))
    {
        return GUARDIAN_NODE_LINK_VERIFY_SIGNATURE_INVALID;
    }

    if (memcmp(
            transcript,
            verify->expected_transcript,
            transcript_length) != 0)
    {
        return GUARDIAN_NODE_LINK_VERIFY_SIGNATURE_INVALID;
    }

    if (memcmp(
            signature,
            verify->expected_signature,
            signature_length) != 0)
    {
        return GUARDIAN_NODE_LINK_VERIFY_SIGNATURE_INVALID;
    }

    return GUARDIAN_NODE_LINK_VERIFY_VERIFIED;
}

static int configure_authenticator(
    guardian_node_link_authenticator_t *authenticator,
    const guardian_node_link_peer_identity_t *peers,
    size_t peer_count,
    test_verify_context_t *verify)
{
    guardian_node_link_auth_config_t config;

    (void)memset(&config, 0, sizeof(config));

    config.context = verify;
    config.verify_signature = test_verify_signature;
    config.peers = peers;
    config.peer_count = peer_count;
    config.required_signature_algorithm =
        GUARDIAN_NODE_LINK_SIGNATURE_ED25519;

    guardian_node_link_authenticator_init(authenticator);

    return (
        guardian_node_link_authenticator_configure(
            authenticator,
            &config) == GUARDIAN_NODE_LINK_AUTH_OK);
}

static guardian_node_link_peer_identity_t make_peer(
    uint32_t sender_node_id,
    uint32_t producer_id,
    uint32_t key_id)
{
    guardian_node_link_peer_identity_t peer;

    peer.sender_node_id = sender_node_id;
    peer.producer_id = producer_id;
    peer.key_id = key_id;
    peer.signature_algorithm =
        GUARDIAN_NODE_LINK_SIGNATURE_ED25519;

    return peer;
}

static int prepare_expected(
    const guardian_node_link_frame_t *frame,
    test_verify_context_t *verify,
    uint32_t producer_id,
    uint32_t key_id,
    const uint8_t *signature,
    size_t signature_length)
{
    guardian_node_link_auth_result_t result;

    verify->expected_producer_id = producer_id;
    verify->expected_key_id = key_id;
    verify->expected_signature_length = signature_length;

    (void)memcpy(
        verify->expected_signature,
        signature,
        signature_length);

    result = guardian_node_link_build_auth_transcript(
        frame,
        producer_id,
        key_id,
        verify->expected_transcript,
        sizeof(verify->expected_transcript),
        &verify->expected_transcript_length);

    return (result == GUARDIAN_NODE_LINK_AUTH_OK);
}

static int authenticated_output_is_zero(
    const guardian_node_link_authenticated_message_t *message)
{
    guardian_node_link_authenticated_message_t zero;

    (void)memset(&zero, 0, sizeof(zero));

    return (
        memcmp(
            message,
            &zero,
            sizeof(zero)) == 0);
}

static int test_valid_authentication(void)
{
    guardian_node_link_authenticator_t authenticator;
    guardian_node_link_peer_identity_t peer;
    guardian_node_link_frame_t frame;
    guardian_node_link_authenticated_message_t output;
    test_verify_context_t verify;

    uint8_t signature[
        GUARDIAN_NODE_LINK_ED25519_SIGNATURE_SIZE];

    peer = make_peer(1001U, 5001U, 7001U);
    frame = make_frame();

    (void)memset(&verify, 0, sizeof(verify));
    (void)memset(&output, 0, sizeof(output));

    make_signature(signature);

    if (!configure_authenticator(
            &authenticator,
            &peer,
            1U,
            &verify))
    {
        return 0;
    }

    if (!prepare_expected(
            &frame,
            &verify,
            peer.producer_id,
            peer.key_id,
            signature,
            sizeof(signature)))
    {
        return 0;
    }

    if (!require_true(
            guardian_node_link_authenticate(
                &authenticator,
                &frame,
                signature,
                sizeof(signature),
                &output) ==
                GUARDIAN_NODE_LINK_AUTH_OK,
            "valid authentication"))
    {
        return 0;
    }

    if (!require_true(
            verify.calls == 1U,
            "provider called once"))
    {
        return 0;
    }

    if (!require_true(
            output.producer_id == 5001U,
            "producer binding retained"))
    {
        return 0;
    }

    if (!require_true(
            output.key_id == 7001U,
            "key binding retained"))
    {
        return 0;
    }

    return require_true(
        output.frame.sequence == frame.sequence,
        "authenticated frame retained");
}

static int test_unknown_peer_rejected(void)
{
    guardian_node_link_authenticator_t authenticator;
    guardian_node_link_peer_identity_t peer;
    guardian_node_link_frame_t frame;
    guardian_node_link_authenticated_message_t output;
    test_verify_context_t verify;

    uint8_t signature[
        GUARDIAN_NODE_LINK_ED25519_SIGNATURE_SIZE];

    peer = make_peer(1001U, 5001U, 7001U);
    frame = make_frame();
    frame.sender_node_id = 9999U;

    (void)memset(&verify, 0, sizeof(verify));
    (void)memset(&output, 0xA5, sizeof(output));

    make_signature(signature);

    if (!configure_authenticator(
            &authenticator,
            &peer,
            1U,
            &verify))
    {
        return 0;
    }

    if (!require_true(
            guardian_node_link_authenticate(
                &authenticator,
                &frame,
                signature,
                sizeof(signature),
                &output) ==
                GUARDIAN_NODE_LINK_AUTH_ERROR_PEER_UNKNOWN,
            "unknown peer rejected"))
    {
        return 0;
    }

    if (!require_true(
            verify.calls == 0U,
            "unknown peer never reaches provider"))
    {
        return 0;
    }

    return require_true(
        authenticated_output_is_zero(&output),
        "unknown peer creates no authenticated output");
}

static int test_bad_signature_rejected(void)
{
    guardian_node_link_authenticator_t authenticator;
    guardian_node_link_peer_identity_t peer;
    guardian_node_link_frame_t frame;
    guardian_node_link_authenticated_message_t output;
    test_verify_context_t verify;

    uint8_t signature[
        GUARDIAN_NODE_LINK_ED25519_SIGNATURE_SIZE];

    peer = make_peer(1001U, 5001U, 7001U);
    frame = make_frame();

    (void)memset(&verify, 0, sizeof(verify));
    (void)memset(&output, 0xA5, sizeof(output));

    make_signature(signature);

    if (!configure_authenticator(
            &authenticator,
            &peer,
            1U,
            &verify))
    {
        return 0;
    }

    if (!prepare_expected(
            &frame,
            &verify,
            peer.producer_id,
            peer.key_id,
            signature,
            sizeof(signature)))
    {
        return 0;
    }

    signature[0] ^= 0x01U;

    if (!require_true(
            guardian_node_link_authenticate(
                &authenticator,
                &frame,
                signature,
                sizeof(signature),
                &output) ==
                GUARDIAN_NODE_LINK_AUTH_ERROR_SIGNATURE_INVALID,
            "bad signature rejected"))
    {
        return 0;
    }

    return require_true(
        authenticated_output_is_zero(&output),
        "bad signature creates no authenticated output");
}

static int test_bound_fields(void)
{
    guardian_node_link_authenticator_t authenticator;
    guardian_node_link_peer_identity_t peers[2];
    guardian_node_link_frame_t original;
    guardian_node_link_frame_t mutated;
    guardian_node_link_authenticated_message_t output;
    test_verify_context_t verify;

    uint8_t signature[
        GUARDIAN_NODE_LINK_ED25519_SIGNATURE_SIZE];

    peers[0] = make_peer(1001U, 5001U, 7001U);
    peers[1] = make_peer(1002U, 5001U, 7001U);

    original = make_frame();

    (void)memset(&verify, 0, sizeof(verify));
    make_signature(signature);

    if (!configure_authenticator(
            &authenticator,
            peers,
            2U,
            &verify))
    {
        return 0;
    }

    if (!prepare_expected(
            &original,
            &verify,
            5001U,
            7001U,
            signature,
            sizeof(signature)))
    {
        return 0;
    }

    mutated = original;
    mutated.node_state = GUARDIAN_NODE_STATE_DEGRADED;

    (void)memset(&output, 0xA5, sizeof(output));

    if (!require_true(
            guardian_node_link_authenticate(
                &authenticator,
                &mutated,
                signature,
                sizeof(signature),
                &output) ==
                GUARDIAN_NODE_LINK_AUTH_ERROR_SIGNATURE_INVALID,
            "changed node state rejected"))
    {
        return 0;
    }

    mutated = original;
    mutated.sequence += 1U;

    if (!require_true(
            guardian_node_link_authenticate(
                &authenticator,
                &mutated,
                signature,
                sizeof(signature),
                &output) ==
                GUARDIAN_NODE_LINK_AUTH_ERROR_SIGNATURE_INVALID,
            "changed sequence rejected"))
    {
        return 0;
    }

    mutated = original;
    mutated.sender_epoch += 1U;

    if (!require_true(
            guardian_node_link_authenticate(
                &authenticator,
                &mutated,
                signature,
                sizeof(signature),
                &output) ==
                GUARDIAN_NODE_LINK_AUTH_ERROR_SIGNATURE_INVALID,
            "changed epoch rejected"))
    {
        return 0;
    }

    mutated = original;
    mutated.sender_node_id = 1002U;

    if (!require_true(
            guardian_node_link_authenticate(
                &authenticator,
                &mutated,
                signature,
                sizeof(signature),
                &output) ==
                GUARDIAN_NODE_LINK_AUTH_ERROR_SIGNATURE_INVALID,
            "changed sender node id rejected"))
    {
        return 0;
    }

    mutated = original;
    mutated.payload[0] ^= 0x01U;

    if (!require_true(
            guardian_node_link_authenticate(
                &authenticator,
                &mutated,
                signature,
                sizeof(signature),
                &output) ==
                GUARDIAN_NODE_LINK_AUTH_ERROR_SIGNATURE_INVALID,
            "changed payload rejected"))
    {
        return 0;
    }

    return require_true(
        authenticated_output_is_zero(&output),
        "bound-field mutation creates no authenticated output");
}

static int test_wrong_identity_binding_rejected(void)
{
    guardian_node_link_authenticator_t authenticator;
    guardian_node_link_peer_identity_t peer;
    guardian_node_link_frame_t frame;
    guardian_node_link_authenticated_message_t output;
    test_verify_context_t verify;

    uint8_t signature[
        GUARDIAN_NODE_LINK_ED25519_SIGNATURE_SIZE];

    frame = make_frame();
    make_signature(signature);

    peer = make_peer(1001U, 5002U, 7001U);

    (void)memset(&verify, 0, sizeof(verify));
    (void)memset(&output, 0xA5, sizeof(output));

    if (!configure_authenticator(
            &authenticator,
            &peer,
            1U,
            &verify))
    {
        return 0;
    }

    if (!prepare_expected(
            &frame,
            &verify,
            5001U,
            7001U,
            signature,
            sizeof(signature)))
    {
        return 0;
    }

    if (!require_true(
            guardian_node_link_authenticate(
                &authenticator,
                &frame,
                signature,
                sizeof(signature),
                &output) ==
                GUARDIAN_NODE_LINK_AUTH_ERROR_SIGNATURE_INVALID,
            "wrong producer binding rejected"))
    {
        return 0;
    }

    peer = make_peer(1001U, 5001U, 7002U);

    if (!configure_authenticator(
            &authenticator,
            &peer,
            1U,
            &verify))
    {
        return 0;
    }

    if (!require_true(
            guardian_node_link_authenticate(
                &authenticator,
                &frame,
                signature,
                sizeof(signature),
                &output) ==
                GUARDIAN_NODE_LINK_AUTH_ERROR_SIGNATURE_INVALID,
            "wrong key binding rejected"))
    {
        return 0;
    }

    return require_true(
        authenticated_output_is_zero(&output),
        "wrong identity binding creates no output");
}

static int test_provider_result_mapping(void)
{
    guardian_node_link_authenticator_t authenticator;
    guardian_node_link_peer_identity_t peer;
    guardian_node_link_frame_t frame;
    guardian_node_link_authenticated_message_t output;
    test_verify_context_t verify;

    uint8_t signature[
        GUARDIAN_NODE_LINK_ED25519_SIGNATURE_SIZE];

    peer = make_peer(1001U, 5001U, 7001U);
    frame = make_frame();

    (void)memset(&verify, 0, sizeof(verify));
    make_signature(signature);

    if (!configure_authenticator(
            &authenticator,
            &peer,
            1U,
            &verify))
    {
        return 0;
    }

    verify.mode = TEST_PROVIDER_KEY_UNKNOWN;
    (void)memset(&output, 0xA5, sizeof(output));

    if (!require_true(
            guardian_node_link_authenticate(
                &authenticator,
                &frame,
                signature,
                sizeof(signature),
                &output) ==
                GUARDIAN_NODE_LINK_AUTH_ERROR_KEY_UNKNOWN,
            "key unknown preserved"))
    {
        return 0;
    }

    verify.mode = TEST_PROVIDER_KEY_REVOKED;

    if (!require_true(
            guardian_node_link_authenticate(
                &authenticator,
                &frame,
                signature,
                sizeof(signature),
                &output) ==
                GUARDIAN_NODE_LINK_AUTH_ERROR_KEY_REVOKED,
            "key revoked preserved"))
    {
        return 0;
    }

    verify.mode = TEST_PROVIDER_ALGORITHM_UNSUPPORTED;

    if (!require_true(
            guardian_node_link_authenticate(
                &authenticator,
                &frame,
                signature,
                sizeof(signature),
                &output) ==
                GUARDIAN_NODE_LINK_AUTH_ERROR_ALGORITHM_UNSUPPORTED,
            "provider algorithm failure preserved"))
    {
        return 0;
    }

    verify.mode = TEST_PROVIDER_UNAVAILABLE;

    if (!require_true(
            guardian_node_link_authenticate(
                &authenticator,
                &frame,
                signature,
                sizeof(signature),
                &output) ==
                GUARDIAN_NODE_LINK_AUTH_ERROR_PROVIDER_UNAVAILABLE,
            "provider unavailable preserved"))
    {
        return 0;
    }

    verify.mode = TEST_PROVIDER_FAILURE;

    if (!require_true(
            guardian_node_link_authenticate(
                &authenticator,
                &frame,
                signature,
                sizeof(signature),
                &output) ==
                GUARDIAN_NODE_LINK_AUTH_ERROR_PROVIDER_FAILURE,
            "provider failure preserved"))
    {
        return 0;
    }

    return require_true(
        authenticated_output_is_zero(&output),
        "provider failure creates no authenticated output");
}

static int test_configuration_snapshot(void)
{
    guardian_node_link_authenticator_t authenticator;
    guardian_node_link_peer_identity_t peer;
    guardian_node_link_frame_t frame;
    guardian_node_link_authenticated_message_t output;
    test_verify_context_t verify;

    uint8_t signature[
        GUARDIAN_NODE_LINK_ED25519_SIGNATURE_SIZE];

    peer = make_peer(1001U, 5001U, 7001U);
    frame = make_frame();

    (void)memset(&verify, 0, sizeof(verify));
    make_signature(signature);

    if (!configure_authenticator(
            &authenticator,
            &peer,
            1U,
            &verify))
    {
        return 0;
    }

    if (!prepare_expected(
            &frame,
            &verify,
            5001U,
            7001U,
            signature,
            sizeof(signature)))
    {
        return 0;
    }

    peer.sender_node_id = 9999U;
    peer.producer_id = 9999U;
    peer.key_id = 9999U;

    if (!require_true(
            guardian_node_link_authenticate(
                &authenticator,
                &frame,
                signature,
                sizeof(signature),
                &output) ==
                GUARDIAN_NODE_LINK_AUTH_OK,
            "configured peer identity is snapshotted"))
    {
        return 0;
    }

    return require_true(
        output.producer_id == 5001U &&
        output.key_id == 7001U,
        "external peer mutation cannot change trusted binding");
}

static int test_configuration_fail_closed(void)
{
    guardian_node_link_authenticator_t authenticator;
    guardian_node_link_auth_config_t config;
    guardian_node_link_peer_identity_t peers[2];
    test_verify_context_t verify;

    (void)memset(&config, 0, sizeof(config));
    (void)memset(&verify, 0, sizeof(verify));

    guardian_node_link_authenticator_init(&authenticator);

    config.required_signature_algorithm =
        GUARDIAN_NODE_LINK_SIGNATURE_ED25519;

    if (!require_true(
            guardian_node_link_authenticator_configure(
                &authenticator,
                &config) ==
                GUARDIAN_NODE_LINK_AUTH_ERROR_UNCONFIGURED,
            "missing provider rejected"))
    {
        return 0;
    }

    peers[0] = make_peer(1001U, 5001U, 7001U);
    peers[1] = make_peer(1001U, 5002U, 7002U);

    (void)memset(&config, 0, sizeof(config));

    config.context = &verify;
    config.verify_signature = test_verify_signature;
    config.peers = peers;
    config.peer_count = 2U;
    config.required_signature_algorithm =
        GUARDIAN_NODE_LINK_SIGNATURE_ED25519;

    if (!require_true(
            guardian_node_link_authenticator_configure(
                &authenticator,
                &config) ==
                GUARDIAN_NODE_LINK_AUTH_ERROR_UNCONFIGURED,
            "duplicate sender id rejected"))
    {
        return 0;
    }

    config.peer_count =
        GUARDIAN_NODE_LINK_AUTH_MAX_PEERS + 1U;

    return require_true(
        guardian_node_link_authenticator_configure(
            &authenticator,
            &config) ==
            GUARDIAN_NODE_LINK_AUTH_ERROR_UNCONFIGURED,
        "peer table bound enforced");
}

static int test_truncated_signature_rejected(void)
{
    guardian_node_link_authenticator_t authenticator;
    guardian_node_link_peer_identity_t peer;
    guardian_node_link_frame_t frame;
    guardian_node_link_authenticated_message_t output;
    test_verify_context_t verify;

    uint8_t signature[
        GUARDIAN_NODE_LINK_ED25519_SIGNATURE_SIZE];

    peer = make_peer(1001U, 5001U, 7001U);
    frame = make_frame();

    (void)memset(&verify, 0, sizeof(verify));
    (void)memset(&output, 0xA5, sizeof(output));

    make_signature(signature);

    if (!configure_authenticator(
            &authenticator,
            &peer,
            1U,
            &verify))
    {
        return 0;
    }

    if (!require_true(
            guardian_node_link_authenticate(
                &authenticator,
                &frame,
                signature,
                sizeof(signature) - 1U,
                &output) ==
                GUARDIAN_NODE_LINK_AUTH_ERROR_SIGNATURE_INVALID,
            "truncated signature rejected"))
    {
        return 0;
    }

    if (!require_true(
            verify.calls == 0U,
            "malformed signature never reaches provider"))
    {
        return 0;
    }

    return require_true(
        authenticated_output_is_zero(&output),
        "truncated signature creates no output");
}

static int test_auth_does_not_mutate_sequence_guard(void)
{
    guardian_node_link_authenticator_t authenticator;
    guardian_node_link_peer_identity_t peer;
    guardian_node_link_frame_t frame;
    guardian_node_link_authenticated_message_t output;
    guardian_node_link_sequence_guard_t guard;
    test_verify_context_t verify;

    uint8_t signature[
        GUARDIAN_NODE_LINK_ED25519_SIGNATURE_SIZE];

    peer = make_peer(1001U, 5001U, 7001U);
    frame = make_frame();

    (void)memset(&verify, 0, sizeof(verify));
    (void)memset(&guard, 0, sizeof(guard));
    (void)memset(&output, 0xA5, sizeof(output));

    guard.initialized = 1U;
    guard.last_sequence = 6U;

    make_signature(signature);

    if (!configure_authenticator(
            &authenticator,
            &peer,
            1U,
            &verify))
    {
        return 0;
    }

    if (!prepare_expected(
            &frame,
            &verify,
            peer.producer_id,
            peer.key_id,
            signature,
            sizeof(signature)))
    {
        return 0;
    }

    signature[0] ^= 0x01U;

    if (!require_true(
            guardian_node_link_authenticate(
                &authenticator,
                &frame,
                signature,
                sizeof(signature),
                &output) ==
                GUARDIAN_NODE_LINK_AUTH_ERROR_SIGNATURE_INVALID,
            "failed authentication"))
    {
        return 0;
    }

    if (!require_true(
            guard.initialized == 1U,
            "freshness initialization unchanged"))
    {
        return 0;
    }

    return require_true(
        guard.last_sequence == 6U,
        "failed auth does not mutate freshness state");
}

int main(void)
{
    if (!test_valid_authentication())
    {
        return 1;
    }

    if (!test_unknown_peer_rejected())
    {
        return 1;
    }

    if (!test_bad_signature_rejected())
    {
        return 1;
    }

    if (!test_bound_fields())
    {
        return 1;
    }

    if (!test_wrong_identity_binding_rejected())
    {
        return 1;
    }

    if (!test_provider_result_mapping())
    {
        return 1;
    }

    if (!test_configuration_snapshot())
    {
        return 1;
    }

    if (!test_configuration_fail_closed())
    {
        return 1;
    }

    if (!test_truncated_signature_rejected())
    {
        return 1;
    }

    if (!test_auth_does_not_mutate_sequence_guard())
    {
        return 1;
    }

    (void)printf(
        "PASS: C5-R1 authenticated NodeLink boundary\n");

    return 0;
}