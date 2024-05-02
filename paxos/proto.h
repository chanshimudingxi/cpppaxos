#pragma once

#include "net/marshall.h"
#include "sys/util.h"
#include "sys/logger.h"
#include <cstddef>
#include <vector>

enum{
	PAXOS_PROTO_PING_MESSAGE = 1,
	PAXOS_PROTO_PREPARE_MESSAGE = 2,
	PAXOS_PROTO_PROMISE_MESSAGE = 3,
	PAXOS_PROTO_ACCEPT_MESSAGE = 4,
	PAXOS_PROTO_PERMIT_MESSAGE = 5,
	PAXOS_PROTO_PREPARE_ACK_MESSAGE = 6,
	PAXOS_PROTO_ACCEPT_ACK_MESSAGE = 7,
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
};

struct PPeer : public Marshallable{
	std::string m_id;
	std::vector<PPeerAddr> m_addrs;

	virtual void marshal(Pack & pk) const{
		pk << m_id << m_addrs;
	}

	virtual void unmarshal(const Unpack &up){
		up >> m_id >> m_addrs;
	}
};


//peer之间心跳协议
struct HeartbeatMessage : public Marshallable{
	enum{cmd = PAXOS_PROTO_PING_MESSAGE};
	uint64_t m_stamp;
	PPeer m_myInfo;

	virtual void marshal(Pack & pk) const{
		pk << m_stamp << m_myInfo;
	}

	virtual void unmarshal(const Unpack &up){
		up >> m_stamp >> m_myInfo;
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
