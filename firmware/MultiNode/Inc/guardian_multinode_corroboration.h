#ifndef GUARDIAN_MULTINODE_CORROBORATION_H
#define GUARDIAN_MULTINODE_CORROBORATION_H
#include <stdint.h>
#define GUARDIAN_CORROBORATION_CAPACITY 5u
#define GUARDIAN_CORROBORATION_COMMITMENT_BYTES 32u
typedef enum { GC_UNCONFIGURED=0, GC_INSUFFICIENT, GC_CORROBORATED,
               GC_CONFLICT_LATCHED, GC_REJOIN_REQUIRED } gc_state;
typedef enum { GC_OK=0, GC_REJECTED, GC_CLOCK_FAILURE, GC_EPOCH_CHANGE,
               GC_CONFLICT, GC_LATCHED } gc_result;
typedef struct { uint32_t id; uint8_t role; } gc_peer;
typedef struct {
    uint32_t generation, policy, profile, round;
    uint64_t max_age;
    uint8_t count, threshold, required_roles;
    gc_peer peers[GUARDIAN_CORROBORATION_CAPACITY];
} gc_config;
typedef struct {
    uint32_t id, generation, policy, profile, round;
    uint64_t epoch, sequence;
    uint8_t authenticated, persistent_freshness, same_round_ack, claims_actuation;
    uint8_t commitment[GUARDIAN_CORROBORATION_COMMITMENT_BYTES];
} gc_report;
typedef struct {
    gc_report report;
    uint64_t received_at;
    uint8_t seen, available;
} gc_slot;
typedef struct {
    gc_state state;
    gc_config config;
    gc_slot slots[GUARDIAN_CORROBORATION_CAPACITY];
    uint64_t last_clock;
    uint8_t clock_seen, conflict_present;
    gc_report conflicting_report;
} gc_context;
typedef struct {
    gc_state state;
    uint8_t eligible_for_local_review, fresh_count, role_coverage;
    gc_config config;
    gc_slot sources[GUARDIAN_CORROBORATION_CAPACITY];
    uint8_t conflict_present;
    gc_report conflicting_report;
} gc_assessment;
void gc_init(gc_context *ctx);
gc_result gc_configure(gc_context *ctx, const gc_config *config);
gc_result gc_submit(gc_context *ctx, const gc_report *report, uint64_t now, uint8_t clock_valid);
gc_result gc_query(gc_context *ctx, uint64_t now, uint8_t clock_valid, gc_assessment *out);
gc_result gc_link_loss(gc_context *ctx, uint32_t id, uint64_t now, uint8_t clock_valid);
void gc_require_rejoin(gc_context *ctx);
#endif
