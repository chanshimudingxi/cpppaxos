#pragma once

#include "messenger.h"
#include "acceptor.h"
#include "proposalid.h"
#include "proposer.h"
#include "learner.h"

class PaxosNode
{
public:
	PaxosNode(Messenger& messenger, const std::string& nodeUID, 
		int quorumSize, int heartbeatPeriod, int heartbeatTimeout, 
		int livenessWindow, std::string leaderUID = "");
	~PaxosNode();

	ProposalID getMyProposalID() const;
	bool isLeader()  const;
	
	bool isActive();
	void setActive(bool active);
	std::string getLeaderUID();
	ProposalID getLeaderProposalID();
	void setLeaderProposalID( const ProposalID& newLeaderUID ); 
	bool isAcquiringLeadership();
	void prepare(bool incrementProposalNumber);
	bool isLeaderAlive();
	bool isPrepareExpire();
	void pollLiveness();
	void receiveHeartbeat(const std::string& fromUID, const ProposalID& proposalID); 
	void pulse();
	void acquireLeadership();
	void receivePrepare(const std::string& fromUID, const ProposalID& proposalID);
	void receivePromise(const std::string& fromUID, const ProposalID& proposalID, 
		const ProposalID& prevAcceptedID, const std::string& prevAcceptedValue);
	void receivePrepareNACK(const std::string& fromUID, const ProposalID& proposalID, 
		const ProposalID& promisedID);
	void receiveAcceptNACK(const std::string& fromUID, const ProposalID& proposalID, 
		const ProposalID& promisedID);
private:
	Messenger& m_messenger;	//通信接口
	Proposer m_proposer;	//proposer状态机
	Acceptor m_acceptor;	//acceptor状态机
	Learner  m_learner;		//learner状态机
	std::string m_nodeUID;	//节点UID

	//leader UID
	std::string	m_leaderUID;
	//leader 协议编号
	ProposalID	m_leaderProposalID;

	//上次心跳的时间戳
	uint64_t	m_lastHeartbeatTimestamp;
	//心跳周期
	uint64_t	m_heartbeatPeriod;
	//心跳超时时间
	uint64_t   	m_heartbeatTimeout;

	//是否需要向集群索要最新的leadership
	bool	m_acquiringLeadership;
	std::set<std::string>	m_acceptNACKs;
};
