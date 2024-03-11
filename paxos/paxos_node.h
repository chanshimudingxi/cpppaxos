#pragma once

#include "messenger.h"
#include "acceptor.h"
#include "proposalid.h"
#include "proposer.h"
#include "learner.h"

#include "sys/util.h"

class PaxosNode
{
public:
	PaxosNode(std::shared_ptr<Messenger> messenger, const std::string& nodeID, 
		int quorumSize, int heartbeatPeriod, int heartbeatTimeout, 
		int livenessWindow, std::string leaderID = "");
	~PaxosNode();

	bool isActive();
	void setActive(bool active);
	std::string getLeaderID();
	ProposalID getLeaderProposalID();
	void setLeaderProposalID( const ProposalID& newLeaderID ); 
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
	void receivePrepareNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID);
	void receiveAcceptNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID);
private:
	std::shared_ptr<Messenger> m_messenger;
	Proposer m_proposer;
	Acceptor m_acceptor;
	Learner  m_learner;
	std::string m_nodeID;

	std::string	m_leaderID;
	ProposalID	m_leaderProposalID;

	uint64_t	m_lastHeartbeatTimestamp;
	uint64_t	m_heartbeatPeriod;
	uint64_t   	m_heartbeatTimeout;

	bool	m_acquiringLeadership;
	std::set<std::string>	m_acceptNACKs;
};
