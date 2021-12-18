
#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <string>
#include <stdint.h>
/**
 * 协议版本
 */

enum {
	SSN_PROTO = 1,		//自定义协议
	PROTO_BUFFER = 2,	//proto buffer协议
};

/**
 * 自定义协议的协议号 
 */
enum{
	SSN_PROTO_TEST_MESSAGE = 1,
};


/**
 * Message是协议承载的业务信息，提供序列化和反序列化的接口
 **/

class Message{
public:
    Message(uint16_t protoVersion, uint32_t protoId):m_protoVersion(protoVersion),m_protoId(protoId){}
    ~Message(){}
    virtual void EncodeToString(std::string* str)const=0;
    virtual bool DecodeFromArray(const char* buf, size_t size)=0;
	uint16_t ProtoVersion()	const	{return m_protoVersion;}
	uint32_t ProtoID() const	{return m_protoId;}
private:
	uint16_t m_protoVersion;
	uint32_t m_protoId;
};

#endif
