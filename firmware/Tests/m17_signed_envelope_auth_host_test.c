#include "guardian_signed_envelope_auth.h"
#include "m17_signed_envelope_auth_vectors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static size_t checks=0U,bit_rejections=0U,length_rejections=0U;
#define CHECK(x) do {checks++;if(!(x)){(void)fprintf(stderr,"FAILED_LINE=%d\n",__LINE__);exit(1);}} while(0)
static gsea_peer peer(void) {
    gsea_peer p;(void)memset(&p,0,sizeof(p));p.sender=11U;p.producer=22U;p.key=33U;
    p.allowed_types=GSEA_BOTH;(void)memcpy(p.public_key,test_public_key,sizeof(p.public_key));return p;
}
static void rejected(const gsea_context *ctx,const uint8_t *wire,size_t length) {
    gsea_signature_evidence out,before;gsea_context old;
    (void)memset(&out,0xa5,sizeof(out));(void)memcpy(&before,&out,sizeof(out));
    (void)memcpy(&old,ctx,sizeof(old));
    CHECK(gsea_verify(ctx,wire,length,&out)!=GSEA_OK);
    CHECK(memcmp(&out,&before,sizeof(out))==0);
    CHECK(memcmp(ctx,&old,sizeof(old))==0);
}
static void packet_cases(gsea_context *ctx,const uint8_t *wire,size_t length) {
    uint8_t changed[GSE_MAX_WIRE+2U];size_t i,bit,n;gsea_signature_evidence out;
    CHECK(gsea_verify(ctx,wire,length,&out)==GSEA_OK);
    CHECK(out.registry_revision==7U && out.envelope.sender==11U && out.envelope.producer==22U && out.envelope.key==33U);
    for(i=0U;i<length;i++){
        for(bit=0U;bit<8U;bit++){
            (void)memcpy(changed,wire,length);changed[i]=(uint8_t)(changed[i]^(uint8_t)(1U<<bit));
            rejected(ctx,changed,length);bit_rejections++;
        }
    }
    (void)memset(changed,0,sizeof(changed));(void)memcpy(changed,wire,length);
    for(n=0U;n<=GSE_MAX_WIRE+1U;n++){
        if(n!=length){rejected(ctx,changed,n);length_rejections++;}
    }
}
int main(void) {
    gsea_context ctx,old;gsea_peer peers[GSEA_MAX_PEERS];gsea_peer p=peer();
    gsea_signature_evidence out,before;size_t i;
    gsea_init(NULL);gsea_init(&ctx);rejected(&ctx,observation_wire,sizeof(observation_wire));
    CHECK(gsea_configure(NULL,&p,1U,7U)==GSEA_INVALID);
    CHECK(gsea_configure(&ctx,&p,1U,7U)==GSEA_OK);
    packet_cases(&ctx,observation_wire,sizeof(observation_wire));
    packet_cases(&ctx,ack_wire,sizeof(ack_wire));
    rejected(&ctx,wrong_domain_wire,sizeof(wrong_domain_wire));
    (void)memset(&out,0x5a,sizeof(out));(void)memcpy(&before,&out,sizeof(out));
    CHECK(gsea_verify(NULL,observation_wire,sizeof(observation_wire),&out)==GSEA_INVALID);
    CHECK(memcmp(&out,&before,sizeof(out))==0);
    rejected(&ctx,NULL,0U);
    CHECK(gsea_verify(&ctx,observation_wire,sizeof(observation_wire),NULL)==GSEA_INVALID);
    (void)memcpy(&old,&ctx,sizeof(old));
    for(i=0U;i<4U;i++){
        gsea_peer q=peer();
        if(i==0U){q.sender++;}else if(i==1U){q.producer++;}else if(i==2U){q.key++;}else{q.public_key[0]^=1U;}
        CHECK(gsea_configure(&ctx,&q,1U,7U)==GSEA_OK);rejected(&ctx,observation_wire,sizeof(observation_wire));
    }
    p=peer();p.revoked=1U;CHECK(gsea_configure(&ctx,&p,1U,7U)==GSEA_OK);
    CHECK(gsea_verify(&ctx,observation_wire,sizeof(observation_wire),&out)==GSEA_KEY_REVOKED);
    rejected(&ctx,ack_wire,sizeof(ack_wire));
    p=peer();p.allowed_types=GSEA_OBSERVATION;CHECK(gsea_configure(&ctx,&p,1U,7U)==GSEA_OK);
    CHECK(gsea_verify(&ctx,observation_wire,sizeof(observation_wire),&out)==GSEA_OK);rejected(&ctx,ack_wire,sizeof(ack_wire));
    p.allowed_types=GSEA_ACK;CHECK(gsea_configure(&ctx,&p,1U,7U)==GSEA_OK);
    CHECK(gsea_verify(&ctx,ack_wire,sizeof(ack_wire),&out)==GSEA_OK);rejected(&ctx,observation_wire,sizeof(observation_wire));
    for(i=0U;i<GSEA_MAX_PEERS;i++){peers[i]=peer();peers[i].sender=(uint32_t)(11U+i);}
    CHECK(gsea_configure(&ctx,peers,GSEA_MAX_PEERS,7U)==GSEA_OK);
    CHECK(gsea_verify(&ctx,observation_wire,sizeof(observation_wire),&out)==GSEA_OK);
    peers[1]=peers[0];CHECK(gsea_configure(&ctx,peers,2U,7U)==GSEA_INVALID);rejected(&ctx,observation_wire,sizeof(observation_wire));
    for(i=0U;i<9U;i++){
        p=peer();CHECK(gsea_configure(&ctx,&p,1U,7U)==GSEA_OK);
        if(i==0U){CHECK(gsea_configure(&ctx,NULL,1U,7U)==GSEA_INVALID);}
        else if(i==1U){CHECK(gsea_configure(&ctx,&p,0U,7U)==GSEA_INVALID);}
        else if(i==2U){CHECK(gsea_configure(&ctx,&p,GSEA_MAX_PEERS+1U,7U)==GSEA_INVALID);}
        else if(i==3U){CHECK(gsea_configure(&ctx,&p,1U,0U)==GSEA_INVALID);}
        else {
            if(i==4U){p.sender=0U;}else if(i==5U){p.producer=0U;}else if(i==6U){p.key=0U;}
            else if(i==7U){p.revoked=2U;}else{(void)memset(p.public_key,0,sizeof(p.public_key));}
            CHECK(gsea_configure(&ctx,&p,1U,7U)==GSEA_INVALID);
        }
        CHECK(ctx.configured==0U);rejected(&ctx,observation_wire,sizeof(observation_wire));
    }
    for(i=0U;i<2U;i++){
        p=peer();p.allowed_types=i==0U ? 0U : 4U;
        CHECK(gsea_configure(&ctx,&p,1U,7U)==GSEA_INVALID);rejected(&ctx,observation_wire,sizeof(observation_wire));
    }
    (void)memcpy(&ctx,&old,sizeof(ctx));ctx.count=GSEA_MAX_PEERS+1U;rejected(&ctx,observation_wire,sizeof(observation_wire));
    (void)memcpy(&ctx,&old,sizeof(ctx));ctx.configured=2U;rejected(&ctx,observation_wire,sizeof(observation_wire));
    (void)memcpy(&ctx,&old,sizeof(ctx));ctx.registry_revision=0U;rejected(&ctx,observation_wire,sizeof(observation_wire));
    (void)memcpy(&ctx,&old,sizeof(ctx));ctx.peers[1]=ctx.peers[0];ctx.count=2U;rejected(&ctx,observation_wire,sizeof(observation_wire));
    (void)memcpy(&ctx,&old,sizeof(ctx));
    {
        union {uint8_t wire[GSE_MAX_WIRE+2U];gsea_signature_evidence evidence;} overlap;
        (void)memcpy(overlap.wire,observation_wire,sizeof(observation_wire));
        CHECK(gsea_verify(&ctx,overlap.wire,sizeof(observation_wire),&overlap.evidence)==GSEA_OK);
        CHECK(overlap.evidence.registry_revision==7U);
    }
    CHECK(bit_rejections==2432U && length_rejections==322U);
    (void)printf("V2_AUTH_CHECKS=%zu\nBIT_MUTATION_REJECTIONS=%zu\nLENGTH_REJECTIONS=%zu\n",checks,bit_rejections,length_rejections);
    (void)puts("V2_SIGNATURE_AND_LOCAL_IDENTITY=PASS_HOST_ONLY");
    (void)puts("SEMANTICS_FRESHNESS_ACK_DURABILITY_AUTHORITY=NOT_ESTABLISHED");
    return 0;
}
