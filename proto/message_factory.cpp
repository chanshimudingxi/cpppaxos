#include "message_factory.h"
#include "test_message.h"

Message* MessageFactory::CreateMessage(uint16_t protoVersion, uint32_t protoId){
	Message* pMsg = nullptr;
	if(protoVersion == SSN_PROTO){
		switch(protoId){
		case SSN_PROTO_TEST_MESSAGE:
			pMsg = new TestMessage(protoVersion, protoId);
			break;
		default:
			break;
		}
	}
	return pMsg;
}
