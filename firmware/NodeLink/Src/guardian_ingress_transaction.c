#include "guardian_ingress_transaction.h"
#include "monocypher.h"
#include <string.h>
#define READY ((uint8_t)1U)
#define PREPARED ((uint8_t)2U)
#define HOLD ((uint8_t)3U)
static void put(uint8_t *p,uint32_t v){size_t i;for(i=0U;i<4U;i++){p[i]=(uint8_t)(v&255U);v>>=8U;}}
static uint32_t get(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8U)|((uint32_t)p[2]<<16U)|((uint32_t)p[3]<<24U);}
static int config_ok(const gi_config *c){return c!=NULL && c->sender!=0U && c->producer!=0U && c->key!=0U && c->epoch!=0U && c->epoch!=UINT32_MAX && c->registry_revision!=0U && c->generation!=0U && c->generation!=UINT32_MAX && c->policy!=0U && c->profile!=0U && c->round!=0U && c->expected_state<=5U;}
void gi_commitment(const gi_config *c,uint8_t out[32]){
    static const uint8_t domain[]="GUARDIAN:CORROBORATION:COMMITMENT:V1";
    uint8_t b[sizeof(domain)-1U+49U];size_t n=sizeof(domain)-1U;
    (void)memcpy(b,domain,n);put(b+n,c->generation);put(b+n+4U,c->policy);
    put(b+n+8U,c->profile);put(b+n+12U,c->round);b[n+16U]=c->expected_state;
    (void)memcpy(b+n+17U,c->measurement_digest,32U);crypto_blake2b(out,32U,b,sizeof(b));
}
static int state_ok(const gi_state *s){
    return s->has_report<=1U && s->pending<=1U && s->eligible<=1U &&
      (s->has_report!=0U || (s->obs_sequence==0U && s->eligible==0U)) &&
      (s->has_report==0U || s->obs_sequence!=0U) &&
      (s->pending==0U || (s->eligible==0U && s->local_epoch!=0U && s->local_sequence!=0U && s->deadline>s->last_time)) &&
      (s->eligible==0U || (s->has_report==1U && s->pending==0U && s->ack_sequence!=0U && s->local_epoch!=0U && s->local_sequence!=0U && s->deadline>s->last_time));
}
gi_result gi_encode_record(const gi_config *c,const gi_state *s,uint8_t b[GI_RECORD_SIZE]){
    uint8_t commitment[32];uint32_t fields[18];size_t i;
    if(!config_ok(c)||s==NULL||b==NULL||!state_ok(s)){return GI_INVALID;}
    fields[0]=s->revision;fields[1]=c->sender;fields[2]=c->producer;fields[3]=c->key;
    fields[4]=c->epoch;fields[5]=c->registry_revision;fields[6]=c->generation;
    fields[7]=c->policy;fields[8]=c->profile;fields[9]=c->round;
    fields[10]=s->obs_sequence;fields[11]=s->ack_sequence;fields[12]=s->local_epoch;
    fields[13]=s->local_sequence;fields[14]=s->deadline;fields[15]=s->last_time;
    fields[16]=0U;fields[17]=0U;
    (void)memset(b,0,GI_RECORD_SIZE);b[0]=0x47U;b[1]=0x49U;b[2]=1U;
    b[4]=s->has_report;b[5]=s->pending;b[6]=s->eligible;b[7]=c->expected_state;
    for(i=0U;i<18U;i++){put(b+8U+4U*i,fields[i]);}
    gi_commitment(c,commitment);(void)memcpy(b+80U,commitment,32U);
    (void)memcpy(b+112U,c->measurement_digest,32U);
    (void)memcpy(b+144U,c->public_key,32U);
    crypto_blake2b(b+224U,32U,b,224U);return GI_OK;
}
static int decode(const gi_context *x,const uint8_t b[GI_RECORD_SIZE],gi_state *s){
    uint8_t exact[GI_RECORD_SIZE];gi_state t;
    (void)memset(&t,0,sizeof(t));t.revision=get(b+8U);t.has_report=b[4];t.pending=b[5];t.eligible=b[6];
    t.obs_sequence=get(b+48U);t.ack_sequence=get(b+52U);t.local_epoch=get(b+56U);
    t.local_sequence=get(b+60U);t.deadline=get(b+64U);t.last_time=get(b+68U);
    if(gi_encode_record(&x->config,&t,exact)!=GI_OK || memcmp(exact,b,GI_RECORD_SIZE)!=0){return 0;}
    *s=t;return 1;
}
static int registry_ok(const gi_context *x,const gsea_context *a){
    size_t i;int found=0;
    if(a==NULL||a->configured!=1U||a->count==0U||a->count>GSEA_MAX_PEERS||a->registry_revision!=x->config.registry_revision){return 0;}
    for(i=0U;i<a->count;i++){
        const gsea_peer *p=&a->peers[i];
        if(p->sender==x->config.sender && p->producer==x->config.producer && p->key==x->config.key){
            if(p->revoked!=0U||p->allowed_types!=GSEA_BOTH||memcmp(p->public_key,x->config.public_key,32U)!=0){return 0;}found++;
        }
    }
    return found==1;
}
gi_result gi_init(gi_context *x,const gi_config *c,const gi_provider *p){
    if(x==NULL){return GI_INVALID;}(void)memset(x,0,sizeof(*x));x->mode=HOLD;
    if(!config_ok(c)||p==NULL||p->read_anchor==NULL||p->read_record==NULL||p->stage==NULL||p->advance==NULL){return GI_INVALID;}
    x->config=*c;x->provider=*p;gi_commitment(c,x->commitment);return GI_OK;
}
gi_result gi_recover(gi_context *x,const gsea_context *a,uint32_t now){
    uint32_t revision;uint8_t hash[32],b[GI_RECORD_SIZE];gi_state s;
    if(x==NULL){return GI_INVALID;}x->mode=HOLD;x->prepared_length=0U;
    if(!registry_ok(x,a)||x->provider.read_anchor==NULL||x->provider.read_record==NULL){return GI_HOLD;}
    if(x->provider.read_anchor(x->provider.user,&revision,hash)!=0 ||
       x->provider.read_record(x->provider.user,revision,b)!=0 || !decode(x,b,&s) ||
       s.revision!=revision || memcmp(hash,b+224U,32U)!=0 || now<s.last_time){return GI_HOLD;}
    x->live=s;x->clock_floor=now;(void)memcpy(x->digest,hash,32U);x->mode=READY;return GI_OK;
}
static gi_result begin(gi_context *x,const gsea_context *a,uint32_t now){
    if(x==NULL){return GI_INVALID;}
    if(x->mode!=READY){return GI_HOLD;}
    if(!registry_ok(x,a)||now<x->live.last_time||now<x->clock_floor){x->mode=HOLD;return GI_HOLD;}
    x->clock_floor=now;
    if(x->live.revision==UINT32_MAX){x->mode=HOLD;return GI_HOLD;}
    return GI_OK;
}
static int semantics(const gi_context *x,const gse_unverified *e){
    const uint8_t *p=e->payload;size_t offset=e->type==7U?20U:28U;
    return e->sender==x->config.sender && e->producer==x->config.producer &&
      e->key==x->config.key && e->sequence!=UINT32_MAX && e->epoch==x->config.epoch && e->state==x->config.expected_state &&
      p[0]==1U && p[1]==(e->type==7U?0U:0xa1U) && p[2]==0U && p[3]==0U &&
      get(p+4U)==x->config.generation && get(p+8U)==x->config.policy &&
      get(p+12U)==x->config.profile && get(p+16U)==x->config.round &&
      memcmp(p+offset,x->commitment,32U)==0;
}
gi_result gi_prepare(gi_context *x,const gsea_context *a,const uint8_t *wire,size_t length,uint32_t now){
    gi_state next;gsea_signature_evidence evidence;const gse_unverified *e;gi_result r=begin(x,a,now);
    if(r!=GI_OK){return r;}
    if(gsea_verify(a,wire,length,&evidence)!=GSEA_OK){return GI_AUTH;}e=&evidence.envelope;
    if(!semantics(x,e)){return GI_SEMANTIC;}next=x->live;
    if(e->type==7U){
        if(e->sequence<=x->live.obs_sequence){return GI_REPLAY;}
        next.obs_sequence=e->sequence;next.has_report=1U;next.eligible=0U;
    }else{
        if(e->sequence<=x->live.ack_sequence){return GI_REPLAY;}
        if(x->live.pending!=1U || x->live.has_report!=1U ||
           get(e->payload+20U)!=x->live.local_epoch || get(e->payload+24U)!=x->live.local_sequence){return GI_ACK;}
        if(now>=x->live.deadline){return GI_EXPIRED;}
        next.ack_sequence=e->sequence;next.pending=0U;next.eligible=1U;
    }
    next.last_time=now;next.revision++;x->proposed=next;x->prepared_kind=e->type;
    (void)memcpy(x->prepared_wire,wire,length);x->prepared_length=length;x->mode=PREPARED;return GI_OK;
}
gi_result gi_prepare_pending(gi_context *x,const gsea_context *a,uint32_t epoch,uint32_t sequence,uint32_t deadline,uint32_t now){
    gi_state next;gi_result r=begin(x,a,now);if(r!=GI_OK){return r;}
    if(epoch==0U||sequence==0U||epoch==UINT32_MAX||sequence==UINT32_MAX||deadline<=now||x->live.pending!=0U||x->live.eligible!=0U ||
       epoch<x->live.local_epoch || (epoch==x->live.local_epoch && sequence<=x->live.local_sequence)){return GI_INVALID;}
    next=x->live;next.local_epoch=epoch;next.local_sequence=sequence;next.pending=1U;
    next.deadline=deadline;next.last_time=now;next.revision++;x->proposed=next;
    x->prepared_length=0U;x->prepared_kind=0U;x->mode=PREPARED;return GI_OK;
}
gi_result gi_commit(gi_context *x,const gsea_context *a,uint32_t now){
    uint8_t b[GI_RECORD_SIZE],readback[GI_RECORD_SIZE],hash[32];uint32_t revision;
    gsea_signature_evidence evidence;gi_state proposed;
    if(x==NULL){return GI_INVALID;}if(x->mode!=PREPARED){return GI_HOLD;}
    /* Every uncertain post-prepare state latches HOLD; never publish RAM first. */
    x->mode=HOLD;proposed=x->proposed;
    if(!registry_ok(x,a)||now<proposed.last_time||now<x->clock_floor){return GI_CONFLICT;}
    if((proposed.pending!=0U||proposed.eligible!=0U)&&now>=proposed.deadline){return GI_EXPIRED;}
    if(x->prepared_kind!=0U && (gsea_verify(a,x->prepared_wire,x->prepared_length,&evidence)!=GSEA_OK || !semantics(x,&evidence.envelope))){return GI_AUTH;}
    x->clock_floor=now;proposed.last_time=now;
    if(gi_encode_record(&x->config,&proposed,b)!=GI_OK){return GI_INVALID;}
    if(x->provider.stage(x->provider.user,proposed.revision,b)!=0){return GI_STORAGE;}
    if(x->provider.advance(x->provider.user,x->live.revision,x->digest,proposed.revision,b+224U)!=0){return GI_STORAGE;}
    if(x->provider.read_anchor(x->provider.user,&revision,hash)!=0 || revision!=proposed.revision || memcmp(hash,b+224U,32U)!=0 ||
       x->provider.read_record(x->provider.user,revision,readback)!=0 || memcmp(readback,b,GI_RECORD_SIZE)!=0){return GI_STORAGE;}
    x->live=proposed;(void)memcpy(x->digest,hash,32U);x->mode=READY;x->prepared_length=0U;return GI_OK;
}
gi_result gi_snapshot(const gi_context *x,const gsea_context *a,uint32_t now,gi_state *out){
    if(x==NULL||out==NULL){return GI_INVALID;}
    if(!registry_ok(x,a)||x->mode!=READY||now<x->live.last_time||now<x->clock_floor || (x->live.eligible!=0U&&now>=x->live.deadline)){return GI_HOLD;}
    *out=x->live;return GI_OK;
}
