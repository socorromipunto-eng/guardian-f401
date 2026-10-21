#include "guardian_signed_envelope.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(e) do {checks++;if(!(e)){(void)fprintf(stderr,"FAIL line %d\n",__LINE__);abort();}}while(0)
int main(void) {
    gse_unverified input,decoded,unchanged,altered;
    uint8_t wire[161],changed[161],transcript[GSE_MAX_TRANSCRIPT],other[GSE_MAX_TRANSCRIPT],saved[GSE_MAX_TRANSCRIPT];
    const uint8_t golden[32]={0x47,0x53,2,8,0,2,1,0,60,0,64,0,1,0,0,0,2,0,0,0,3,0,0,0,4,0,0,0,5,0,0,0};
    size_t n,t,m,i,j;unsigned v;
    (void)memset(&input,0,sizeof(input));input.type=8U;input.state=2U;input.payload_length=60U;
    input.sender=1U;input.producer=2U;input.key=3U;input.epoch=4U;input.sequence=5U;
    for(i=0U;i<60U;i++){input.payload[i]=(uint8_t)i;}
    for(i=0U;i<64U;i++){input.signature[i]=(uint8_t)(i+1U);}
    CHECK(gse_encode(&input,wire,sizeof(wire),&n)==GSE_OK);CHECK(n==156U);CHECK(memcmp(wire,golden,32U)==0);
    CHECK(memcmp(wire+32,input.payload,60U)==0);CHECK(memcmp(wire+92,input.signature,64U)==0);
    CHECK(gse_decode(wire,n,&decoded)==GSE_OK);CHECK(decoded.sequence==5U);CHECK(memcmp(decoded.payload,input.payload,60U)==0);
    CHECK(gse_transcript(&input,transcript,sizeof(transcript),&t)==GSE_OK);
    CHECK(t==GSE_DOMAIN_SIZE+92U);CHECK(memcmp(transcript,GSE_DOMAIN,GSE_DOMAIN_SIZE)==0);
    CHECK(memcmp(transcript+GSE_DOMAIN_SIZE,wire,92U)==0);
    (void)memset(&unchanged,0xA5,sizeof(unchanged));
    for(i=0U;i<n;i++){(void)memcpy(&decoded,&unchanged,sizeof(decoded));CHECK(gse_decode(wire,i,&decoded)==GSE_INVALID);CHECK(memcmp(&decoded,&unchanged,sizeof(decoded))==0);}
    for(i=n+1U;i<=161U;i++){CHECK(gse_decode(wire,i,&decoded)==GSE_INVALID);}
    for(i=0U;i<12U;i++){
        if(i==3U || i==5U){continue;}
        for(v=0U;v<256U;v++){
            if(v==(unsigned)wire[i]){continue;}
            (void)memcpy(changed,wire,n);changed[i]=(uint8_t)v;
            CHECK(gse_decode(changed,n,&decoded)==GSE_INVALID);
        }
    }
    for(v=0U;v<256U;v++){if(v!=8U){(void)memcpy(changed,wire,n);changed[3]=(uint8_t)v;CHECK(gse_decode(changed,n,&decoded)==GSE_INVALID);}}
    for(v=6U;v<256U;v++){(void)memcpy(changed,wire,n);changed[5]=(uint8_t)v;CHECK(gse_decode(changed,n,&decoded)==GSE_INVALID);}
    for(i=12U;i<32U;i+=4U){(void)memcpy(changed,wire,n);(void)memset(changed+i,0,4U);CHECK(gse_decode(changed,n,&decoded)==GSE_INVALID);}
    for(i=0U;i<92U;i++){for(j=0U;j<8U;j++){
        (void)memcpy(changed,wire,n);changed[i]^=(uint8_t)(1U<<j);
        if(gse_decode(changed,n,&altered)==GSE_OK){CHECK(gse_transcript(&altered,other,sizeof(other),&m)==GSE_OK);CHECK(m==t);CHECK(memcmp(other,transcript,t)!=0);}
    }}
    /* Signature is unverified input: codec must not pretend to verify it. */
    for(i=92U;i<n;i++){(void)memcpy(changed,wire,n);changed[i]^=1U;CHECK(gse_decode(changed,n,&altered)==GSE_OK);CHECK(gse_transcript(&altered,other,sizeof(other),&m)==GSE_OK);CHECK(memcmp(other,transcript,t)==0);}
    for(i=0U;i<n;i++){(void)memset(changed,0xA5,sizeof(changed));CHECK(gse_encode(&input,changed,i,&m)==GSE_CAPACITY);CHECK(m==0U);for(j=0U;j<sizeof(changed);j++){CHECK(changed[j]==0xA5U);}}
    for(i=0U;i<t;i++){(void)memset(other,0xA5,sizeof(other));(void)memcpy(saved,other,sizeof(saved));CHECK(gse_transcript(&input,other,i,&m)==GSE_CAPACITY);CHECK(m==0U);CHECK(memcmp(other,saved,sizeof(other))==0);}
    CHECK(gse_encode(NULL,wire,sizeof(wire),&m)==GSE_INVALID);CHECK(gse_encode(&input,NULL,0U,&m)==GSE_INVALID);
    CHECK(gse_encode(&input,wire,sizeof(wire),NULL)==GSE_INVALID);CHECK(gse_decode(NULL,n,&decoded)==GSE_INVALID);CHECK(gse_decode(wire,n,NULL)==GSE_INVALID);
    CHECK(gse_transcript(NULL,other,sizeof(other),&m)==GSE_INVALID);CHECK(gse_transcript(&input,NULL,0U,&m)==GSE_INVALID);CHECK(gse_transcript(&input,other,sizeof(other),NULL)==GSE_INVALID);
    input.type=7U;input.payload_length=52U;input.sender=UINT32_MAX;input.producer=UINT32_MAX;input.key=UINT32_MAX;input.epoch=UINT32_MAX;input.sequence=UINT32_MAX;
    CHECK(gse_encode(&input,wire,sizeof(wire),&n)==GSE_OK);CHECK(n==148U);CHECK(gse_decode(wire,n,&decoded)==GSE_OK);CHECK(decoded.sequence==UINT32_MAX);
    (void)printf("SIGNED_ENVELOPE_CHECKS=%u\n",checks);
    (void)puts("SIGNED_ENVELOPE_CODEC_TRANSCRIPT=PASS_HOST_ONLY");
    (void)puts("CRYPTOGRAPHIC_VERIFICATION_FRESHNESS_AUTHORITY=NOT_ESTABLISHED");return 0;
}
