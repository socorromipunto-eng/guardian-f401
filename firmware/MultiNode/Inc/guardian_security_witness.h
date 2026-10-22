#ifndef GUARDIAN_SECURITY_WITNESS_H
#define GUARDIAN_SECURITY_WITNESS_H

#include "guardian_crypto.h"
#include "guardian_multinode.h"

#include <stddef.h>
#include <stdint.h>

#define GUARDIAN_SECURITY_WITNESS_SCHEMA_VERSION \
    ((uint8_t)1U)

#define GUARDIAN_SECURITY_WITNESS_SIGNATURE_ED25519 \
    ((uint8_t)1U)

#define GUARDIAN_SECURITY_WITNESS_SIGNATURE_SIZE \
    ((size_t)64U)

#define GUARDIAN_SECURITY_WITNESS_MAX_PAYLOAD \
    ((size_t)256U)

#define GUARDIAN_SECURITY_WITNESS_MAX_SUBJECTS \
    ((size_t)5U)

#define GUARDIAN_SECURITY_WITNESS_TRANSCRIPT_MAX_SIZE \
    ((size_t)128U)

#define GUARDIAN_SECURITY_WITNESS_AUTHORITY_NONE \
    ((uint8_t)0U)

#define GUARDIAN_SECURITY_WITNESS_DOMAIN \
    "GUARDIAN-F401:M17:EVIDENCE-WITNESS:V1"

typedef enum
{
    GUARDIAN_SECURITY_WITNESS_EVIDENCE_INVALID = 0,

    GUARDIAN_SECURITY_WITNESS_EVIDENCE_OBSERVATION,

    GUARDIAN_SECURITY_WITNESS_EVIDENCE_SUPERVISION,

    GUARDIAN_SECURITY_WITNESS_EVIDENCE_DISAGREEMENT,

    GUARDIAN_SECURITY_WITNESS_EVIDENCE_RECOVERY_REQUEST,

    GUARDIAN_SECURITY_WITNESS_EVIDENCE_FAULT,

    GUARDIAN_SECURITY_WITNESS_EVIDENCE_EXECUTION_RESULT

} guardian_security_witness_evidence_type_t;

typedef enum
{
    GUARDIAN_SECURITY_WITNESS_PROVIDER_OK = 0,

    GUARDIAN_SECURITY_WITNESS_PROVIDER_SIGNATURE_INVALID,

    GUARDIAN_SECURITY_WITNESS_PROVIDER_KEY_UNKNOWN,

    GUARDIAN_SECURITY_WITNESS_PROVIDER_KEY_REVOKED,

    GUARDIAN_SECURITY_WITNESS_PROVIDER_UNAVAILABLE,

    GUARDIAN_SECURITY_WITNESS_PROVIDER_FAILURE

} guardian_security_witness_provider_result_t;

typedef enum
{
    GUARDIAN_SECURITY_WITNESS_OK = 0,

    GUARDIAN_SECURITY_WITNESS_ERROR_NULL_ARGUMENT,

    GUARDIAN_SECURITY_WITNESS_ERROR_INVALID_NODE,

    GUARDIAN_SECURITY_WITNESS_ERROR_INVALID_CONFIG,

    GUARDIAN_SECURITY_WITNESS_ERROR_INVALID_EVIDENCE,

    GUARDIAN_SECURITY_WITNESS_ERROR_TRANSCRIPT,

    GUARDIAN_SECURITY_WITNESS_ERROR_REPLAY_OR_REGRESSION,

    GUARDIAN_SECURITY_WITNESS_ERROR_SUBJECT_TABLE_FULL,

    GUARDIAN_SECURITY_WITNESS_ERROR_SIGNATURE_INVALID,

    GUARDIAN_SECURITY_WITNESS_ERROR_KEY_UNKNOWN,

    GUARDIAN_SECURITY_WITNESS_ERROR_KEY_REVOKED,

    GUARDIAN_SECURITY_WITNESS_ERROR_PROVIDER_UNAVAILABLE,

    GUARDIAN_SECURITY_WITNESS_ERROR_PROVIDER_FAILURE,

    GUARDIAN_SECURITY_WITNESS_ERROR_ACTUATION_PROHIBITED

} guardian_security_witness_result_t;

typedef guardian_security_witness_provider_result_t
(*guardian_security_witness_sign_fn)(
    void *context,
    uint8_t signature_algorithm,
    uint32_t key_id,
    const uint8_t *message,
    size_t message_length,
    uint8_t *signature,
    size_t signature_capacity);

typedef guardian_security_witness_provider_result_t
(*guardian_security_witness_verify_fn)(
    void *context,
    uint8_t signature_algorithm,
    uint32_t key_id,
    const uint8_t *message,
    size_t message_length,
    const uint8_t *signature,
    size_t signature_length);

typedef struct
{
    void *context;

    guardian_security_witness_sign_fn
        sign;

    guardian_security_witness_verify_fn
        verify;

    uint8_t signature_algorithm;

} guardian_security_witness_provider_t;

typedef struct
{
    uint32_t subject_node_id;

    uint32_t subject_producer_id;

    uint32_t subject_key_id;

    uint32_t epoch;

    uint32_t sequence;

    guardian_security_witness_evidence_type_t
        evidence_type;

    const uint8_t *payload;

    size_t payload_length;

} guardian_security_witness_input_t;

typedef struct
{
    uint8_t schema_version;

    guardian_security_witness_evidence_type_t
        evidence_type;

    uint8_t signature_algorithm;

    uint8_t authority_flags;

    uint32_t witness_node_id;

    uint32_t witness_producer_id;

    uint32_t witness_key_id;

    uint32_t subject_node_id;

    uint32_t subject_producer_id;

    uint32_t subject_key_id;

    uint32_t epoch;

    uint32_t sequence;

    uint32_t payload_length;

    uint8_t payload_digest[
        GUARDIAN_SHA256_SIZE];

    uint8_t signature[
        GUARDIAN_SECURITY_WITNESS_SIGNATURE_SIZE];

} guardian_security_witness_record_t;

typedef struct
{
    uint32_t subject_node_id;

    uint32_t subject_producer_id;

    uint32_t subject_key_id;

    uint32_t accepted_epoch;

    uint32_t accepted_sequence;

    uint8_t active;

} guardian_security_witness_subject_state_t;

typedef struct
{
    guardian_multinode_node_t self;

    guardian_security_witness_provider_t
        provider;

    guardian_security_witness_subject_state_t
        subjects[
            GUARDIAN_SECURITY_WITNESS_MAX_SUBJECTS];

    size_t subject_count;

    uint8_t initialized;

} guardian_security_witness_node_t;

void guardian_security_witness_init(
    guardian_security_witness_node_t *node);

guardian_security_witness_result_t
guardian_security_witness_configure(
    guardian_security_witness_node_t *node,
    const guardian_multinode_node_t *self,
    const guardian_security_witness_provider_t *provider);

guardian_security_witness_result_t
guardian_security_witness_build_transcript(
    const guardian_security_witness_record_t *record,
    uint8_t *output,
    size_t output_capacity,
    size_t *output_length);

guardian_security_witness_result_t
guardian_security_witness_create(
    guardian_security_witness_node_t *node,
    const guardian_security_witness_input_t *input,
    guardian_security_witness_record_t *record);

guardian_security_witness_result_t
guardian_security_witness_verify(
    const guardian_security_witness_node_t *node,
    const guardian_security_witness_record_t *record);

guardian_security_witness_result_t
guardian_security_witness_can_actuate(
    const guardian_security_witness_node_t *node,
    uint8_t *allowed);

#endif
