#pragma once

#include <string>
#include <cstddef>
#include <netinet/in.h>
#include <memory>

#include "sys/log.h"
#include "net/proto_parser.h"
#include "sys/util.h"
#include "message_factory.h"
#include "proto.h"

typedef bool (*Message_Callback)(std::shared_ptr<Message> pMsg, SocketBase* s, void* instance);
typedef bool (*Close_Callback)(SocketBase* s, void* instance);

class CommonProtoParser: public ProtoParser{
public:
	CommonProtoParser();
	~CommonProtoParser();
    static 	int PackMessage(const Message& msg, std::string* buffer); 
    size_t  PacketMaxSize();
    virtual int HandlePacket(const char* data, size_t size, SocketBase* s);
	virtual void HandleClose(SocketBase* s);
	void SetMessageCallback(Message_Callback callback, void* instance);
	void SetCloseCallback(Close_Callback callback, void* instance);
private:
	Message_Callback m_msgCallback;
	void* m_msgInstance;
	Close_Callback m_closeCallback;
	void* m_closeInstance;
}; 