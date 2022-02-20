#pragma once

#include "message.h"
#include "net/serializer.h"
#include <cstddef>
#include <vector>

//协议版本
#define PAXOS_PROTO 1 //自定义协议版本

//自定义协议版本的协议号 
#define PAXOS_PROTO_PING_MESSAGE 1


struct ProtoPeerAddr{
	uint32_t m_ip;
	uint16_t m_port;
	uint8_t m_socketType;	//0-tcp,1-udp

	ProtoPeerAddr(){}
	~ProtoPeerAddr(){}

    virtual void EncodeToString(std::string* str) const{
		Serializer::PutUint32(m_ip, str);
		Serializer::PutUint16(m_port, str);
		Serializer::PutUint8(m_socketType, str);
	}

    virtual bool DecodeFromArray(const char* buf, size_t size, size_t* rSize){
		size_t offset = 0;

		if(!Serializer::GetUint32(buf + offset, size-offset, &m_ip)){
			return false;
		}
		offset += 4;

		if(!Serializer::GetUint16(buf + offset, size-offset, &m_port)){
			return false;
		}
		offset += 2;


		if(!Serializer::GetUint8(buf + offset, size-offset, &m_socketType)){
			return false;
		}
		offset += 1;
		
		*rSize = offset;
		return true;
	}
};

struct ProtoPeer{
	std::string m_id;
	std::vector<ProtoPeerAddr> m_addrs;

	ProtoPeer(){}
	~ProtoPeer(){}

	virtual void EncodeToString(std::string* str) const{
		Serializer::PutString(m_id, str);
		size_t size = m_addrs.size();
		Serializer::PutUint32(size, str);
		for(int i=0; i<size; ++i){
			m_addrs[i].EncodeToString(str);
		}
	}

	virtual bool DecodeFromArray(const char* buf, size_t size, size_t* rSize){
		size_t offset = 0;

		if(!Serializer::GetString(buf + offset, size-offset, &m_id)){
			return false;
		}
		offset += m_id.size() + 4;

		uint32_t vsize = 0;
		if(!Serializer::GetUint32(buf + offset, size-offset, &vsize)){
			return false;
		}
		offset += 4;

		for(int i=0; i<vsize; ++i){
			size_t len = 0;
			if(!m_addrs[i].DecodeFromArray(buf + offset, size-offset, &len)){	
				return false;
			}
			offset += len;
		}

		*rSize = offset;
    	return true;
	}
};


//peer之间心跳协议
struct PingMessage : public Message{
	uint64_t m_stamp;
	ProtoPeer m_myInfo;

    PingMessage(uint16_t protoVersion = PAXOS_PROTO, uint32_t protoId = PAXOS_PROTO_PING_MESSAGE):Message(protoVersion,protoId){}
    ~PingMessage(){}

    virtual void EncodeToString(std::string* str) const{
		Serializer::PutUint64(m_stamp, str);
		m_myInfo.EncodeToString(str);
	}

    virtual bool DecodeFromArray(const char* buf, size_t size, size_t* rSize){
		size_t offset = 0;

		if(!Serializer::GetUint64(buf + offset, size-offset, &m_stamp)){
			return false;
		}
		offset += 8;

		size_t len = 0;
		if(!m_myInfo.DecodeFromArray(buf + offset, size - offset, &len)){
			return false;
		}
		offset += len;

		*rSize = offset;
		return true;
	}
};
