#pragma once

#include "net/marshall.h"
#include "sys/util.h"
#include "sys/log.h"
#include <cstddef>
#include <vector>
#include <sstream>
#include <set>

#include "proposalid.h"
#include "peer.h"

enum{
	PAXOS_PROTO_PING_MESSAGE = 1,
	PAXOS_PROTO_PONG_MESSAGE,
	PAXOS_PROTO_HEARTBEAT_MESSAGE,
	PAXOS_PROTO_PREPARE_MESSAGE,
	PAXOS_PROTO_PROMISE_MESSAGE,
	PAXOS_PROTO_ACCEPT_MESSAGE,
	PAXOS_PROTO_PERMIT_MESSAGE,
	PAXOS_PROTO_PREPARE_ACK_MESSAGE,
	PAXOS_PROTO_ACCEPT_ACK_MESSAGE,
};


struct PingMessage : public deps::Marshallable{
	enum{cmd = PAXOS_PROTO_PING_MESSAGE};
	uint64_t m_timestamp;
	PeerInfo m_myInfo;
	std::set<PeerInfo> m_peers;

	virtual void marshal(deps::Pack & pk) const{
		pk << m_timestamp << m_myInfo << m_peers;
	}

	virtual void unmarshal(const deps::Unpack &up){
		up >> m_timestamp >> m_myInfo >> m_peers;
	}
};

struct PongMessage : public deps::Marshallable{
	enum{cmd = PAXOS_PROTO_PONG_MESSAGE};
	uint64_t m_timestamp;
	PeerInfo m_myInfo;

	virtual void marshal(deps::Pack & pk) const{
		pk << m_timestamp << m_myInfo;
	}

	virtual void unmarshal(const deps::Unpack &up){
		up >> m_timestamp >> m_myInfo;
	}
};

//master Proposer发给slave Proposer的心跳
struct HeartbeatMessage : public deps::Marshallable{
	enum{cmd = PAXOS_PROTO_HEARTBEAT_MESSAGE};
	PeerInfo m_myInfo;
	ProposalID m_leaderProposalID;

	virtual void marshal(deps::Pack & pk) const{
		pk << m_myInfo << m_leaderProposalID;
	}

	virtual void unmarshal(const deps::Unpack &up){
		up >> m_myInfo >> m_leaderProposalID;
	}
};

/**
 * @brief prepare请求协议
 * 
 */
struct PrepareMessage : public deps::Marshallable{
	enum {cmd = PAXOS_PROTO_PREPARE_MESSAGE};
	PeerInfo m_myInfo;
	ProposalID m_proposalID;
	
	virtual void marshal(deps::Pack & pk) const{
		pk << m_myInfo << m_proposalID;
	}

	virtual void unmarshal(const deps::Unpack &up){
		up >> m_myInfo >> m_proposalID;
	}
};

/**
 * @brief prepare请求的响应
 * 
 */
struct PromiseMessage : public deps::Marshallable{
	enum {cmd = PAXOS_PROTO_PROMISE_MESSAGE};
	PeerInfo m_myInfo;
	ProposalID m_proposalID;
	ProposalID m_acceptID;
	std::string m_acceptValue;

	virtual void marshal(deps::Pack & pk) const{
		pk << m_myInfo << m_proposalID << m_acceptID << m_acceptValue;
	}

	virtual void unmarshal(const deps::Unpack &up){
		up >> m_myInfo >> m_proposalID >> m_acceptID >> m_acceptValue;
	}
};

/**
 * @brief accept请求协议
 * 
 */
struct AcceptMessage : public deps::Marshallable{
	enum {cmd = PAXOS_PROTO_ACCEPT_MESSAGE};
	PeerInfo m_myInfo;
	ProposalID m_proposalID;
	std::string m_proposalValue;

	virtual void marshal(deps::Pack & pk) const{
		pk << m_myInfo << m_proposalID << m_proposalValue;
	}

	virtual void unmarshal(const deps::Unpack &up){
		up >> m_myInfo >> m_proposalID >> m_proposalValue;
	}
};


/**
 * @brief accept请求的响应
 */
struct PermitMessage : public deps::Marshallable{
	enum {cmd = PAXOS_PROTO_PERMIT_MESSAGE};
	PeerInfo m_myInfo;
	ProposalID m_proposalID;
	std::string m_acceptedValue;

	virtual void marshal(deps::Pack & pk) const{
		pk << m_myInfo << m_proposalID << m_acceptedValue;
	}

	virtual void unmarshal(const deps::Unpack &up){
		up >> m_myInfo >> m_proposalID >> m_acceptedValue;
	}
};


/**
 * @brief Acceptor返回一些本地关键信息给Proposer做决策，不是承诺也不是批准
 */
struct PrepareAckMessage : public deps::Marshallable{
	enum {cmd=PAXOS_PROTO_PREPARE_ACK_MESSAGE};
	PeerInfo m_myInfo;
	ProposalID m_proposalID;
	ProposalID m_promiseID;

	virtual void marshal(deps::Pack & pk) const{
		pk << m_myInfo << m_proposalID << m_promiseID;
	}

	virtual void unmarshal(const deps::Unpack &up){
		up >> m_myInfo >> m_proposalID >> m_promiseID;
	}
};


/**
 * @brief Acceptor返回一些本地关键信息给Proposer做决策，不是承诺也不是批准
 */
struct AcceptAckMessage : public deps::Marshallable{
	enum {cmd = PAXOS_PROTO_ACCEPT_ACK_MESSAGE};
	PeerInfo m_myInfo;
	ProposalID m_proposalID;
	ProposalID m_promiseID;

	virtual void marshal(deps::Pack & pk) const{
		pk << m_myInfo << m_proposalID << m_promiseID;
	}

	virtual void unmarshal(const deps::Unpack &up){
		up >> m_myInfo >> m_proposalID >> m_promiseID;
	}
};

