/* Include the public cryptographic primitive declarations. */
#include "guardian_crypto.h"

/* Include memory helpers for bounded block construction. */
#include <string.h>

/* Define the SHA-256 compression block width. */
#define GUARDIAN_SHA256_BLOCK_SIZE ((size_t)64U)

/* Store one streaming SHA-256 calculation context. */
typedef struct
{
    /* Store the eight SHA-256 chaining words. */
    uint32_t state[8];

    /* Store a partial compression block. */
    uint8_t buffer[GUARDIAN_SHA256_BLOCK_SIZE];

    /* Store the current partial block length. */
    size_t buffer_length;

    /* Store the total number of message bytes processed before padding. */
    uint64_t total_length;
} guardian_sha256_context_t;

/* Store the sixty-four SHA-256 round constants. */
static const uint32_t guardian_sha256_k[64] =
{
    0x428A2F98UL, 0x71374491UL, 0xB5C0FBCFUL, 0xE9B5DBA5UL,
    0x3956C25BUL, 0x59F111F1UL, 0x923F82A4UL, 0xAB1C5ED5UL,
    0xD807AA98UL, 0x12835B01UL, 0x243185BEUL, 0x550C7DC3UL,
    0x72BE5D74UL, 0x80DEB1FEUL, 0x9BDC06A7UL, 0xC19BF174UL,
    0xE49B69C1UL, 0xEFBE4786UL, 0x0FC19DC6UL, 0x240CA1CCUL,
    0x2DE92C6FUL, 0x4A7484AAUL, 0x5CB0A9DCUL, 0x76F988DAUL,
    0x983E5152UL, 0xA831C66DUL, 0xB00327C8UL, 0xBF597FC7UL,
    0xC6E00BF3UL, 0xD5A79147UL, 0x06CA6351UL, 0x14292967UL,
    0x27B70A85UL, 0x2E1B2138UL, 0x4D2C6DFCUL, 0x53380D13UL,
    0x650A7354UL, 0x766A0ABBUL, 0x81C2C92EUL, 0x92722C85UL,
    0xA2BFE8A1UL, 0xA81A664BUL, 0xC24B8B70UL, 0xC76C51A3UL,
    0xD192E819UL, 0xD6990624UL, 0xF40E3585UL, 0x106AA070UL,
    0x19A4C116UL, 0x1E376C08UL, 0x2748774CUL, 0x34B0BCB5UL,
    0x391C0CB3UL, 0x4ED8AA4AUL, 0x5B9CCA4FUL, 0x682E6FF3UL,
    0x748F82EEUL, 0x78A5636FUL, 0x84C87814UL, 0x8CC70208UL,
    0x90BEFFFAUL, 0xA4506CEBUL, 0xBEF9A3F7UL, 0xC67178F2UL
};

/* Rotate one 32-bit word right by a bounded count. */
static uint32_t guardian_sha256_rotr(
    uint32_t value,
    uint32_t count)
{
    /* Combine the two shifted halves. */
    return (value >> count) |
           (value << (32U - count));
}

/* Read one big-endian 32-bit word. */
static uint32_t guardian_sha256_read_u32_be(
    const uint8_t *data)
{
    /* Combine four bytes explicitly to avoid alignment assumptions. */
    return
        ((uint32_t)data[0] << 24U) |
        ((uint32_t)data[1] << 16U) |
        ((uint32_t)data[2] << 8U) |
        (uint32_t)data[3];
}

/* Write one big-endian 32-bit word. */
static void guardian_sha256_write_u32_be(
    uint8_t *data,
    uint32_t value)
{
    /* Write the most-significant byte first. */
    data[0] =
        (uint8_t)((value >> 24U) & 0xFFU);

    /* Write the next byte. */
    data[1] =
        (uint8_t)((value >> 16U) & 0xFFU);

    /* Write the next byte. */
    data[2] =
        (uint8_t)((value >> 8U) & 0xFFU);

    /* Write the least-significant byte last. */
    data[3] =
        (uint8_t)(value & 0xFFU);
}

/* Initialize one SHA-256 context with the standardized IV. */
static void guardian_sha256_init(
    guardian_sha256_context_t *context)
{
    /* Publish the eight SHA-256 initial chaining values. */
    context->state[0] = 0x6A09E667UL;
    context->state[1] = 0xBB67AE85UL;
    context->state[2] = 0x3C6EF372UL;
    context->state[3] = 0xA54FF53AUL;
    context->state[4] = 0x510E527FUL;
    context->state[5] = 0x9B05688CUL;
    context->state[6] = 0x1F83D9ABUL;
    context->state[7] = 0x5BE0CD19UL;

    /* Start without buffered input. */
    context->buffer_length = 0U;

    /* Start without processed message bytes. */
    context->total_length = 0U;
}

/* Compress one complete SHA-256 block. */
static void guardian_sha256_transform(
    guardian_sha256_context_t *context,
    const uint8_t block[GUARDIAN_SHA256_BLOCK_SIZE])
{
    /* Store the complete message schedule. */
    uint32_t schedule[64] = {0};

    /* Store the eight working variables. */
    uint32_t a = 0U;
    uint32_t b = 0U;
    uint32_t c = 0U;
    uint32_t d = 0U;
    uint32_t e = 0U;
    uint32_t f = 0U;
    uint32_t g = 0U;
    uint32_t h = 0U;

    /* Track the current schedule/round index. */
    uint32_t index = 0U;

    /* Decode the first sixteen schedule words from the input block. */
    for (index = 0U; index < 16U; ++index)
    {
        /* Read one standardized big-endian message word. */
        schedule[index] =
            guardian_sha256_read_u32_be(
                &block[index * 4U]);
    }

    /* Expand the remaining schedule words. */
    for (index = 16U; index < 64U; ++index)
    {
        /* Calculate the first SHA-256 message-schedule sigma function. */
        uint32_t s0 =
            guardian_sha256_rotr(
                schedule[index - 15U],
                7U) ^
            guardian_sha256_rotr(
                schedule[index - 15U],
                18U) ^
            (schedule[index - 15U] >> 3U);

        /* Calculate the second SHA-256 message-schedule sigma function. */
        uint32_t s1 =
            guardian_sha256_rotr(
                schedule[index - 2U],
                17U) ^
            guardian_sha256_rotr(
                schedule[index - 2U],
                19U) ^
            (schedule[index - 2U] >> 10U);

        /* Publish the expanded schedule word. */
        schedule[index] =
            schedule[index - 16U] +
            s0 +
            schedule[index - 7U] +
            s1;
    }

    /* Load the current chaining state into working variables. */
    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    f = context->state[5];
    g = context->state[6];
    h = context->state[7];

    /* Execute all sixty-four SHA-256 compression rounds. */
    for (index = 0U; index < 64U; ++index)
    {
        /* Calculate the upper-case sigma-one function. */
        uint32_t sigma1 =
            guardian_sha256_rotr(e, 6U) ^
            guardian_sha256_rotr(e, 11U) ^
            guardian_sha256_rotr(e, 25U);

        /* Calculate the choose function. */
        uint32_t choose =
            (e & f) ^
            ((~e) & g);

        /* Calculate the first temporary compression word. */
        uint32_t temp1 =
            h +
            sigma1 +
            choose +
            guardian_sha256_k[index] +
            schedule[index];

        /* Calculate the upper-case sigma-zero function. */
        uint32_t sigma0 =
            guardian_sha256_rotr(a, 2U) ^
            guardian_sha256_rotr(a, 13U) ^
            guardian_sha256_rotr(a, 22U);

        /* Calculate the majority function. */
        uint32_t majority =
            (a & b) ^
            (a & c) ^
            (b & c);

        /* Calculate the second temporary compression word. */
        uint32_t temp2 =
            sigma0 +
            majority;

        /* Rotate the working variables for the next round. */
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    /* Add the completed working variables back into the chaining state. */
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;

    /* Erase the expanded message schedule from stack memory. */
    guardian_crypto_zero(
        schedule,
        sizeof(schedule));
}

/* Feed arbitrary bytes into one SHA-256 context. */
static void guardian_sha256_update(
    guardian_sha256_context_t *context,
    const uint8_t *data,
    size_t length)
{
    /* Track the current source byte index. */
    size_t index = 0U;

    /* Allow a null pointer only for an empty message. */
    if ((data == NULL) && (length != 0U))
    {
        /* Ignore invalid internal input defensively. */
        return;
    }

    /* Preserve the total unpadded message length. */
    context->total_length +=
        (uint64_t)length;

    /* Process every caller byte. */
    while (index < length)
    {
        /* Calculate remaining space in the partial compression block. */
        size_t available =
            GUARDIAN_SHA256_BLOCK_SIZE -
            context->buffer_length;

        /* Calculate remaining caller bytes. */
        size_t remaining =
            length -
            index;

        /* Copy only the bytes that fit in the current partial block. */
        size_t copy_length =
            (remaining < available)
            ? remaining
            : available;

        /* Append caller bytes to the partial block. */
        (void)memcpy(
            &context->buffer[
                context->buffer_length],
            &data[index],
            copy_length);

        /* Advance buffered-byte count. */
        context->buffer_length +=
            copy_length;

        /* Advance caller source ownership. */
        index +=
            copy_length;

        /* Compress each completed 64-byte block immediately. */
        if (context->buffer_length ==
            GUARDIAN_SHA256_BLOCK_SIZE)
        {
            /* Compress the complete block. */
            guardian_sha256_transform(
                context,
                context->buffer);

            /* Reset partial-block ownership. */
            context->buffer_length = 0U;
        }
    }
}

/* Finalize one SHA-256 calculation with standardized padding. */
static void guardian_sha256_final(
    guardian_sha256_context_t *context,
    uint8_t digest[GUARDIAN_SHA256_SIZE])
{
    /* Store the original message length in bits. */
    uint64_t bit_length =
        context->total_length * 8ULL;

    /* Track digest word output. */
    uint32_t index = 0U;

    /* Append the mandatory one bit represented as byte 0x80. */
    context->buffer[
        context->buffer_length] =
        0x80U;

    /* Advance buffered length past the one-bit marker byte. */
    context->buffer_length += 1U;

    /* Compress an extra block when the 64-bit length field no longer fits. */
    if (context->buffer_length > 56U)
    {
        /* Zero-fill the remainder of the current block. */
        (void)memset(
            &context->buffer[
                context->buffer_length],
            0,
            GUARDIAN_SHA256_BLOCK_SIZE -
            context->buffer_length);

        /* Compress the padded block. */
        guardian_sha256_transform(
            context,
            context->buffer);

        /* Start the final block empty. */
        context->buffer_length = 0U;
    }

    /* Zero-fill through the first byte of the final length field. */
    (void)memset(
        &context->buffer[
            context->buffer_length],
        0,
        56U -
        context->buffer_length);

    /* Write the original 64-bit bit length in big-endian order. */
    context->buffer[56] =
        (uint8_t)((bit_length >> 56U) & 0xFFU);
    context->buffer[57] =
        (uint8_t)((bit_length >> 48U) & 0xFFU);
    context->buffer[58] =
        (uint8_t)((bit_length >> 40U) & 0xFFU);
    context->buffer[59] =
        (uint8_t)((bit_length >> 32U) & 0xFFU);
    context->buffer[60] =
        (uint8_t)((bit_length >> 24U) & 0xFFU);
    context->buffer[61] =
        (uint8_t)((bit_length >> 16U) & 0xFFU);
    context->buffer[62] =
        (uint8_t)((bit_length >> 8U) & 0xFFU);
    context->buffer[63] =
        (uint8_t)(bit_length & 0xFFU);

    /* Compress the final padded block. */
    guardian_sha256_transform(
        context,
        context->buffer);

    /* Serialize the eight chaining words into the digest. */
    for (index = 0U; index < 8U; ++index)
    {
        /* Write one big-endian digest word. */
        guardian_sha256_write_u32_be(
            &digest[index * 4U],
            context->state[index]);
    }

    /* Erase the complete internal SHA-256 state. */
    guardian_crypto_zero(
        context,
        sizeof(*context));
}

/* Calculate one SHA-256 digest. */
void guardian_sha256(
    const uint8_t *data,
    size_t length,
    uint8_t digest[GUARDIAN_SHA256_SIZE])
{
    /* Create one stack-local streaming context. */
    guardian_sha256_context_t context = {0};

    /* Ignore a missing output pointer defensively. */
    if (digest == NULL)
    {
        /* Return without writing memory. */
        return;
    }

    /* Initialize standardized SHA-256 state. */
    guardian_sha256_init(
        &context);

    /* Hash the complete caller message. */
    guardian_sha256_update(
        &context,
        data,
        length);

    /* Finalize into the caller digest. */
    guardian_sha256_final(
        &context,
        digest);
}

/* Calculate one HMAC-SHA-256 authentication code. */
void guardian_hmac_sha256(
    const uint8_t *key,
    size_t key_length,
    const uint8_t *data,
    size_t data_length,
    uint8_t digest[GUARDIAN_SHA256_SIZE])
{
    /* Store the block-sized normalized HMAC key. */
    uint8_t key_block[GUARDIAN_SHA256_BLOCK_SIZE] = {0};

    /* Store the inner keyed pad. */
    uint8_t inner_pad[GUARDIAN_SHA256_BLOCK_SIZE] = {0};

    /* Store the outer keyed pad. */
    uint8_t outer_pad[GUARDIAN_SHA256_BLOCK_SIZE] = {0};

    /* Store the intermediate inner digest. */
    uint8_t inner_digest[GUARDIAN_SHA256_SIZE] = {0};

    /* Store a digest of oversized keys. */
    uint8_t key_digest[GUARDIAN_SHA256_SIZE] = {0};

    /* Track the block index. */
    size_t index = 0U;

    /* Create inner and outer SHA-256 contexts. */
    guardian_sha256_context_t inner_context = {0};
    guardian_sha256_context_t outer_context = {0};

    /* Ignore a missing output pointer defensively. */
    if (digest == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Reject a missing non-empty key defensively. */
    if ((key == NULL) && (key_length != 0U))
    {
        /* Publish deterministic zero output for invalid internal use. */
        (void)memset(
            digest,
            0,
            GUARDIAN_SHA256_SIZE);

        /* Return without undefined memory reads. */
        return;
    }

    /* Hash keys larger than one SHA-256 compression block. */
    if (key_length >
        GUARDIAN_SHA256_BLOCK_SIZE)
    {
        /* Compress the oversized key to one digest. */
        guardian_sha256(
            key,
            key_length,
            key_digest);

        /* Copy the digest into the normalized HMAC key block. */
        (void)memcpy(
            key_block,
            key_digest,
            GUARDIAN_SHA256_SIZE);
    }
    else if (key_length != 0U)
    {
        /* Copy the original short key into the zero-padded key block. */
        (void)memcpy(
            key_block,
            key,
            key_length);
    }

    /* Build HMAC inner and outer pads. */
    for (index = 0U;
         index < GUARDIAN_SHA256_BLOCK_SIZE;
         ++index)
    {
        /* XOR normalized key with standardized inner pad constant. */
        inner_pad[index] =
            (uint8_t)(
                key_block[index] ^
                0x36U);

        /* XOR normalized key with standardized outer pad constant. */
        outer_pad[index] =
            (uint8_t)(
                key_block[index] ^
                0x5CU);
    }

    /* Calculate SHA256(inner_pad || message). */
    guardian_sha256_init(
        &inner_context);

    /* Feed the complete inner pad. */
    guardian_sha256_update(
        &inner_context,
        inner_pad,
        sizeof(inner_pad));

    /* Feed the caller message. */
    guardian_sha256_update(
        &inner_context,
        data,
        data_length);

    /* Finalize the inner digest. */
    guardian_sha256_final(
        &inner_context,
        inner_digest);

    /* Calculate SHA256(outer_pad || inner_digest). */
    guardian_sha256_init(
        &outer_context);

    /* Feed the complete outer pad. */
    guardian_sha256_update(
        &outer_context,
        outer_pad,
        sizeof(outer_pad));

    /* Feed the inner digest. */
    guardian_sha256_update(
        &outer_context,
        inner_digest,
        sizeof(inner_digest));

    /* Finalize the HMAC digest. */
    guardian_sha256_final(
        &outer_context,
        digest);

    /* Erase normalized key material. */
    guardian_crypto_zero(
        key_block,
        sizeof(key_block));

    /* Erase the inner pad. */
    guardian_crypto_zero(
        inner_pad,
        sizeof(inner_pad));

    /* Erase the outer pad. */
    guardian_crypto_zero(
        outer_pad,
        sizeof(outer_pad));

    /* Erase the intermediate inner digest. */
    guardian_crypto_zero(
        inner_digest,
        sizeof(inner_digest));

    /* Erase any oversized-key digest. */
    guardian_crypto_zero(
        key_digest,
        sizeof(key_digest));
}

/* Compare two secret byte strings without data-dependent early exit. */
int guardian_crypto_constant_time_equal(
    const uint8_t *left,
    const uint8_t *right,
    size_t length)
{
    /* Store the accumulated byte difference. */
    uint8_t difference = 0U;

    /* Track the current byte. */
    size_t index = 0U;

    /* Reject missing non-empty inputs. */
    if (((left == NULL) ||
         (right == NULL)) &&
        (length != 0U))
    {
        /* Report mismatch. */
        return 0;
    }

    /* Compare every byte regardless of earlier differences. */
    for (index = 0U;
         index < length;
         ++index)
    {
        /* Accumulate all XOR differences. */
        difference |=
            (uint8_t)(
                left[index] ^
                right[index]);
    }

    /* Return one only when every byte matched. */
    return (difference == 0U)
        ? 1
        : 0;
}

/* Erase sensitive memory through volatile writes. */
void guardian_crypto_zero(
    void *data,
    size_t length)
{
    /* Convert caller storage into volatile bytes so writes are retained. */
    volatile uint8_t *bytes =
        (volatile uint8_t *)data;

    /* Ignore a missing pointer defensively. */
    if (bytes == NULL)
    {
        /* Return without touching memory. */
        return;
    }

    /* Overwrite every requested byte. */
    while (length != 0U)
    {
        /* Clear the current byte. */
        *bytes = 0U;

        /* Advance the volatile pointer. */
        bytes += 1;

        /* Consume one remaining byte. */
        length -= 1U;
    }
}
