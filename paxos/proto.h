#pragma once

#include "net/marshall.h"
#include "sys/util.h"
#include "sys/logger.h"
#include <cstddef>
#include <vector>
#include <sstream>
#include <set>

enum{
	PAXOS_PROTO_PING_MESSAGE = 1,
	PAXOS_PROTO_PING_MESSAGE_RSP,
	PAXOS_PROTO_PREPARE_MESSAGE,
	PAXOS_PROTO_PROMISE_MESSAGE,
	PAXOS_PROTO_ACCEPT_MESSAGE,
	PAXOS_PROTO_PERMIT_MESSAGE,
	PAXOS_PROTO_PREPARE_ACK_MESSAGE,
	PAXOS_PROTO_ACCEPT_ACK_MESSAGE,
};

struct PPeerAddr : public Marshallable{
	uint32_t m_ip;
	uint16_t m_port;
	uint8_t m_socketType;	//0-tcp,1-udp

	virtual void marshal(Pack & pk) const{
		pk << m_ip << m_port << m_socketType;
	}

	virtual void unmarshal(const Unpack &up){
		up >> m_ip >> m_port >> m_socketType;
	}

	bool operator !=(const PPeerAddr& addr) const{
		return m_ip != addr.m_ip || m_port != addr.m_port || m_socketType != addr.m_socketType;
	}
	bool operator ==(const PPeerAddr& addr) const{
		return m_ip == addr.m_ip && m_port == addr.m_port && m_socketType == addr.m_socketType;
	}
	bool operator <(const PPeerAddr& addr) const{
		if(m_ip < addr.m_ip){
			return true;
		}else if(m_ip == addr.m_ip){
			if(m_port < addr.m_port){
				return true;
			}else if(m_port == addr.m_port){
				return m_socketType < addr.m_socketType;
			}
		}
		return false;
	}
	bool operator >(const PPeerAddr& addr) const{
		if(m_ip > addr.m_ip){
			return true;
		}else if(m_ip == addr.m_ip){
			if(m_port > addr.m_port){
				return true;
			}else if(m_port == addr.m_port){
				return m_socketType > addr.m_socketType;
			}
		}
		return false;
	}

	bool operator <=(const PPeerAddr& addr) const{
		return *this < addr || *this == addr;
	}

	bool operator >=(const PPeerAddr& addr) const{
		return *this > addr || *this == addr;
	}

	std::string toString() const{
		std::stringstream os;
		os<<"ip:"<<Util::UintIP2String(m_ip)
			<<" port:"<<m_port
			<<" type:"<<(m_socketType == 0 ? "tcp": "udp");
		return os.str();
	}
};

struct PPeer : public Marshallable{
	std::string m_id;
	PPeerAddr m_addr;

	virtual void marshal(Pack & pk) const{
		pk << m_id << m_addr;
	}

	virtual void unmarshal(const Unpack &up){
		up >> m_id >> m_addr;
	}
};


//peer之间心跳协议
struct HeartbeatMessage : public Marshallable{
	enum{cmd = PAXOS_PROTO_PING_MESSAGE};
	uint64_t m_timestamp;
	PPeer m_myinfo;

	virtual void marshal(Pack & pk) const{
		pk << m_timestamp << m_myinfo;
	}

	virtual void unmarshal(const Unpack &up){
		up >> m_timestamp >> m_myinfo;
	}
};

struct HeartbeatMessageRsp : public Marshallable{
	enum{cmd = PAXOS_PROTO_PING_MESSAGE_RSP};
	uint64_t m_timestamp;
	PPeer m_myinfo;

	virtual void marshal(Pack & pk) const{
		pk << m_timestamp << m_myinfo;
	}

	virtual void unmarshal(const Unpack &up){
		up >> m_timestamp >> m_myinfo;
	}
};

struct PProposalID : public Marshallable{
	uint64_t m_number;
	std::string m_uid;

	virtual void marshal(Pack & pk) const{
		pk << m_number << m_uid;
	}

	virtual void unmarshal(const Unpack &up){
		up >> m_number >> m_uid;
	}
};

/**
 * @brief prepare请求协议
 * 
 */
struct PrepareMessage : public Marshallable{
	enum {cmd = PAXOS_PROTO_PREPARE_MESSAGE};
	PPeer m_myInfo;
	PProposalID m_proposalID;
	
	virtual void marshal(Pack & pk) const{
		pk << m_myInfo << m_proposalID;
	}

	virtual void unmarshal(const Unpack &up){
		up >> m_myInfo >> m_proposalID;
	}
};

/**
 * @brief prepare请求的响应
 * 
 */
struct PromiseMessage : public Marshallable{
	enum {cmd = PAXOS_PROTO_PROMISE_MESSAGE};
	PPeer m_myInfo;
	PProposalID m_proposalID;
	PProposalID m_acceptID;
	std::string m_acceptValue;

	virtual void marshal(Pack & pk) const{
		pk << m_myInfo << m_proposalID << m_acceptID << m_acceptValue;
	}

	virtual void unmarshal(const Unpack &up){
		up >> m_myInfo >> m_proposalID >> m_acceptID >> m_acceptValue;
	}
};

/**
 * @brief accept请求协议
 * 
 */
struct AcceptMessage : public Marshallable{
	enum {cmd = PAXOS_PROTO_ACCEPT_MESSAGE};
	PPeer m_myInfo;
	PProposalID m_proposalID;
	std::string m_proposalValue;

	virtual void marshal(Pack & pk) const{
		pk << m_myInfo << m_proposalID << m_proposalValue;
	}

	virtual void unmarshal(const Unpack &up){
		up >> m_myInfo >> m_proposalID >> m_proposalValue;
	}
};


/**
 * @brief accept请求的响应
 */
struct PermitMessage : public Marshallable{
	enum {cmd = PAXOS_PROTO_PERMIT_MESSAGE};
	PPeer m_myInfo;
	PProposalID m_proposalID;
	std::string m_acceptedValue;

	virtual void marshal(Pack & pk) const{
		pk << m_myInfo << m_proposalID << m_acceptedValue;
	}

	virtual void unmarshal(const Unpack &up){
		up >> m_myInfo >> m_proposalID >> m_acceptedValue;
	}
};


/**
 * @brief Acceptor返回一些本地关键信息给Proposer做决策，不是承诺也不是批准
 */
struct PrepareAckMessage : public Marshallable{
	enum {cmd=PAXOS_PROTO_PREPARE_ACK_MESSAGE};
	PPeer m_myInfo;
	PProposalID m_proposalID;
	PProposalID m_promiseID;

	virtual void marshal(Pack & pk) const{
		pk << m_myInfo << m_proposalID << m_promiseID;
	}

	virtual void unmarshal(const Unpack &up){
		up >> m_myInfo >> m_proposalID >> m_promiseID;
	}
};


/**
 * @brief Acceptor返回一些本地关键信息给Proposer做决策，不是承诺也不是批准
 */
struct AcceptAckMessage : public Marshallable{
	enum {cmd = PAXOS_PROTO_ACCEPT_ACK_MESSAGE};
	PPeer m_myInfo;
	PProposalID m_proposalID;
	PProposalID m_promiseID;

	virtual void marshal(Pack & pk) const{
		pk << m_myInfo << m_proposalID << m_promiseID;
	}

	virtual void unmarshal(const Unpack &up){
		up >> m_myInfo >> m_proposalID >> m_promiseID;
	}
};
