#pragma once

#include "message.h"
#include "net/serializer.h"
#include "sys/util.h"
#include "sys/log.h"
#include <cstddef>
#include <vector>

//协议版本
#define PAXOS_PROTO 1 //自定义协议版本

//自定义协议版本的协议号 
#define PAXOS_PROTO_PING_MESSAGE 1


struct PPeerAddr{
	uint32_t m_ip;
	uint16_t m_port;
	uint8_t m_socketType;	//0-tcp,1-udp

	PPeerAddr(){}
	~PPeerAddr(){}

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

struct PPeer{
	std::string m_id;
	std::vector<PPeerAddr> m_addrs;

	PPeer(){}
	~PPeer(){}

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
			PPeerAddr addr;
			if(!addr.DecodeFromArray(buf + offset, size-offset, rSize)){
				return false;
			}
			offset += *rSize;
			m_addrs.push_back(addr);
		}

		*rSize = offset;
    	return true;
	}
};


//peer之间心跳协议
struct HeartbeatMessage : public Message{
	uint64_t m_stamp;
	PPeer m_myInfo;

    HeartbeatMessage(uint16_t protoVersion = PAXOS_PROTO, uint32_t protoId = PAXOS_PROTO_PING_MESSAGE):Message(protoVersion,protoId){}
    ~HeartbeatMessage(){}

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

		if(!m_myInfo.DecodeFromArray(buf + offset, size-offset, rSize)){
			return false;
		}
		offset += *rSize;

		*rSize = offset;
		return true;
	}
};

struct PProposalID{
	uint64_t m_number;
	std::string m_uid;

    virtual void EncodeToString(std::string* str) const{
		Serializer::PutUint64(m_number, str);
		Serializer::PutString(m_uid, str);
	}

    virtual bool DecodeFromArray(const char* buf, size_t size, size_t* rSize){
		size_t offset = 0;

		if(!Serializer::GetUint64(buf + offset, size-offset, &m_number)){
			return false;
		}
		offset += 8;

		if(!Serializer::GetString(buf + offset, size-offset, &m_uid)){
			return false;
		}
		offset += m_uid.size() + 4;

		*rSize = offset;
		return true;
	}
};

/**
 * @brief prepare请求协议
 * 
 */
struct PrepareMessage : public Message{
	PPeer m_myInfo;
	PProposalID m_proposalID;
	
    PrepareMessage(uint16_t protoVersion = PAXOS_PROTO, uint32_t protoId = PAXOS_PROTO_PING_MESSAGE):
		Message(protoVersion,protoId){}
    ~PrepareMessage(){}

    virtual void EncodeToString(std::string* str) const{
		m_proposalID.EncodeToString(str);
		m_myInfo.EncodeToString(str);
	}

    virtual bool DecodeFromArray(const char* buf, size_t size, size_t* rSize){
		size_t offset = 0;

		if(!m_proposalID.DecodeFromArray(buf + offset, size-offset, rSize)){
			return false;
		}
		offset += *rSize;

		if(!m_myInfo.DecodeFromArray(buf + offset, size-offset, rSize)){
			return false;
		}
		offset += *rSize;

		*rSize = offset;
		return true;
	}
};

/**
 * @brief prepare请求的响应
 * 
 */
struct PromiseMessage : public Message{
	PPeer m_myInfo;
	PProposalID m_proposalID;
	PProposalID m_acceptID;
	std::string m_acceptValue;

	PromiseMessage(uint16_t protoVersion = PAXOS_PROTO, uint32_t protoId = PAXOS_PROTO_PING_MESSAGE):
		Message(protoVersion,protoId){}
	~PromiseMessage(){}

	virtual void EncodeToString(std::string* str) const{
		m_proposalID.EncodeToString(str);
		m_acceptID.EncodeToString(str);
		m_myInfo.EncodeToString(str);
		Serializer::PutString(m_acceptValue, str);
	}

	virtual bool DecodeFromArray(const char* buf, size_t size, size_t* rSize){
		size_t offset = 0;

		if(!m_proposalID.DecodeFromArray(buf + offset, size-offset, rSize)){
			return false;
		}
		offset += *rSize;

		if(!m_acceptID.DecodeFromArray(buf + offset, size-offset, rSize)){
			return false;
		}
		offset += *rSize;

		if(!m_myInfo.DecodeFromArray(buf + offset, size-offset, rSize)){
			return false;
		}
		offset += *rSize;

		if(!Serializer::GetString(buf + offset, size-offset, &m_acceptValue)){
			return false;
		}
		offset += m_acceptValue.size() + 4;

		*rSize = offset;
		return true;
	}
};

/**
 * @brief accept请求协议
 * 
 */
struct AcceptMessage : public Message{
	PPeer m_myInfo;
	PProposalID m_proposalID;
	std::string m_proposalValue;

	AcceptMessage(uint16_t protoVersion = PAXOS_PROTO, uint32_t protoId = PAXOS_PROTO_PING_MESSAGE):
		Message(protoVersion,protoId){}
	~AcceptMessage(){}

	virtual void EncodeToString(std::string* str) const{
		m_proposalID.EncodeToString(str);
		m_myInfo.EncodeToString(str);
		Serializer::PutString(m_proposalValue, str);
	}

	virtual bool DecodeFromArray(const char* buf, size_t size, size_t* rSize){
		size_t offset = 0;

		if(!m_proposalID.DecodeFromArray(buf + offset, size-offset, rSize)){
			return false;
		}
		offset += *rSize;

		if(!m_myInfo.DecodeFromArray(buf + offset, size-offset, rSize)){
			return false;
		}
		offset += *rSize;

		if(!Serializer::GetString(buf + offset, size-offset, &m_proposalValue)){
			return false;
		}
		offset += m_proposalValue.size() + 4;

		*rSize = offset;
		return true;
	}
};


/**
 * @brief accept请求的响应
 * 
 */
struct PermitMessage : public Message{
	PPeer m_myInfo;
	PProposalID m_proposalID;
	std::string m_acceptedValue;

	PermitMessage(uint16_t protoVersion = PAXOS_PROTO, uint32_t protoId = PAXOS_PROTO_PING_MESSAGE):
		Message(protoVersion,protoId){}
	~PermitMessage(){}

	virtual void EncodeToString(std::string* str) const{
		m_proposalID.EncodeToString(str);
		m_myInfo.EncodeToString(str);
		Serializer::PutString(m_acceptedValue, str);
	}

	virtual bool DecodeFromArray(const char* buf, size_t size, size_t* rSize){
		size_t offset = 0;

		if(!m_proposalID.DecodeFromArray(buf + offset, size-offset, rSize)){
			return false;
		}
		offset += *rSize;

		if(!m_myInfo.DecodeFromArray(buf + offset, size-offset, rSize)){
			return false;
		}
		offset += *rSize;

		if(!Serializer::GetString(buf + offset, size-offset, &m_acceptedValue)){
			return false;
		}
		offset += m_acceptedValue.size() + 4;

		*rSize = offset;
		return true;
	}
};
