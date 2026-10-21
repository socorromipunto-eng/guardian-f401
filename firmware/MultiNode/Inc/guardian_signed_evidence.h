#ifndef GUARDIAN_SIGNED_EVIDENCE_H
#define GUARDIAN_SIGNED_EVIDENCE_H

#include "guardian_crypto.h"
#include "guardian_multinode.h"
#include "guardian_security_witness.h"

#include <stddef.h>
#include <stdint.h>

#define GUARDIAN_SIGNED_EVIDENCE_SCHEMA_VERSION ((uint8_t)1U)
#define GUARDIAN_SIGNED_EVIDENCE_AUTHORITY_NONE ((uint8_t)0U)
#define GUARDIAN_SIGNED_EVIDENCE_MAX_TRACKED_PRODUCERS ((size_t)5U)
#define GUARDIAN_SIGNED_EVIDENCE_TRANSCRIPT_MAX_SIZE ((size_t)192U)
#define GUARDIAN_SIGNED_EVIDENCE_DIGEST_INPUT_MAX_SIZE ((size_t)320U)
#define GUARDIAN_SIGNED_EVIDENCE_DOMAIN "GUARDIAN-F401:M17:SIGNED-EVIDENCE:V1"
#define GUARDIAN_SIGNED_EVIDENCE_DIGEST_DOMAIN "GUARDIAN-F401:M17:SIGNED-EVIDENCE-DIGEST:V1"

typedef enum
{
    GUARDIAN_SIGNED_EVIDENCE_CLASS_INVALID = 0,
    GUARDIAN_SIGNED_EVIDENCE_CLASS_MEASUREMENT,
    GUARDIAN_SIGNED_EVIDENCE_CLASS_ANALYSIS,
    GUARDIAN_SIGNED_EVIDENCE_CLASS_FUSION,
    GUARDIAN_SIGNED_EVIDENCE_CLASS_SUPERVISION,
    GUARDIAN_SIGNED_EVIDENCE_CLASS_RECOVERY
} guardian_signed_evidence_class_t;

typedef enum
{
    GUARDIAN_SIGNED_EVIDENCE_TRUST_UNKNOWN = 0,
    GUARDIAN_SIGNED_EVIDENCE_TRUST_TRUSTED,
    GUARDIAN_SIGNED_EVIDENCE_TRUST_UNTRUSTED,
    GUARDIAN_SIGNED_EVIDENCE_TRUST_REVOKED
} guardian_signed_evidence_trust_t;

typedef enum
{
    GUARDIAN_SIGNED_EVIDENCE_OK = 0,
    GUARDIAN_SIGNED_EVIDENCE_ERROR_NULL_ARGUMENT,
    GUARDIAN_SIGNED_EVIDENCE_ERROR_INVALID_CONFIG,
    GUARDIAN_SIGNED_EVIDENCE_ERROR_INVALID_PRODUCER,
    GUARDIAN_SIGNED_EVIDENCE_ERROR_INVALID_ENVELOPE,
    GUARDIAN_SIGNED_EVIDENCE_ERROR_TRANSCRIPT,
    GUARDIAN_SIGNED_EVIDENCE_ERROR_SIGNATURE_INVALID,
    GUARDIAN_SIGNED_EVIDENCE_ERROR_KEY_UNKNOWN,
    GUARDIAN_SIGNED_EVIDENCE_ERROR_KEY_REVOKED,
    GUARDIAN_SIGNED_EVIDENCE_ERROR_PROVIDER_UNAVAILABLE,
    GUARDIAN_SIGNED_EVIDENCE_ERROR_PROVIDER_FAILURE,
    GUARDIAN_SIGNED_EVIDENCE_ERROR_UNTRUSTED,
    GUARDIAN_SIGNED_EVIDENCE_ERROR_UNAUTHORIZED_ROLE,
    GUARDIAN_SIGNED_EVIDENCE_ERROR_CHAIN_MISMATCH,
    GUARDIAN_SIGNED_EVIDENCE_ERROR_REPLAY_OR_REGRESSION,
    GUARDIAN_SIGNED_EVIDENCE_ERROR_TRACKING_TABLE_FULL,
    GUARDIAN_SIGNED_EVIDENCE_ERROR_ACTUATION_PROHIBITED
} guardian_signed_evidence_result_t;

typedef struct
{
    uint8_t schema_version;
    guardian_signed_evidence_class_t evidence_class;
    guardian_multinode_role_t producer_role;
    uint8_t actuation_authorized;
    uint32_t producer_node_id;
    uint32_t producer_id;
    uint32_t producer_key_id;
    uint32_t epoch;
    uint32_t sequence;
    uint8_t payload_digest[GUARDIAN_SHA256_SIZE];
    uint8_t analysis_digest[GUARDIAN_SHA256_SIZE];
    uint8_t previous_evidence_digest[GUARDIAN_SHA256_SIZE];
} guardian_signed_evidence_core_t;

typedef struct
{
    guardian_signed_evidence_core_t core;
    guardian_security_witness_record_t witness_record;
    uint8_t evidence_digest[GUARDIAN_SHA256_SIZE];
} guardian_signed_evidence_envelope_t;

typedef struct
{
    uint8_t signature_valid;
    uint8_t trusted;
    uint8_t authorized;
    uint8_t chain_valid;
    uint8_t fresh;
    uint8_t accepted;
    uint8_t actuation_authorized;
} guardian_signed_evidence_verdict_t;

typedef int (*guardian_signed_evidence_lookup_fn)(
    void *context,
    uint32_t node_id,
    uint32_t producer_id,
    uint32_t key_id,
    guardian_multinode_node_t *resolved_node,
    guardian_signed_evidence_trust_t *trust);

typedef struct
{
    guardian_signed_evidence_lookup_fn lookup;
    void *lookup_context;
} guardian_signed_evidence_config_t;

typedef struct
{
    uint32_t producer_node_id;
    uint32_t producer_id;
    uint32_t producer_key_id;
    guardian_multinode_role_t producer_role;
    uint32_t accepted_epoch;
    uint32_t accepted_sequence;
    uint8_t last_evidence_digest[GUARDIAN_SHA256_SIZE];
    uint8_t active;
} guardian_signed_evidence_producer_state_t;

typedef struct
{
    guardian_security_witness_node_t *witness;
    guardian_signed_evidence_config_t config;
    guardian_signed_evidence_producer_state_t producers[
        GUARDIAN_SIGNED_EVIDENCE_MAX_TRACKED_PRODUCERS];
    size_t producer_count;
    uint8_t initialized;
} guardian_signed_evidence_t;

void guardian_signed_evidence_init(
    guardian_signed_evidence_t *context);

guardian_signed_evidence_result_t guardian_signed_evidence_configure(
    guardian_signed_evidence_t *context,
    guardian_security_witness_node_t *witness,
    const guardian_signed_evidence_config_t *config);

guardian_signed_evidence_result_t guardian_signed_evidence_build_transcript(
    const guardian_signed_evidence_core_t *core,
    uint8_t *output,
    size_t output_capacity,
    size_t *output_length);

guardian_signed_evidence_result_t guardian_signed_evidence_create(
    guardian_signed_evidence_t *context,
    const guardian_multinode_node_t *producer,
    guardian_signed_evidence_class_t evidence_class,
    uint32_t epoch,
    uint32_t sequence,
    const uint8_t payload_digest[GUARDIAN_SHA256_SIZE],
    const uint8_t analysis_digest[GUARDIAN_SHA256_SIZE],
    const uint8_t previous_evidence_digest[GUARDIAN_SHA256_SIZE],
    guardian_signed_evidence_envelope_t *envelope);

guardian_signed_evidence_result_t guardian_signed_evidence_verify_and_accept(
    guardian_signed_evidence_t *context,
    const guardian_signed_evidence_envelope_t *envelope,
    guardian_signed_evidence_verdict_t *verdict);

guardian_signed_evidence_result_t guardian_signed_evidence_can_actuate(
    const guardian_signed_evidence_t *context,
    uint8_t *allowed);

#endif
