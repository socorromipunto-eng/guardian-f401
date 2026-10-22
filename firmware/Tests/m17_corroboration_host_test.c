#include "guardian_multinode_corroboration.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static gc_config configuration(void) {
    gc_config p; unsigned int i; memset(&p,0,sizeof(p));
    p.generation=1;p.policy=2;p.profile=3;p.round=4;p.max_age=10;
    p.count=5;p.threshold=3;p.required_roles=11; /* control, supervisor, witness */
    for(i=0;i<5;++i) { p.peers[i].id=i+1u;p.peers[i].role=(uint8_t)i; } return p;
}
static gc_context configured(void) {
    gc_context c; gc_config p=configuration();gc_init(&c);assert(gc_configure(&c,&p)==GC_OK);return c;
}
static gc_report report(uint32_t id,uint64_t seq) {
    gc_report r;memset(&r,0,sizeof(r));r.id=id;r.generation=1;r.policy=2;r.profile=3;r.round=4;
    r.epoch=1;r.sequence=seq;r.authenticated=1;r.persistent_freshness=1;r.same_round_ack=1;r.commitment[0]=42;return r;
}
static void send(gc_context *c,uint32_t id,uint64_t now) { gc_report r=report(id,1);assert(gc_submit(c,&r,now,1)==GC_OK); }
static void positive(gc_context *c) { send(c,1,100);send(c,2,100);send(c,4,100); }
static void denied(gc_context *c,uint64_t now) { gc_assessment a;assert(gc_query(c,now,1,&a)==GC_OK);assert(a.eligible_for_local_review==0); }
int main(void) {
    gc_context c,before;gc_config p;gc_report r;gc_assessment a; unsigned int i,cases=0;
    gc_init(&c);assert(gc_query(&c,0,1,&a)==GC_REJECTED);assert(a.eligible_for_local_review==0);++cases;
    p=configuration();p.count=6;assert(gc_configure(&c,&p)==GC_REJECTED);++cases;
    p=configuration();p.peers[1].id=1;assert(gc_configure(&c,&p)==GC_REJECTED);++cases;
    p=configuration();p.threshold=0;assert(gc_configure(&c,&p)==GC_REJECTED);++cases;
    p=configuration();p.threshold=6;assert(gc_configure(&c,&p)==GC_REJECTED);++cases;
    p=configuration();p.peers[3].role=2;assert(gc_configure(&c,&p)==GC_REJECTED);++cases;
    p=configuration();p.required_roles=128;assert(gc_configure(&c,&p)==GC_REJECTED);++cases;
    c=configured();denied(&c,100);++cases;
    c=configured();positive(&c);assert(gc_query(&c,110,1,&a)==GC_OK);assert(a.eligible_for_local_review==1);assert(a.fresh_count==3);assert(a.role_coverage==11);++cases;
    assert(gc_query(&c,111,1,&a)==GC_OK);assert(a.eligible_for_local_review==0);++cases;
    c=configured();send(&c,1,100);send(&c,2,100);send(&c,3,100);denied(&c,100);++cases;
    c=configured();send(&c,1,100);send(&c,4,100);denied(&c,100);++cases;
    c=configured();r=report(1,1);r.same_round_ack=0;assert(gc_submit(&c,&r,100,1)==GC_OK);send(&c,2,100);send(&c,4,100);denied(&c,100);++cases;
    c=configured();positive(&c);before=c;r=report(1,1);assert(gc_submit(&c,&r,100,1)==GC_REJECTED);assert(memcmp(&c,&before,sizeof(c))==0);++cases;
    assert(gc_link_loss(&c,4,100,1)==GC_OK);denied(&c,100);r=report(4,1);assert(gc_submit(&c,&r,100,1)==GC_REJECTED);r.sequence=2;assert(gc_submit(&c,&r,100,1)==GC_OK);assert(gc_query(&c,100,1,&a)==GC_OK);assert(a.eligible_for_local_review==1);++cases;
    c=configured();positive(&c);r=report(3,1);r.commitment[0]=43;assert(gc_submit(&c,&r,100,1)==GC_CONFLICT);denied(&c,100);assert(c.state==GC_CONFLICT_LATCHED);assert(c.conflict_present==1);assert(c.conflicting_report.id==3);++cases;
    r=report(5,1);assert(gc_submit(&c,&r,100,1)==GC_LATCHED);denied(&c,200);p=configuration();assert(gc_configure(&c,&p)==GC_REJECTED);gc_require_rejoin(&c);assert(c.state==GC_CONFLICT_LATCHED);++cases;
    c=configured();positive(&c);r=report(1,2);r.commitment[1]=1;assert(gc_submit(&c,&r,100,1)==GC_CONFLICT);++cases;
    c=configured();r=report(2,1);r.claims_actuation=1;assert(gc_submit(&c,&r,100,1)==GC_CONFLICT);denied(&c,100);++cases;
    c=configured();r=report(1,1);r.claims_actuation=1;assert(gc_submit(&c,&r,100,1)==GC_OK);denied(&c,100);++cases;
    c=configured();positive(&c);assert(gc_query(&c,99,1,&a)==GC_CLOCK_FAILURE);assert(a.eligible_for_local_review==0);assert(c.state==GC_REJOIN_REQUIRED);++cases;
    c=configured();positive(&c);assert(gc_query(&c,100,0,&a)==GC_CLOCK_FAILURE);assert(a.eligible_for_local_review==0);++cases;
    c=configured();positive(&c);r=report(1,2);r.epoch=2;assert(gc_submit(&c,&r,100,1)==GC_EPOCH_CHANGE);assert(c.state==GC_REJOIN_REQUIRED);denied(&c,100);++cases;
    c=configured();positive(&c);gc_require_rejoin(&c);denied(&c,100);r=report(1,2);assert(gc_submit(&c,&r,100,1)==GC_LATCHED);++cases;
    c=configured();send(&c,1,UINT64_MAX-5u);send(&c,2,UINT64_MAX-5u);send(&c,4,UINT64_MAX-5u);assert(gc_query(&c,UINT64_MAX,1,&a)==GC_OK);assert(a.eligible_for_local_review==1);++cases;
    for(i=0;i<10;++i) {
        c=configured();positive(&c);before=c;r=report(1,2);
        switch(i) {
        case 0:r.id=99;break;case 1:r.generation=2;break;case 2:r.policy=99;break;
        case 3:r.profile=99;break;case 4:r.round=99;break;case 5:r.epoch=0;break;
        case 6:r.sequence=0;break;case 7:r.authenticated=0;break;
        case 8:r.persistent_freshness=0;break;default:r.same_round_ack=2;break;
        }
        assert(gc_submit(&c,&r,100,1)==GC_REJECTED);assert(memcmp(&c,&before,sizeof(c))==0);++cases;
    }
    for(i=2;i<256;++i) {
        c=configured();before=c;r=report(1,1);r.authenticated=(uint8_t)i;
        assert(gc_submit(&c,&r,100,1)==GC_REJECTED);assert(memcmp(&c,&before,sizeof(c))==0);++cases;
        r=report(1,1);r.claims_actuation=(uint8_t)i;assert(gc_submit(&c,&r,100,1)==GC_REJECTED);assert(memcmp(&c,&before,sizeof(c))==0);++cases;
    }
    c=configured();assert(gc_submit(NULL,&r,0,1)==GC_REJECTED);assert(gc_submit(&c,NULL,0,1)==GC_REJECTED);assert(gc_query(&c,0,1,NULL)==GC_REJECTED);++cases;
    printf("CORROBORATION_HOST_CASES=%u\nCORROBORATION_HOST_TEST=PASS\n",cases);return 0;
}
