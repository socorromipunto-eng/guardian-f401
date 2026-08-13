/* Include the public authenticated-session declarations. */
#include "guardian_security.h"

/* Include memory helpers for bounded transcript construction. */
#include <string.h>

/* Define domain-separation labels for every HMAC purpose. */
static const uint8_t guardian_security_server_label[] =
    "GF-M10-SERVER";

static const uint8_t guardian_security_client_label[] =
    "GF-M10-CLIENT";

static const uint8_t guardian_security_session_label[] =
    "GF-M10-SESSION";

static const uint8_t guardian_security_request_label[] =
    "GF-M10-REQUEST";

static const uint8_t guardian_security_response_label[] =
    "GF-M10-RESPONSE";

/* Define one bounded stack transcript large enough for every M10 MAC input. */
#define GUARDIAN_SECURITY_TRANSCRIPT_CAPACITY ((size_t)288U)

/* Saturating-increment one unsigned 32-bit security diagnostic. */
static void guardian_security_increment_u32(
    uint32_t *value)
{
    /* Ignore missing diagnostic storage defensively. */
    if (value == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Avoid diagnostic counter wrap. */
    if (*value != 0xFFFFFFFFUL)
    {
        /* Increment only while representable. */
        *value += 1U;
    }
}

/* Write one unsigned 16-bit integer in Guardian big-endian order. */
static void guardian_security_write_u16_be(
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
static void guardian_security_write_u32_be(
    uint8_t *output,
    uint32_t value)
{
    /* Write byte three first. */
    output[0] =
        (uint8_t)((value >> 24U) & 0xFFU);

    /* Write byte two. */
    output[1] =
        (uint8_t)((value >> 16U) & 0xFFU);

    /* Write byte one. */
    output[2] =
        (uint8_t)((value >> 8U) & 0xFFU);

    /* Write byte zero last. */
    output[3] =
        (uint8_t)(value & 0xFFU);
}

/* Write one unsigned 64-bit integer in Guardian big-endian order. */
static void guardian_security_write_u64_be(
    uint8_t *output,
    uint64_t value)
{
    /* Write every byte from most significant to least significant. */
    output[0] = (uint8_t)((value >> 56U) & 0xFFU);
    output[1] = (uint8_t)((value >> 48U) & 0xFFU);
    output[2] = (uint8_t)((value >> 40U) & 0xFFU);
    output[3] = (uint8_t)((value >> 32U) & 0xFFU);
    output[4] = (uint8_t)((value >> 24U) & 0xFFU);
    output[5] = (uint8_t)((value >> 16U) & 0xFFU);
    output[6] = (uint8_t)((value >> 8U) & 0xFFU);
    output[7] = (uint8_t)(value & 0xFFU);
}

/* Read one unsigned 16-bit integer in Guardian big-endian order. */
static uint16_t guardian_security_read_u16_be(
    const uint8_t *input)
{
    /* Combine both wire bytes explicitly. */
    return (uint16_t)(
        ((uint16_t)input[0] << 8U) |
        (uint16_t)input[1]
    );
}

/* Read one unsigned 32-bit integer in Guardian big-endian order. */
static uint32_t guardian_security_read_u32_be(
    const uint8_t *input)
{
    /* Combine all four wire bytes explicitly. */
    return
        ((uint32_t)input[0] << 24U) |
        ((uint32_t)input[1] << 16U) |
        ((uint32_t)input[2] << 8U) |
        (uint32_t)input[3];
}

/* Read one unsigned 64-bit integer in Guardian big-endian order. */
static uint64_t guardian_security_read_u64_be(
    const uint8_t *input)
{
    /* Combine all eight wire bytes explicitly. */
    return
        ((uint64_t)input[0] << 56U) |
        ((uint64_t)input[1] << 48U) |
        ((uint64_t)input[2] << 40U) |
        ((uint64_t)input[3] << 32U) |
        ((uint64_t)input[4] << 24U) |
        ((uint64_t)input[5] << 16U) |
        ((uint64_t)input[6] << 8U) |
        (uint64_t)input[7];
}

/* Erase one pending handshake transcript. */
static void guardian_security_clear_pending(
    guardian_security_t *security)
{
    /* Erase nonce and session transcript state. */
    guardian_crypto_zero(
        &security->pending,
        sizeof(security->pending));
}

/* Erase one active authenticated session. */
static void guardian_security_clear_session(
    guardian_security_t *security)
{
    /* Erase the derived session key and counters. */
    guardian_crypto_zero(
        &security->session,
        sizeof(security->session));
}

/* Expire a pending handshake after its short lifetime. */
static void guardian_security_expire_pending(
    guardian_security_t *security,
    uint32_t now_seconds)
{
    /* Ignore absent pending state. */
    if (security->pending.valid == 0U)
    {
        /* Return without work. */
        return;
    }

    /* Calculate wrap-safe elapsed monotonic seconds. */
    uint32_t elapsed =
        now_seconds -
        security->pending.started_seconds;

    /* Erase stale unauthenticated challenge state. */
    if (elapsed >=
        (uint32_t)GUARDIAN_SECURITY_PENDING_TIMEOUT_SECONDS)
    {
        /* Clear the expired pending transcript. */
        guardian_security_clear_pending(
            security);
    }
}

/* Expire one authenticated session after configured inactivity. */
static void guardian_security_expire_session(
    guardian_security_t *security,
    uint32_t now_seconds)
{
    /* Ignore absent active state. */
    if (security->session.valid == 0U)
    {
        /* Return without work. */
        return;
    }

    /* Calculate wrap-safe elapsed monotonic seconds. */
    uint32_t elapsed =
        now_seconds -
        security->session.last_activity_seconds;

    /* Expire at or beyond the configured inactivity bound. */
    if (elapsed >=
        (uint32_t)security->config.session_timeout_seconds)
    {
        /* Erase the derived session key and authenticated state. */
        guardian_security_clear_session(
            security);
    }
}

/* Append bounded bytes to one transcript buffer. */
static int guardian_security_append(
    uint8_t *transcript,
    size_t capacity,
    size_t *length,
    const uint8_t *data,
    size_t data_length)
{
    /* Reject missing required transcript state. */
    if ((transcript == NULL) ||
        (length == NULL) ||
        ((data == NULL) && (data_length != 0U)))
    {
        /* Report append failure. */
        return 0;
    }

    /* Reject transcript overflow. */
    if ((*length + data_length) >
        capacity)
    {
        /* Report append failure. */
        return 0;
    }

    /* Copy only when bytes are present. */
    if (data_length != 0U)
    {
        /* Append caller bytes. */
        (void)memcpy(
            &transcript[*length],
            data,
            data_length);
    }

    /* Publish the new transcript length. */
    *length +=
        data_length;

    /* Report successful bounded append. */
    return 1;
}

/* Calculate one handshake proof or session key using domain separation. */
static int guardian_security_handshake_mac(
    const guardian_security_t *security,
    const uint8_t *label,
    size_t label_length,
    guardian_security_role_t role,
    uint32_t session_id,
    const uint8_t client_nonce[GUARDIAN_SECURITY_NONCE_SIZE],
    const uint8_t device_nonce[GUARDIAN_SECURITY_NONCE_SIZE],
    uint8_t digest[GUARDIAN_SHA256_SIZE])
{
    /* Store the complete bounded handshake transcript. */
    uint8_t transcript[GUARDIAN_SECURITY_TRANSCRIPT_CAPACITY] = {0};

    /* Track valid transcript bytes. */
    size_t length = 0U;

    /* Store canonical scalar fields. */
    uint8_t scalar[6] = {0};

    /* Append the domain-separation label. */
    if (guardian_security_append(
            transcript,
            sizeof(transcript),
            &length,
            label,
            label_length) == 0)
    {
        /* Report internal transcript overflow. */
        return 0;
    }

    /* Bind the security schema version. */
    scalar[0] =
        GUARDIAN_SECURITY_SCHEMA_VERSION;

    /* Bind the requested/granted authorization role. */
    scalar[1] =
        (uint8_t)role;

    /* Encode the random session identifier. */
    guardian_security_write_u32_be(
        &scalar[2],
        session_id);

    /* Append canonical scalar fields. */
    if (guardian_security_append(
            transcript,
            sizeof(transcript),
            &length,
            scalar,
            sizeof(scalar)) == 0)
    {
        /* Report internal transcript overflow. */
        return 0;
    }

    /* Bind the host challenge nonce. */
    if (guardian_security_append(
            transcript,
            sizeof(transcript),
            &length,
            client_nonce,
            GUARDIAN_SECURITY_NONCE_SIZE) == 0)
    {
        /* Report internal transcript overflow. */
        return 0;
    }

    /* Bind the device challenge nonce. */
    if (guardian_security_append(
            transcript,
            sizeof(transcript),
            &length,
            device_nonce,
            GUARDIAN_SECURITY_NONCE_SIZE) == 0)
    {
        /* Report internal transcript overflow. */
        return 0;
    }

    /* Authenticate/derive with the provisioned 256-bit PSK. */
    guardian_hmac_sha256(
        security->config.psk,
        GUARDIAN_SECURITY_PSK_SIZE,
        transcript,
        length,
        digest);

    /* Erase the complete handshake transcript. */
    guardian_crypto_zero(
        transcript,
        sizeof(transcript));

    /* Report successful MAC calculation. */
    return 1;
}

/* Calculate one secure request tag. */
static int guardian_security_request_tag(
    const guardian_security_t *security,
    const guardian_frame_t *outer_request,
    uint32_t session_id,
    uint64_t counter,
    uint8_t inner_command,
    const uint8_t *inner_payload,
    uint16_t inner_length,
    uint8_t tag[GUARDIAN_SHA256_SIZE])
{
    /* Store the complete bounded authenticated request transcript. */
    uint8_t transcript[GUARDIAN_SECURITY_TRANSCRIPT_CAPACITY] = {0};

    /* Track valid transcript bytes. */
    size_t length = 0U;

    /* Store canonical fixed request fields. */
    uint8_t scalar[20] = {0};

    /* Append the request-specific domain label. */
    if (guardian_security_append(
            transcript,
            sizeof(transcript),
            &length,
            guardian_security_request_label,
            sizeof(guardian_security_request_label) - 1U) == 0)
    {
        /* Report internal transcript overflow. */
        return 0;
    }

    /* Bind schema revision. */
    scalar[0] =
        GUARDIAN_SECURITY_SCHEMA_VERSION;

    /* Encode session identifier. */
    guardian_security_write_u32_be(
        &scalar[1],
        session_id);

    /* Encode strict anti-replay counter. */
    guardian_security_write_u64_be(
        &scalar[5],
        counter);

    /* Bind outer request correlation sequence. */
    guardian_security_write_u32_be(
        &scalar[13],
        outer_request->sequence);

    /* Bind privileged command identifier. */
    scalar[17] =
        inner_command;

    /* Bind exact inner payload length. */
    guardian_security_write_u16_be(
        &scalar[18],
        inner_length);

    /* Append canonical fixed fields. */
    if (guardian_security_append(
            transcript,
            sizeof(transcript),
            &length,
            scalar,
            sizeof(scalar)) == 0)
    {
        /* Report internal transcript overflow. */
        return 0;
    }

    /* Bind the complete inner request payload. */
    if (guardian_security_append(
            transcript,
            sizeof(transcript),
            &length,
            inner_payload,
            inner_length) == 0)
    {
        /* Report internal transcript overflow. */
        return 0;
    }

    /* Calculate full HMAC-SHA-256 with the derived session key. */
    guardian_hmac_sha256(
        security->session.session_key,
        GUARDIAN_SECURITY_SESSION_KEY_SIZE,
        transcript,
        length,
        tag);

    /* Erase the authenticated transcript. */
    guardian_crypto_zero(
        transcript,
        sizeof(transcript));

    /* Report successful tag calculation. */
    return 1;
}

/* Calculate one secure response tag. */
static int guardian_security_response_tag(
    const guardian_security_t *security,
    const guardian_frame_t *outer_request,
    uint32_t session_id,
    uint64_t counter,
    const guardian_frame_t *inner_response,
    uint8_t tag[GUARDIAN_SHA256_SIZE])
{
    /* Store the complete bounded authenticated response transcript. */
    uint8_t transcript[GUARDIAN_SECURITY_TRANSCRIPT_CAPACITY] = {0};

    /* Track valid transcript bytes. */
    size_t length = 0U;

    /* Store canonical fixed response fields. */
    uint8_t scalar[21] = {0};

    /* Append the response-specific domain label. */
    if (guardian_security_append(
            transcript,
            sizeof(transcript),
            &length,
            guardian_security_response_label,
            sizeof(guardian_security_response_label) - 1U) == 0)
    {
        /* Report internal transcript overflow. */
        return 0;
    }

    /* Bind schema revision. */
    scalar[0] =
        GUARDIAN_SECURITY_SCHEMA_VERSION;

    /* Encode session identifier. */
    guardian_security_write_u32_be(
        &scalar[1],
        session_id);

    /* Encode request counter echoed into the response. */
    guardian_security_write_u64_be(
        &scalar[5],
        counter);

    /* Bind the original outer request sequence. */
    guardian_security_write_u32_be(
        &scalar[13],
        outer_request->sequence);

    /* Bind inner response message type. */
    scalar[17] =
        (uint8_t)inner_response->message_type;

    /* Bind inner command identifier. */
    scalar[18] =
        inner_response->command;

    /* Bind exact inner response payload length. */
    guardian_security_write_u16_be(
        &scalar[19],
        inner_response->payload_length);

    /* Append canonical fixed fields. */
    if (guardian_security_append(
            transcript,
            sizeof(transcript),
            &length,
            scalar,
            sizeof(scalar)) == 0)
    {
        /* Report internal transcript overflow. */
        return 0;
    }

    /* Bind the complete inner response payload. */
    if (guardian_security_append(
            transcript,
            sizeof(transcript),
            &length,
            inner_response->payload,
            inner_response->payload_length) == 0)
    {
        /* Report internal transcript overflow. */
        return 0;
    }

    /* Calculate full HMAC-SHA-256 with the active session key. */
    guardian_hmac_sha256(
        security->session.session_key,
        GUARDIAN_SECURITY_SESSION_KEY_SIZE,
        transcript,
        length,
        tag);

    /* Erase the authenticated transcript. */
    guardian_crypto_zero(
        transcript,
        sizeof(transcript));

    /* Report successful tag calculation. */
    return 1;
}

/* Initialize one unprovisioned security context. */
void guardian_security_init(
    guardian_security_t *security)
{
    /* Ignore missing caller storage defensively. */
    if (security == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Clear every secret and diagnostic field. */
    (void)memset(
        security,
        0,
        sizeof(*security));
}

/* Install provisioned PSK, role ceiling and cryptographic nonce callback. */
guardian_security_result_t guardian_security_configure(
    guardian_security_t *security,
    const guardian_security_config_t *config)
{
    /* Reject missing required storage. */
    if ((security == NULL) ||
        (config == NULL))
    {
        /* Report invalid caller state. */
        return GUARDIAN_SECURITY_ERROR_NULL_ARGUMENT;
    }

    /* Require a supported non-zero authorization ceiling. */
    if ((config->max_role <
         GUARDIAN_SECURITY_ROLE_OBSERVER) ||
        (config->max_role >
         GUARDIAN_SECURITY_ROLE_ADMIN))
    {
        /* Reject undefined authorization policy. */
        return GUARDIAN_SECURITY_ERROR_INVALID_PAYLOAD;
    }

    /* Require a platform cryptographic nonce source. */
    if (config->random == NULL)
    {
        /* Refuse insecure nonce generation. */
        return GUARDIAN_SECURITY_ERROR_INVALID_PAYLOAD;
    }

    /* Erase any previous key and session material before replacement. */
    guardian_security_clear(
        security);

    /* Copy the complete fixed provisioning configuration by value. */
    security->config =
        *config;

    /* Apply the documented timeout when the caller supplied zero. */
    if (security->config.session_timeout_seconds == 0U)
    {
        /* Use the conservative default inactivity timeout. */
        security->config.session_timeout_seconds =
            GUARDIAN_SECURITY_DEFAULT_SESSION_TIMEOUT_SECONDS;
    }

    /* Mark provisioning usable only after all validation succeeded. */
    security->configured = 1U;

    /* Report successful provisioning. */
    return GUARDIAN_SECURITY_OK;
}

/* Erase provisioning, pending transcript and active session key material. */
void guardian_security_clear(
    guardian_security_t *security)
{
    /* Ignore missing caller storage defensively. */
    if (security == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Erase the complete structure including copied PSK and session key. */
    guardian_crypto_zero(
        security,
        sizeof(*security));
}

/* Process one AUTH_BEGIN payload and build its fixed response payload. */
guardian_security_result_t guardian_security_auth_begin(
    guardian_security_t *security,
    const uint8_t *request_payload,
    uint16_t request_length,
    uint32_t now_seconds,
    uint8_t *response_payload,
    uint16_t response_capacity,
    uint16_t *response_length)
{
    /* Store fresh random session identifier and device nonce bytes together. */
    uint8_t random_bytes[20] = {0};

    /* Store the full HMAC server proof before truncation. */
    uint8_t server_proof[GUARDIAN_SHA256_SIZE] = {0};

    /* Reject missing required storage. */
    if ((security == NULL) ||
        (request_payload == NULL) ||
        (response_payload == NULL) ||
        (response_length == NULL))
    {
        /* Report invalid caller state. */
        return GUARDIAN_SECURITY_ERROR_NULL_ARGUMENT;
    }

    /* Require installed PSK and nonce source. */
    if (security->configured == 0U)
    {
        /* Fail closed before authentication. */
        return GUARDIAN_SECURITY_ERROR_UNAUTHORIZED;
    }

    /* Require the exact fixed AUTH_BEGIN request size. */
    if (request_length !=
        GUARDIAN_SECURITY_AUTH_BEGIN_REQUEST_SIZE)
    {
        /* Count malformed authentication traffic. */
        guardian_security_increment_u32(
            &security->auth_failures);

        /* Reject malformed authentication payload. */
        return GUARDIAN_SECURITY_ERROR_INVALID_PAYLOAD;
    }

    /* Require sufficient response capacity. */
    if (response_capacity <
        GUARDIAN_SECURITY_AUTH_BEGIN_RESPONSE_SIZE)
    {
        /* Report bounded output failure. */
        return GUARDIAN_SECURITY_ERROR_OUTPUT_TOO_SMALL;
    }

    /* Require schema revision one. */
    if (request_payload[0] !=
        GUARDIAN_SECURITY_SCHEMA_VERSION)
    {
        /* Count unsupported authentication semantics. */
        guardian_security_increment_u32(
            &security->auth_failures);

        /* Reject unsupported schema. */
        return GUARDIAN_SECURITY_ERROR_INVALID_PAYLOAD;
    }

    /* Decode the requested authorization role. */
    guardian_security_role_t requested_role =
        (guardian_security_role_t)request_payload[1];

    /* Require a published role within this key's authorization ceiling. */
    if ((requested_role <
         GUARDIAN_SECURITY_ROLE_OBSERVER) ||
        (requested_role >
         security->config.max_role))
    {
        /* Count authorization-policy rejection. */
        guardian_security_increment_u32(
            &security->unauthorized_rejections);

        /* Reject role escalation before challenge creation. */
        return GUARDIAN_SECURITY_ERROR_UNAUTHORIZED;
    }

    /* Expire any stale pending challenge. */
    guardian_security_expire_pending(
        security,
        now_seconds);

    /* Request one unpredictable session identifier plus device nonce. */
    if (security->config.random(
            security->config.random_context,
            random_bytes,
            sizeof(random_bytes)) == 0)
    {
        /* Erase failed random output. */
        guardian_crypto_zero(
            random_bytes,
            sizeof(random_bytes));

        /* Refuse to construct predictable authentication challenges. */
        return GUARDIAN_SECURITY_ERROR_RANDOM;
    }

    /* Replace only pending challenge state, leaving the current active session intact. */
    guardian_security_clear_pending(
        security);

    /* Mark the new challenge pending. */
    security->pending.valid = 1U;

    /* Bind the requested authorization role. */
    security->pending.role =
        requested_role;

    /* Decode the random session identifier from the first four bytes. */
    security->pending.session_id =
        guardian_security_read_u32_be(
            random_bytes);

    /* Reserve zero as the no-session identifier. */
    if (security->pending.session_id == 0U)
    {
        /* Normalize the negligible zero case to one. */
        security->pending.session_id = 1U;
    }

    /* Copy the host challenge nonce. */
    (void)memcpy(
        security->pending.client_nonce,
        &request_payload[2],
        GUARDIAN_SECURITY_NONCE_SIZE);

    /* Copy the fresh device nonce. */
    (void)memcpy(
        security->pending.device_nonce,
        &random_bytes[4],
        GUARDIAN_SECURITY_NONCE_SIZE);

    /* Store pending challenge creation time. */
    security->pending.started_seconds =
        now_seconds;

    /* Calculate the device-authentication proof. */
    if (guardian_security_handshake_mac(
            security,
            guardian_security_server_label,
            sizeof(guardian_security_server_label) - 1U,
            requested_role,
            security->pending.session_id,
            security->pending.client_nonce,
            security->pending.device_nonce,
            server_proof) == 0)
    {
        /* Erase the unusable pending challenge. */
        guardian_security_clear_pending(
            security);

        /* Erase temporary secrets. */
        guardian_crypto_zero(
            random_bytes,
            sizeof(random_bytes));

        /* Report internal bounded transcript failure. */
        return GUARDIAN_SECURITY_ERROR_INVALID_PAYLOAD;
    }

    /* Publish response schema revision. */
    response_payload[0] =
        GUARDIAN_SECURITY_SCHEMA_VERSION;

    /* Publish the granted role. */
    response_payload[1] =
        (uint8_t)requested_role;

    /* Publish the random session identifier. */
    guardian_security_write_u32_be(
        &response_payload[2],
        security->pending.session_id);

    /* Publish the device nonce. */
    (void)memcpy(
        &response_payload[6],
        security->pending.device_nonce,
        GUARDIAN_SECURITY_NONCE_SIZE);

    /* Publish the first 128 bits of the server proof. */
    (void)memcpy(
        &response_payload[22],
        server_proof,
        GUARDIAN_SECURITY_TAG_SIZE);

    /* Publish the exact fixed response length. */
    *response_length =
        GUARDIAN_SECURITY_AUTH_BEGIN_RESPONSE_SIZE;

    /* Erase temporary random bytes. */
    guardian_crypto_zero(
        random_bytes,
        sizeof(random_bytes));

    /* Erase the full server proof after truncation. */
    guardian_crypto_zero(
        server_proof,
        sizeof(server_proof));

    /* Report successful challenge creation. */
    return GUARDIAN_SECURITY_OK;
}

/* Process one AUTH_FINISH payload and activate a verified session. */
guardian_security_result_t guardian_security_auth_finish(
    guardian_security_t *security,
    const uint8_t *request_payload,
    uint16_t request_length,
    uint32_t now_seconds,
    uint8_t *response_payload,
    uint16_t response_capacity,
    uint16_t *response_length)
{
    /* Store the expected full client proof. */
    uint8_t expected_proof[GUARDIAN_SHA256_SIZE] = {0};

    /* Store the derived full session key. */
    uint8_t session_key[GUARDIAN_SHA256_SIZE] = {0};

    /* Reject missing required storage. */
    if ((security == NULL) ||
        (request_payload == NULL) ||
        (response_payload == NULL) ||
        (response_length == NULL))
    {
        /* Report invalid caller state. */
        return GUARDIAN_SECURITY_ERROR_NULL_ARGUMENT;
    }

    /* Require installed provisioning. */
    if (security->configured == 0U)
    {
        /* Fail closed before authentication. */
        return GUARDIAN_SECURITY_ERROR_UNAUTHORIZED;
    }

    /* Expire stale pending challenges before validation. */
    guardian_security_expire_pending(
        security,
        now_seconds);

    /* Require the exact fixed AUTH_FINISH request size. */
    if (request_length !=
        GUARDIAN_SECURITY_AUTH_FINISH_REQUEST_SIZE)
    {
        /* Count malformed authentication traffic. */
        guardian_security_increment_u32(
            &security->auth_failures);

        /* Reject malformed authentication payload. */
        return GUARDIAN_SECURITY_ERROR_INVALID_PAYLOAD;
    }

    /* Require sufficient response capacity. */
    if (response_capacity <
        GUARDIAN_SECURITY_AUTH_FINISH_RESPONSE_SIZE)
    {
        /* Report bounded output failure. */
        return GUARDIAN_SECURITY_ERROR_OUTPUT_TOO_SMALL;
    }

    /* Require one currently pending challenge. */
    if (security->pending.valid == 0U)
    {
        /* Count invalid authentication sequence. */
        guardian_security_increment_u32(
            &security->auth_failures);

        /* Reject finish without matching begin. */
        return GUARDIAN_SECURITY_ERROR_UNAUTHORIZED;
    }

    /* Require schema revision one. */
    if (request_payload[0] !=
        GUARDIAN_SECURITY_SCHEMA_VERSION)
    {
        /* Count unsupported authentication semantics. */
        guardian_security_increment_u32(
            &security->auth_failures);

        /* Reject unsupported schema. */
        return GUARDIAN_SECURITY_ERROR_INVALID_PAYLOAD;
    }

    /* Decode and validate role binding. */
    guardian_security_role_t role =
        (guardian_security_role_t)request_payload[1];

    /* Decode and validate session identifier binding. */
    uint32_t session_id =
        guardian_security_read_u32_be(
            &request_payload[2]);

    /* Require exact pending transcript identity. */
    if ((role !=
         security->pending.role) ||
        (session_id !=
         security->pending.session_id))
    {
        /* Count transcript mismatch. */
        guardian_security_increment_u32(
            &security->auth_failures);

        /* Erase the failed pending challenge. */
        guardian_security_clear_pending(
            security);

        /* Reject transcript substitution. */
        return GUARDIAN_SECURITY_ERROR_UNAUTHORIZED;
    }

    /* Calculate the expected client-authentication proof. */
    if (guardian_security_handshake_mac(
            security,
            guardian_security_client_label,
            sizeof(guardian_security_client_label) - 1U,
            role,
            session_id,
            security->pending.client_nonce,
            security->pending.device_nonce,
            expected_proof) == 0)
    {
        /* Erase the pending challenge. */
        guardian_security_clear_pending(
            security);

        /* Report internal transcript failure. */
        return GUARDIAN_SECURITY_ERROR_INVALID_PAYLOAD;
    }

    /* Compare the transmitted truncated proof in constant time. */
    if (guardian_crypto_constant_time_equal(
            expected_proof,
            &request_payload[6],
            GUARDIAN_SECURITY_TAG_SIZE) == 0)
    {
        /* Count invalid client proof. */
        guardian_security_increment_u32(
            &security->auth_failures);

        /* Erase the failed pending challenge. */
        guardian_security_clear_pending(
            security);

        /* Erase expected proof. */
        guardian_crypto_zero(
            expected_proof,
            sizeof(expected_proof));

        /* Reject authentication. */
        return GUARDIAN_SECURITY_ERROR_UNAUTHORIZED;
    }

    /* Derive one unique per-session HMAC key from both nonces and session identity. */
    if (guardian_security_handshake_mac(
            security,
            guardian_security_session_label,
            sizeof(guardian_security_session_label) - 1U,
            role,
            session_id,
            security->pending.client_nonce,
            security->pending.device_nonce,
            session_key) == 0)
    {
        /* Erase the pending challenge. */
        guardian_security_clear_pending(
            security);

        /* Erase proof material. */
        guardian_crypto_zero(
            expected_proof,
            sizeof(expected_proof));

        /* Report internal transcript failure. */
        return GUARDIAN_SECURITY_ERROR_INVALID_PAYLOAD;
    }

    /* Replace any previous active session only after client proof succeeded. */
    guardian_security_clear_session(
        security);

    /* Mark the authenticated session active. */
    security->session.valid = 1U;

    /* Publish authenticated authorization role. */
    security->session.role =
        role;

    /* Publish random session identifier. */
    security->session.session_id =
        session_id;

    /* Copy the derived full 256-bit session key. */
    (void)memcpy(
        security->session.session_key,
        session_key,
        GUARDIAN_SECURITY_SESSION_KEY_SIZE);

    /* Require the first authenticated request counter to be exactly one. */
    security->session.next_counter = 1ULL;

    /* Start inactivity tracking at authentication completion. */
    security->session.last_activity_seconds =
        now_seconds;

    /* Count successful authentication. */
    guardian_security_increment_u32(
        &security->auth_successes);

    /* Publish schema revision one. */
    response_payload[0] =
        GUARDIAN_SECURITY_SCHEMA_VERSION;

    /* Publish authenticated role. */
    response_payload[1] =
        (uint8_t)role;

    /* Publish active session identifier. */
    guardian_security_write_u32_be(
        &response_payload[2],
        session_id);

    /* Publish configured inactivity timeout. */
    guardian_security_write_u16_be(
        &response_payload[6],
        security->config.session_timeout_seconds);

    /* Publish exact fixed response length. */
    *response_length =
        GUARDIAN_SECURITY_AUTH_FINISH_RESPONSE_SIZE;

    /* Erase completed pending challenge nonces. */
    guardian_security_clear_pending(
        security);

    /* Erase temporary expected proof. */
    guardian_crypto_zero(
        expected_proof,
        sizeof(expected_proof));

    /* Erase temporary session-key copy. */
    guardian_crypto_zero(
        session_key,
        sizeof(session_key));

    /* Report successful authenticated-session establishment. */
    return GUARDIAN_SECURITY_OK;
}

/* Decode, authenticate and anti-replay-check one SECURE_COMMAND request. */
guardian_security_result_t guardian_security_unwrap_request(
    guardian_security_t *security,
    const guardian_frame_t *outer_request,
    uint32_t now_seconds,
    guardian_frame_t *inner_request,
    uint64_t *counter)
{
    /* Store the expected full HMAC request tag. */
    uint8_t expected_tag[GUARDIAN_SHA256_SIZE] = {0};

    /* Reject missing required storage. */
    if ((security == NULL) ||
        (outer_request == NULL) ||
        (inner_request == NULL) ||
        (counter == NULL))
    {
        /* Report invalid caller state. */
        return GUARDIAN_SECURITY_ERROR_NULL_ARGUMENT;
    }

    /* Expire inactive sessions before accepting authenticated traffic. */
    guardian_security_expire_session(
        security,
        now_seconds);

    /* Require an active authenticated session. */
    if (security->session.valid == 0U)
    {
        /* Count unauthorized secure traffic. */
        guardian_security_increment_u32(
            &security->unauthorized_rejections);

        /* Reject traffic without an active session. */
        return GUARDIAN_SECURITY_ERROR_UNAUTHORIZED;
    }

    /* Require at least the fixed secure request overhead. */
    if (outer_request->payload_length <
        GUARDIAN_SECURITY_REQUEST_OVERHEAD)
    {
        /* Count malformed authentication traffic. */
        guardian_security_increment_u32(
            &security->auth_failures);

        /* Reject truncated secure envelopes. */
        return GUARDIAN_SECURITY_ERROR_INVALID_PAYLOAD;
    }

    /* Require schema revision one. */
    if (outer_request->payload[0] !=
        GUARDIAN_SECURITY_SCHEMA_VERSION)
    {
        /* Count unsupported secure semantics. */
        guardian_security_increment_u32(
            &security->auth_failures);

        /* Reject unsupported schema. */
        return GUARDIAN_SECURITY_ERROR_INVALID_PAYLOAD;
    }

    /* Decode the bound session identifier. */
    uint32_t session_id =
        guardian_security_read_u32_be(
            &outer_request->payload[1]);

    /* Require exact active session identity. */
    if (session_id !=
        security->session.session_id)
    {
        /* Count unauthorized session substitution. */
        guardian_security_increment_u32(
            &security->unauthorized_rejections);

        /* Reject wrong-session traffic. */
        return GUARDIAN_SECURITY_ERROR_UNAUTHORIZED;
    }

    /* Decode strict monotonic request counter. */
    uint64_t request_counter =
        guardian_security_read_u64_be(
            &outer_request->payload[5]);

    /* Reject both duplicates and skipped counters. */
    if (request_counter !=
        security->session.next_counter)
    {
        /* Count strict anti-replay rejection. */
        guardian_security_increment_u32(
            &security->replay_rejections);

        /* Reject replay or out-of-order traffic. */
        return GUARDIAN_SECURITY_ERROR_REPLAY;
    }

    /* Decode inner privileged command. */
    uint8_t inner_command =
        outer_request->payload[13];

    /* Decode exact inner payload length. */
    uint16_t inner_length =
        guardian_security_read_u16_be(
            &outer_request->payload[14]);

    /* Enforce the request envelope payload bound. */
    if (inner_length >
        GUARDIAN_SECURITY_MAX_REQUEST_INNER_PAYLOAD)
    {
        /* Count malformed authenticated traffic. */
        guardian_security_increment_u32(
            &security->auth_failures);

        /* Reject impossible inner length. */
        return GUARDIAN_SECURITY_ERROR_INVALID_PAYLOAD;
    }

    /* Calculate the exact expected envelope length. */
    uint16_t expected_length =
        (uint16_t)(
            GUARDIAN_SECURITY_REQUEST_OVERHEAD +
            inner_length);

    /* Require exact framing without trailing authenticated ambiguity. */
    if (outer_request->payload_length !=
        expected_length)
    {
        /* Count malformed authenticated traffic. */
        guardian_security_increment_u32(
            &security->auth_failures);

        /* Reject length mismatch. */
        return GUARDIAN_SECURITY_ERROR_INVALID_PAYLOAD;
    }

    /* Calculate the expected request HMAC. */
    if (guardian_security_request_tag(
            security,
            outer_request,
            session_id,
            request_counter,
            inner_command,
            &outer_request->payload[16],
            inner_length,
            expected_tag) == 0)
    {
        /* Report internal transcript failure. */
        return GUARDIAN_SECURITY_ERROR_INVALID_PAYLOAD;
    }

    /* Locate the transmitted tag after the variable inner payload. */
    const uint8_t *received_tag =
        &outer_request->payload[
            16U +
            inner_length];

    /* Compare the transmitted 128-bit tag in constant time. */
    if (guardian_crypto_constant_time_equal(
            expected_tag,
            received_tag,
            GUARDIAN_SECURITY_TAG_SIZE) == 0)
    {
        /* Count invalid authenticated-message tag. */
        guardian_security_increment_u32(
            &security->auth_failures);

        /* Erase expected tag. */
        guardian_crypto_zero(
            expected_tag,
            sizeof(expected_tag));

        /* Reject message authenticity failure. */
        return GUARDIAN_SECURITY_ERROR_UNAUTHORIZED;
    }

    /* Erase expected full HMAC after verification. */
    guardian_crypto_zero(
        expected_tag,
        sizeof(expected_tag));

    /* Build one ordinary inner Guardian request for existing command handlers. */
    (void)memset(
        inner_request,
        0,
        sizeof(*inner_request));

    /* Publish inner request message class. */
    inner_request->message_type =
        GUARDIAN_MESSAGE_REQUEST;

    /* Publish authenticated inner command. */
    inner_request->command =
        inner_command;

    /* Use protocol v0.1 flags for inner dispatch. */
    inner_request->flags =
        GUARDIAN_SUPPORTED_FLAGS;

    /* Reuse the outer request sequence for diagnostic correlation. */
    inner_request->sequence =
        outer_request->sequence;

    /* Publish authenticated inner payload length. */
    inner_request->payload_length =
        inner_length;

    /* Copy authenticated inner payload bytes. */
    if (inner_length != 0U)
    {
        /* Copy only the authenticated payload bytes. */
        (void)memcpy(
            inner_request->payload,
            &outer_request->payload[16],
            inner_length);
    }

    /* Return the verified counter to the response wrapper. */
    *counter =
        request_counter;

    /* Consume this counter exactly once before application authorization/dispatch. */
    security->session.next_counter +=
        1ULL;

    /* Avoid wrapping strict anti-replay counter through zero. */
    if (security->session.next_counter == 0ULL)
    {
        /* Expire the exhausted session rather than accepting counter wrap. */
        guardian_security_clear_session(
            security);

        /* Report replay/security exhaustion. */
        return GUARDIAN_SECURITY_ERROR_REPLAY;
    }

    /* Refresh authenticated activity time only after successful MAC verification. */
    security->session.last_activity_seconds =
        now_seconds;

    /* Report successfully authenticated request. */
    return GUARDIAN_SECURITY_OK;
}

/* Check active role against the minimum role for one privileged command. */
int guardian_security_authorize(
    guardian_security_t *security,
    uint8_t command)
{
    /* Reject missing security state or absent session. */
    if ((security == NULL) ||
        (security->session.valid == 0U))
    {
        /* Report authorization failure. */
        return 0;
    }

    /* Default secure inner operations to ADMIN-only until explicitly classified. */
    guardian_security_role_t required_role =
        GUARDIAN_SECURITY_ROLE_ADMIN;

    /* Permit M8 baseline mutation to OPERATOR and ADMIN roles. */
    if (command ==
        (uint8_t)GUARDIAN_COMMAND_BASELINE_CONTROL)
    {
        /* Require OPERATOR. */
        required_role =
            GUARDIAN_SECURITY_ROLE_OPERATOR;
    }
    else if (command ==
             (uint8_t)GUARDIAN_COMMAND_CONTROL_COMMAND)
    {
        /* Permit M9 supervisory actions to OPERATOR and ADMIN. */
        required_role =
            GUARDIAN_SECURITY_ROLE_OPERATOR;
    }

    /* Compare authenticated role with the command's minimum role. */
    if (security->session.role <
        required_role)
    {
        /* Count authenticated authorization denial. */
        guardian_security_increment_u32(
            &security->unauthorized_rejections);

        /* Reject insufficient role. */
        return 0;
    }

    /* Report sufficient authorization. */
    return 1;
}

/* Wrap one privileged inner response with a session-authenticated response tag. */
guardian_security_result_t guardian_security_wrap_response(
    const guardian_security_t *security,
    const guardian_frame_t *outer_request,
    uint64_t counter,
    const guardian_frame_t *inner_response,
    guardian_frame_t *outer_response)
{
    /* Store the full response HMAC before truncation. */
    uint8_t tag[GUARDIAN_SHA256_SIZE] = {0};

    /* Reject missing required storage. */
    if ((security == NULL) ||
        (outer_request == NULL) ||
        (inner_response == NULL) ||
        (outer_response == NULL))
    {
        /* Report invalid caller state. */
        return GUARDIAN_SECURITY_ERROR_NULL_ARGUMENT;
    }

    /* Require the same authenticated session to remain active during response construction. */
    if (security->session.valid == 0U)
    {
        /* Reject response without session-key material. */
        return GUARDIAN_SECURITY_ERROR_UNAUTHORIZED;
    }

    /* Require bounded inner response payload. */
    if (inner_response->payload_length >
        GUARDIAN_SECURITY_MAX_RESPONSE_INNER_PAYLOAD)
    {
        /* Reject response that cannot fit in one Guardian frame. */
        return GUARDIAN_SECURITY_ERROR_OUTPUT_TOO_SMALL;
    }

    /* Clear complete outer response storage. */
    (void)memset(
        outer_response,
        0,
        sizeof(*outer_response));

    /* Publish successful outer secure-envelope response class. */
    outer_response->message_type =
        GUARDIAN_MESSAGE_RESPONSE;

    /* Preserve the outer SECURE_COMMAND identifier. */
    outer_response->command =
        outer_request->command;

    /* Use protocol v0.1 flags. */
    outer_response->flags =
        GUARDIAN_SUPPORTED_FLAGS;

    /* Preserve outer request correlation. */
    outer_response->sequence =
        outer_request->sequence;

    /* Publish schema revision one. */
    outer_response->payload[0] =
        GUARDIAN_SECURITY_SCHEMA_VERSION;

    /* Publish active session identifier. */
    guardian_security_write_u32_be(
        &outer_response->payload[1],
        security->session.session_id);

    /* Echo the authenticated request counter. */
    guardian_security_write_u64_be(
        &outer_response->payload[5],
        counter);

    /* Publish authenticated inner response message class. */
    outer_response->payload[13] =
        (uint8_t)inner_response->message_type;

    /* Publish authenticated inner command. */
    outer_response->payload[14] =
        inner_response->command;

    /* Publish exact authenticated inner payload length. */
    guardian_security_write_u16_be(
        &outer_response->payload[15],
        inner_response->payload_length);

    /* Copy the inner response payload. */
    if (inner_response->payload_length != 0U)
    {
        /* Copy only the bounded inner payload. */
        (void)memcpy(
            &outer_response->payload[17],
            inner_response->payload,
            inner_response->payload_length);
    }

    /* Calculate the authenticated response tag. */
    if (guardian_security_response_tag(
            security,
            outer_request,
            security->session.session_id,
            counter,
            inner_response,
            tag) == 0)
    {
        /* Report internal transcript failure. */
        return GUARDIAN_SECURITY_ERROR_INVALID_PAYLOAD;
    }

    /* Copy the truncated 128-bit tag after the inner payload. */
    (void)memcpy(
        &outer_response->payload[
            17U +
            inner_response->payload_length],
        tag,
        GUARDIAN_SECURITY_TAG_SIZE);

    /* Publish exact secure response envelope length. */
    outer_response->payload_length =
        (uint16_t)(
            GUARDIAN_SECURITY_RESPONSE_OVERHEAD +
            inner_response->payload_length);

    /* Erase the full response HMAC. */
    guardian_crypto_zero(
        tag,
        sizeof(tag));

    /* Report successful response wrapping. */
    return GUARDIAN_SECURITY_OK;
}

/* Return a public diagnostics snapshot with session expiry applied. */
guardian_security_status_t guardian_security_status(
    guardian_security_t *security,
    uint32_t now_seconds)
{
    /* Create deterministic zero-initialized public status. */
    guardian_security_status_t status = {0};

    /* Return empty unconfigured status for a missing pointer. */
    if (security == NULL)
    {
        /* Return deterministic zero status. */
        return status;
    }

    /* Expire stale pending state. */
    guardian_security_expire_pending(
        security,
        now_seconds);

    /* Expire stale authenticated state. */
    guardian_security_expire_session(
        security,
        now_seconds);

    /* Publish whether provisioning exists. */
    status.configured =
        security->configured;

    /* Publish configured timeout when provisioned. */
    status.timeout_seconds =
        security->configured
        ? security->config.session_timeout_seconds
        : 0U;

    /* Publish cumulative authentication diagnostics. */
    status.auth_successes =
        security->auth_successes;

    /* Publish cumulative authentication failures. */
    status.auth_failures =
        security->auth_failures;

    /* Publish strict anti-replay diagnostics. */
    status.replay_rejections =
        security->replay_rejections;

    /* Publish authorization diagnostics. */
    status.unauthorized_rejections =
        security->unauthorized_rejections;

    /* Publish active session fields only when currently authenticated. */
    if (security->session.valid != 0U)
    {
        /* Mark authenticated session active. */
        status.active = 1U;

        /* Publish active role. */
        status.active_role =
            security->session.role;

        /* Publish active random session identifier. */
        status.session_id =
            security->session.session_id;

        /* Publish exact next counter. */
        status.next_counter =
            security->session.next_counter;

        /* Calculate wrap-safe inactivity age. */
        uint32_t elapsed =
            now_seconds -
            security->session.last_activity_seconds;

        /* Calculate bounded remaining lifetime. */
        if (elapsed <
            (uint32_t)security->config.session_timeout_seconds)
        {
            /* Publish remaining inactivity seconds. */
            status.remaining_seconds =
                (uint16_t)(
                    (uint32_t)security->config.session_timeout_seconds -
                    elapsed);
        }
    }

    /* Return the public diagnostics snapshot by value. */
    return status;
}

/* Encode one fixed GET_SECURITY_STATUS payload. */
guardian_security_result_t guardian_security_encode_status_payload(
    const guardian_security_status_t *status,
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
        return GUARDIAN_SECURITY_ERROR_NULL_ARGUMENT;
    }

    /* Require the complete fixed output capacity. */
    if (payload_capacity <
        GUARDIAN_SECURITY_STATUS_PAYLOAD_SIZE)
    {
        /* Report bounded output failure. */
        return GUARDIAN_SECURITY_ERROR_OUTPUT_TOO_SMALL;
    }

    /* Publish schema revision one. */
    payload[0] =
        GUARDIAN_SECURITY_SCHEMA_VERSION;

    /* Publish provisioning state. */
    payload[1] =
        status->configured;

    /* Publish authenticated-session state. */
    payload[2] =
        status->active;

    /* Publish active role or NONE. */
    payload[3] =
        (uint8_t)status->active_role;

    /* Publish active session identifier. */
    guardian_security_write_u32_be(
        &payload[4],
        status->session_id);

    /* Publish exact next request counter. */
    guardian_security_write_u64_be(
        &payload[8],
        status->next_counter);

    /* Publish configured inactivity timeout. */
    guardian_security_write_u16_be(
        &payload[16],
        status->timeout_seconds);

    /* Publish current remaining session lifetime. */
    guardian_security_write_u16_be(
        &payload[18],
        status->remaining_seconds);

    /* Publish successful authentication count. */
    guardian_security_write_u32_be(
        &payload[20],
        status->auth_successes);

    /* Publish failed authentication/tag count. */
    guardian_security_write_u32_be(
        &payload[24],
        status->auth_failures);

    /* Publish strict anti-replay rejection count. */
    guardian_security_write_u32_be(
        &payload[28],
        status->replay_rejections);

    /* Publish authorization rejection count. */
    guardian_security_write_u32_be(
        &payload[32],
        status->unauthorized_rejections);

    /* Publish exact fixed payload size. */
    *payload_length =
        GUARDIAN_SECURITY_STATUS_PAYLOAD_SIZE;

    /* Report successful serialization. */
    return GUARDIAN_SECURITY_OK;
}
