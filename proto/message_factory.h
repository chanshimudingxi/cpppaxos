#pragma once

#include "message.h"
#include "proto.h"

class MessageFactory{
public:
	static Message* CreateMessage(uint16_t protoVersion, uint32_t protoId);
};