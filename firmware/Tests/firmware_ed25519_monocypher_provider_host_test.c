#include "guardian_firmware_trusted_signature_adapter.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TEST_KEY_ID 77u

static const uint8_t g_pk1[32] = {
    0xd7u,0x5au,0x98u,0x01u,0x82u,0xb1u,0x0au,0xb7u,0xd5u,0x4bu,0xfeu,0xd3u,0xc9u,0x64u,0x07u,0x3au,
    0x0eu,0xe1u,0x72u,0xf3u,0xdau,0xa6u,0x23u,0x25u,0xafu,0x02u,0x1au,0x68u,0xf7u,0x07u,0x51u,0x1au
};
static const uint8_t g_sig1[64] = {
    0xe5u,0x56u,0x43u,0x00u,0xc3u,0x60u,0xacu,0x72u,0x90u,0x86u,0xe2u,0xccu,0x80u,0x6eu,0x82u,0x8au,
    0x84u,0x87u,0x7fu,0x1eu,0xb8u,0xe5u,0xd9u,0x74u,0xd8u,0x73u,0xe0u,0x65u,0x22u,0x49u,0x01u,0x55u,
    0x5fu,0xb8u,0x82u,0x15u,0x90u,0xa3u,0x3bu,0xacu,0xc6u,0x1eu,0x39u,0x70u,0x1cu,0xf9u,0xb4u,0x6bu,
    0xd2u,0x5bu,0xf5u,0xf0u,0x59u,0x5bu,0xbeu,0x24u,0x65u,0x51u,0x41u,0x43u,0x8eu,0x7au,0x10u,0x0bu
};
static const uint8_t g_pk2[32] = {
    0x3du,0x40u,0x17u,0xc3u,0xe8u,0x43u,0x89u,0x5au,0x92u,0xb7u,0x0au,0xa7u,0x4du,0x1bu,0x7eu,0xbcu,
    0x9cu,0x98u,0x2cu,0xcfu,0x2eu,0xc4u,0x96u,0x8cu,0xc0u,0xcdu,0x55u,0xf1u,0x2au,0xf4u,0x66u,0x0cu
};
static const uint8_t g_msg2[1] = { 0x72u };
static const uint8_t g_sig2[64] = {
    0x92u,0xa0u,0x09u,0xa9u,0xf0u,0xd4u,0xcau,0xb8u,0x72u,0x0eu,0x82u,0x0bu,0x5fu,0x64u,0x25u,0x40u,
    0xa2u,0xb2u,0x7bu,0x54u,0x16u,0x50u,0x3fu,0x8fu,0xb3u,0x76u,0x22u,0x23u,0xebu,0xdbu,0x69u,0xdau,
    0x08u,0x5au,0xc1u,0xe4u,0x3eu,0x15u,0x99u,0x6eu,0x45u,0x8fu,0x36u,0x13u,0xd0u,0xf1u,0x1du,0x8cu,
    0x38u,0x7bu,0x2eu,0xaeu,0xb4u,0x30u,0x2au,0xeeu,0xb0u,0x0du,0x29u,0x16u,0x12u,0xbbu,0x0cu,0x00u
};

static const guardian_firmware_trusted_key_record_t g_test_record = {
    TEST_KEY_ID,
    GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519,
    GUARDIAN_FIRMWARE_TRUSTED_KEY_STATE_ACTIVE,
    {
        0x3du,0x40u,0x17u,0xc3u,0xe8u,0x43u,0x89u,0x5au,0x92u,0xb7u,0x0au,0xa7u,0x4du,0x1bu,0x7eu,0xbcu,
        0x9cu,0x98u,0x2cu,0xcfu,0x2eu,0xc4u,0x96u,0x8cu,0xc0u,0xcdu,0x55u,0xf1u,0x2au,0xf4u,0x66u,0x0cu
    },
    GUARDIAN_FIRMWARE_TRUSTED_KEY_PROVENANCE_PRODUCTION_APPROVED,
    {
        0x01u,0u,0u,0u,0u,0u,0u,0u,
        0u,0u,0u,0u,0u,0u,0u,0u,
        0u,0u,0u,0u,0u,0u,0u,0u,
        0u,0u,0u,0u,0u,0u,0u,0u
    }
};

guardian_firmware_trusted_key_resolution_t guardian_firmware_trusted_key_provider_resolve(
    uint32_t key_id,
    guardian_firmware_trusted_key_algorithm_t required_algorithm,
    const guardian_firmware_trusted_key_record_t **out_record)
{
    if (out_record == NULL) {
        return GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_INVALID_ARGUMENT;
    }
    *out_record = NULL;
    if (key_id != TEST_KEY_ID) {
        return GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_UNKNOWN_KEY;
    }
    if (required_algorithm != GUARDIAN_FIRMWARE_TRUSTED_KEY_ALGORITHM_ED25519) {
        return GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_ALGORITHM_MISMATCH;
    }
    *out_record = &g_test_record;
    return GUARDIAN_FIRMWARE_TRUSTED_KEY_RESOLUTION_OK;
}

guardian_firmware_trusted_key_resolution_t guardian_firmware_trusted_key_provider_resolve_production(
    uint32_t key_id,
    guardian_firmware_trusted_key_algorithm_t required_algorithm,
    const guardian_firmware_trusted_key_record_t **out_record)
{
    return guardian_firmware_trusted_key_provider_resolve(
        key_id,
        required_algorithm,
        out_record);
}

static void test_rfc8032_vector1_empty(void)
{
    assert(guardian_firmware_ed25519_backend_verify(g_sig1, g_pk1, NULL, 0u) == 1);
    (void)printf("RFC8032_VECTOR_1_EMPTY=PASS\n");
}

static void test_rfc8032_vector2_onebyte(void)
{
    assert(guardian_firmware_ed25519_backend_verify(g_sig2, g_pk2, g_msg2, sizeof(g_msg2)) == 1);
    (void)printf("RFC8032_VECTOR_2_ONEBYTE=PASS\n");
}

static void test_negative_vectors(void)
{
    uint8_t sig[64];
    uint8_t key[32];
    uint8_t bad_message[1] = { 0x73u };
    uint8_t zero_signature[64] = {0};

    assert(guardian_firmware_ed25519_backend_verify(g_sig2, g_pk2, bad_message, sizeof(bad_message)) == 0);
    (void)printf("WRONG_MESSAGE=REJECTED\n");

    (void)memcpy(sig, g_sig2, sizeof(sig));
    sig[20] ^= 0x08u;
    assert(guardian_firmware_ed25519_backend_verify(sig, g_pk2, g_msg2, sizeof(g_msg2)) == 0);
    (void)printf("SIGNATURE_BIT_FLIP=REJECTED\n");

    (void)memcpy(key, g_pk2, sizeof(key));
    key[0] ^= 0x01u;
    assert(guardian_firmware_ed25519_backend_verify(g_sig2, key, g_msg2, sizeof(g_msg2)) == 0);
    (void)printf("WRONG_PUBLIC_KEY=REJECTED\n");

    assert(guardian_firmware_ed25519_backend_verify(zero_signature, g_pk2, g_msg2, sizeof(g_msg2)) == 0);
    (void)printf("ALL_ZERO_SIGNATURE=REJECTED\n");
}

static void test_backend_guards(void)
{
    assert(guardian_firmware_ed25519_backend_verify(NULL, g_pk2, g_msg2, sizeof(g_msg2)) == 0);
    assert(guardian_firmware_ed25519_backend_verify(g_sig2, NULL, g_msg2, sizeof(g_msg2)) == 0);
    assert(guardian_firmware_ed25519_backend_verify(g_sig2, g_pk2, NULL, 1u) == 0);
    assert(guardian_firmware_ed25519_backend_verify(g_sig1, g_pk1, NULL, 0u) == 1);
    (void)printf("BACKEND_NULL_GUARDS=PASS\n");
}

static void test_adapter_real_backend(void)
{
    uint8_t bad_sig[64];
    (void)memcpy(bad_sig, g_sig2, sizeof(bad_sig));
    bad_sig[40] ^= 0x10u;

    assert(guardian_firmware_trusted_signature_verify(NULL,
        GUARDIAN_FIRMWARE_SIGNATURE_ED25519,
        TEST_KEY_ID,
        g_msg2,
        sizeof(g_msg2),
        g_sig2,
        GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES) == 1);
    (void)printf("ADAPTER_REAL_BACKEND_VECTOR_2=PASS\n");

    assert(guardian_firmware_trusted_signature_verify(NULL,
        GUARDIAN_FIRMWARE_SIGNATURE_ED25519,
        999u,
        g_msg2,
        sizeof(g_msg2),
        g_sig2,
        GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES) == 0);
    (void)printf("ADAPTER_UNKNOWN_KEY=REJECTED\n");

    assert(guardian_firmware_trusted_signature_verify(NULL,
        GUARDIAN_FIRMWARE_SIGNATURE_DEMO_HMAC_SHA256,
        TEST_KEY_ID,
        g_msg2,
        sizeof(g_msg2),
        g_sig2,
        GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES) == 0);
    (void)printf("ADAPTER_ALGORITHM_CONFUSION=REJECTED\n");

    assert(guardian_firmware_trusted_signature_verify(NULL,
        GUARDIAN_FIRMWARE_SIGNATURE_ED25519,
        TEST_KEY_ID,
        g_msg2,
        sizeof(g_msg2),
        g_sig2,
        (uint8_t)(GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES - 1u)) == 0);
    (void)printf("ADAPTER_TRUNCATED_SIGNATURE=REJECTED\n");

    assert(guardian_firmware_trusted_signature_verify(NULL,
        GUARDIAN_FIRMWARE_SIGNATURE_ED25519,
        TEST_KEY_ID,
        g_msg2,
        sizeof(g_msg2),
        bad_sig,
        GUARDIAN_FIRMWARE_ED25519_SIGNATURE_BYTES) == 0);
}

int main(void)
{
    test_rfc8032_vector1_empty();
    test_rfc8032_vector2_onebyte();
    test_negative_vectors();
    test_backend_guards();
    test_adapter_real_backend();
    (void)printf("M17_PRODUCTION_ED25519_PROVIDER_HOST_TEST=PASS\n");
    return 0;
}
