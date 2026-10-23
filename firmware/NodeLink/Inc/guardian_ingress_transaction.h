#ifndef GUARDIAN_INGRESS_TRANSACTION_H
#define GUARDIAN_INGRESS_TRANSACTION_H
#include "guardian_signed_envelope_auth.h"
#define GI_RECORD_SIZE ((size_t)256U)
#define GI_DIGEST_SIZE ((size_t)32U)
/* Fixed single-peer, single-round REFERENCE profile. No actuator API.
 * One trusted serialized owner; no concurrent registry/provider changes.
 * Device providers must independently qualify durability, antirollback, atomic
 * anchor compare-and-advance, capacity, reset/power-loss and clock properties. */
typedef struct {
    uint32_t sender,producer,key,epoch,registry_revision;
    uint32_t generation,policy,profile,round;
    uint8_t expected_state,measurement_digest[32],public_key[32];
} gi_config;
typedef struct {
    uint32_t revision,obs_sequence,ack_sequence,local_epoch,local_sequence;
    uint32_t deadline,last_time;
    uint8_t has_report,pending,eligible;
} gi_state;
typedef struct {
    void *user;
    int (*read_anchor)(void *,uint32_t *,uint8_t[32]);
    int (*read_record)(void *,uint32_t,uint8_t[GI_RECORD_SIZE]);
    int (*stage)(void *,uint32_t,const uint8_t[GI_RECORD_SIZE]);
    int (*advance)(void *,uint32_t,const uint8_t[32],uint32_t,const uint8_t[32]);
} gi_provider;
typedef struct {
    gi_config config;
    gi_state live,proposed;
    gi_provider provider;
    uint8_t digest[32],commitment[32],prepared_wire[GSE_MAX_WIRE];
    size_t prepared_length;
    uint32_t clock_floor;
    uint8_t mode,prepared_kind;
} gi_context;
typedef enum {GI_OK=0,GI_INVALID,GI_HOLD,GI_AUTH,GI_SEMANTIC,GI_REPLAY,GI_ACK,
              GI_EXPIRED,GI_STORAGE,GI_CONFLICT} gi_result;
/* init never bootstraps storage; exact provisioned genesis required. */
gi_result gi_init(gi_context *,const gi_config *,const gi_provider *);
gi_result gi_recover(gi_context *,const gsea_context *,uint32_t now);
gi_result gi_prepare(gi_context *,const gsea_context *,const uint8_t *,size_t,uint32_t now);
/* Trusted outbound caller must first establish the referenced local observation
 * was generated/sent for this exact round and commitment. This API does not sign
 * or transmit. Persists the pending reference with the same transaction engine. */
gi_result gi_prepare_pending(gi_context *,const gsea_context *,uint32_t local_epoch,
                              uint32_t local_sequence,uint32_t deadline,uint32_t now);
gi_result gi_commit(gi_context *,const gsea_context *,uint32_t now);
/* Read-only snapshot: eligibility is corroboration evidence, NEVER actuation. */
gi_result gi_snapshot(const gi_context *,const gsea_context *,uint32_t now,gi_state *);
void gi_commitment(const gi_config *,uint8_t[32]);
/* Offline provisioning/reference backend only; record is canonical, not struct.
 * Not an automatic initialization/recovery mechanism. */
gi_result gi_encode_record(const gi_config *,const gi_state *,uint8_t[GI_RECORD_SIZE]);
#endif
