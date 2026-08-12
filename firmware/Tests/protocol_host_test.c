/* Include the incremental parser API under test. */
#include "guardian_parser.h"

/* Include the canonical protocol codec API under test. */
#include "guardian_protocol.h"

/* Include assertion support for host-side deterministic tests. */
#include <assert.h>

/* Include fixed-width integer types used by test vectors. */
#include <stdint.h>

/* Include standard output for one concise success message. */
#include <stdio.h>

/* Include memory comparison support for byte-for-byte vectors. */
#include <string.h>

/* Verify the standard IEEE CRC32 check value before frame-specific tests. */
static void test_crc32_reference_vector(void)
{
    /* Define the canonical ASCII CRC32 reference input. */
    static const uint8_t input[] = "123456789";

    /* Calculate the CRC while excluding the C string terminator. */
    uint32_t crc = guardian_crc32(input, sizeof(input) - 1U);

    /* Require exact agreement with the canonical IEEE CRC32 check value. */
    assert(crc == 0xCBF43926UL);
}

/* Verify that the C encoder produces the canonical PING frame byte for byte. */
static void test_ping_encode_vector(void)
{
    /* Define the exact canonical PING request bytes shared with Python tests. */
    static const uint8_t expected[] = {
        0x47U, 0x46U, 0x01U, 0x01U,
        0x01U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x00U, 0x00U,
        0x34U, 0x02U, 0x5EU, 0x68U
    };

    /* Allocate one frame using deterministic zero initialization. */
    guardian_frame_t frame = {0};

    /* Allocate the largest legal encoded output buffer on the host test stack. */
    uint8_t encoded[GUARDIAN_MAX_FRAME_SIZE] = {0};

    /* Store the exact number of bytes produced by the encoder. */
    size_t encoded_size = 0U;

    /* Store the structured encoder outcome. */
    guardian_protocol_result_t result = GUARDIAN_PROTOCOL_OK;

    /* Configure this frame as a PING request. */
    frame.message_type = GUARDIAN_MESSAGE_REQUEST;

    /* Configure the published PING command identifier. */
    frame.command = (uint8_t)GUARDIAN_COMMAND_PING;

    /* Use the only supported v0.1 flags value. */
    frame.flags = GUARDIAN_SUPPORTED_FLAGS;

    /* Use the sequence value published by the canonical test vector. */
    frame.sequence = 1U;

    /* Define an empty request payload. */
    frame.payload_length = 0U;

    /* Encode the high-level frame into its wire representation. */
    result =
        guardian_protocol_encode(
            &frame,
            encoded,
            sizeof(encoded),
            &encoded_size);

    /* Require the encoder to succeed. */
    assert(result == GUARDIAN_PROTOCOL_OK);

    /* Require the exact canonical frame length. */
    assert(encoded_size == sizeof(expected));

    /* Require every encoded byte to match the shared cross-language vector. */
    assert(memcmp(encoded, expected, sizeof(expected)) == 0);
}

/* Verify that the C decoder reconstructs the canonical PONG response. */
static void test_pong_decode_vector(void)
{
    /* Define the exact canonical PONG response bytes shared with Python tests. */
    static const uint8_t encoded[] = {
        0x47U, 0x46U, 0x01U, 0x02U,
        0x01U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x00U, 0x04U,
        0x50U, 0x4FU, 0x4EU, 0x47U,
        0xD4U, 0xB5U, 0x2CU, 0x94U
    };

    /* Allocate the decoded frame with deterministic zero initialization. */
    guardian_frame_t frame = {0};

    /* Store the structured decoder outcome. */
    guardian_protocol_result_t result = GUARDIAN_PROTOCOL_OK;

    /* Decode and validate the canonical response frame. */
    result =
        guardian_protocol_decode(
            encoded,
            sizeof(encoded),
            &frame);

    /* Require complete structural and CRC validation. */
    assert(result == GUARDIAN_PROTOCOL_OK);

    /* Require the published RESPONSE message class. */
    assert(frame.message_type == GUARDIAN_MESSAGE_RESPONSE);

    /* Require the response to correlate to the PING command. */
    assert(frame.command == (uint8_t)GUARDIAN_COMMAND_PING);

    /* Require response/request correlation by sequence number. */
    assert(frame.sequence == 1U);

    /* Require the published four-byte response payload. */
    assert(frame.payload_length == 4U);

    /* Require the payload bytes to spell PONG exactly. */
    assert(memcmp(frame.payload, "PONG", 4U) == 0);
}

/* Verify that CRC corruption is rejected before payload becomes trusted state. */
static void test_crc_corruption_is_rejected(void)
{
    /* Define a mutable canonical empty-payload PING request. */
    uint8_t encoded[] = {
        0x47U, 0x46U, 0x01U, 0x01U,
        0x01U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x00U, 0x00U,
        0x34U, 0x02U, 0x5EU, 0x68U
    };

    /* Allocate decoded frame storage. */
    guardian_frame_t frame = {0};

    /* Store the structured decoder outcome. */
    guardian_protocol_result_t result = GUARDIAN_PROTOCOL_OK;

    /* Corrupt the final CRC byte without changing protected frame bytes. */
    encoded[sizeof(encoded) - 1U] ^= 0xFFU;

    /* Attempt to decode the corrupted frame. */
    result =
        guardian_protocol_decode(
            encoded,
            sizeof(encoded),
            &frame);

    /* Require explicit CRC rejection. */
    assert(result == GUARDIAN_PROTOCOL_ERROR_CRC);
}

/* Verify that the byte-at-a-time parser handles realistic UART fragmentation. */
static void test_incremental_parser(void)
{
    /* Define one canonical valid PONG frame. */
    static const uint8_t encoded[] = {
        0x47U, 0x46U, 0x01U, 0x02U,
        0x01U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x00U, 0x04U,
        0x50U, 0x4FU, 0x4EU, 0x47U,
        0xD4U, 0xB5U, 0x2CU, 0x94U
    };

    /* Allocate parser state without dynamic memory. */
    guardian_parser_t parser = {0};

    /* Allocate output frame storage. */
    guardian_frame_t frame = {0};

    /* Initialize the most recent parser outcome. */
    guardian_parser_result_t result = GUARDIAN_PARSER_NO_FRAME;

    /* Track the current byte index. */
    size_t index = 0U;

    /* Initialize every parser field deterministically. */
    guardian_parser_init(&parser);

    /* Feed the complete frame exactly as a UART receive loop would. */
    for (index = 0U; index < sizeof(encoded); ++index)
    {
        /* Push exactly one transport byte into the parser. */
        result =
            guardian_parser_push_byte(
                &parser,
                encoded[index],
                &frame);

        /* Require every non-final byte to remain incomplete. */
        if (index < (sizeof(encoded) - 1U))
        {
            /* Reject premature frame completion. */
            assert(result == GUARDIAN_PARSER_NO_FRAME);
        }
    }

    /* Require the final CRC byte to complete one validated frame. */
    assert(result == GUARDIAN_PARSER_FRAME_READY);

    /* Require exactly one lifetime parser success count. */
    assert(parser.frames_received == 1U);

    /* Require the decoded payload to survive incremental collection. */
    assert(memcmp(frame.payload, "PONG", 4U) == 0);
}

/* Execute every host-side protocol test in a deterministic order. */
int main(void)
{
    /* Verify the low-level CRC implementation first. */
    test_crc32_reference_vector();

    /* Verify byte-for-byte C encoding compatibility. */
    test_ping_encode_vector();

    /* Verify byte-for-byte C decoding compatibility. */
    test_pong_decode_vector();

    /* Verify explicit CRC failure handling. */
    test_crc_corruption_is_rejected();

    /* Verify byte-at-a-time stream parser integration. */
    test_incremental_parser();

    /* Print one concise success message for local and CI logs. */
    (void)printf("Guardian Protocol C host tests: PASS\n");

    /* Return the conventional successful process status. */
    return 0;
}
