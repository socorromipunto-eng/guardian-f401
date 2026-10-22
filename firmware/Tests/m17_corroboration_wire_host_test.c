#include "guardian_corroboration_wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned cases;
static gc_wire_observation example(void)
{
    gc_wire_observation value;
    size_t i;
    memset(&value,0,sizeof(value));
    value.generation=0x12345678u; value.policy=0x01020304u;
    value.profile=0xA1B2C3D4u; value.round=0x11223344u;
    value.claims_actuation=1u;
    for (i=0u;i<GC_WIRE_COMMITMENT_SIZE;i++) { value.commitment[i]=(uint8_t)i; }
    return value;
}
static void rejected(const uint8_t *wire, size_t length)
{
    gc_wire_observation output, before;
    memset(&output,0xA5,sizeof(output)); memcpy(&before,&output,sizeof(before));
    assert(gc_wire_decode(wire,length,&output)==GC_WIRE_INVALID);
    assert(memcmp(&output,&before,sizeof(output))==0);
    cases++;
}
int main(void)
{
    static const uint8_t prefix[20]={1u,1u,0u,0u,0x78u,0x56u,0x34u,0x12u,
        4u,3u,2u,1u,0xD4u,0xC3u,0xB2u,0xA1u,0x44u,0x33u,0x22u,0x11u};
    gc_wire_observation value=example(), output;
    uint8_t wire[104], original[104], altered[104], saved[104];
    size_t length, i, offset;
    unsigned byte;
    memset(wire,0xCC,sizeof(wire)); length=999u;
    assert(gc_wire_encode(&value,wire,sizeof(wire),&length)==GC_WIRE_OK);
    assert(length==GC_WIRE_SIZE && memcmp(wire,prefix,sizeof(prefix))==0);
    assert(memcmp(wire+20u,value.commitment,GC_WIRE_COMMITMENT_SIZE)==0);
    for(i=GC_WIRE_SIZE;i<sizeof(wire);i++) { assert(wire[i]==0xCCu); }
    assert(gc_wire_decode(wire,length,&output)==GC_WIRE_OK);
    assert(output.generation==value.generation && output.policy==value.policy);
    assert(output.profile==value.profile && output.round==value.round);
    assert(output.claims_actuation==1u);
    assert(memcmp(output.commitment,value.commitment,GC_WIRE_COMMITMENT_SIZE)==0);
    memcpy(original,wire,sizeof(wire));cases++;
    for(i=0u;i<=sizeof(wire);i++) { if(i!=GC_WIRE_SIZE) { rejected(wire,i); } }
    rejected(NULL,GC_WIRE_SIZE);
    assert(gc_wire_decode(wire,GC_WIRE_SIZE,NULL)==GC_WIRE_INVALID);cases++;
    for(byte=0u;byte<=255u;byte++) {
        memcpy(altered,original,sizeof(altered));altered[0]=(uint8_t)byte;
        if(byte!=1u) { rejected(altered,GC_WIRE_SIZE); }
        memcpy(altered,original,sizeof(altered));altered[1]=(uint8_t)byte;
        if(byte>1u) { rejected(altered,GC_WIRE_SIZE); }
    }
    for(offset=2u;offset<=3u;offset++) {
        for(byte=1u;byte<=255u;byte++) {
            memcpy(altered,original,sizeof(altered));altered[offset]=(uint8_t)byte;
            rejected(altered,GC_WIRE_SIZE);
        }
    }
    for(offset=4u;offset<=16u;offset+=4u) {
        memcpy(altered,original,sizeof(altered));memset(altered+offset,0,4u);
        rejected(altered,GC_WIRE_SIZE);
    }
    for(i=0u;i<GC_WIRE_SIZE;i++) {
        memcpy(saved,wire,sizeof(saved));length=999u;
        assert(gc_wire_encode(&value,wire,i,&length)==GC_WIRE_CAPACITY);
        assert(length==0u && memcmp(wire,saved,sizeof(wire))==0);cases++;
    }
    for(byte=2u;byte<=255u;byte++) {
        value=example();value.claims_actuation=(uint8_t)byte;
        memcpy(saved,wire,sizeof(saved));length=999u;
        assert(gc_wire_encode(&value,wire,sizeof(wire),&length)==GC_WIRE_INVALID);
        assert(length==0u && memcmp(wire,saved,sizeof(wire))==0);cases++;
    }
    for(i=0u;i<4u;i++) {
        value=example();
        if(i==0u) { value.generation=0u; }
        if(i==1u) { value.policy=0u; }
        if(i==2u) { value.profile=0u; }
        if(i==3u) { value.round=0u; }
        length=999u;memcpy(saved,wire,sizeof(saved));
        assert(gc_wire_encode(&value,wire,sizeof(wire),&length)==GC_WIRE_INVALID);
        assert(length==0u && memcmp(wire,saved,sizeof(wire))==0);cases++;
    }
    value=example();length=999u;memcpy(saved,wire,sizeof(saved));
    assert(gc_wire_encode(NULL,wire,sizeof(wire),&length)==GC_WIRE_INVALID);
    assert(length==0u && memcmp(wire,saved,sizeof(wire))==0);cases++;
    length=999u;assert(gc_wire_encode(&value,NULL,sizeof(wire),&length)==GC_WIRE_INVALID);
    assert(length==0u);cases++;
    assert(gc_wire_encode(&value,wire,sizeof(wire),NULL)==GC_WIRE_INVALID);
    assert(memcmp(wire,saved,sizeof(wire))==0);cases++;
    for(byte=0u;byte<=1u;byte++) {
        value=example();value.claims_actuation=(uint8_t)byte;
        value.generation=UINT32_MAX;value.policy=UINT32_MAX;
        value.profile=UINT32_MAX;value.round=UINT32_MAX;
        assert(gc_wire_encode(&value,wire,sizeof(wire),&length)==GC_WIRE_OK);
        assert(gc_wire_decode(wire,length,&output)==GC_WIRE_OK);
        assert(output.generation==UINT32_MAX && output.policy==UINT32_MAX);
        assert(output.profile==UINT32_MAX && output.round==UINT32_MAX);
        assert(output.claims_actuation==byte);cases++;
    }
    printf("CORROBORATION_WIRE_HOST_CASES=%u\n",cases);
    puts("CORROBORATION_WIRE_HOST_TEST=PASS");return 0;
}
