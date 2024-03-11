#pragma once

#include "message.h"
#include "proto.h"

class MessageFactory{
public:
	static Message* CreateMessage(uint16_t protoVersion, uint32_t protoId)
	{
		Message* pMsg = nullptr;
		if(protoVersion == PAXOS_PROTO){
			switch(protoId){
			case PAXOS_PROTO_PING_MESSAGE:
				pMsg = new HeartbeatMessage(protoVersion, protoId);
				break;
			default:
				break;
			}
		}
		return pMsg;
	}
};