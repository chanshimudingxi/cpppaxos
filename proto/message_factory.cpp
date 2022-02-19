#include "message_factory.h"

Message* MessageFactory::CreateMessage(uint16_t protoVersion, uint32_t protoId){
	Message* pMsg = nullptr;
	if(protoVersion == PAXOS_PROTO){
		switch(protoId){
		case PAXOS_PROTO_PING_MESSAGE:
			pMsg = new PingMessage(protoVersion, protoId);
			break;
		default:
			break;
		}
	}
	return pMsg;
}
