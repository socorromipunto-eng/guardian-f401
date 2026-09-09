#ifndef GUARDIAN_NODE_LINK_AUTH_H
#define GUARDIAN_NODE_LINK_AUTH_H

#include "guardian_node_link.h"

#include <stddef.h>
#include <stdint.h>

#define GUARDIAN_NODE_LINK_SIGNATURE_ED25519 ((uint8_t)0x01U)

#define GUARDIAN_NODE_LINK_ED25519_SIGNATURE_SIZE ((size_t)64U)

#define GUARDIAN_NODE_LINK_AUTH_MAX_PEERS ((size_t)8U)

#define GUARDIAN_NODE_LINK_AUTH_DOMAIN \
    "GUARDIAN-F401:NODELINK:AUTH:V1"

#define GUARDIAN_NODE_LINK_AUTH_DOMAIN_SIZE ((size_t)30U)

#define GUARDIAN_NODE_LINK_AUTH_TRANSCRIPT_FIXED_SIZE ((size_t)56U)

#define GUARDIAN_NODE_LINK_AUTH_TRANSCRIPT_MAX_SIZE \
    (GUARDIAN_NODE_LINK_AUTH_TRANSCRIPT_FIXED_SIZE + \
     GUARDIAN_NODE_LINK_MAX_PAYLOAD)

typedef enum
{
    GUARDIAN_NODE_LINK_VERIFY_VERIFIED = 0,
    GUARDIAN_NODE_LINK_VERIFY_SIGNATURE_INVALID,
    GUARDIAN_NODE_LINK_VERIFY_KEY_UNKNOWN,
    GUARDIAN_NODE_LINK_VERIFY_KEY_REVOKED,
    GUARDIAN_NODE_LINK_VERIFY_ALGORITHM_UNSUPPORTED,
    GUARDIAN_NODE_LINK_VERIFY_PROVIDER_UNAVAILABLE,
    GUARDIAN_NODE_LINK_VERIFY_PROVIDER_FAILURE
} guardian_node_link_verify_result_t;

typedef enum
{
    GUARDIAN_NODE_LINK_AUTH_OK = 0,
    GUARDIAN_NODE_LINK_AUTH_ERROR_NULL_ARGUMENT,
    GUARDIAN_NODE_LINK_AUTH_ERROR_UNCONFIGURED,
    GUARDIAN_NODE_LINK_AUTH_ERROR_PEER_UNKNOWN,
    GUARDIAN_NODE_LINK_AUTH_ERROR_KEY_UNKNOWN,
    GUARDIAN_NODE_LINK_AUTH_ERROR_KEY_REVOKED,
    GUARDIAN_NODE_LINK_AUTH_ERROR_ALGORITHM_UNSUPPORTED,
    GUARDIAN_NODE_LINK_AUTH_ERROR_TRANSCRIPT,
    GUARDIAN_NODE_LINK_AUTH_ERROR_SIGNATURE_INVALID,
    GUARDIAN_NODE_LINK_AUTH_ERROR_PROVIDER_UNAVAILABLE,
    GUARDIAN_NODE_LINK_AUTH_ERROR_PROVIDER_FAILURE
} guardian_node_link_auth_result_t;

typedef struct
{
    uint32_t sender_node_id;
    uint32_t producer_id;
    uint32_t key_id;
    uint8_t signature_algorithm;
} guardian_node_link_peer_identity_t;

typedef struct
{
    guardian_node_link_frame_t frame;
    uint32_t producer_id;
    uint32_t key_id;
    uint8_t signature_algorithm;
} guardian_node_link_authenticated_message_t;

typedef guardian_node_link_verify_result_t
(*guardian_node_link_verify_signature_fn)(
    void *context,
    uint8_t signature_algorithm,
    uint32_t producer_id,
    uint32_t key_id,
    const uint8_t *transcript,
    size_t transcript_length,
    const uint8_t *signature,
    size_t signature_length);

typedef struct
{
    void *context;
    guardian_node_link_verify_signature_fn verify_signature;
    const guardian_node_link_peer_identity_t *peers;
    size_t peer_count;
    uint8_t required_signature_algorithm;
} guardian_node_link_auth_config_t;

typedef struct
{
    void *context;
    guardian_node_link_verify_signature_fn verify_signature;

    guardian_node_link_peer_identity_t
        peers[GUARDIAN_NODE_LINK_AUTH_MAX_PEERS];

    size_t peer_count;
    uint8_t required_signature_algorithm;
    uint8_t configured;
} guardian_node_link_authenticator_t;

void guardian_node_link_authenticator_init(
    guardian_node_link_authenticator_t *authenticator);

guardian_node_link_auth_result_t
guardian_node_link_authenticator_configure(
    guardian_node_link_authenticator_t *authenticator,
    const guardian_node_link_auth_config_t *config);

guardian_node_link_auth_result_t
guardian_node_link_build_auth_transcript(
    const guardian_node_link_frame_t *frame,
    uint32_t producer_id,
    uint32_t key_id,
    uint8_t *output,
    size_t output_capacity,
    size_t *output_length);

guardian_node_link_auth_result_t
guardian_node_link_authenticate(
    const guardian_node_link_authenticator_t *authenticator,
    const guardian_node_link_frame_t *frame,
    const uint8_t *signature,
    size_t signature_length,
    guardian_node_link_authenticated_message_t *authenticated_message);

#endif