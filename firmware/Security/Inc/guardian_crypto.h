#ifndef GUARDIAN_CRYPTO_H
#define GUARDIAN_CRYPTO_H

/* Include size_t for bounded buffers and message lengths. */
#include <stddef.h>

/* Include fixed-width integer types for deterministic digest storage. */
#include <stdint.h>

/* Define the SHA-256 digest width. */
#define GUARDIAN_SHA256_SIZE ((size_t)32U)

/* Calculate one SHA-256 digest. */
void guardian_sha256(
    const uint8_t *data,
    size_t length,
    uint8_t digest[GUARDIAN_SHA256_SIZE]);

/* Calculate one HMAC-SHA-256 authentication code. */
void guardian_hmac_sha256(
    const uint8_t *key,
    size_t key_length,
    const uint8_t *data,
    size_t data_length,
    uint8_t digest[GUARDIAN_SHA256_SIZE]);

/* Compare two secret byte strings without data-dependent early exit. */
int guardian_crypto_constant_time_equal(
    const uint8_t *left,
    const uint8_t *right,
    size_t length);

/* Erase sensitive memory through volatile writes. */
void guardian_crypto_zero(
    void *data,
    size_t length);

#endif
