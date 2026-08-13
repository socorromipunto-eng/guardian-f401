/* Include the production M10 authenticated-session API. */
#include "guardian_security.h"

/* Include assertion support for fail-fast sanitizer campaigns. */
#include <assert.h>

/* Include standard output for one concise campaign summary. */
#include <stdio.h>

/* Include memory helpers for bounded transcript construction. */
#include <string.h>

/* Define the deterministic mutation count. */
#define GUARDIAN_SECURITY_FUZZ_DEFAULT_ITERATIONS ((uint32_t)10000U)

/* Define the maximum randomized inner payload used by this driver. */
#define GUARDIAN_SECURITY_FUZZ_INNER_MAX ((uint16_t)32U)

/* Return one deterministic xorshift32 pseudo-random word. */
static uint32_t guardian_security_fuzz_next(
    uint32_t *state)
{
    /* Require caller-owned PRNG state. */
    assert(state != NULL);

    /* Avoid the xorshift all-zero fixed point. */
    if (*state == 0U)
    {
        /* Replace zero with one fixed non-zero seed. */
        *state = 0x9E3779B9UL;
    }

    /* Apply the first xorshift step. */
    *state ^=
        *state << 13U;

    /* Apply the second xorshift step. */
    *state ^=
        *state >> 17U;

    /* Apply the third xorshift step. */
    *state ^=
        *state << 5U;

    /* Return the updated deterministic word. */
    return *state;
}

/* Write one big-endian unsigned 16-bit value. */
static void guardian_security_fuzz_write_u16_be(
    uint8_t *output,
    uint16_t value)
{
    /* Write the high byte first. */
    output[0] =
        (uint8_t)((value >> 8U) & 0xFFU);

    /* Write the low byte second. */
    output[1] =
        (uint8_t)(value & 0xFFU);
}

/* Write one big-endian unsigned 32-bit value. */
static void guardian_security_fuzz_write_u32_be(
    uint8_t *output,
    uint32_t value)
{
    /* Write all four bytes explicitly. */
    output[0] =
        (uint8_t)((value >> 24U) & 0xFFU);
    output[1] =
        (uint8_t)((value >> 16U) & 0xFFU);
    output[2] =
        (uint8_t)((value >> 8U) & 0xFFU);
    output[3] =
        (uint8_t)(value & 0xFFU);
}

/* Write one big-endian unsigned 64-bit value. */
static void guardian_security_fuzz_write_u64_be(
    uint8_t *output,
    uint64_t value)
{
    /* Write all eight bytes explicitly. */
    output[0] =
        (uint8_t)((value >> 56U) & 0xFFU);
    output[1] =
        (uint8_t)((value >> 48U) & 0xFFU);
    output[2] =
        (uint8_t)((value >> 40U) & 0xFFU);
    output[3] =
        (uint8_t)((value >> 32U) & 0xFFU);
    output[4] =
        (uint8_t)((value >> 24U) & 0xFFU);
    output[5] =
        (uint8_t)((value >> 16U) & 0xFFU);
    output[6] =
        (uint8_t)((value >> 8U) & 0xFFU);
    output[7] =
        (uint8_t)(value & 0xFFU);
}

/* Initialize one directly active deterministic session for envelope fuzzing. */
static void guardian_security_fuzz_activate(
    guardian_security_t *security)
{
    /* Require caller-owned security storage. */
    assert(security != NULL);

    /* Clear every runtime field first. */
    guardian_security_init(
        security);

    /* Mark the security context provisioned for diagnostic consistency. */
    security->configured =
        1U;

    /* Set a non-zero inactivity timeout so now=0 does not expire the session. */
    security->config.session_timeout_seconds =
        GUARDIAN_SECURITY_DEFAULT_SESSION_TIMEOUT_SECONDS;

    /* Mark one active session. */
    security->session.valid =
        1U;

    /* Grant OPERATOR authorization. */
    security->session.role =
        GUARDIAN_SECURITY_ROLE_OPERATOR;

    /* Publish one stable session identifier. */
    security->session.session_id =
        0xA1B2C3D4UL;

    /* Require counter one first. */
    security->session.next_counter =
        1ULL;

    /* Start activity time at zero for deterministic calls with now=0. */
    security->session.last_activity_seconds =
        0U;

    /* Fill the session key with deterministic non-secret test bytes. */
    size_t index = 0U;

    /* Publish all thirty-two key bytes. */
    for (index = 0U;
         index < GUARDIAN_SECURITY_SESSION_KEY_SIZE;
         ++index)
    {
        /* Use a deterministic byte pattern. */
        security->session.session_key[index] =
            (uint8_t)(
                index ^
                0xA5U);
    }
}

/* Build one fully valid authenticated request using the active test session key. */
static void guardian_security_fuzz_build_valid(
    const guardian_security_t *security,
    uint64_t counter,
    uint32_t sequence,
    uint8_t inner_command,
    const uint8_t *inner_payload,
    uint16_t inner_length,
    guardian_frame_t *outer)
{
    /* Define the exact M10 request domain label. */
    static const uint8_t label[] =
        "GF-M10-REQUEST";

    /* Store the complete bounded MAC transcript. */
    uint8_t transcript[288] = {0};

    /* Store the full HMAC before truncation. */
    uint8_t digest[GUARDIAN_SHA256_SIZE] = {0};

    /* Track transcript bytes. */
    size_t transcript_length = 0U;

    /* Require valid caller state. */
    assert(security != NULL);
    assert(outer != NULL);

    /* Require the randomized inner payload to fit the M10 request envelope. */
    assert(
        inner_length <=
        GUARDIAN_SECURITY_MAX_REQUEST_INNER_PAYLOAD);

    /* Require a payload pointer only when bytes are present. */
    assert(
        (inner_payload != NULL) ||
        (inner_length == 0U));

    /* Clear the complete outer frame. */
    (void)memset(
        outer,
        0,
        sizeof(*outer));

    /* Publish ordinary outer REQUEST semantics. */
    outer->message_type =
        GUARDIAN_MESSAGE_REQUEST;

    /* Select the authenticated command envelope. */
    outer->command =
        (uint8_t)GUARDIAN_COMMAND_SECURE_COMMAND;

    /* Use protocol v0.1 flags. */
    outer->flags =
        GUARDIAN_SUPPORTED_FLAGS;

    /* Publish the outer request sequence bound into the HMAC. */
    outer->sequence =
        sequence;

    /* Publish M10 schema revision one. */
    outer->payload[0] =
        GUARDIAN_SECURITY_SCHEMA_VERSION;

    /* Publish active session identifier. */
    guardian_security_fuzz_write_u32_be(
        &outer->payload[1],
        security->session.session_id);

    /* Publish caller-selected strict counter. */
    guardian_security_fuzz_write_u64_be(
        &outer->payload[5],
        counter);

    /* Publish inner command identifier. */
    outer->payload[13] =
        inner_command;

    /* Publish exact inner payload length. */
    guardian_security_fuzz_write_u16_be(
        &outer->payload[14],
        inner_length);

    /* Copy inner payload bytes when present. */
    if (inner_length != 0U)
    {
        /* Copy only bounded inner bytes. */
        (void)memcpy(
            &outer->payload[16],
            inner_payload,
            inner_length);
    }

    /* Append the domain-separation label to the MAC transcript. */
    (void)memcpy(
        &transcript[transcript_length],
        label,
        sizeof(label) - 1U);

    /* Advance past the label. */
    transcript_length +=
        sizeof(label) - 1U;

    /* Bind schema revision. */
    transcript[
        transcript_length] =
        GUARDIAN_SECURITY_SCHEMA_VERSION;

    /* Advance past schema. */
    transcript_length +=
        1U;

    /* Bind session identifier. */
    guardian_security_fuzz_write_u32_be(
        &transcript[
            transcript_length],
        security->session.session_id);

    /* Advance past session identifier. */
    transcript_length +=
        4U;

    /* Bind strict request counter. */
    guardian_security_fuzz_write_u64_be(
        &transcript[
            transcript_length],
        counter);

    /* Advance past counter. */
    transcript_length +=
        8U;

    /* Bind outer request sequence. */
    guardian_security_fuzz_write_u32_be(
        &transcript[
            transcript_length],
        sequence);

    /* Advance past outer sequence. */
    transcript_length +=
        4U;

    /* Bind inner command. */
    transcript[
        transcript_length] =
        inner_command;

    /* Advance past inner command. */
    transcript_length +=
        1U;

    /* Bind inner payload length. */
    guardian_security_fuzz_write_u16_be(
        &transcript[
            transcript_length],
        inner_length);

    /* Advance past inner length. */
    transcript_length +=
        2U;

    /* Bind inner payload bytes when present. */
    if (inner_length != 0U)
    {
        /* Copy authenticated inner payload. */
        (void)memcpy(
            &transcript[
                transcript_length],
            inner_payload,
            inner_length);

        /* Advance transcript ownership. */
        transcript_length +=
            inner_length;
    }

    /* Calculate full HMAC-SHA-256. */
    guardian_hmac_sha256(
        security->session.session_key,
        GUARDIAN_SECURITY_SESSION_KEY_SIZE,
        transcript,
        transcript_length,
        digest);

    /* Append the transmitted 128-bit tag. */
    (void)memcpy(
        &outer->payload[
            16U +
            inner_length],
        digest,
        GUARDIAN_SECURITY_TAG_SIZE);

    /* Publish exact secure request envelope size. */
    outer->payload_length =
        (uint16_t)(
            GUARDIAN_SECURITY_REQUEST_OVERHEAD +
            inner_length);

    /* Erase full HMAC material after truncation. */
    guardian_crypto_zero(
        digest,
        sizeof(digest));

    /* Erase the authenticated transcript. */
    guardian_crypto_zero(
        transcript,
        sizeof(transcript));
}

/* Mutate one valid secure request after its HMAC has been calculated. */
static void guardian_security_fuzz_mutate(
    guardian_frame_t *frame,
    uint32_t *state)
{
    /* Require valid mutable frame and PRNG state. */
    assert(frame != NULL);
    assert(state != NULL);

    /* Select one of seven authenticated-envelope faults. */
    uint32_t mutation =
        guardian_security_fuzz_next(
            state) %
        7U;

    /* Flip one payload bit. */
    if (mutation == 0U)
    {
        /* Select one valid payload byte. */
        size_t index =
            (size_t)(
                guardian_security_fuzz_next(
                    state) %
                (uint32_t)frame->payload_length);

        /* Flip one selected bit. */
        frame->payload[index] ^=
            (uint8_t)(
                1U <<
                (guardian_security_fuzz_next(
                    state) %
                 8U));
    }
    /* Replace one payload byte. */
    else if (mutation == 1U)
    {
        /* Select one valid payload byte. */
        size_t index =
            (size_t)(
                guardian_security_fuzz_next(
                    state) %
                (uint32_t)frame->payload_length);

        /* Preserve the original byte. */
        uint8_t original =
            frame->payload[index];

        /* Generate a guaranteed-different replacement. */
        frame->payload[index] =
            (uint8_t)(
                original +
                1U +
                (guardian_security_fuzz_next(
                    state) %
                 255U));
    }
    /* Truncate one byte from the secure envelope. */
    else if (mutation == 2U)
    {
        /* Require non-empty payload by construction. */
        assert(
            frame->payload_length != 0U);

        /* Shorten the transmitted envelope. */
        frame->payload_length -=
            1U;
    }
    /* Append one unauthenticated trailing byte. */
    else if (mutation == 3U)
    {
        /* Require remaining Guardian payload capacity. */
        assert(
            frame->payload_length <
            GUARDIAN_MAX_PAYLOAD_SIZE);

        /* Append deterministic pseudo-random data. */
        frame->payload[
            frame->payload_length] =
            (uint8_t)guardian_security_fuzz_next(
                state);

        /* Publish the larger envelope length. */
        frame->payload_length +=
            1U;
    }
    /* Change the outer sequence without recomputing HMAC. */
    else if (mutation == 4U)
    {
        /* Change the bound outer correlation field. */
        frame->sequence ^=
            0x00010000UL;
    }
    /* Corrupt the encoded inner payload length. */
    else if (mutation == 5U)
    {
        /* Change one length byte covered by HMAC. */
        frame->payload[15] ^=
            0x01U;
    }
    /* Corrupt one transmitted authentication-tag byte. */
    else
    {
        /* Select one of the final sixteen tag bytes. */
        size_t index =
            (size_t)frame->payload_length -
            GUARDIAN_SECURITY_TAG_SIZE +
            (size_t)(
                guardian_security_fuzz_next(
                    state) %
                (uint32_t)GUARDIAN_SECURITY_TAG_SIZE);

        /* Flip one tag bit. */
        frame->payload[index] ^=
            0x01U;
    }
}

/* Execute deterministic authenticated-envelope mutation and counter campaigns. */
int main(
    int argc,
    char **argv)
{
    /* Preserve one deterministic default iteration count. */
    uint32_t iterations =
        GUARDIAN_SECURITY_FUZZ_DEFAULT_ITERATIONS;

    /* Accept one optional decimal iteration count. */
    if (argc == 2)
    {
        /* Store parsed decimal value. */
        unsigned long parsed = 0UL;

        /* Require successful integer parsing. */
        if (sscanf(
                argv[1],
                "%lu",
                &parsed) != 1)
        {
            /* Report invalid local test configuration. */
            return 2;
        }

        /* Require a non-zero uint32_t value. */
        if ((parsed == 0UL) ||
            (parsed > 0xFFFFFFFFUL))
        {
            /* Report invalid local test configuration. */
            return 2;
        }

        /* Publish caller-selected campaign size. */
        iterations =
            (uint32_t)parsed;
    }

    /* Initialize deterministic mutation state. */
    uint32_t state =
        0xC0FFEE11UL;

    /* Track current iteration. */
    uint32_t iteration = 0U;

    /* Execute every requested test case. */
    for (iteration = 0U;
         iteration < iterations;
         ++iteration)
    {
        /* Create one fresh active M10 session. */
        guardian_security_t security = {0};

        /* Activate deterministic session state. */
        guardian_security_fuzz_activate(
            &security);

        /* Generate one bounded random inner payload length. */
        uint16_t inner_length =
            (uint16_t)(
                guardian_security_fuzz_next(
                    &state) %
                ((uint32_t)GUARDIAN_SECURITY_FUZZ_INNER_MAX +
                 1U));

        /* Store randomized inner payload bytes. */
        uint8_t inner_payload[
            GUARDIAN_SECURITY_FUZZ_INNER_MAX] = {0};

        /* Track randomized payload byte. */
        uint16_t index = 0U;

        /* Fill every selected inner payload byte. */
        for (index = 0U;
             index < inner_length;
             ++index)
        {
            /* Store deterministic pseudo-random test data. */
            inner_payload[index] =
                (uint8_t)guardian_security_fuzz_next(
                    &state);
        }

        /* Build one valid authenticated request at counter one. */
        guardian_frame_t valid = {0};

        /* Create the fully authenticated envelope. */
        guardian_security_fuzz_build_valid(
            &security,
            1ULL,
            0x11223344UL,
            (uint8_t)GUARDIAN_COMMAND_BASELINE_CONTROL,
            inner_payload,
            inner_length,
            &valid);

        /* Copy it before applying one after-MAC fault. */
        guardian_frame_t mutated =
            valid;

        /* Mutate one authenticated field/tag/length after HMAC calculation. */
        guardian_security_fuzz_mutate(
            &mutated,
            &state);

        /* Create inner output storage. */
        guardian_frame_t inner = {0};

        /* Store verified counter output. */
        uint64_t counter = 0ULL;

        /* Require the tampered request never to authenticate successfully. */
        guardian_security_result_t tampered_result =
            guardian_security_unwrap_request(
                &security,
                &mutated,
                0U,
                &inner,
                &counter);

        /* Fail if any after-MAC mutation is accepted. */
        assert(
            tampered_result !=
            GUARDIAN_SECURITY_OK);

        /* Require failed authenticity/length processing not to consume counter one. */
        assert(
            security.session.next_counter ==
            1ULL);

        /* Require the original untouched request still authenticates exactly once. */
        assert(
            guardian_security_unwrap_request(
                &security,
                &valid,
                0U,
                &inner,
                &counter) ==
            GUARDIAN_SECURITY_OK);

        /* Require exact verified counter. */
        assert(
            counter ==
            1ULL);

        /* Require counter two next. */
        assert(
            security.session.next_counter ==
            2ULL);

        /* Replay the exact accepted request. */
        assert(
            guardian_security_unwrap_request(
                &security,
                &valid,
                0U,
                &inner,
                &counter) ==
            GUARDIAN_SECURITY_ERROR_REPLAY);

        /* Require replay rejection not to advance expected counter. */
        assert(
            security.session.next_counter ==
            2ULL);

        /* Build one valid-MAC request that skips expected counter two. */
        guardian_frame_t gap = {0};

        /* Authenticate the skipped counter exactly. */
        guardian_security_fuzz_build_valid(
            &security,
            3ULL,
            0x55667788UL,
            (uint8_t)GUARDIAN_COMMAND_CONTROL_COMMAND,
            inner_payload,
            inner_length,
            &gap);

        /* Require strict out-of-order rejection. */
        assert(
            guardian_security_unwrap_request(
                &security,
                &gap,
                0U,
                &inner,
                &counter) ==
            GUARDIAN_SECURITY_ERROR_REPLAY);

        /* Require gap rejection not to advance expected counter. */
        assert(
            security.session.next_counter ==
            2ULL);
    }

    /* Print one concise sanitizer-campaign success line. */
    (void)printf(
        "Guardian M11 security mutation driver: PASS (%lu cases)\n",
        (unsigned long)iterations);

    /* Return conventional success. */
    return 0;
}
