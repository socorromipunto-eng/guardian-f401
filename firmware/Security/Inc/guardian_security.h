#ifndef GUARDIAN_SECURITY_H
#define GUARDIAN_SECURITY_H

/* Include portable HMAC and constant-time primitives. */
#include "guardian_crypto.h"

/* Include Guardian frame and protocol result types. */
#include "guardian_protocol.h"

/* Include size_t for bounded random and payload callbacks. */
#include <stddef.h>

/* Include fixed-width integer types for session identifiers and counters. */
#include <stdint.h>

/* Define the first authenticated-session schema revision. */
#define GUARDIAN_SECURITY_SCHEMA_VERSION ((uint8_t)0x01U)

/* Define the required provisioned pre-shared-key width. */
#define GUARDIAN_SECURITY_PSK_SIZE ((size_t)32U)

/* Define client and device nonce width. */
#define GUARDIAN_SECURITY_NONCE_SIZE ((size_t)16U)

/* Define the full internal session-key width. */
#define GUARDIAN_SECURITY_SESSION_KEY_SIZE ((size_t)32U)

/* Define the transmitted truncated HMAC tag width. */
#define GUARDIAN_SECURITY_TAG_SIZE ((size_t)16U)

/* Define the fixed AUTH_BEGIN request size. */
#define GUARDIAN_SECURITY_AUTH_BEGIN_REQUEST_SIZE ((uint16_t)18U)

/* Define the fixed AUTH_BEGIN response size. */
#define GUARDIAN_SECURITY_AUTH_BEGIN_RESPONSE_SIZE ((uint16_t)38U)

/* Define the fixed AUTH_FINISH request size. */
#define GUARDIAN_SECURITY_AUTH_FINISH_REQUEST_SIZE ((uint16_t)22U)

/* Define the fixed AUTH_FINISH response size. */
#define GUARDIAN_SECURITY_AUTH_FINISH_RESPONSE_SIZE ((uint16_t)8U)

/* Define the fixed GET_SECURITY_STATUS response size. */
#define GUARDIAN_SECURITY_STATUS_PAYLOAD_SIZE ((uint16_t)36U)

/* Define secure request envelope overhead excluding inner payload. */
#define GUARDIAN_SECURITY_REQUEST_OVERHEAD ((uint16_t)32U)

/* Define secure response envelope overhead excluding inner payload. */
#define GUARDIAN_SECURITY_RESPONSE_OVERHEAD ((uint16_t)33U)

/* Define the largest privileged request payload that fits in one Guardian frame. */
#define GUARDIAN_SECURITY_MAX_REQUEST_INNER_PAYLOAD \
    ((uint16_t)(GUARDIAN_MAX_PAYLOAD_SIZE - GUARDIAN_SECURITY_REQUEST_OVERHEAD))

/* Define the largest privileged response payload that fits in one Guardian frame. */
#define GUARDIAN_SECURITY_MAX_RESPONSE_INNER_PAYLOAD \
    ((uint16_t)(GUARDIAN_MAX_PAYLOAD_SIZE - GUARDIAN_SECURITY_RESPONSE_OVERHEAD))

/* Define the default authenticated-session inactivity timeout. */
#define GUARDIAN_SECURITY_DEFAULT_SESSION_TIMEOUT_SECONDS ((uint16_t)300U)

/* Define a short pending-handshake lifetime. */
#define GUARDIAN_SECURITY_PENDING_TIMEOUT_SECONDS ((uint16_t)30U)

/* Define authenticated authorization roles. */
typedef enum
{
    /* Represent no authenticated authorization. */
    GUARDIAN_SECURITY_ROLE_NONE = 0,

    /* Permit authenticated monitoring-only operations. */
    GUARDIAN_SECURITY_ROLE_OBSERVER = 1,

    /* Permit baseline and supervisory-control operations. */
    GUARDIAN_SECURITY_ROLE_OPERATOR = 2,

    /* Reserve the highest role for future firmware-lifecycle operations. */
    GUARDIAN_SECURITY_ROLE_ADMIN = 3
} guardian_security_role_t;

/* Define internal M10 security-processing outcomes. */
typedef enum
{
    /* Indicate successful security processing. */
    GUARDIAN_SECURITY_OK = 0,

    /* Indicate a missing required pointer. */
    GUARDIAN_SECURITY_ERROR_NULL_ARGUMENT,

    /* Indicate malformed or unsupported security payload semantics. */
    GUARDIAN_SECURITY_ERROR_INVALID_PAYLOAD,

    /* Indicate missing provisioning, invalid proof or invalid message tag. */
    GUARDIAN_SECURITY_ERROR_UNAUTHORIZED,

    /* Indicate a rejected secure-message counter. */
    GUARDIAN_SECURITY_ERROR_REPLAY,

    /* Indicate failure of the platform cryptographic nonce source. */
    GUARDIAN_SECURITY_ERROR_RANDOM,

    /* Indicate a bounded output buffer was too small. */
    GUARDIAN_SECURITY_ERROR_OUTPUT_TOO_SMALL
} guardian_security_result_t;

/* Fill caller storage with cryptographically strong unpredictable bytes. */
typedef int (*guardian_security_random_fn)(
    void *context,
    uint8_t *output,
    size_t length);

/* Store immutable M10 provisioning configuration. */
typedef struct
{
    /* Store one 256-bit pre-shared key. */
    uint8_t psk[GUARDIAN_SECURITY_PSK_SIZE];

    /* Store the maximum role this key may authenticate. */
    guardian_security_role_t max_role;

    /* Store the required cryptographic nonce callback. */
    guardian_security_random_fn random;

    /* Store opaque platform state passed to the nonce callback. */
    void *random_context;

    /* Store the bounded inactivity timeout in seconds. */
    uint16_t session_timeout_seconds;
} guardian_security_config_t;

/* Store one pending challenge-response transcript. */
typedef struct
{
    /* Mark whether a pending challenge exists. */
    uint8_t valid;

    /* Store the requested role bound into proofs. */
    guardian_security_role_t role;

    /* Store the pending random session identifier. */
    uint32_t session_id;

    /* Store the host-provided challenge nonce. */
    uint8_t client_nonce[GUARDIAN_SECURITY_NONCE_SIZE];

    /* Store the device-provided challenge nonce. */
    uint8_t device_nonce[GUARDIAN_SECURITY_NONCE_SIZE];

    /* Store the monotonic creation time for pending-handshake expiry. */
    uint32_t started_seconds;
} guardian_security_pending_t;

/* Store one active authenticated session. */
typedef struct
{
    /* Mark whether an authenticated session exists. */
    uint8_t valid;

    /* Store the authenticated authorization role. */
    guardian_security_role_t role;

    /* Store the active random session identifier. */
    uint32_t session_id;

    /* Store the derived per-session HMAC key. */
    uint8_t session_key[GUARDIAN_SECURITY_SESSION_KEY_SIZE];

    /* Store the exact next request counter accepted by strict anti-replay policy. */
    uint64_t next_counter;

    /* Store monotonic last authenticated activity time. */
    uint32_t last_activity_seconds;
} guardian_security_session_t;

/* Store host-visible security diagnostics without secret material. */
typedef struct
{
    /* Store whether a PSK and nonce callback are provisioned. */
    uint8_t configured;

    /* Store whether an authenticated session is currently active. */
    uint8_t active;

    /* Store the active authorization role or NONE. */
    guardian_security_role_t active_role;

    /* Store the active session identifier or zero. */
    uint32_t session_id;

    /* Store the exact next accepted secure request counter. */
    uint64_t next_counter;

    /* Store configured inactivity timeout. */
    uint16_t timeout_seconds;

    /* Store remaining inactivity lifetime when active. */
    uint16_t remaining_seconds;

    /* Count successful authenticated session establishments. */
    uint32_t auth_successes;

    /* Count invalid proofs, tags and malformed authentication attempts. */
    uint32_t auth_failures;

    /* Count rejected secure counters. */
    uint32_t replay_rejections;

    /* Count authenticated operations denied by authorization policy. */
    uint32_t unauthorized_rejections;
} guardian_security_status_t;

/* Store the complete runtime M10 security state. */
typedef struct
{
    /* Store copied provisioning configuration including the PSK. */
    guardian_security_config_t config;

    /* Mark whether valid provisioning has been installed. */
    uint8_t configured;

    /* Store one pending challenge without disturbing the current active session. */
    guardian_security_pending_t pending;

    /* Store one active authenticated session. */
    guardian_security_session_t session;

    /* Count successful session establishments. */
    uint32_t auth_successes;

    /* Count invalid proofs, tags and malformed authentication attempts. */
    uint32_t auth_failures;

    /* Count strict anti-replay rejections. */
    uint32_t replay_rejections;

    /* Count authorization-policy rejections. */
    uint32_t unauthorized_rejections;
} guardian_security_t;

/* Initialize one unprovisioned security context. */
void guardian_security_init(
    guardian_security_t *security);

/* Install a provisioned PSK, role ceiling and cryptographic nonce callback. */
guardian_security_result_t guardian_security_configure(
    guardian_security_t *security,
    const guardian_security_config_t *config);

/* Erase provisioning, pending transcript and active session key material. */
void guardian_security_clear(
    guardian_security_t *security);

/* Process one AUTH_BEGIN payload and build its fixed response payload. */
guardian_security_result_t guardian_security_auth_begin(
    guardian_security_t *security,
    const uint8_t *request_payload,
    uint16_t request_length,
    uint32_t now_seconds,
    uint8_t *response_payload,
    uint16_t response_capacity,
    uint16_t *response_length);

/* Process one AUTH_FINISH payload and activate a verified session. */
guardian_security_result_t guardian_security_auth_finish(
    guardian_security_t *security,
    const uint8_t *request_payload,
    uint16_t request_length,
    uint32_t now_seconds,
    uint8_t *response_payload,
    uint16_t response_capacity,
    uint16_t *response_length);

/* Decode, authenticate and anti-replay-check one SECURE_COMMAND request. */
guardian_security_result_t guardian_security_unwrap_request(
    guardian_security_t *security,
    const guardian_frame_t *outer_request,
    uint32_t now_seconds,
    guardian_frame_t *inner_request,
    uint64_t *counter);

/* Check the active role against the minimum role for one privileged command. */
int guardian_security_authorize(
    guardian_security_t *security,
    uint8_t command);

/* Wrap one privileged inner response with a session-authenticated response tag. */
guardian_security_result_t guardian_security_wrap_response(
    const guardian_security_t *security,
    const guardian_frame_t *outer_request,
    uint64_t counter,
    const guardian_frame_t *inner_response,
    guardian_frame_t *outer_response);

/* Return a public diagnostics snapshot with session expiry applied. */
guardian_security_status_t guardian_security_status(
    guardian_security_t *security,
    uint32_t now_seconds);

/* Encode one fixed GET_SECURITY_STATUS payload. */
guardian_security_result_t guardian_security_encode_status_payload(
    const guardian_security_status_t *status,
    uint8_t *payload,
    uint16_t payload_capacity,
    uint16_t *payload_length);

#endif
