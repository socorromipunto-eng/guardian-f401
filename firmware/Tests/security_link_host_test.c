/* Include the complete transport-independent middleware under M10 integration test. */
#include "guardian_embedded_link.h"

/* Include assertion support. */
#include <assert.h>

/* Include standard output for one concise CI success line. */
#include <stdio.h>

/* Include memory helpers for fake transport and MAC transcripts. */
#include <string.h>

/* Define bounded fake transport storage. */
#define TEST_QUEUE_CAPACITY ((size_t)1024U)

/* Store deterministic transport, time and nonce state. */
typedef struct
{
    /* Store host-to-device encoded bytes. */
    uint8_t rx[TEST_QUEUE_CAPACITY];

    /* Store valid host-to-device byte count. */
    size_t rx_size;

    /* Store next host-to-device byte index. */
    size_t rx_index;

    /* Store device-to-host encoded bytes. */
    uint8_t tx[TEST_QUEUE_CAPACITY];

    /* Store valid device-to-host byte count. */
    size_t tx_size;

    /* Store fake monotonic uptime seconds. */
    uint32_t uptime_seconds;

    /* Store deterministic 20-byte AUTH_BEGIN random output. */
    uint8_t random_bytes[20];
} test_context_t;

/* Read one fake transport byte without blocking. */
static int test_read_byte(
    void *context,
    uint8_t *byte)
{
    /* Recover fake state. */
    test_context_t *test =
        (test_context_t *)context;

    /* Report no byte when queue is exhausted. */
    if ((test == NULL) ||
        (byte == NULL) ||
        (test->rx_index >= test->rx_size))
    {
        /* Publish empty queue. */
        return 0;
    }

    /* Copy the next byte. */
    *byte =
        test->rx[
            test->rx_index];

    /* Advance queue ownership. */
    test->rx_index += 1U;

    /* Report one byte. */
    return 1;
}

/* Capture one complete TX block. */
static size_t test_write(
    void *context,
    const uint8_t *data,
    size_t length)
{
    /* Recover fake state. */
    test_context_t *test =
        (test_context_t *)context;

    /* Reject missing state, missing data or overflow. */
    if ((test == NULL) ||
        ((data == NULL) && (length != 0U)) ||
        ((test->tx_size + length) >
         sizeof(test->tx)))
    {
        /* Reject the complete write. */
        return 0U;
    }

    /* Append output bytes. */
    (void)memcpy(
        &test->tx[
            test->tx_size],
        data,
        length);

    /* Publish new TX size. */
    test->tx_size +=
        length;

    /* Report complete acceptance. */
    return length;
}

/* Return fake monotonic uptime. */
static uint32_t test_uptime(
    void *context)
{
    /* Recover fake state. */
    test_context_t *test =
        (test_context_t *)context;

    /* Require deterministic test state. */
    assert(test != NULL);

    /* Return current fake uptime. */
    return test->uptime_seconds;
}

/* Fill deterministic M10 random bytes. */
static int test_random(
    void *context,
    uint8_t *output,
    size_t length)
{
    /* Recover fake state. */
    test_context_t *test =
        (test_context_t *)context;

    /* Require valid state and expected byte count. */
    assert(test != NULL);
    assert(output != NULL);
    assert(length == sizeof(test->random_bytes));

    /* Copy deterministic session id and device nonce. */
    (void)memcpy(
        output,
        test->random_bytes,
        length);

    /* Report successful entropy callback. */
    return 1;
}

/* Write one big-endian 16-bit integer. */
static void test_write_u16_be(
    uint8_t *output,
    uint16_t value)
{
    /* Write both bytes explicitly. */
    output[0] = (uint8_t)((value >> 8U) & 0xFFU);
    output[1] = (uint8_t)(value & 0xFFU);
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

/* Calculate one handshake HMAC using the M10 canonical transcript. */
static void test_handshake_hmac(
    const uint8_t psk[32],
    const char *label,
    uint8_t role,
    uint32_t session_id,
    const uint8_t client_nonce[16],
    const uint8_t device_nonce[16],
    uint8_t digest[32])
{
    /* Store bounded transcript. */
    uint8_t transcript[80] = {0};

    /* Measure label. */
    size_t label_length =
        strlen(label);

    /* Track transcript length. */
    size_t length = 0U;

    /* Append label. */
    (void)memcpy(
        &transcript[length],
        label,
        label_length);

    /* Advance label bytes. */
    length +=
        label_length;

    /* Bind schema. */
    transcript[length++] =
        GUARDIAN_SECURITY_SCHEMA_VERSION;

    /* Bind role. */
    transcript[length++] =
        role;

    /* Bind session id. */
    test_write_u32_be(
        &transcript[length],
        session_id);

    /* Advance session id. */
    length += 4U;

    /* Bind client nonce. */
    (void)memcpy(
        &transcript[length],
        client_nonce,
        16U);

    /* Advance client nonce. */
    length += 16U;

    /* Bind device nonce. */
    (void)memcpy(
        &transcript[length],
        device_nonce,
        16U);

    /* Advance device nonce. */
    length += 16U;

    /* Calculate HMAC. */
    guardian_hmac_sha256(
        psk,
        32U,
        transcript,
        length,
        digest);

    /* Erase transcript. */
    guardian_crypto_zero(
        transcript,
        sizeof(transcript));
}

/* Build one authenticated SECURE_COMMAND outer request. */
static guardian_frame_t test_secure_request(
    const uint8_t session_key[32],
    uint32_t session_id,
    uint64_t counter,
    uint32_t sequence,
    uint8_t inner_command,
    const uint8_t *inner_payload,
    uint16_t inner_length)
{
    /* Create deterministic request. */
    guardian_frame_t frame = {0};

    /* Store request MAC transcript. */
    uint8_t transcript[288] = {0};

    /* Store full HMAC. */
    uint8_t digest[32] = {0};

    /* Define request domain label. */
    static const char label[] =
        "GF-M10-REQUEST";

    /* Track transcript length. */
    size_t length = 0U;

    /* Publish outer request class. */
    frame.message_type =
        GUARDIAN_MESSAGE_REQUEST;

    /* Publish SECURE_COMMAND. */
    frame.command =
        (uint8_t)GUARDIAN_COMMAND_SECURE_COMMAND;

    /* Publish outer sequence. */
    frame.sequence =
        sequence;

    /* Publish schema. */
    frame.payload[0] =
        GUARDIAN_SECURITY_SCHEMA_VERSION;

    /* Publish session id. */
    test_write_u32_be(
        &frame.payload[1],
        session_id);

    /* Publish counter. */
    test_write_u64_be(
        &frame.payload[5],
        counter);

    /* Publish inner command. */
    frame.payload[13] =
        inner_command;

    /* Publish inner length. */
    test_write_u16_be(
        &frame.payload[14],
        inner_length);

    /* Copy inner payload. */
    (void)memcpy(
        &frame.payload[16],
        inner_payload,
        inner_length);

    /* Append domain label. */
    (void)memcpy(
        transcript,
        label,
        sizeof(label) - 1U);

    /* Publish label length. */
    length =
        sizeof(label) - 1U;

    /* Bind schema. */
    transcript[length++] =
        GUARDIAN_SECURITY_SCHEMA_VERSION;

    /* Bind session id. */
    test_write_u32_be(
        &transcript[length],
        session_id);

    /* Advance. */
    length += 4U;

    /* Bind counter. */
    test_write_u64_be(
        &transcript[length],
        counter);

    /* Advance. */
    length += 8U;

    /* Bind outer sequence. */
    test_write_u32_be(
        &transcript[length],
        sequence);

    /* Advance. */
    length += 4U;

    /* Bind inner command. */
    transcript[length++] =
        inner_command;

    /* Bind inner length. */
    test_write_u16_be(
        &transcript[length],
        inner_length);

    /* Advance. */
    length += 2U;

    /* Bind payload. */
    (void)memcpy(
        &transcript[length],
        inner_payload,
        inner_length);

    /* Advance. */
    length +=
        inner_length;

    /* Calculate full request HMAC. */
    guardian_hmac_sha256(
        session_key,
        32U,
        transcript,
        length,
        digest);

    /* Append truncated tag. */
    (void)memcpy(
        &frame.payload[
            16U +
            inner_length],
        digest,
        GUARDIAN_SECURITY_TAG_SIZE);

    /* Publish exact envelope size. */
    frame.payload_length =
        (uint16_t)(
            GUARDIAN_SECURITY_REQUEST_OVERHEAD +
            inner_length);

    /* Erase temporary secret material. */
    guardian_crypto_zero(
        digest,
        sizeof(digest));

    /* Erase transcript. */
    guardian_crypto_zero(
        transcript,
        sizeof(transcript));

    /* Return request. */
    return frame;
}

/* Queue one complete Guardian request and clear prior response bytes. */
static void test_queue_request(
    test_context_t *test,
    const guardian_frame_t *request)
{
    /* Store encoded byte count. */
    size_t encoded_size = 0U;

    /* Reset fake RX ownership. */
    test->rx_index = 0U;

    /* Clear previous TX response. */
    test->tx_size = 0U;

    /* Encode request directly into RX storage. */
    assert(
        guardian_protocol_encode(
            request,
            test->rx,
            sizeof(test->rx),
            &encoded_size) ==
        GUARDIAN_PROTOCOL_OK);

    /* Publish valid RX bytes. */
    test->rx_size =
        encoded_size;
}

/* Poll the complete queued request and decode one response. */
static guardian_frame_t test_exchange(
    guardian_embedded_link_t *link,
    test_context_t *test,
    const guardian_frame_t *request)
{
    /* Queue the request. */
    test_queue_request(
        test,
        request);

    /* Process enough bytes for one maximum-size frame. */
    guardian_embedded_link_poll(
        link,
        GUARDIAN_MAX_FRAME_SIZE);

    /* Create decoded response storage. */
    guardian_frame_t response = {0};

    /* Decode the captured response. */
    assert(
        guardian_protocol_decode(
            test->tx,
            test->tx_size,
            &response) ==
        GUARDIAN_PROTOCOL_OK);

    /* Return decoded response by value. */
    return response;
}

/* Verify direct privilege rejection, authenticated baseline mutation and replay rejection. */
static void test_secure_embedded_link(void)
{
    /* Create fake transport state. */
    test_context_t test = {0};

    /* Create middleware instance. */
    guardian_embedded_link_t link = {0};

    /* Create I/O callbacks. */
    guardian_embedded_io_t io = {0};

    /* Create public device identity. */
    guardian_device_identity_t identity = {0};

    /* Create M10 provisioning. */
    guardian_security_config_t security_config = {0};

    /* Store client nonce. */
    uint8_t client_nonce[16] = {0};

    /* Store temporary HMAC. */
    uint8_t digest[32] = {0};

    /* Store derived session key. */
    uint8_t session_key[32] = {0};

    /* Track setup bytes. */
    uint8_t index = 0U;

    /* Connect fake RX. */
    io.read_byte =
        test_read_byte;

    /* Connect fake TX. */
    io.write =
        test_write;

    /* Connect fake uptime. */
    io.uptime_seconds =
        test_uptime;

    /* Share fake state. */
    io.context =
        &test;

    /* Publish deterministic identity. */
    identity.model =
        "Guardian-F401-M10-TEST";

    /* Publish milestone firmware version. */
    identity.firmware_minor =
        10U;

    /* Publish deterministic device id. */
    identity.device_id =
        0xF4010010UL;

    /* Initialize embedded middleware. */
    assert(
        guardian_embedded_link_init(
            &link,
            &io,
            &identity) ==
        GUARDIAN_PROTOCOL_OK);

    /* Fill test PSK with 0x00..0x1F. */
    for (index = 0U;
         index < 32U;
         ++index)
    {
        /* Publish deterministic key byte. */
        security_config.psk[index] =
            index;
    }

    /* Grant OPERATOR role ceiling. */
    security_config.max_role =
        GUARDIAN_SECURITY_ROLE_OPERATOR;

    /* Connect deterministic nonce source. */
    security_config.random =
        test_random;

    /* Share fake state. */
    security_config.random_context =
        &test;

    /* Use default timeout. */
    security_config.session_timeout_seconds =
        0U;

    /* Publish deterministic session id A1B2C3D4. */
    test.random_bytes[0] = 0xA1U;
    test.random_bytes[1] = 0xB2U;
    test.random_bytes[2] = 0xC3U;
    test.random_bytes[3] = 0xD4U;

    /* Fill device nonce and client nonce. */
    for (index = 0U;
         index < 16U;
         ++index)
    {
        /* Publish device nonce 0x20..0x2F. */
        test.random_bytes[4U + index] =
            (uint8_t)(0x20U + index);

        /* Publish client nonce 0x10..0x1F. */
        client_nonce[index] =
            (uint8_t)(0x10U + index);
    }

    /* Install security provisioning. */
    assert(
        guardian_embedded_link_configure_security(
            &link,
            &security_config) ==
        GUARDIAN_SECURITY_OK);

    /* Enable direct privileged-command rejection. */
    guardian_embedded_link_require_security(
        &link,
        1U);

    /* Build one valid direct baseline command. */
    guardian_frame_t request = {0};

    /* Publish direct request class. */
    request.message_type =
        GUARDIAN_MESSAGE_REQUEST;

    /* Select protected baseline command. */
    request.command =
        (uint8_t)GUARDIAN_COMMAND_BASELINE_CONTROL;

    /* Publish sequence. */
    request.sequence =
        1U;

    /* Publish baseline START payload. */
    request.payload_length =
        4U;

    /* Publish schema, START and sixteen-sample target. */
    request.payload[0] = 0x01U;
    request.payload[1] = 0x01U;
    request.payload[2] = 0x00U;
    request.payload[3] = 0x10U;

    /* Execute direct request. */
    guardian_frame_t response =
        test_exchange(
            &link,
            &test,
            &request);

    /* Require direct privileged rejection. */
    assert(
        response.message_type ==
        GUARDIAN_MESSAGE_ERROR);

    /* Require UNAUTHORIZED. */
    assert(
        response.payload[0] ==
        (uint8_t)GUARDIAN_ERROR_UNAUTHORIZED);

    /* Build AUTH_BEGIN. */
    (void)memset(
        &request,
        0,
        sizeof(request));

    /* Publish request class. */
    request.message_type =
        GUARDIAN_MESSAGE_REQUEST;

    /* Select AUTH_BEGIN. */
    request.command =
        (uint8_t)GUARDIAN_COMMAND_AUTH_BEGIN;

    /* Publish sequence. */
    request.sequence =
        2U;

    /* Publish fixed payload size. */
    request.payload_length =
        GUARDIAN_SECURITY_AUTH_BEGIN_REQUEST_SIZE;

    /* Publish schema and OPERATOR role. */
    request.payload[0] =
        GUARDIAN_SECURITY_SCHEMA_VERSION;
    request.payload[1] =
        (uint8_t)GUARDIAN_SECURITY_ROLE_OPERATOR;

    /* Copy client nonce. */
    (void)memcpy(
        &request.payload[2],
        client_nonce,
        16U);

    /* Execute challenge. */
    response =
        test_exchange(
            &link,
            &test,
            &request);

    /* Require successful challenge response. */
    assert(
        response.message_type ==
        GUARDIAN_MESSAGE_RESPONSE);

    /* Calculate client proof. */
    test_handshake_hmac(
        security_config.psk,
        "GF-M10-CLIENT",
        (uint8_t)GUARDIAN_SECURITY_ROLE_OPERATOR,
        0xA1B2C3D4UL,
        client_nonce,
        &test.random_bytes[4],
        digest);

    /* Build AUTH_FINISH. */
    (void)memset(
        &request,
        0,
        sizeof(request));

    /* Publish request class. */
    request.message_type =
        GUARDIAN_MESSAGE_REQUEST;

    /* Select AUTH_FINISH. */
    request.command =
        (uint8_t)GUARDIAN_COMMAND_AUTH_FINISH;

    /* Publish sequence. */
    request.sequence =
        3U;

    /* Publish fixed payload size. */
    request.payload_length =
        GUARDIAN_SECURITY_AUTH_FINISH_REQUEST_SIZE;

    /* Publish schema and role. */
    request.payload[0] =
        GUARDIAN_SECURITY_SCHEMA_VERSION;
    request.payload[1] =
        (uint8_t)GUARDIAN_SECURITY_ROLE_OPERATOR;

    /* Publish session id. */
    test_write_u32_be(
        &request.payload[2],
        0xA1B2C3D4UL);

    /* Publish client proof. */
    (void)memcpy(
        &request.payload[6],
        digest,
        GUARDIAN_SECURITY_TAG_SIZE);

    /* Execute authentication finish. */
    response =
        test_exchange(
            &link,
            &test,
            &request);

    /* Require successful authentication. */
    assert(
        response.message_type ==
        GUARDIAN_MESSAGE_RESPONSE);

    /* Derive session key. */
    test_handshake_hmac(
        security_config.psk,
        "GF-M10-SESSION",
        (uint8_t)GUARDIAN_SECURITY_ROLE_OPERATOR,
        0xA1B2C3D4UL,
        client_nonce,
        &test.random_bytes[4],
        session_key);

    /* Define protected baseline payload. */
    const uint8_t baseline_payload[4] =
    {
        0x01U,
        0x01U,
        0x00U,
        0x10U
    };

    /* Build authenticated counter-one baseline request. */
    request =
        test_secure_request(
            session_key,
            0xA1B2C3D4UL,
            1ULL,
            4U,
            (uint8_t)GUARDIAN_COMMAND_BASELINE_CONTROL,
            baseline_payload,
            sizeof(baseline_payload));

    /* Execute authenticated privileged request. */
    response =
        test_exchange(
            &link,
            &test,
            &request);

    /* Require successful outer secure response. */
    assert(
        response.message_type ==
        GUARDIAN_MESSAGE_RESPONSE);

    /* Require baseline state actually changed through authenticated dispatch. */
    assert(
        guardian_embedded_link_health_status(
            &link
        ).state ==
        GUARDIAN_HEALTH_STATE_LEARNING);

    /* Replay the exact same authenticated request. */
    response =
        test_exchange(
            &link,
            &test,
            &request);

    /* Require explicit replay rejection. */
    assert(
        response.message_type ==
        GUARDIAN_MESSAGE_ERROR);

    /* Require REPLAY_DETECTED. */
    assert(
        response.payload[0] ==
        (uint8_t)GUARDIAN_ERROR_REPLAY_DETECTED);
}

/* Execute the complete M10 embedded-link integration suite. */
int main(void)
{
    /* Verify secure command gate, handshake, authenticated dispatch and replay defense. */
    test_secure_embedded_link();

    /* Print one concise success line for CI logs. */
    (void)printf("Guardian M10 security link host tests: PASS\n");

    /* Return conventional successful process status. */
    return 0;
}
