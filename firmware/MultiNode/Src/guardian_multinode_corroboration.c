#include "guardian_multinode_corroboration.h"
#include <stddef.h>
#include <string.h>
static int peer_index(const gc_context *c, uint32_t id) {
    uint8_t i;
    for (i=0; i<c->config.count; ++i) if (c->config.peers[i].id==id) return (int)i;
    return -1;
}
void gc_init(gc_context *c) { if (c!=NULL) { memset(c,0,sizeof(*c)); c->state=GC_UNCONFIGURED; } }
gc_result gc_configure(gc_context *c, const gc_config *p) {
    uint8_t i,j,roles=0;
    if (c==NULL || p==NULL || c->state!=GC_UNCONFIGURED) return GC_REJECTED;
    if (p->generation==0 || p->policy==0 || p->profile==0 || p->round==0 || p->max_age==0 ||
        p->count==0 || p->count>GUARDIAN_CORROBORATION_CAPACITY || p->threshold==0 ||
        p->threshold>p->count || p->required_roles==0 || (p->required_roles & 0xe0u)!=0) return GC_REJECTED;
    for (i=0;i<p->count;++i) {
        if (p->peers[i].id==0 || p->peers[i].role>=5u) return GC_REJECTED;
        roles=(uint8_t)(roles | (uint8_t)(1u<<p->peers[i].role));
        for (j=0;j<i;++j) if (p->peers[j].id==p->peers[i].id) return GC_REJECTED;
    }
    if ((roles & p->required_roles)!=p->required_roles) return GC_REJECTED;
    c->config=*p; c->state=GC_INSUFFICIENT; return GC_OK;
}
void gc_require_rejoin(gc_context *c) {
    if (c!=NULL && c->state!=GC_UNCONFIGURED && c->state!=GC_CONFLICT_LATCHED) c->state=GC_REJOIN_REQUIRED;
}
static gc_result clock_check(gc_context *c,uint64_t now,uint8_t valid) {
    if (valid!=1u || (c->clock_seen!=0u && now<c->last_clock)) {
        gc_require_rejoin(c); return GC_CLOCK_FAILURE;
    }
    c->last_clock=now; c->clock_seen=1; return GC_OK;
}
static void evaluate(gc_context *c,uint64_t now,uint8_t *count,uint8_t *roles) {
    uint8_t i; *count=0; *roles=0;
    for (i=0;i<c->config.count;++i) {
        const gc_slot *s=&c->slots[i];
        if (s->seen!=0u && s->available!=0u && s->report.same_round_ack==1u &&
            now>=s->received_at && now-s->received_at<=c->config.max_age) {
            ++(*count); *roles=(uint8_t)(*roles | (uint8_t)(1u<<c->config.peers[i].role));
        }
    }
    if (c->state==GC_INSUFFICIENT || c->state==GC_CORROBORATED) {
        c->state=(*count>=c->config.threshold && (*roles & c->config.required_roles)==c->config.required_roles)
            ? GC_CORROBORATED : GC_INSUFFICIENT;
    }
}
gc_result gc_submit(gc_context *c,const gc_report *r,uint64_t now,uint8_t valid) {
    int index; uint8_t i,count,roles; gc_slot *s;
    if (c==NULL || r==NULL || c->state==GC_UNCONFIGURED) return GC_REJECTED;
    index=peer_index(c,r->id);
    if (index<0 || r->generation!=c->config.generation || r->policy!=c->config.policy ||
        r->profile!=c->config.profile || r->round!=c->config.round || r->epoch==0 || r->sequence==0 ||
        r->authenticated!=1u || r->persistent_freshness!=1u || r->same_round_ack>1u || r->claims_actuation>1u) return GC_REJECTED;
    s=&c->slots[(unsigned int)index];
    if (s->seen!=0u && r->epoch==s->report.epoch && r->sequence<=s->report.sequence) return GC_REJECTED;
    if (clock_check(c,now,valid)!=GC_OK) return GC_CLOCK_FAILURE;
    if (c->state==GC_CONFLICT_LATCHED || c->state==GC_REJOIN_REQUIRED) return GC_LATCHED;
    if (s->seen!=0u && r->epoch!=s->report.epoch) { gc_require_rejoin(c); return GC_EPOCH_CHANGE; }
    if (r->claims_actuation==1u && c->config.peers[(unsigned int)index].role!=0u) {
        c->state=GC_CONFLICT_LATCHED; c->conflicting_report=*r; c->conflict_present=1; return GC_CONFLICT;
    }
    for (i=0;i<c->config.count;++i) {
        if (c->slots[i].seen!=0u && memcmp(r->commitment,c->slots[i].report.commitment,GUARDIAN_CORROBORATION_COMMITMENT_BYTES)!=0) {
            c->state=GC_CONFLICT_LATCHED; c->conflicting_report=*r; c->conflict_present=1; return GC_CONFLICT;
        }
    }
    s->report=*r; s->received_at=now; s->seen=1; s->available=1;
    evaluate(c,now,&count,&roles); return GC_OK;
}
gc_result gc_query(gc_context *c,uint64_t now,uint8_t valid,gc_assessment *out) {
    gc_result result; uint8_t count=0,roles=0;
    if (out==NULL) return GC_REJECTED;
    memset(out,0,sizeof(*out));
    if (c==NULL || c->state==GC_UNCONFIGURED) return GC_REJECTED;
    result=clock_check(c,now,valid); evaluate(c,now,&count,&roles);
    out->state=c->state; out->fresh_count=count; out->role_coverage=roles;
    out->eligible_for_local_review=(uint8_t)(result==GC_OK && c->state==GC_CORROBORATED);
    out->config=c->config; memcpy(out->sources,c->slots,sizeof(out->sources));
    out->conflict_present=c->conflict_present; out->conflicting_report=c->conflicting_report;
    return result;
}
gc_result gc_link_loss(gc_context *c,uint32_t id,uint64_t now,uint8_t valid) {
    int i; uint8_t count,roles;
    if (c==NULL || c->state==GC_UNCONFIGURED) return GC_REJECTED;
    i=peer_index(c,id); if (i<0) return GC_REJECTED;
    if (clock_check(c,now,valid)!=GC_OK) return GC_CLOCK_FAILURE;
    c->slots[(unsigned int)i].available=0; evaluate(c,now,&count,&roles); return GC_OK;
}
