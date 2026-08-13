/* Include the bounded Guardian incremental parser under test. */
#include "guardian_parser.h"

/* Include assertion support for fail-fast sanitizer campaigns. */
#include <assert.h>

/* Include standard output for one concise campaign summary. */
#include <stdio.h>

/* Include memory helpers for deterministic byte mutations. */
#include <string.h>

/* Define a bounded mutation workspace larger than one maximum Guardian frame. */
#define GUARDIAN_FUZZ_STREAM_CAPACITY ((size_t)544U)

/* Define the default deterministic mutation count. */
#define GUARDIAN_FUZZ_DEFAULT_ITERATIONS ((uint32_t)10000U)

/* Return one deterministic xorshift32 pseudo-random word. */
static uint32_t guardian_fuzz_next(
    uint32_t *state)
{
    /* Require caller-owned PRNG state. */
    assert(state != NULL);

    /* Avoid the xorshift all-zero fixed point. */
    if (*state == 0U)
    {
        /* Replace zero with one fixed non-zero seed. */
        *state = 0x6D2B79F5UL;
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

/* Build one canonical PING request used as the mutation seed and recovery probe. */
static size_t guardian_fuzz_ping(
    uint32_t sequence,
    uint8_t output[GUARDIAN_MAX_FRAME_SIZE])
{
    /* Create one zero-initialized frame. */
    guardian_frame_t frame = {0};

    /* Store the encoded frame size. */
    size_t output_size = 0U;

    /* Publish request message class. */
    frame.message_type =
        GUARDIAN_MESSAGE_REQUEST;

    /* Select PING. */
    frame.command =
        (uint8_t)GUARDIAN_COMMAND_PING;

    /* Use protocol v0.1 flags. */
    frame.flags =
        GUARDIAN_SUPPORTED_FLAGS;

    /* Publish caller-selected correlation sequence. */
    frame.sequence =
        sequence;

    /* Publish empty PING payload. */
    frame.payload_length =
        0U;

    /* Encode the complete canonical frame. */
    assert(
        guardian_protocol_encode(
            &frame,
            output,
            GUARDIAN_MAX_FRAME_SIZE,
            &output_size) ==
        GUARDIAN_PROTOCOL_OK);

    /* Return the exact encoded size. */
    return output_size;
}

/* Mutate one canonical frame into a bounded arbitrary byte stream. */
static size_t guardian_fuzz_mutate(
    const uint8_t *source,
    size_t source_length,
    uint8_t *output,
    size_t output_capacity,
    uint32_t *state)
{
    /* Require valid caller buffers and deterministic state. */
    assert(source != NULL);
    assert(output != NULL);
    assert(state != NULL);

    /* Require source to fit output storage. */
    assert(source_length <= output_capacity);

    /* Start from an exact canonical copy. */
    (void)memcpy(
        output,
        source,
        source_length);

    /* Preserve the initial stream length. */
    size_t length =
        source_length;

    /* Select one of seven mutation classes. */
    uint32_t mutation =
        guardian_fuzz_next(
            state) %
        7U;

    /* Flip one random bit. */
    if (mutation == 0U)
    {
        /* Select one valid byte index. */
        size_t index =
            (size_t)(
                guardian_fuzz_next(
                    state) %
                (uint32_t)length);

        /* Select one bit index. */
        uint8_t bit =
            (uint8_t)(
                guardian_fuzz_next(
                    state) %
                8U);

        /* Flip exactly one bit. */
        output[index] ^=
            (uint8_t)(
                1U << bit);
    }
    /* Replace one byte. */
    else if (mutation == 1U)
    {
        /* Select one valid byte index. */
        size_t index =
            (size_t)(
                guardian_fuzz_next(
                    state) %
                (uint32_t)length);

        /* Replace it with deterministic pseudo-random data. */
        output[index] =
            (uint8_t)guardian_fuzz_next(
                state);
    }
    /* Delete one byte. */
    else if (mutation == 2U)
    {
        /* Select one valid byte index. */
        size_t index =
            (size_t)(
                guardian_fuzz_next(
                    state) %
                (uint32_t)length);

        /* Shift the suffix over the deleted byte. */
        (void)memmove(
            &output[index],
            &output[index + 1U],
            length - index - 1U);

        /* Shorten the stream by one byte. */
        length -= 1U;
    }
    /* Insert bounded pseudo-random noise. */
    else if (mutation == 3U)
    {
        /* Select one to sixteen noise bytes. */
        size_t noise_length =
            (size_t)(
                (guardian_fuzz_next(
                    state) %
                 16U) +
                1U);

        /* Clamp insertion to remaining workspace. */
        if ((length + noise_length) >
            output_capacity)
        {
            /* Use all remaining capacity only. */
            noise_length =
                output_capacity -
                length;
        }

        /* Select one insertion point including EOF. */
        size_t insertion =
            (size_t)(
                guardian_fuzz_next(
                    state) %
                (uint32_t)(
                    length +
                    1U));

        /* Make space for inserted bytes. */
        (void)memmove(
            &output[
                insertion +
                noise_length],
            &output[insertion],
            length -
                insertion);

        /* Fill inserted bytes deterministically. */
        size_t index = 0U;

        /* Generate every inserted byte. */
        for (index = 0U;
             index < noise_length;
             ++index)
        {
            /* Store one pseudo-random byte. */
            output[
                insertion +
                index] =
                (uint8_t)guardian_fuzz_next(
                    state);
        }

        /* Publish the expanded stream size. */
        length +=
            noise_length;
    }
    /* Truncate the stream. */
    else if (mutation == 4U)
    {
        /* Choose a cut strictly before original EOF. */
        length =
            (size_t)(
                guardian_fuzz_next(
                    state) %
                (uint32_t)length);
    }
    /* Poison the two-byte payload length. */
    else if (mutation == 5U)
    {
        /* Require the complete fixed header before changing its length field. */
        if (length >=
            GUARDIAN_HEADER_SIZE)
        {
            /* Publish arbitrary high length byte. */
            output[
                GUARDIAN_OFFSET_PAYLOAD_LENGTH] =
                (uint8_t)guardian_fuzz_next(
                    state);

            /* Publish arbitrary low length byte. */
            output[
                GUARDIAN_OFFSET_PAYLOAD_LENGTH +
                1U] =
                (uint8_t)guardian_fuzz_next(
                    state);
        }
    }
    /* Corrupt one CRC byte. */
    else
    {
        /* Require the CRC trailer to exist. */
        if (length >=
            GUARDIAN_CRC_SIZE)
        {
            /* Select one of the final four bytes. */
            size_t index =
                length -
                GUARDIAN_CRC_SIZE +
                (size_t)(
                    guardian_fuzz_next(
                        state) %
                    (uint32_t)GUARDIAN_CRC_SIZE);

            /* Flip one low-order bit. */
            output[index] ^=
                (uint8_t)(
                    1U <<
                    (guardian_fuzz_next(
                        state) %
                     8U));
        }
    }

    /* Return the mutated stream length. */
    return length;
}

/* Feed one byte stream into the parser while checking bounded internal invariants. */
static void guardian_fuzz_feed(
    guardian_parser_t *parser,
    const uint8_t *data,
    size_t length,
    int *recovered)
{
    /* Require valid parser and stream pointers. */
    assert(parser != NULL);
    assert((data != NULL) || (length == 0U));

    /* Track current stream byte. */
    size_t index = 0U;

    /* Create output-frame storage reused for each push. */
    guardian_frame_t frame = {0};

    /* Consume every byte independently. */
    for (index = 0U;
         index < length;
         ++index)
    {
        /* Push exactly one byte through the production parser. */
        guardian_parser_result_t result =
            guardian_parser_push_byte(
                parser,
                data[index],
                &frame);

        /* Require parser index never to exceed fixed frame storage. */
        assert(
            parser->index <=
            GUARDIAN_MAX_FRAME_SIZE);

        /* Require expected size to remain bounded whenever known. */
        assert(
            (parser->expected_size == 0U) ||
            (parser->expected_size <=
             GUARDIAN_MAX_FRAME_SIZE));

        /* Record the distinctive recovery PING only when requested by caller. */
        if ((recovered != NULL) &&
            (result ==
             GUARDIAN_PARSER_FRAME_READY) &&
            (frame.command ==
             (uint8_t)GUARDIAN_COMMAND_PING) &&
            (frame.sequence ==
             0xA5A55A5AUL))
        {
            /* Mark successful bounded parser recovery. */
            *recovered = 1;
        }
    }
}

/* Execute deterministic parser mutations under sanitizer instrumentation. */
int main(
    int argc,
    char **argv)
{
    /* Preserve one deterministic default iteration count. */
    uint32_t iterations =
        GUARDIAN_FUZZ_DEFAULT_ITERATIONS;

    /* Accept one optional decimal iteration count. */
    if (argc == 2)
    {
        /* Parse the compact integer argument. */
        unsigned long parsed = 0UL;

        /* Require successful positive decimal parsing. */
        if (sscanf(
                argv[1],
                "%lu",
                &parsed) != 1)
        {
            /* Report invalid local test configuration. */
            return 2;
        }

        /* Require the value to fit uint32_t and remain non-zero. */
        if ((parsed == 0UL) ||
            (parsed > 0xFFFFFFFFUL))
        {
            /* Report invalid local test configuration. */
            return 2;
        }

        /* Publish the requested campaign size. */
        iterations =
            (uint32_t)parsed;
    }

    /* Build one canonical mutation seed frame. */
    uint8_t canonical_ping[
        GUARDIAN_MAX_FRAME_SIZE] = {0};

    /* Store canonical seed size. */
    size_t canonical_size =
        guardian_fuzz_ping(
            1U,
            canonical_ping);

    /* Build one distinctive canonical recovery frame. */
    uint8_t recovery_ping[
        GUARDIAN_MAX_FRAME_SIZE] = {0};

    /* Store recovery frame size. */
    size_t recovery_size =
        guardian_fuzz_ping(
            0xA5A55A5AUL,
            recovery_ping);

    /* Store bounded mutation bytes. */
    uint8_t mutated[
        GUARDIAN_FUZZ_STREAM_CAPACITY] = {0};

    /* Store zero flush bytes that cannot begin Guardian magic. */
    uint8_t flush[
        GUARDIAN_MAX_FRAME_SIZE] = {0};

    /* Initialize deterministic campaign PRNG state. */
    uint32_t state =
        0xC0FFEE11UL;

    /* Track the current mutation iteration. */
    uint32_t iteration = 0U;

    /* Execute every requested deterministic mutation. */
    for (iteration = 0U;
         iteration < iterations;
         ++iteration)
    {
        /* Create independent parser state for each mutation. */
        guardian_parser_t parser = {0};

        /* Initialize production parser state. */
        guardian_parser_init(
            &parser);

        /* Mutate one canonical frame. */
        size_t mutated_length =
            guardian_fuzz_mutate(
                canonical_ping,
                canonical_size,
                mutated,
                sizeof(mutated),
                &state);

        /* Consume arbitrary mutated bytes. */
        guardian_fuzz_feed(
            &parser,
            mutated,
            mutated_length,
            NULL);

        /* Complete any maximum-size attacker-controlled candidate with zero bytes. */
        guardian_fuzz_feed(
            &parser,
            flush,
            sizeof(flush),
            NULL);

        /* Start without recovery proof. */
        int recovered =
            0;

        /* Feed the canonical recovery frame. */
        guardian_fuzz_feed(
            &parser,
            recovery_ping,
            recovery_size,
            &recovered);

        /* Require recovery after every bounded malformed stream. */
        assert(
            recovered != 0);
    }

    /* Print one concise sanitizer-campaign success line. */
    (void)printf(
        "Guardian M11 parser mutation driver: PASS (%lu cases)\n",
        (unsigned long)iterations);

    /* Return conventional success. */
    return 0;
}
