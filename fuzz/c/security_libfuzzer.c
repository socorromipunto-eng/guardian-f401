/* Include the production M10 authenticated-session API. */
#include "guardian_security.h"

/* Include size_t for the libFuzzer entry point. */
#include <stddef.h>

/* Include fixed-width byte types for fuzzer input. */
#include <stdint.h>

/* Include memory helpers for bounded payload copies. */
#include <string.h>

/* Activate one deterministic in-memory session before each fuzz input. */
static void guardian_security_libfuzzer_activate(
    guardian_security_t *security)
{
    /* Clear complete security state. */
    guardian_security_init(
        security);

    /* Preserve a non-zero session timeout. */
    security->config.session_timeout_seconds =
        GUARDIAN_SECURITY_DEFAULT_SESSION_TIMEOUT_SECONDS;

    /* Mark one active session. */
    security->session.valid =
        1U;

    /* Grant OPERATOR role for any successful structured input. */
    security->session.role =
        GUARDIAN_SECURITY_ROLE_OPERATOR;

    /* Publish one fixed session identifier. */
    security->session.session_id =
        0xA1B2C3D4UL;

    /* Require request counter one. */
    security->session.next_counter =
        1ULL;

    /* Fill deterministic non-secret test session-key bytes. */
    size_t index = 0U;

    /* Publish every key byte. */
    for (index = 0U;
         index < GUARDIAN_SECURITY_SESSION_KEY_SIZE;
         ++index)
    {
        /* Store one deterministic test byte. */
        security->session.session_key[index] =
            (uint8_t)(
                index ^
                0x5AU);
    }
}

/* Feed arbitrary secure-envelope bytes into the production M10 decoder. */
int LLVMFuzzerTestOneInput(
    const uint8_t *data,
    size_t size)
{
    /* Create fresh M10 state for this input. */
    guardian_security_t security = {0};

    /* Activate deterministic session state. */
    guardian_security_libfuzzer_activate(
        &security);

    /* Create one outer SECURE_COMMAND frame. */
    guardian_frame_t outer = {0};

    /* Publish ordinary request semantics. */
    outer.message_type =
        GUARDIAN_MESSAGE_REQUEST;

    /* Select secure envelope command. */
    outer.command =
        (uint8_t)GUARDIAN_COMMAND_SECURE_COMMAND;

    /* Use a stable sequence number for raw-envelope fuzzing. */
    outer.sequence =
        0x01020304UL;

    /* Bound fuzzer bytes to Guardian payload capacity. */
    size_t copy_length =
        (size < GUARDIAN_MAX_PAYLOAD_SIZE)
        ? size
        : GUARDIAN_MAX_PAYLOAD_SIZE;

    /* Copy arbitrary fuzzer bytes into the secure payload. */
    if (copy_length != 0U)
    {
        /* Copy only bounded bytes. */
        (void)memcpy(
            outer.payload,
            data,
            copy_length);
    }

    /* Publish exact bounded input length. */
    outer.payload_length =
        (uint16_t)copy_length;

    /* Create authenticated inner output storage. */
    guardian_frame_t inner = {0};

    /* Store verified counter when one exists. */
    uint64_t counter = 0ULL;

    /* Exercise the complete production secure-request decoder. */
    (void)guardian_security_unwrap_request(
        &security,
        &outer,
        0U,
        &inner,
        &counter);

    /* Exercise public diagnostics after arbitrary security input. */
    (void)guardian_security_status(
        &security,
        0U);

    /* Exercise one fixed AUTH_BEGIN decoder path with arbitrary bytes as well. */
    uint8_t response[
        GUARDIAN_MAX_PAYLOAD_SIZE] = {0};

    /* Store any bounded response length. */
    uint16_t response_length = 0U;

    /* Deliberately leave the security context unprovisioned for fail-closed AUTH_BEGIN coverage. */
    guardian_security_t unprovisioned = {0};

    /* Initialize explicit unprovisioned state. */
    guardian_security_init(
        &unprovisioned);

    /* Exercise authentication parsing without secrets or entropy callbacks. */
    (void)guardian_security_auth_begin(
        &unprovisioned,
        data,
        (uint16_t)(
            (size < 0xFFFFU)
            ? size
            : 0xFFFFU),
        0U,
        response,
        sizeof(response),
        &response_length);

    /* Report successful handling of this arbitrary input. */
    return 0;
}
