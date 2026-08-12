/* Include the public Guardian Protocol declarations. */
#include "guardian_protocol.h"

/* Include memory-copy support for bounded payload transfer. */
#include <string.h>

/* Define the reflected IEEE CRC32 polynomial used on the wire. */
#define GUARDIAN_CRC32_POLYNOMIAL ((uint32_t)0xEDB88320UL)

/* Define the CRC32 initial accumulator. */
#define GUARDIAN_CRC32_INITIAL ((uint32_t)0xFFFFFFFFUL)

/* Define the final CRC32 XOR mask. */
#define GUARDIAN_CRC32_FINAL_XOR ((uint32_t)0xFFFFFFFFUL)

/* Read one big-endian unsigned 16-bit integer from a byte buffer. */
static uint16_t guardian_read_u16_be(const uint8_t *input)
{
    /* Promote the most-significant byte before shifting it into position. */
    uint16_t high = (uint16_t)((uint16_t)input[0] << 8U);

    /* Promote the least-significant byte without shifting it. */
    uint16_t low = (uint16_t)input[1];

    /* Combine both wire bytes into the host integer. */
    return (uint16_t)(high | low);
}

/* Read one big-endian unsigned 32-bit integer from a byte buffer. */
static uint32_t guardian_read_u32_be(const uint8_t *input)
{
    /* Promote and place the most-significant sequence byte. */
    uint32_t byte_0 = ((uint32_t)input[0] << 24U);

    /* Promote and place the second sequence byte. */
    uint32_t byte_1 = ((uint32_t)input[1] << 16U);

    /* Promote and place the third sequence byte. */
    uint32_t byte_2 = ((uint32_t)input[2] << 8U);

    /* Promote the least-significant sequence byte. */
    uint32_t byte_3 = (uint32_t)input[3];

    /* Combine all four wire bytes into one host integer. */
    return byte_0 | byte_1 | byte_2 | byte_3;
}

/* Write one unsigned 16-bit integer using big-endian wire order. */
static void guardian_write_u16_be(uint8_t *output, uint16_t value)
{
    /* Write the most-significant byte first. */
    output[0] = (uint8_t)((value >> 8U) & 0xFFU);

    /* Write the least-significant byte second. */
    output[1] = (uint8_t)(value & 0xFFU);
}

/* Write one unsigned 32-bit integer using big-endian wire order. */
static void guardian_write_u32_be(uint8_t *output, uint32_t value)
{
    /* Write the most-significant byte first. */
    output[0] = (uint8_t)((value >> 24U) & 0xFFU);

    /* Write the second byte. */
    output[1] = (uint8_t)((value >> 16U) & 0xFFU);

    /* Write the third byte. */
    output[2] = (uint8_t)((value >> 8U) & 0xFFU);

    /* Write the least-significant byte last. */
    output[3] = (uint8_t)(value & 0xFFU);
}

/* Check whether a raw message type belongs to the v0.1 registry. */
static int guardian_message_type_is_valid(uint8_t message_type)
{
    /* Accept the first contiguous published message type. */
    if (message_type < (uint8_t)GUARDIAN_MESSAGE_REQUEST)
    {
        /* Reject values below the registry. */
        return 0;
    }

    /* Reject values above the last contiguous published message type. */
    if (message_type > (uint8_t)GUARDIAN_MESSAGE_TELEMETRY)
    {
        /* Reject values above the registry. */
        return 0;
    }

    /* Accept every value inside the published contiguous range. */
    return 1;
}

/* Calculate the reflected IEEE CRC32 value used by Guardian Protocol v0.1. */
uint32_t guardian_crc32(const uint8_t *data, size_t length)
{
    /* Initialize the accumulator to the protocol-defined initial value. */
    uint32_t crc = GUARDIAN_CRC32_INITIAL;

    /* Track the current input byte index. */
    size_t index = 0U;

    /* Treat a null pointer as valid only for an empty byte block. */
    if ((data == NULL) && (length != 0U))
    {
        /* Return zero because this API cannot report a structured pointer error. */
        return 0U;
    }

    /* Process every input byte in wire order. */
    for (index = 0U; index < length; ++index)
    {
        /* Mix the next byte into the low accumulator bits. */
        crc ^= (uint32_t)data[index];

        /* Track the reflected bit operation inside this byte. */
        uint8_t bit = 0U;

        /* Process all eight bits of the current byte. */
        for (bit = 0U; bit < 8U; ++bit)
        {
            /* Test the reflected least-significant accumulator bit. */
            if ((crc & 0x00000001UL) != 0U)
            {
                /* Shift and apply polynomial feedback when the bit is set. */
                crc = (crc >> 1U) ^ GUARDIAN_CRC32_POLYNOMIAL;
            }
            else
            {
                /* Shift without polynomial feedback when the bit is clear. */
                crc >>= 1U;
            }
        }
    }

    /* Apply the protocol-defined final XOR mask. */
    return crc ^ GUARDIAN_CRC32_FINAL_XOR;
}

/* Encode one validated frame into its deterministic wire representation. */
guardian_protocol_result_t guardian_protocol_encode(
    const guardian_frame_t *frame,
    uint8_t *output,
    size_t output_capacity,
    size_t *output_size)
{
    /* Store the exact frame length after validating the payload bound. */
    size_t required_size = 0U;

    /* Store the calculated CRC value before big-endian encoding. */
    uint32_t crc = 0U;

    /* Reject missing required pointers before dereferencing them. */
    if ((frame == NULL) || (output == NULL) || (output_size == NULL))
    {
        /* Report a deterministic null-argument failure. */
        return GUARDIAN_PROTOCOL_ERROR_NULL_ARGUMENT;
    }

    /* Reject message classes that are not part of protocol v0.1. */
    if (guardian_message_type_is_valid((uint8_t)frame->message_type) == 0)
    {
        /* Report the unsupported message class. */
        return GUARDIAN_PROTOCOL_ERROR_MESSAGE_TYPE;
    }

    /* Reject future flag semantics until they are explicitly implemented. */
    if (frame->flags != GUARDIAN_SUPPORTED_FLAGS)
    {
        /* Report the unsupported flags value. */
        return GUARDIAN_PROTOCOL_ERROR_FLAGS;
    }

    /* Reject payloads that exceed the fixed embedded storage bound. */
    if ((size_t)frame->payload_length > GUARDIAN_MAX_PAYLOAD_SIZE)
    {
        /* Report the bounded-payload violation. */
        return GUARDIAN_PROTOCOL_ERROR_PAYLOAD_TOO_LARGE;
    }

    /* Calculate the exact number of output bytes required by this frame. */
    required_size =
        GUARDIAN_HEADER_SIZE + (size_t)frame->payload_length + GUARDIAN_CRC_SIZE;

    /* Reject an output buffer that cannot hold the complete encoded frame. */
    if (output_capacity < required_size)
    {
        /* Report the caller-provided capacity problem. */
        return GUARDIAN_PROTOCOL_ERROR_OUTPUT_TOO_SMALL;
    }

    /* Write the first synchronization byte. */
    output[GUARDIAN_OFFSET_MAGIC_0] = GUARDIAN_MAGIC_0;

    /* Write the second synchronization byte. */
    output[GUARDIAN_OFFSET_MAGIC_1] = GUARDIAN_MAGIC_1;

    /* Write the protocol version byte. */
    output[GUARDIAN_OFFSET_VERSION] = GUARDIAN_PROTOCOL_VERSION;

    /* Write the validated message type. */
    output[GUARDIAN_OFFSET_MESSAGE_TYPE] = (uint8_t)frame->message_type;

    /* Write the command identifier without application-level interpretation. */
    output[GUARDIAN_OFFSET_COMMAND] = frame->command;

    /* Write the flags field, which is zero in protocol version 0.1. */
    output[GUARDIAN_OFFSET_FLAGS] = frame->flags;

    /* Encode the request correlation sequence number in big-endian order. */
    guardian_write_u32_be(
        &output[GUARDIAN_OFFSET_SEQUENCE],
        frame->sequence);

    /* Encode the bounded payload length in big-endian order. */
    guardian_write_u16_be(
        &output[GUARDIAN_OFFSET_PAYLOAD_LENGTH],
        frame->payload_length);

    /* Copy the payload only when meaningful payload bytes exist. */
    if (frame->payload_length != 0U)
    {
        /* Copy exactly the validated payload length into the wire buffer. */
        (void)memcpy(
            &output[GUARDIAN_HEADER_SIZE],
            frame->payload,
            (size_t)frame->payload_length);
    }

    /* Calculate CRC over the fixed header and variable payload only. */
    crc = guardian_crc32(
        output,
        GUARDIAN_HEADER_SIZE + (size_t)frame->payload_length);

    /* Encode the CRC immediately after the final payload byte. */
    guardian_write_u32_be(
        &output[GUARDIAN_HEADER_SIZE + (size_t)frame->payload_length],
        crc);

    /* Return the exact number of encoded bytes to the transport caller. */
    *output_size = required_size;

    /* Report successful encoding. */
    return GUARDIAN_PROTOCOL_OK;
}

/* Decode and validate exactly one complete encoded frame. */
guardian_protocol_result_t guardian_protocol_decode(
    const uint8_t *input,
    size_t input_size,
    guardian_frame_t *frame)
{
    /* Store the payload length extracted from the fixed header. */
    uint16_t payload_length = 0U;

    /* Store the exact size implied by the bounded payload length. */
    size_t expected_size = 0U;

    /* Store the received CRC trailer value. */
    uint32_t received_crc = 0U;

    /* Store the CRC calculated over the protected frame bytes. */
    uint32_t calculated_crc = 0U;

    /* Reject missing required pointers before reading external input. */
    if ((input == NULL) || (frame == NULL))
    {
        /* Report the deterministic null-argument failure. */
        return GUARDIAN_PROTOCOL_ERROR_NULL_ARGUMENT;
    }

    /* Reject input shorter than a header plus CRC trailer. */
    if (input_size < (GUARDIAN_HEADER_SIZE + GUARDIAN_CRC_SIZE))
    {
        /* Report a truncated frame. */
        return GUARDIAN_PROTOCOL_ERROR_FRAME_TOO_SHORT;
    }

    /* Verify the first synchronization byte. */
    if (input[GUARDIAN_OFFSET_MAGIC_0] != GUARDIAN_MAGIC_0)
    {
        /* Report invalid synchronization immediately. */
        return GUARDIAN_PROTOCOL_ERROR_MAGIC;
    }

    /* Verify the second synchronization byte. */
    if (input[GUARDIAN_OFFSET_MAGIC_1] != GUARDIAN_MAGIC_1)
    {
        /* Report invalid synchronization immediately. */
        return GUARDIAN_PROTOCOL_ERROR_MAGIC;
    }

    /* Reject protocol versions not implemented by this decoder. */
    if (input[GUARDIAN_OFFSET_VERSION] != GUARDIAN_PROTOCOL_VERSION)
    {
        /* Report the unsupported version. */
        return GUARDIAN_PROTOCOL_ERROR_VERSION;
    }

    /* Reject message classes outside the published v0.1 registry. */
    if (guardian_message_type_is_valid(input[GUARDIAN_OFFSET_MESSAGE_TYPE]) == 0)
    {
        /* Report the unsupported message class. */
        return GUARDIAN_PROTOCOL_ERROR_MESSAGE_TYPE;
    }

    /* Reject undefined flags instead of silently changing future semantics. */
    if (input[GUARDIAN_OFFSET_FLAGS] != GUARDIAN_SUPPORTED_FLAGS)
    {
        /* Report the unsupported flags value. */
        return GUARDIAN_PROTOCOL_ERROR_FLAGS;
    }

    /* Decode the two-byte big-endian payload length. */
    payload_length =
        guardian_read_u16_be(&input[GUARDIAN_OFFSET_PAYLOAD_LENGTH]);

    /* Reject payload declarations that exceed fixed embedded storage. */
    if ((size_t)payload_length > GUARDIAN_MAX_PAYLOAD_SIZE)
    {
        /* Report the bounded-payload violation. */
        return GUARDIAN_PROTOCOL_ERROR_PAYLOAD_TOO_LARGE;
    }

    /* Calculate the exact complete size declared by the header. */
    expected_size =
        GUARDIAN_HEADER_SIZE + (size_t)payload_length + GUARDIAN_CRC_SIZE;

    /* Reject truncated frames and frames containing trailing unframed bytes. */
    if (input_size != expected_size)
    {
        /* Report the disagreement between header length and actual bytes. */
        return GUARDIAN_PROTOCOL_ERROR_LENGTH_MISMATCH;
    }

    /* Decode the received CRC immediately after the payload. */
    received_crc =
        guardian_read_u32_be(&input[GUARDIAN_HEADER_SIZE + (size_t)payload_length]);

    /* Calculate CRC over every byte preceding the CRC trailer. */
    calculated_crc =
        guardian_crc32(input, GUARDIAN_HEADER_SIZE + (size_t)payload_length);

    /* Reject corrupted frames before copying payload into application-visible state. */
    if (received_crc != calculated_crc)
    {
        /* Report the integrity failure. */
        return GUARDIAN_PROTOCOL_ERROR_CRC;
    }

    /* Populate the validated message class. */
    frame->message_type =
        (guardian_message_type_t)input[GUARDIAN_OFFSET_MESSAGE_TYPE];

    /* Populate the command identifier. */
    frame->command = input[GUARDIAN_OFFSET_COMMAND];

    /* Populate the validated flags field. */
    frame->flags = input[GUARDIAN_OFFSET_FLAGS];

    /* Decode the big-endian request correlation sequence number. */
    frame->sequence =
        guardian_read_u32_be(&input[GUARDIAN_OFFSET_SEQUENCE]);

    /* Store the validated payload length. */
    frame->payload_length = payload_length;

    /* Copy payload bytes only after all structural and CRC checks pass. */
    if (payload_length != 0U)
    {
        /* Copy exactly the validated number of payload bytes. */
        (void)memcpy(
            frame->payload,
            &input[GUARDIAN_HEADER_SIZE],
            (size_t)payload_length);
    }

    /* Report a completely validated frame. */
    return GUARDIAN_PROTOCOL_OK;
}

/* Convert one deterministic result value into a stable diagnostic string. */
const char *guardian_protocol_result_string(guardian_protocol_result_t result)
{
    /* Select the diagnostic string corresponding to the structured result. */
    switch (result)
    {
        /* Describe successful validation or encoding. */
        case GUARDIAN_PROTOCOL_OK:
            return "ok";

        /* Describe a missing mandatory pointer. */
        case GUARDIAN_PROTOCOL_ERROR_NULL_ARGUMENT:
            return "null argument";

        /* Describe insufficient caller-provided output capacity. */
        case GUARDIAN_PROTOCOL_ERROR_OUTPUT_TOO_SMALL:
            return "output buffer too small";

        /* Describe a truncated frame. */
        case GUARDIAN_PROTOCOL_ERROR_FRAME_TOO_SHORT:
            return "frame too short";

        /* Describe synchronization-byte validation failure. */
        case GUARDIAN_PROTOCOL_ERROR_MAGIC:
            return "invalid magic";

        /* Describe unsupported protocol version. */
        case GUARDIAN_PROTOCOL_ERROR_VERSION:
            return "unsupported version";

        /* Describe unsupported message class. */
        case GUARDIAN_PROTOCOL_ERROR_MESSAGE_TYPE:
            return "unsupported message type";

        /* Describe undefined protocol flags. */
        case GUARDIAN_PROTOCOL_ERROR_FLAGS:
            return "unsupported flags";

        /* Describe a payload that exceeds the embedded bound. */
        case GUARDIAN_PROTOCOL_ERROR_PAYLOAD_TOO_LARGE:
            return "payload too large";

        /* Describe encoded-size disagreement. */
        case GUARDIAN_PROTOCOL_ERROR_LENGTH_MISMATCH:
            return "frame length mismatch";

        /* Describe CRC-protected data corruption. */
        case GUARDIAN_PROTOCOL_ERROR_CRC:
            return "CRC mismatch";

        /* Protect diagnostics from future enum values not handled here. */
        default:
            return "unknown protocol result";
    }
}
