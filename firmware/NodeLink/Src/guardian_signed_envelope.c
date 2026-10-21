#include "guardian_signed_envelope.h"
#include <string.h>
static void put32(uint8_t *p,uint32_t n) {
    size_t i;for(i=0U;i<4U;i++){p[i]=(uint8_t)(n&UINT32_C(255));n>>=8U;}
}
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0]|((uint32_t)p[1]<<8U)|((uint32_t)p[2]<<16U)|((uint32_t)p[3]<<24U);
}
static int valid(const gse_unverified *p) {
    return p!=NULL && p->state<=5U && p->sender!=0U && p->producer!=0U &&
        p->key!=0U && p->epoch!=0U && p->sequence!=0U &&
        ((p->type==7U && p->payload_length==52U)||(p->type==8U && p->payload_length==60U));
}
static void header(const gse_unverified *p,uint8_t *b) {
    (void)memset(b,0,GSE_HEADER);b[0]=0x47U;b[1]=0x53U;b[2]=2U;b[3]=p->type;
    b[5]=p->state;b[6]=1U;b[8]=(uint8_t)p->payload_length;b[10]=64U;
    put32(b+12,p->sender);put32(b+16,p->producer);put32(b+20,p->key);
    put32(b+24,p->epoch);put32(b+28,p->sequence);
}
gse_result gse_encode(const gse_unverified *input,uint8_t *wire,size_t capacity,size_t *length) {
    uint8_t temp[GSE_MAX_WIRE];size_t n;
    if(length==NULL){return GSE_INVALID;}*length=0U;
    if(wire==NULL || !valid(input)){return GSE_INVALID;}
    n=GSE_HEADER+(size_t)input->payload_length+GSE_SIGNATURE;
    if(capacity<n){return GSE_CAPACITY;}
    header(input,temp);(void)memcpy(temp+GSE_HEADER,input->payload,input->payload_length);
    (void)memcpy(temp+GSE_HEADER+input->payload_length,input->signature,GSE_SIGNATURE);
    (void)memcpy(wire,temp,n);*length=n;return GSE_OK;
}
gse_result gse_decode(const uint8_t *wire,size_t length,gse_unverified *output) {
    gse_unverified temp;uint16_t payload;
    if(wire==NULL || output==NULL || length<GSE_HEADER+GSE_SIGNATURE || length>GSE_MAX_WIRE){return GSE_INVALID;}
    if(wire[0]!=0x47U || wire[1]!=0x53U || wire[2]!=2U || wire[4]!=0U || wire[6]!=1U || wire[7]!=0U || wire[9]!=0U || wire[10]!=64U || wire[11]!=0U){return GSE_INVALID;}
    payload=(uint16_t)wire[8];
    if(length!=GSE_HEADER+(size_t)payload+GSE_SIGNATURE || payload>GSE_MAX_PAYLOAD){return GSE_INVALID;}
    (void)memset(&temp,0,sizeof(temp));temp.type=wire[3];temp.state=wire[5];temp.payload_length=payload;
    temp.sender=get32(wire+12);temp.producer=get32(wire+16);temp.key=get32(wire+20);
    temp.epoch=get32(wire+24);temp.sequence=get32(wire+28);
    if(!valid(&temp)){return GSE_INVALID;}
    (void)memcpy(temp.payload,wire+GSE_HEADER,payload);
    (void)memcpy(temp.signature,wire+GSE_HEADER+payload,GSE_SIGNATURE);
    (void)memcpy(output,&temp,sizeof(temp));return GSE_OK;
}
gse_result gse_transcript(const gse_unverified *input,uint8_t *bytes,size_t capacity,size_t *length) {
    uint8_t temp[GSE_MAX_TRANSCRIPT];size_t n;
    if(length==NULL){return GSE_INVALID;}*length=0U;
    if(bytes==NULL || !valid(input)){return GSE_INVALID;}
    n=GSE_DOMAIN_SIZE+GSE_HEADER+(size_t)input->payload_length;
    if(capacity<n){return GSE_CAPACITY;}
    (void)memcpy(temp,GSE_DOMAIN,GSE_DOMAIN_SIZE);header(input,temp+GSE_DOMAIN_SIZE);
    (void)memcpy(temp+GSE_DOMAIN_SIZE+GSE_HEADER,input->payload,input->payload_length);
    (void)memcpy(bytes,temp,n);*length=n;return GSE_OK;
}
