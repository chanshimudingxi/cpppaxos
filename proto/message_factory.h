#ifndef MESSAGE_FACTORY_H_
#define MESSAGE_FACTORY_H_

#include "message.h"
#include "proto.h"

class MessageFactory{
public:
	static Message* CreateMessage(uint16_t protoVersion, uint32_t protoId);
};


#endif