#include <stdint.h>
extern "C" {
#include "eap_fuzz_Cproxy.h"
}

#include <src/libfuzzer/libfuzzer_macro.h>
#include "eap_fuzz.pb.h"

#define S_MALLOC(var, size) \
do { \
 if ((var = (uint8_t *)malloc(size)) == NULL) { \
     return; \
 } \
} while(0)

void write_header(uint8_t *packet, uint16_t data_size, uint8_t type)
{
    data_size += EAP_HEADERLEN;
    //the packet type
    *(packet)++ = type&0xff;
    //id
    *(packet)++ = 0x0;
    //the length as big endian short
    *(packet)++ = ((data_size >> 8)&0xff);
    *(packet)++ = data_size&0xff;
}

DEFINE_BINARY_PROTO_FUZZER(const eap_fuzz::proto::PacketSet &packets){
    init();

    for(const eap_fuzz::proto::Packet& packet: packets.packets()){
       uint8_t *fuzz_packet = NULL;
       size_t packet_len = 0;
       std::string data = "";
       uint8_t packet_type = -1;
       switch(packet.PacketType_case()){
           case eap_fuzz::proto::Packet::kEapRequest: {
                packet_type = EAP_REQUEST;
                uint8_t eap_request_type = -1;
                auto eap_request = packet.eap_request();
                switch(eap_request.EapRequestType_case()){
                    case eap_fuzz::proto::EapRequest::kIdentity: {
                        eap_request_type = EAPT_IDENTITY;
                        data = eap_request.identity().data();
                        S_MALLOC(fuzz_packet, data.size()+EAP_HEADERLEN+1);
                        break;
                    }
                    case eap_fuzz::proto::EapRequest::kNotification: {
                        eap_request_type = EAPT_NOTIFICATION;
                        data = eap_request.notification().data();
                        S_MALLOC(fuzz_packet, data.size()+EAP_HEADERLEN+1);
                        break;
                    }
                    case eap_fuzz::proto::EapRequest::kMd5Chap: {
                        eap_request_type = EAPT_MD5CHAP;
                        data = eap_request.md5chap().data();
                        S_MALLOC(fuzz_packet, data.size()+EAP_HEADERLEN+1);
                        break;
                    }
                    case eap_fuzz::proto::EapRequest::kSrp: {
                        auto request_srp = eap_request.srp();
                        eap_request_type = EAPT_SRP;
                        uint8_t srp_type = -1;
                        switch(request_srp.EspMessage_case()){
                            case eap_fuzz::proto::EaptRequestSRP::kSrpChallenge:{
                                data = request_srp.srp_challenge().data();
                                srp_type = EAPSRP_CHALLENGE;
                                break;
                            }
                            case eap_fuzz::proto::EaptRequestSRP::kSrpValidator:{
                                data = request_srp.srp_validator().data();
                                srp_type = EAPSRP_SVALIDATOR;
                                break;
                            }
                            case eap_fuzz::proto::EaptRequestSRP::kSrpKey:{
                                data = request_srp.srp_key().data();
                                srp_type = EAPSRP_SKEY;
                                break;
                            }
                            case eap_fuzz::proto::EaptRequestSRP::kSrpLwreChallenge:{
                                data = request_srp.srp_lwre_challenge().data();
                                srp_type = EAPSRP_LWRECHALLENGE;
                                break;
                            }
                            case eap_fuzz::proto::EaptRequestSRP::ESPMESSAGE_NOT_SET:{
                                return;
                            }

                        }
                        S_MALLOC(fuzz_packet, data.size()+EAP_HEADERLEN+2);
                        *(fuzz_packet+EAP_HEADERLEN+1) = srp_type;
                        packet_len++;
                        break;
                    }
                    case eap_fuzz::proto::EapRequest::EAPREQUESTTYPE_NOT_SET: {
                        return;
                    }
                }
                *(fuzz_packet+EAP_HEADERLEN) = eap_request_type;
                ++packet_len;
                break;
           }

           case eap_fuzz::proto::Packet::kEapResponse: {
                packet_type = EAP_RESPONSE;
                auto eap_response = packet.eap_response();
                uint8_t eap_response_type = -1;
                switch(eap_response.EapResponseType_case()){
                    case eap_fuzz::proto::EapResponse::kIdentity: {
                        eap_response_type = EAPT_IDENTITY;
                        data = eap_response.identity().data();
                        S_MALLOC(fuzz_packet, data.size()+EAP_HEADERLEN+1);
                        break;
                    }
                    case eap_fuzz::proto::EapResponse::kNotification: {
                        eap_response_type = EAPT_NOTIFICATION;
                        data = eap_response.notification().data();
                        S_MALLOC(fuzz_packet, data.size()+EAP_HEADERLEN+1);
                        break;
                    }
                    case eap_fuzz::proto::EapResponse::kMd5Chap: {
                        eap_response_type = EAPT_MD5CHAP;
                        data = eap_response.md5chap().data();
                        S_MALLOC(fuzz_packet, data.size()+EAP_HEADERLEN+1);
                        break;
                    }
                    case eap_fuzz::proto::EapResponse::kNak: {
                        eap_response_type = EAPT_NAK;
                        auto response_nak = eap_response.nak();
                        uint8_t nak_type = -1;
                        switch(response_nak.EaptResponseNAKType_case()){
                            case eap_fuzz::proto::EaptResponseNAK::kSrp:{
                                nak_type = EAPT_SRP;
                                break;

                            }
                            case eap_fuzz::proto::EaptResponseNAK::kMd5Chap:{
                                nak_type = EAPT_MD5CHAP;
                                break;

                            }
                            case eap_fuzz::proto::EaptResponseNAK::EAPTRESPONSENAKTYPE_NOT_SET:{
                                return;
                            }
                        }
                        S_MALLOC(fuzz_packet, data.size()+EAP_HEADERLEN+2);
                        *(fuzz_packet+EAP_HEADERLEN+1) = nak_type;
                        packet_len++;
                        break;
                    }
                    case eap_fuzz::proto::EapResponse::kSrp: {
                        auto response_srp = eap_response.srp();
                        eap_response_type = EAPT_SRP;
                        uint8_t srp_type = -1;
                        switch(response_srp.EspMessage_case()){
                            case eap_fuzz::proto::EaptResponseSRP::kSrpChallenge:{
                                data = response_srp.srp_challenge().data();
                                srp_type = EAPSRP_LWRECHALLENGE;
                                break;
                            }
                            case eap_fuzz::proto::EaptResponseSRP::kSrpCvalidator:{
                                data = response_srp.srp_cvalidator().data();
                                srp_type = EAPSRP_CVALIDATOR;
                                break;
                            }
                            case eap_fuzz::proto::EaptResponseSRP::kSrpCkey:{
                                data = response_srp.srp_ckey().data();
                                srp_type = EAPSRP_CKEY;
                                break;
                            }
                            case eap_fuzz::proto::EaptResponseSRP::kSrpAck:{
                                data = response_srp.srp_ack().data();
                                srp_type = EAPSRP_ACK;
                                break;
                            }
                            case eap_fuzz::proto::EaptResponseSRP::ESPMESSAGE_NOT_SET:{
                                return;
                            }

                        }
                        S_MALLOC(fuzz_packet, data.size()+EAP_HEADERLEN+2);
                        *(fuzz_packet+EAP_HEADERLEN+1) = srp_type;
                        packet_len++;
                        break;
                    }
                    case eap_fuzz::proto::EapResponse::EAPRESPONSETYPE_NOT_SET: {
                        return;
                    }
                }
                *(fuzz_packet+EAP_HEADERLEN) = eap_response_type;
                ++packet_len;
                break;
           }
           case eap_fuzz::proto::Packet::kEapSuccess: {
                packet_type = EAP_SUCCESS;
                data = packet.eap_success().data();
                S_MALLOC(fuzz_packet, data.size()+EAP_HEADERLEN);
                break;
           }
           case eap_fuzz::proto::Packet::kEapFailure: {
                packet_type = EAP_FAILURE;
                data = packet.eap_failure().data();
                S_MALLOC(fuzz_packet, data.size()+EAP_HEADERLEN);
                break;
           }
           case eap_fuzz::proto::Packet::PACKETTYPE_NOT_SET: {
                return;
         }
       }
       write_header(fuzz_packet, data.size()+packet_len, packet_type);
       memcpy(fuzz_packet+EAP_HEADERLEN+packet_len, data.data(), data.size());
       proxy_packet(fuzz_packet, data.size()+EAP_HEADERLEN+packet_len);
       free(fuzz_packet);
    }
}
