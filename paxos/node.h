#pragma once

#include "paxos.h"

class Node : public Paxos
{
public:
	Node(const Messenger& messenger, const std::string& proposerUID, int quorumSize, const std::string& leaderUID, int heartbeatPeriod, int livenessWindow);
	~Node(){}

	long timestamp();
	std::string getLeaderUID();
	ProposalID getLeaderProposalID();
	void setLeaderProposalID( const ProposalID& newLeaderID ); 
	bool isAcquiringLeadership();
	void prepare(bool incrementProposalNumber);
	bool leaderIsAlive();
	bool observedRecentPrepare();
	void pollLiveness();
	void receiveHeartbeat(const std::string& fromUID, const ProposalID& proposalID); 
	void pulse();
	void acquireLeadership();
	void receivePrepare(const std::string& fromUID, const ProposalID& proposalID);
	void receivePromise(const std::string& fromUID, const ProposalID& proposalID, const ProposalID& prevAcceptedID, const std::string& prevAcceptedValue);
	void receivePrepareNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID);
	void receiveAcceptNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID);
private:
	std::string	m_leaderUID;
	ProposalID	m_leaderProposalID;
	long	m_lastHeartbeatTimestamp;
	long	m_lastPrepareTimestamp;
	long	m_heartbeatPeriod         = 1000; // Milliseconds
	long	m_livenessWindow          = 5000; // Milliseconds
	bool	m_acquiringLeadership     = false;
	std::set<std::string>	m_acceptNACKs;

	Messenger m_messenger;
};