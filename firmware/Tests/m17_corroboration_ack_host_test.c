#include "guardian_corroboration_ack.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned int checks;
#define CHECK(x) do { ++checks; assert(x); } while (0)
static gc_ack reference(void) {
    gc_ack a; size_t i; memset(&a,0,sizeof(a));
    a.generation=0x04030201u; a.policy=0x08070605u;
    a.profile=0x0c0b0a09u; a.round=0x100f0e0du;
    a.observation_epoch=0x14131211u; a.observation_sequence=0x18171615u;
    for (i=0u;i<32u;++i) { a.commitment[i]=(uint8_t)(0x20u+i); }
    return a;
}
static const uint8_t golden[60]={
    1,0xa1,0,0, 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
    17,18,19,20,21,22,23,24,
    32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
    48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63
};
static void set_field(gc_ack *a, size_t field, uint32_t value) {
    switch (field) {
        case 0: a->generation=value; break;
        case 1: a->policy=value; break;
        case 2: a->profile=value; break;
        case 3: a->round=value; break;
        case 4: a->observation_epoch=value; break;
        default: a->observation_sequence=value; break;
    }
}
int main(void) {
    gc_ack a=reference(), b, saved, pending; uint8_t wire[61], sentinel[61];
    size_t size, i, j; unsigned int v;
    memset(sentinel,0x55,sizeof(sentinel));
    CHECK(GC_ACK_SIZE==60u);
    CHECK(gc_ack_encode(&a,wire,sizeof(wire),&size)==GC_ACK_OK);
    CHECK(size==60u && memcmp(wire,golden,60u)==0);
    CHECK(gc_ack_decode(golden,60u,&b)==GC_ACK_OK);
    CHECK(gc_ack_match(&b,&a)==GC_ACK_OK);
    /* N02: exact lengths, every truncated frame, extra byte, unchanged output. */
    for (i=0u;i<=61u;++i) {
        if (i==60u) { continue; }
        memset(&b,0x55,sizeof(b)); memcpy(&saved,&b,sizeof(saved));
        CHECK(gc_ack_decode(wire,i,&b)==GC_ACK_INVALID);
        CHECK(memcmp(&b,&saved,sizeof(b))==0);
    }
    /* N03: noncanonical version/kind/reserved values exhaustive per byte. */
    for (i=0u;i<4u;++i) {
        for (v=0u;v<=255u;++v) {
            if ((uint8_t)v==golden[i]) { continue; }
            memcpy(wire,golden,60u); wire[i]=(uint8_t)v;
            memset(&b,0x55,sizeof(b)); memcpy(&saved,&b,sizeof(saved));
            CHECK(gc_ack_decode(wire,60u,&b)==GC_ACK_INVALID);
            CHECK(memcmp(&b,&saved,sizeof(b))==0);
        }
    }
    /* N04: zero reference field rejected both directions and match. */
    for (i=0u;i<6u;++i) {
        b=a; set_field(&b,i,0u); memcpy(wire,sentinel,sizeof(wire)); size=99u;
        CHECK(gc_ack_encode(&b,wire,sizeof(wire),&size)==GC_ACK_INVALID);
        CHECK(size==0u && memcmp(wire,sentinel,sizeof(wire))==0);
        CHECK(gc_ack_match(&b,&a)==GC_ACK_INVALID);
        CHECK(gc_ack_match(&a,&b)==GC_ACK_INVALID);
        memcpy(wire,golden,60u); memset(wire+4u+4u*i,0,4u);
        memset(&b,0x55,sizeof(b)); memcpy(&saved,&b,sizeof(saved));
        CHECK(gc_ack_decode(wire,60u,&b)==GC_ACK_INVALID);
        CHECK(memcmp(&b,&saved,sizeof(b))==0);
    }
    /* N05: every reference byte changed must fail pending binding.
     * A structurally valid altered ACK is not an authenticated receipt. */
    for (i=4u;i<60u;++i) {
        for (j=0u;j<8u;++j) {
            memcpy(wire,golden,60u); wire[i]^=(uint8_t)(1u<<j);
            CHECK(gc_ack_decode(wire,60u,&b)==GC_ACK_OK);
            CHECK(gc_ack_match(&b,&a)==GC_ACK_MISMATCH);
        }
    }
    /* N06: output capacities and no partial write. */
    for (i=0u;i<60u;++i) {
        memcpy(wire,sentinel,sizeof(wire)); size=99u;
        CHECK(gc_ack_encode(&a,wire,i,&size)==GC_ACK_CAPACITY);
        CHECK(size==0u && memcmp(wire,sentinel,sizeof(wire))==0);
    }
    /* N07: null contracts and output preservation. */
    memcpy(wire,sentinel,sizeof(wire)); size=99u;
    CHECK(gc_ack_encode(NULL,wire,61u,&size)==GC_ACK_INVALID);
    CHECK(size==0u && memcmp(wire,sentinel,sizeof(wire))==0);
    CHECK(gc_ack_encode(&a,NULL,61u,&size)==GC_ACK_INVALID && size==0u);
    CHECK(gc_ack_encode(&a,wire,61u,NULL)==GC_ACK_INVALID);
    memset(&b,0x55,sizeof(b)); memcpy(&saved,&b,sizeof(saved));
    CHECK(gc_ack_decode(NULL,60u,&b)==GC_ACK_INVALID);
    CHECK(memcmp(&b,&saved,sizeof(b))==0);
    CHECK(gc_ack_decode(golden,60u,NULL)==GC_ACK_INVALID);
    CHECK(gc_ack_match(NULL,&a)==GC_ACK_INVALID);
    CHECK(gc_ack_match(&a,NULL)==GC_ACK_INVALID);
    /* P02: maximum width and opaque zero commitment, not truthful proof. */
    b=a; for (i=0u;i<6u;++i) { set_field(&b,i,UINT32_MAX); }
    memset(b.commitment,0,32u); pending=b;
    CHECK(gc_ack_encode(&b,wire,60u,&size)==GC_ACK_OK);
    CHECK(gc_ack_decode(wire,60u,&b)==GC_ACK_OK);
    CHECK(gc_ack_match(&b,&pending)==GC_ACK_OK);
    /* P03: documented overlapping buffers use temporary serialization. */
    { union { gc_ack ack; uint8_t bytes[64]; } overlap;
      overlap.ack=a;
      CHECK(gc_ack_encode(&overlap.ack,overlap.bytes,64u,&size)==GC_ACK_OK);
      CHECK(memcmp(overlap.bytes,golden,60u)==0);
      CHECK(gc_ack_decode(overlap.bytes,60u,&overlap.ack)==GC_ACK_OK);
      CHECK(gc_ack_match(&overlap.ack,&a)==GC_ACK_OK); }
    printf("ACK_CONTRACT_CHECKS=%u\nACK_CODEC_AND_REFERENCE_BINDING=PASS_HOST_ONLY\n",checks);
    puts("AUTHENTICATION_FRESHNESS_PERSISTENCE_ACTUATION=NOT_ESTABLISHED");
    return 0;
}
