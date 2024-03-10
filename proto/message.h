
#pragma once

#include <string>
#include <stdint.h>

/**
 * Message是协议承载的业务信息，提供序列化和反序列化的接口
 **/

class Message{
public:
    Message(uint16_t protoVersion, uint32_t protoId):m_protoVersion(protoVersion),m_protoId(protoId){}
    virtual ~Message(){}
    virtual void EncodeToString(std::string* str)const=0;
    virtual bool DecodeFromArray(const char* buf, size_t size, size_t* rSize)=0;
	uint16_t ProtoVersion()	const	{return m_protoVersion;}
	uint32_t ProtoID() const	{return m_protoId;}
private:
	uint16_t m_protoVersion;
	uint32_t m_protoId;
};

