#include "guardian_ingress_transaction.h"
#include "m17_ingress_transaction_vectors.h"
#include <stdio.h>
#include <string.h>
static unsigned checks=0U,scenarios=0U;
#define CHECK(c) do {checks++;if(!(c)){(void)fprintf(stderr,"FAILED_LINE=%u\n",(unsigned)__LINE__);return 1;}}while(0)
typedef struct {uint8_t slots[2][GI_RECORD_SIZE],hash[32];uint32_t revision;int cut;unsigned writes;} store;
static int anchor(void *v,uint32_t *n,uint8_t h[32]){store *s=v;if(s->cut==6 && s->writes>0U){return -1;}*n=s->revision;(void)memcpy(h,s->hash,32U);return 0;}
static int record(void *v,uint32_t n,uint8_t b[GI_RECORD_SIZE]){store *s=v;if(s->cut==7 && s->writes>0U){return -1;}(void)memcpy(b,s->slots[n%2U],GI_RECORD_SIZE);if(s->cut==8 && s->writes>0U){b[200]^=1U;}return 0;}
static int stage(void *v,uint32_t n,const uint8_t b[GI_RECORD_SIZE]){store *s=v;s->writes++;if(s->cut==1){return -1;}if(s->cut==2){(void)memcpy(s->slots[n%2U],b,100U);return -1;}(void)memcpy(s->slots[n%2U],b,GI_RECORD_SIZE);return s->cut==3?-1:0;}
static int advance(void *v,uint32_t old,const uint8_t before[32],uint32_t next,const uint8_t after[32]){store *s=v;if(s->cut==4){return -1;}if(old!=s->revision || memcmp(before,s->hash,32U)!=0 || next!=old+1U){return -1;}s->revision=next;(void)memcpy(s->hash,after,32U);return s->cut==5?-1:0;}
static gi_config config(void){gi_config c;size_t i;(void)memset(&c,0,sizeof(c));c.sender=11U;c.producer=22U;c.key=33U;c.epoch=44U;c.registry_revision=1U;c.generation=1U;c.policy=2U;c.profile=3U;c.round=4U;c.expected_state=1U;(void)memcpy(c.public_key,test_key,32U);for(i=0U;i<32U;i++){c.measurement_digest[i]=(uint8_t)i;}return c;}
static int setup(gi_context *x,gsea_context *a,store *s){gi_config c=config();gi_state genesis;gi_provider p;gsea_peer peer;
 (void)memset(s,0,sizeof(*s));(void)memset(&genesis,0,sizeof(genesis));(void)memset(&peer,0,sizeof(peer));peer.sender=11U;peer.producer=22U;peer.key=33U;peer.allowed_types=GSEA_BOTH;(void)memcpy(peer.public_key,test_key,32U);gsea_init(a);
 CHECK(gsea_configure(a,&peer,1U,1U)==GSEA_OK);CHECK(gi_encode_record(&c,&genesis,s->slots[0])==GI_OK);(void)memcpy(s->hash,s->slots[0]+224U,32U);
 p.user=s;p.read_anchor=anchor;p.read_record=record;p.stage=stage;p.advance=advance;
 CHECK(gi_init(x,&c,&p)==GI_OK);CHECK(gi_recover(x,a,1U)==GI_OK);CHECK(memcmp(x->commitment,expected_commitment,32U)==0);return 0;
}
static int receive(gi_context *x,gsea_context *a,const uint8_t *b,size_t n,uint32_t now){CHECK(gi_prepare(x,a,b,n,now)==GI_OK);CHECK(gi_commit(x,a,now)==GI_OK);return 0;}
static int pending(gi_context *x,gsea_context *a){CHECK(gi_prepare_pending(x,a,88U,99U,100U,2U)==GI_OK);CHECK(gi_commit(x,a,2U)==GI_OK);return 0;}
static int rejection(gi_context *x,gsea_context *a,store *s,const uint8_t *b,size_t n,uint32_t now,gi_result expected){gi_state before=x->live;store disk=*s;CHECK(gi_prepare(x,a,b,n,now)==expected);CHECK(memcmp(&before,&x->live,sizeof(before))==0);CHECK(memcmp(&disk,s,sizeof(disk))==0);return 0;}
static int suite(void){gi_context x,restarted;gsea_context a;store s;gi_state view,before;gi_provider p;gi_config c;uint8_t altered[GSE_MAX_WIRE];size_t i;unsigned bit;int cut;uint32_t expected;const uint8_t *invalid[]={bad_policy,bad_profile,bad_round,bad_generation,bad_state,claims_actuation,bad_commit,bad_epoch,bad_version};
 scenarios++;CHECK(setup(&x,&a,&s)==0);CHECK(pending(&x,&a)==0);CHECK(receive(&x,&a,obs,sizeof(obs),3U)==0);CHECK(gi_snapshot(&x,&a,3U,&view)==GI_OK);CHECK(view.eligible==0U&&view.pending==1U&&view.has_report==1U);CHECK(receive(&x,&a,ack,sizeof(ack),4U)==0);CHECK(gi_snapshot(&x,&a,4U,&view)==GI_OK);CHECK(view.eligible==1U&&view.pending==0U&&view.revision==3U);CHECK(rejection(&x,&a,&s,ack,sizeof(ack),5U,GI_REPLAY)==0);CHECK(rejection(&x,&a,&s,ack2,sizeof(ack2),5U,GI_ACK)==0);
 scenarios++;p=x.provider;c=x.config;CHECK(gi_init(&restarted,&c,&p)==GI_OK);CHECK(gi_recover(&restarted,&a,5U)==GI_OK);CHECK(gi_snapshot(&restarted,&a,5U,&view)==GI_OK);CHECK(view.eligible==1U&&view.pending==0U);CHECK(rejection(&restarted,&a,&s,obs,sizeof(obs),5U,GI_REPLAY)==0);CHECK(gi_snapshot(&restarted,&a,101U,&view)==GI_HOLD);
 scenarios++;CHECK(setup(&x,&a,&s)==0);CHECK(rejection(&x,&a,&s,ack,sizeof(ack),2U,GI_ACK)==0);CHECK(pending(&x,&a)==0);CHECK(rejection(&x,&a,&s,ack,sizeof(ack),3U,GI_ACK)==0);CHECK(receive(&x,&a,obs,sizeof(obs),3U)==0);CHECK(rejection(&x,&a,&s,wrong_ref,sizeof(wrong_ref),4U,GI_ACK)==0);CHECK(rejection(&x,&a,&s,ack,sizeof(ack),100U,GI_EXPIRED)==0);CHECK(rejection(&x,&a,&s,ack,sizeof(ack),101U,GI_EXPIRED)==0);
 for(i=0U;i<sizeof(invalid)/sizeof(invalid[0]);i++){scenarios++;CHECK(setup(&x,&a,&s)==0);CHECK(rejection(&x,&a,&s,invalid[i],148U,2U,GI_SEMANTIC)==0);}
 scenarios++;CHECK(setup(&x,&a,&s)==0);CHECK(rejection(&x,&a,&s,wrong_peer,sizeof(wrong_peer),2U,GI_AUTH)==0);CHECK(rejection(&x,&a,&s,wrong_domain,sizeof(wrong_domain),2U,GI_AUTH)==0);
 /* Real signatures are never stubbed: every bit of OBS and ACK is attacked. */
 for(i=0U;i<sizeof(obs);i++){for(bit=0U;bit<8U;bit++){(void)memcpy(altered,obs,sizeof(obs));altered[i]^=(uint8_t)(1U<<bit);CHECK(rejection(&x,&a,&s,altered,sizeof(obs),2U,GI_AUTH)==0);}}
 CHECK(pending(&x,&a)==0);CHECK(receive(&x,&a,obs,sizeof(obs),3U)==0);
 for(i=0U;i<sizeof(ack);i++){for(bit=0U;bit<8U;bit++){(void)memcpy(altered,ack,sizeof(ack));altered[i]^=(uint8_t)(1U<<bit);CHECK(rejection(&x,&a,&s,altered,sizeof(ack),4U,GI_AUTH)==0);}}
 for(i=0U;i<=GSE_MAX_WIRE;i++){if(i!=sizeof(obs)){CHECK(rejection(&x,&a,&s,obs,i,4U,GI_AUTH)==0);}if(i!=sizeof(ack)){CHECK(rejection(&x,&a,&s,ack,i,4U,GI_AUTH)==0);}}
 /* Each stage/anchor/readback failure: no RAM acceptance; restart reconciles. */
 for(cut=1;cut<=8;cut++){scenarios++;CHECK(setup(&x,&a,&s)==0);CHECK(pending(&x,&a)==0);CHECK(receive(&x,&a,obs,sizeof(obs),3U)==0);CHECK(gi_prepare(&x,&a,ack,sizeof(ack),4U)==GI_OK);before=x.live;s.cut=cut;CHECK(gi_commit(&x,&a,4U)==GI_STORAGE);CHECK(memcmp(&before,&x.live,sizeof(before))==0);CHECK(gi_snapshot(&x,&a,4U,&view)==GI_HOLD);expected=cut<=4?2U:3U;s.cut=0;p=x.provider;c=x.config;CHECK(gi_init(&restarted,&c,&p)==GI_OK);CHECK(gi_recover(&restarted,&a,5U)==GI_OK);CHECK(gi_snapshot(&restarted,&a,5U,&view)==GI_OK);CHECK(view.revision==expected);CHECK(view.eligible==(uint8_t)(expected==3U));CHECK(view.pending==(uint8_t)(expected==2U));if(expected==3U){CHECK(rejection(&restarted,&a,&s,ack,sizeof(ack),5U,GI_REPLAY)==0);}}
 /* Each persisted byte corruption fails recovery against the trusted anchor. */
 CHECK(setup(&x,&a,&s)==0);CHECK(pending(&x,&a)==0);CHECK(receive(&x,&a,obs,sizeof(obs),3U)==0);CHECK(receive(&x,&a,ack,sizeof(ack),4U)==0);p=x.provider;c=x.config;
 for(i=0U;i<GI_RECORD_SIZE;i++){s.slots[s.revision%2U][i]^=1U;CHECK(gi_init(&restarted,&c,&p)==GI_OK);CHECK(gi_recover(&restarted,&a,5U)==GI_HOLD);s.slots[s.revision%2U][i]^=1U;}
 scenarios++;CHECK(gi_init(&restarted,&c,&p)==GI_OK);s.revision--;CHECK(gi_recover(&restarted,&a,5U)==GI_HOLD);s.revision++;
 scenarios++;CHECK(gi_init(&restarted,&c,&p)==GI_OK);CHECK(gi_recover(&restarted,&a,3U)==GI_HOLD);CHECK(gi_snapshot(&restarted,&a,5U,&view)==GI_HOLD);
 scenarios++;CHECK(setup(&x,&a,&s)==0);CHECK(gi_prepare(&x,&a,obs,sizeof(obs),2U)==GI_OK);a.registry_revision++;CHECK(gi_commit(&x,&a,2U)==GI_CONFLICT);CHECK(s.writes==0U);
 scenarios++;CHECK(setup(&x,&a,&s)==0);CHECK(gi_prepare(&x,&a,obs,sizeof(obs),2U)==GI_OK);a.peers[0].revoked=1U;CHECK(gi_commit(&x,&a,2U)==GI_CONFLICT);CHECK(s.writes==0U);
 scenarios++;CHECK(setup(&x,&a,&s)==0);a.peers[0].allowed_types=GSEA_OBSERVATION;CHECK(gi_prepare(&x,&a,obs,sizeof(obs),2U)==GI_HOLD);CHECK(s.writes==0U);
 scenarios++;CHECK(setup(&x,&a,&s)==0);CHECK(pending(&x,&a)==0);CHECK(receive(&x,&a,obs,sizeof(obs),3U)==0);CHECK(gi_prepare(&x,&a,ack,sizeof(ack),4U)==GI_OK);CHECK(gi_commit(&x,&a,101U)==GI_EXPIRED);CHECK(gi_snapshot(&x,&a,4U,&view)==GI_HOLD);
 scenarios++;CHECK(setup(&x,&a,&s)==0);CHECK(gi_prepare(&x,&a,obs,sizeof(obs),2U)==GI_OK);CHECK(gi_prepare(&x,&a,obs,sizeof(obs),2U)==GI_HOLD);CHECK(gi_commit(&x,&a,2U)==GI_OK);CHECK(gi_commit(&x,&a,2U)==GI_HOLD);
 scenarios++;CHECK(setup(&x,&a,&s)==0);CHECK(gi_prepare(&x,&a,obs,sizeof(obs),2U)==GI_OK);s.hash[0]^=1U;CHECK(gi_commit(&x,&a,2U)==GI_STORAGE);CHECK(gi_snapshot(&x,&a,2U,&view)==GI_HOLD);
 scenarios++;CHECK(setup(&x,&a,&s)==0);CHECK(receive(&x,&a,obs,sizeof(obs),3U)==0);CHECK(rejection(&x,&a,&s,obs,sizeof(obs),4U,GI_REPLAY)==0);CHECK(receive(&x,&a,obs2,sizeof(obs2),4U)==0);
 scenarios++;CHECK(setup(&x,&a,&s)==0);CHECK(gi_prepare_pending(&x,&a,0U,1U,10U,2U)==GI_INVALID);CHECK(gi_prepare_pending(&x,&a,88U,99U,2U,2U)==GI_INVALID);CHECK(pending(&x,&a)==0);CHECK(gi_prepare_pending(&x,&a,88U,99U,100U,3U)==GI_INVALID);
 scenarios++;CHECK(setup(&x,&a,&s)==0);x.live.revision=UINT32_MAX;CHECK(gi_prepare(&x,&a,obs,sizeof(obs),2U)==GI_HOLD);
 scenarios++;CHECK(setup(&x,&a,&s)==0);CHECK(gi_prepare(&x,&a,obs,sizeof(obs),0U)==GI_HOLD);
 scenarios++;CHECK(setup(&x,&a,&s)==0);CHECK(pending(&x,&a)==0);CHECK(receive(&x,&a,obs,sizeof(obs),3U)==0);CHECK(receive(&x,&a,ack,sizeof(ack),4U)==0);a.peers[0].revoked=1U;CHECK(gi_snapshot(&x,&a,4U,&view)==GI_HOLD);a.peers[0].revoked=0U;a.peers[0].public_key[0]^=1U;CHECK(gi_snapshot(&x,&a,4U,&view)==GI_HOLD);CHECK(gi_prepare(&x,&a,obs2,sizeof(obs2),5U)==GI_HOLD);
 /* Repeat all eight crash/failure cuts during pending creation and observation. */
 for(bit=0U;bit<2U;bit++){for(cut=1;cut<=8;cut++){scenarios++;CHECK(setup(&x,&a,&s)==0);if(bit==0U){CHECK(gi_prepare_pending(&x,&a,88U,99U,100U,2U)==GI_OK);}else{CHECK(pending(&x,&a)==0);CHECK(gi_prepare(&x,&a,obs,sizeof(obs),3U)==GI_OK);}before=x.live;s.cut=cut;CHECK(gi_commit(&x,&a,3U)==GI_STORAGE);CHECK(memcmp(&before,&x.live,sizeof(before))==0);CHECK(gi_snapshot(&x,&a,3U,&view)==GI_HOLD);s.cut=0;p=x.provider;c=x.config;CHECK(gi_init(&restarted,&c,&p)==GI_OK);CHECK(gi_recover(&restarted,&a,4U)==GI_OK);CHECK(gi_snapshot(&restarted,&a,4U,&view)==GI_OK);expected=bit+(cut<=4?0U:1U);CHECK(view.revision==expected);CHECK(view.eligible==0U);CHECK(view.pending==(uint8_t)(expected!=0U));CHECK(view.has_report==(uint8_t)(bit==1U&&cut>=5));}}
 scenarios++;CHECK(gi_init(NULL,NULL,NULL)==GI_INVALID);CHECK(gi_recover(NULL,NULL,0U)==GI_INVALID);CHECK(gi_prepare(NULL,NULL,NULL,0U,0U)==GI_INVALID);CHECK(gi_commit(NULL,NULL,0U)==GI_INVALID);CHECK(gi_snapshot(NULL,NULL,0U,NULL)==GI_INVALID);
 return 0;}
int main(void){if(suite()!=0){return 1;}(void)printf("INGRESS_CHECKS=%u\nINGRESS_SCENARIO_GROUPS=%u\nINGRESS_SEVEN_STAGES=PASS_REFERENCE_HOST_ONLY\nBIT_MUTATION_REJECTIONS=2432\nPERSISTENCE_FAULT_CUTS=24\nPHYSICAL_DURABILITY_TARGET_PQC_CONFORMANCE=NOT_ESTABLISHED\n",checks,scenarios);return 0;}
