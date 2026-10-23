#include "guardian_signed_envelope_auth.h"
#include "monocypher-ed25519.h"
#include <string.h>
void gsea_init(gsea_context *context) {
    if (context!=NULL) {(void)memset(context,0,sizeof(*context));}
}
static int valid_peer(const gsea_peer *peer) {
    uint8_t aggregate=0U;size_t i;
    for(i=0U;i<GSEA_PUBLIC_KEY_SIZE;i++){aggregate=(uint8_t)(aggregate|peer->public_key[i]);}
    return peer->sender!=0U && peer->producer!=0U && peer->key!=0U &&
           peer->revoked<=1U && peer->allowed_types>=GSEA_OBSERVATION &&
           peer->allowed_types<=GSEA_BOTH && aggregate!=0U;
}
gsea_result gsea_configure(gsea_context *context,const gsea_peer *peers,
                           size_t count,uint32_t registry_revision) {
    gsea_context candidate;size_t i,j;
    if(context==NULL){return GSEA_INVALID;}
    (void)memset(&candidate,0,sizeof(candidate));
    if(peers==NULL || count==0U || count>GSEA_MAX_PEERS || registry_revision==0U){
        gsea_init(context);return GSEA_INVALID;
    }
    for(i=0U;i<count;i++){
        if(!valid_peer(&peers[i])){gsea_init(context);return GSEA_INVALID;}
        for(j=0U;j<i;j++){
            if(peers[i].sender==peers[j].sender && peers[i].producer==peers[j].producer && peers[i].key==peers[j].key){
                gsea_init(context);return GSEA_INVALID;
            }
        }
        (void)memcpy(&candidate.peers[i],&peers[i],sizeof(peers[i]));
    }
    candidate.count=count;candidate.registry_revision=registry_revision;candidate.configured=1U;
    (void)memcpy(context,&candidate,sizeof(candidate));return GSEA_OK;
}
gsea_result gsea_verify(const gsea_context *context,const uint8_t *wire,
                        size_t length,gsea_signature_evidence *output) {
    gse_unverified decoded;gsea_signature_evidence candidate;
    uint8_t transcript[GSE_MAX_TRANSCRIPT];size_t transcript_length=0U,i;
    const gsea_peer *peer=NULL;uint8_t required_type;
    if(context==NULL || wire==NULL || output==NULL){return GSEA_INVALID;}
    if(context->configured!=1U || context->count==0U || context->count>GSEA_MAX_PEERS || context->registry_revision==0U){return GSEA_UNCONFIGURED;}
    if(gse_decode(wire,length,&decoded)!=GSE_OK){return GSEA_INVALID;}
    for(i=0U;i<context->count;i++){
        if(context->peers[i].sender==decoded.sender && context->peers[i].producer==decoded.producer && context->peers[i].key==decoded.key){
            if(peer!=NULL){return GSEA_INVALID;}
            peer=&context->peers[i];
        }
    }
    if(peer==NULL){return GSEA_PEER_UNKNOWN;}
    if(peer->revoked!=0U){return GSEA_KEY_REVOKED;}
    required_type=decoded.type==7U ? GSEA_OBSERVATION : GSEA_ACK;
    if((peer->allowed_types&required_type)==0U){return GSEA_TYPE_DENIED;}
    if(gse_transcript(&decoded,transcript,sizeof(transcript),&transcript_length)!=GSE_OK){return GSEA_INVALID;}
    if(crypto_ed25519_check(decoded.signature,peer->public_key,transcript,transcript_length)!=0){return GSEA_SIGNATURE_INVALID;}
    (void)memset(&candidate,0,sizeof(candidate));
    (void)memcpy(&candidate.envelope,&decoded,sizeof(decoded));
    candidate.registry_revision=context->registry_revision;
    (void)memcpy(output,&candidate,sizeof(candidate));return GSEA_OK;
}
