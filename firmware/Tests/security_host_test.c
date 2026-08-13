/* Include portable M10 authentication/session policy under test. */
#include "guardian_security.h"

/* Include assertion support for deterministic host verification. */
#include <assert.h>

/* Include standard output for one concise CI success line. */
#include <stdio.h>

/* Include memory helpers for deterministic transcript construction. */
#include <string.h>

/* Store deterministic random callback state. */
typedef struct
{
    /* Store the next 20 bytes returned to AUTH_BEGIN. */
    uint8_t bytes[20];
} test_random_t;

/* Fill one deterministic nonce/session block for portable tests. */
static int test_random_fill(
    void *context,
    uint8_t *output,
    size_t length)
{
    /* Recover deterministic random state. */
    test_random_t *random =
        (test_random_t *)context;

    /* Require expected test storage. */
    assert(random != NULL);

    /* Require the exact AUTH_BEGIN random request width. */
    assert(length == sizeof(random->bytes));

    /* Copy deterministic test bytes. */
    (void)memcpy(
        output,
        random->bytes,
        length);

    /* Report successful entropy delivery. */
    return 1;
}

/* Write one big-endian 16-bit integer. */
static void test_write_u16_be(
    uint8_t *output,
    uint16_t value)
{
    /* Write most-significant byte first. */
    output[0] =
        (uint8_t)((value >> 8U) & 0xFFU);

    /* Write least-significant byte second. */
    output[1] =
        (uint8_t)(value & 0xFFU);
}

/* Write one big-endian 32-bit integer. */
static void test_write_u32_be(
    uint8_t *output,
    uint32_t value)
{
    /* Write all four bytes explicitly. */
    output[0] = (uint8_t)((value >> 24U) & 0xFFU);
    output[1] = (uint8_t)((value >> 16U) & 0xFFU);
    output[2] = (uint8_t)((value >> 8U) & 0xFFU);
    output[3] = (uint8_t)(value & 0xFFU);
}

/* Write one big-endian 64-bit integer. */
static void test_write_u64_be(
    uint8_t *output,
    uint64_t value)
{
    /* Write all eight bytes explicitly. */
    output[0] = (uint8_t)((value >> 56U) & 0xFFU);
    output[1] = (uint8_t)((value >> 48U) & 0xFFU);
    output[2] = (uint8_t)((value >> 40U) & 0xFFU);
    output[3] = (uint8_t)((value >> 32U) & 0xFFU);
    output[4] = (uint8_t)((value >> 24U) & 0xFFU);
    output[5] = (uint8_t)((value >> 16U) & 0xFFU);
    output[6] = (uint8_t)((value >> 8U) & 0xFFU);
    output[7] = (uint8_t)(value & 0xFFU);
}

/* Calculate one M10 handshake HMAC using the published canonical transcript. */
static void test_handshake_hmac(
    const uint8_t psk[32],
    const char *label,
    uint8_t role,
    uint32_t session_id,
    const uint8_t client_nonce[16],
    const uint8_t device_nonce[16],
    uint8_t digest[32])
{
    /* Store the complete small handshake transcript. */
    uint8_t transcript[80] = {0};

    /* Measure the ASCII domain label. */
    size_t label_length =
        strlen(label);

    /* Track the current transcript length. */
    size_t length = 0U;

    /* Copy the domain label. */
    (void)memcpy(
        &transcript[length],
        label,
        label_length);

    /* Advance transcript ownership. */
    length +=
        label_length;

    /* Bind schema revision one. */
    transcript[length++] =
        GUARDIAN_SECURITY_SCHEMA_VERSION;

    /* Bind authorization role. */
    transcript[length++] =
        role;

    /* Bind session identifier. */
    test_write_u32_be(
        &transcript[length],
        session_id);

    /* Advance past session identifier. */
    length += 4U;

    /* Bind host nonce. */
    (void)memcpy(
        &transcript[length],
        client_nonce,
        16U);

    /* Advance past host nonce. */
    length += 16U;

    /* Bind device nonce. */
    (void)memcpy(
        &transcript[length],
        device_nonce,
        16U);

    /* Advance past device nonce. */
    length += 16U;

    /* Calculate full HMAC-SHA-256. */
    guardian_hmac_sha256(
        psk,
        32U,
        transcript,
        length,
        digest);

    /* Erase the temporary transcript. */
    guardian_crypto_zero(
        transcript,
        sizeof(transcript));
}

/* Build one valid M10 secure request frame. */
static guardian_frame_t test_secure_request(
    const uint8_t session_key[32],
    uint32_t session_id,
    uint64_t counter,
    uint32_t sequence,
    uint8_t inner_command,
    const uint8_t *inner_payload,
    uint16_t inner_length)
{
    /* Create deterministic outer frame storage. */
    guardian_frame_t frame = {0};

    /* Store the request-MAC canonical transcript. */
    uint8_t transcript[288] = {0};

    /* Store full request HMAC. */
    uint8_t digest[32] = {0};

    /* Define exact request domain label. */
    static const char label[] =
        "GF-M10-REQUEST";

    /* Track transcript length. */
    size_t length = 0U;

    /* Publish ordinary outer request semantics. */
    frame.message_type =
        GUARDIAN_MESSAGE_REQUEST;

    /* Select the M10 secure envelope command. */
    frame.command =
        (uint8_t)GUARDIAN_COMMAND_SECURE_COMMAND;

    /* Publish correlation sequence. */
    frame.sequence =
        sequence;

    /* Publish schema revision one. */
    frame.payload[0] =
        GUARDIAN_SECURITY_SCHEMA_VERSION;

    /* Publish session identifier. */
    test_write_u32_be(
        &frame.payload[1],
        session_id);

    /* Publish strict anti-replay counter. */
    test_write_u64_be(
        &frame.payload[5],
        counter);

    /* Publish inner command. */
    frame.payload[13] =
        inner_command;

    /* Publish exact inner payload size. */
    test_write_u16_be(
        &frame.payload[14],
        inner_length);

    /* Copy inner payload. */
    if (inner_length != 0U)
    {
        /* Copy only bounded inner bytes. */
        (void)memcpy(
            &frame.payload[16],
            inner_payload,
            inner_length);
    }

    /* Build canonical MAC transcript domain label. */
    (void)memcpy(
        &transcript[length],
        label,
        sizeof(label) - 1U);

    /* Advance label length. */
    length +=
        sizeof(label) - 1U;

    /* Bind schema revision. */
    transcript[length++] =
        GUARDIAN_SECURITY_SCHEMA_VERSION;

    /* Bind session identifier. */
    test_write_u32_be(
        &transcript[length],
        session_id);

    /* Advance field length. */
    length += 4U;

    /* Bind request counter. */
    test_write_u64_be(
        &transcript[length],
        counter);

    /* Advance field length. */
    length += 8U;

    /* Bind outer sequence. */
    test_write_u32_be(
        &transcript[length],
        sequence);

    /* Advance field length. */
    length += 4U;

    /* Bind inner command. */
    transcript[length++] =
        inner_command;

    /* Bind inner payload size. */
    test_write_u16_be(
        &transcript[length],
        inner_length);

    /* Advance field length. */
    length += 2U;

    /* Bind inner payload bytes. */
    if (inner_length != 0U)
    {
        /* Copy payload into canonical transcript. */
        (void)memcpy(
            &transcript[length],
            inner_payload,
            inner_length);

        /* Advance transcript length. */
        length +=
            inner_length;
    }

    /* Calculate full session HMAC. */
    guardian_hmac_sha256(
        session_key,
        32U,
        transcript,
        length,
        digest);

    /* Copy the transmitted 128-bit tag. */
    (void)memcpy(
        &frame.payload[
            16U +
            inner_length],
        digest,
        GUARDIAN_SECURITY_TAG_SIZE);

    /* Publish exact envelope length. */
    frame.payload_length =
        (uint16_t)(
            GUARDIAN_SECURITY_REQUEST_OVERHEAD +
            inner_length);

    /* Erase temporary transcript and digest. */
    guardian_crypto_zero(
        transcript,
        sizeof(transcript));

    /* Erase full HMAC after truncation. */
    guardian_crypto_zero(
        digest,
        sizeof(digest));

    /* Return complete secure frame by value. */
    return frame;
}

/* Verify SHA-256 and HMAC-SHA-256 against published standard vectors. */
static void test_crypto_vectors(void)
{
    /* Store SHA-256 digest. */
    uint8_t digest[32] = {0};

    /* Define SHA-256("abc"). */
    static const uint8_t expected_sha256[32] =
    {
        0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA,
        0x41, 0x41, 0x40, 0xDE, 0x5D, 0xAE, 0x22, 0x23,
        0xB0, 0x03, 0x61, 0xA3, 0x96, 0x17, 0x7A, 0x9C,
        0xB4, 0x10, 0xFF, 0x61, 0xF2, 0x00, 0x15, 0xAD
    };

    /* Hash the canonical three-byte message. */
    guardian_sha256(
        (const uint8_t *)"abc",
        3U,
        digest);

    /* Require exact SHA-256 vector equality. */
    assert(
        memcmp(
            digest,
            expected_sha256,
            sizeof(expected_sha256)) == 0);

    /* Define RFC 4231 test-case-one key. */
    uint8_t hmac_key[20] = {0};

    /* Fill the RFC key with byte 0x0B. */
    (void)memset(
        hmac_key,
        0x0B,
        sizeof(hmac_key));

    /* Define RFC 4231 expected HMAC-SHA-256. */
    static const uint8_t expected_hmac[32] =
    {
        0xB0, 0x34, 0x4C, 0x61, 0xD8, 0xDB, 0x38, 0x53,
        0x5C, 0xA8, 0xAF, 0xCE, 0xAF, 0x0B, 0xF1, 0x2B,
        0x88, 0x1D, 0xC2, 0x00, 0xC9, 0x83, 0x3D, 0xA7,
        0x26, 0xE9, 0x37, 0x6C, 0x2E, 0x32, 0xCF, 0xF7
    };

    /* Calculate RFC HMAC test vector. */
    guardian_hmac_sha256(
        hmac_key,
        sizeof(hmac_key),
        (const uint8_t *)"Hi There",
        8U,
        digest);

    /* Require exact RFC vector equality. */
    assert(
        memcmp(
            digest,
            expected_hmac,
            sizeof(expected_hmac)) == 0);
}

/* Establish one deterministic OPERATOR session for anti-replay tests. */
static void test_authenticated_session_and_replay(void)
{
    /* Create unprovisioned security state. */
    guardian_security_t security = {0};

    /* Create deterministic provisioning. */
    guardian_security_config_t config = {0};

    /* Create deterministic random callback state. */
    test_random_t random = {0};

    /* Create AUTH_BEGIN request bytes. */
    uint8_t begin_request[GUARDIAN_SECURITY_AUTH_BEGIN_REQUEST_SIZE] = {0};

    /* Create response storage. */
    uint8_t response[GUARDIAN_MAX_PAYLOAD_SIZE] = {0};

    /* Store response length. */
    uint16_t response_length = 0U;

    /* Store deterministic client nonce. */
    uint8_t client_nonce[16] = {0};

    /* Store expected client proof. */
    uint8_t client_proof[32] = {0};

    /* Store derived session key. */
    uint8_t session_key[32] = {0};

    /* Track setup bytes. */
    uint8_t index = 0U;

    /* Initialize security state. */
    guardian_security_init(
        &security);

    /* Fill deterministic 256-bit PSK with 0x00..0x1F. */
    for (index = 0U; index < 32U; ++index)
    {
        /* Publish one deterministic key byte. */
        config.psk[index] =
            index;
    }

    /* Grant OPERATOR ceiling. */
    config.max_role =
        GUARDIAN_SECURITY_ROLE_OPERATOR;

    /* Connect deterministic random callback. */
    config.random =
        test_random_fill;

    /* Share deterministic random state. */
    config.random_context =
        &random;

    /* Use default timeout through zero configuration. */
    config.session_timeout_seconds = 0U;

    /* Fill random session id A1B2C3D4. */
    random.bytes[0] = 0xA1U;
    random.bytes[1] = 0xB2U;
    random.bytes[2] = 0xC3U;
    random.bytes[3] = 0xD4U;

    /* Fill deterministic device nonce 0x20..0x2F. */
    for (index = 0U; index < 16U; ++index)
    {
        /* Publish one device nonce byte. */
        random.bytes[4U + index] =
            (uint8_t)(0x20U + index);

        /* Publish one client nonce byte 0x10..0x1F. */
        client_nonce[index] =
            (uint8_t)(0x10U + index);
    }

    /* Install provisioning. */
    assert(
        guardian_security_configure(
            &security,
            &config) ==
        GUARDIAN_SECURITY_OK);

    /* Build AUTH_BEGIN schema and role. */
    begin_request[0] =
        GUARDIAN_SECURITY_SCHEMA_VERSION;

    /* Request OPERATOR. */
    begin_request[1] =
        (uint8_t)GUARDIAN_SECURITY_ROLE_OPERATOR;

    /* Copy client nonce. */
    (void)memcpy(
        &begin_request[2],
        client_nonce,
        sizeof(client_nonce));

    /* Create deterministic challenge. */
    assert(
        guardian_security_auth_begin(
            &security,
            begin_request,
            sizeof(begin_request),
            100U,
            response,
            sizeof(response),
            &response_length) ==
        GUARDIAN_SECURITY_OK);

    /* Require exact challenge size. */
    assert(
        response_length ==
        GUARDIAN_SECURITY_AUTH_BEGIN_RESPONSE_SIZE);

    /* Require deterministic session id. */
    assert(
        response[2] == 0xA1U &&
        response[3] == 0xB2U &&
        response[4] == 0xC3U &&
        response[5] == 0xD4U);

    /* Require known cross-language server proof. */
    static const uint8_t expected_server_proof[16] =
    {
        0x2F, 0xB8, 0xE4, 0xEA, 0xCE, 0x99, 0xC1, 0xE3,
        0x80, 0xCB, 0x72, 0xC5, 0x42, 0x7F, 0xC3, 0x51
    };

    /* Compare transmitted proof. */
    assert(
        memcmp(
            &response[22],
            expected_server_proof,
            sizeof(expected_server_proof)) == 0);

    /* Calculate deterministic client proof. */
    test_handshake_hmac(
        config.psk,
        "GF-M10-CLIENT",
        (uint8_t)GUARDIAN_SECURITY_ROLE_OPERATOR,
        0xA1B2C3D4UL,
        client_nonce,
        &random.bytes[4],
        client_proof);

    /* Build fixed AUTH_FINISH request. */
    uint8_t finish_request[GUARDIAN_SECURITY_AUTH_FINISH_REQUEST_SIZE] = {0};

    /* Publish schema revision one. */
    finish_request[0] =
        GUARDIAN_SECURITY_SCHEMA_VERSION;

    /* Publish OPERATOR role. */
    finish_request[1] =
        (uint8_t)GUARDIAN_SECURITY_ROLE_OPERATOR;

    /* Publish session identifier. */
    test_write_u32_be(
        &finish_request[2],
        0xA1B2C3D4UL);

    /* Publish truncated client proof. */
    (void)memcpy(
        &finish_request[6],
        client_proof,
        GUARDIAN_SECURITY_TAG_SIZE);

    /* Finish authentication. */
    assert(
        guardian_security_auth_finish(
            &security,
            finish_request,
            sizeof(finish_request),
            101U,
            response,
            sizeof(response),
            &response_length) ==
        GUARDIAN_SECURITY_OK);

    /* Require exact session acknowledgement size. */
    assert(
        response_length ==
        GUARDIAN_SECURITY_AUTH_FINISH_RESPONSE_SIZE);

    /* Derive deterministic session key. */
    test_handshake_hmac(
        config.psk,
        "GF-M10-SESSION",
        (uint8_t)GUARDIAN_SECURITY_ROLE_OPERATOR,
        0xA1B2C3D4UL,
        client_nonce,
        &random.bytes[4],
        session_key);

    /* Require known cross-language session key. */
    static const uint8_t expected_session_key[32] =
    {
        0x9B, 0x2A, 0xA5, 0xA0, 0x6D, 0x31, 0xAF, 0x37,
        0xE7, 0xC3, 0x6F, 0x0A, 0x65, 0x47, 0x2C, 0x5C,
        0xC6, 0x1C, 0xC8, 0x64, 0x4B, 0x5E, 0x3A, 0xA8,
        0x02, 0x06, 0x6A, 0x05, 0x50, 0xD2, 0x81, 0xA0
    };

    /* Require exact session-key derivation parity. */
    assert(
        memcmp(
            session_key,
            expected_session_key,
            sizeof(expected_session_key)) == 0);

    /* Define one valid M8 baseline-control payload. */
    const uint8_t baseline_payload[4] =
    {
        0x01U,
        0x01U,
        0x00U,
        0x10U
    };

    /* Build valid secure counter-one request. */
    guardian_frame_t outer =
        test_secure_request(
            session_key,
            0xA1B2C3D4UL,
            1ULL,
            77U,
            (uint8_t)GUARDIAN_COMMAND_BASELINE_CONTROL,
            baseline_payload,
            sizeof(baseline_payload));

    /* Create inner request storage. */
    guardian_frame_t inner = {0};

    /* Store verified counter. */
    uint64_t verified_counter = 0ULL;

    /* Authenticate and unwrap counter one. */
    assert(
        guardian_security_unwrap_request(
            &security,
            &outer,
            102U,
            &inner,
            &verified_counter) ==
        GUARDIAN_SECURITY_OK);

    /* Require exact verified counter. */
    assert(
        verified_counter ==
        1ULL);

    /* Require OPERATOR authorization for baseline control. */
    assert(
        guardian_security_authorize(
            &security,
            inner.command) ==
        1);

    /* Replay the exact same authenticated request. */
    assert(
        guardian_security_unwrap_request(
            &security,
            &outer,
            103U,
            &inner,
            &verified_counter) ==
        GUARDIAN_SECURITY_ERROR_REPLAY);

    /* Build valid counter-two request. */
    outer =
        test_secure_request(
            session_key,
            0xA1B2C3D4UL,
            2ULL,
            78U,
            (uint8_t)GUARDIAN_COMMAND_CONTROL_COMMAND,
            (const uint8_t *)"\x01\x01",
            2U);

    /* Corrupt one transmitted tag byte. */
    outer.payload[
        outer.payload_length - 1U] ^=
        0x01U;

    /* Require tag failure without consuming counter two. */
    assert(
        guardian_security_unwrap_request(
            &security,
            &outer,
            104U,
            &inner,
            &verified_counter) ==
        GUARDIAN_SECURITY_ERROR_UNAUTHORIZED);

    /* Rebuild the correct counter-two request. */
    outer =
        test_secure_request(
            session_key,
            0xA1B2C3D4UL,
            2ULL,
            79U,
            (uint8_t)GUARDIAN_COMMAND_CONTROL_COMMAND,
            (const uint8_t *)"\x01\x01",
            2U);

    /* Require valid counter two still succeeds. */
    assert(
        guardian_security_unwrap_request(
            &security,
            &outer,
            105U,
            &inner,
            &verified_counter) ==
        GUARDIAN_SECURITY_OK);

    /* Read public diagnostics. */
    guardian_security_status_t status =
        guardian_security_status(
            &security,
            105U);

    /* Require active authenticated session. */
    assert(status.active == 1U);

    /* Require two valid secure requests consumed. */
    assert(status.next_counter == 3ULL);

    /* Require one replay rejection. */
    assert(status.replay_rejections == 1U);

    /* Require at least one authentication/tag failure. */
    assert(status.auth_failures >= 1U);
}

/* Execute every portable M10 security test. */
int main(void)
{
    /* Verify cryptographic primitive interoperability. */
    test_crypto_vectors();

    /* Verify challenge-response, session key, tags and strict anti-replay. */
    test_authenticated_session_and_replay();

    /* Print one concise success line for local and CI logs. */
    (void)printf("Guardian M10 security host tests: PASS\n");

    /* Return conventional successful process status. */
    return 0;
}
