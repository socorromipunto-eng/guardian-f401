/* Include the production bounded Guardian stream parser. */
#include "guardian_parser.h"

/* Include size_t for the libFuzzer entry point. */
#include <stddef.h>

/* Include fixed-width byte types for the libFuzzer entry point. */
#include <stdint.h>

/* Feed arbitrary fuzzer-provided bytes through the production parser. */
int LLVMFuzzerTestOneInput(
    const uint8_t *data,
    size_t size)
{
    /* Create independent parser state for this input. */
    guardian_parser_t parser = {0};

    /* Create one reusable decoded frame. */
    guardian_frame_t frame = {0};

    /* Track current input byte. */
    size_t index = 0U;

    /* Initialize deterministic parser state. */
    guardian_parser_init(
        &parser);

    /* Feed every fuzzer-provided byte through the production API. */
    for (index = 0U;
         index < size;
         ++index)
    {
        /* Ignore semantic result because sanitizer findings are the harness oracle. */
        (void)guardian_parser_push_byte(
            &parser,
            data[index],
            &frame);

        /* Trap impossible internal buffer growth for fuzzing. */
        if (parser.index >
            GUARDIAN_MAX_FRAME_SIZE)
        {
            /* Force a deterministic sanitizer-visible failure. */
            __builtin_trap();
        }

        /* Trap impossible declared complete-frame growth. */
        if ((parser.expected_size != 0U) &&
            (parser.expected_size >
             GUARDIAN_MAX_FRAME_SIZE))
        {
            /* Force a deterministic sanitizer-visible failure. */
            __builtin_trap();
        }
    }

    /* Report successful handling of this arbitrary input. */
    return 0;
}
