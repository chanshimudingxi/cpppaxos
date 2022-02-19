#include "paxosnode.h"

#include <time.h>

PaxosNode::PaxosNode(const Messenger& messenger, const std::string& proposerUID, int quorumSize, const std::string& leaderUID, 
	int heartbeatPeriod, int livenessWindow): Paxos(messenger, proposerUID, quorumSize)
{
	
	m_messenger       = messenger;
	m_leaderUID       = leaderUID;
	m_heartbeatPeriod = heartbeatPeriod;
	m_livenessWindow  = livenessWindow;
	m_lastHeartbeatTimestamp = timestamp();
	m_lastPrepareTimestamp   = timestamp();
	
	if (!m_leaderUID.empty() && proposerUID == m_leaderUID)
	{
		setLeader(true);
	}
}

long PaxosNode::timestamp() 
{
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	uint64_t utime = (uint64_t)time.tv_sec * 1000;
	utime += time.tv_nsec/1000000;

	return  utime;
}

std::string PaxosNode::getLeaderUID()
{
	return m_leaderUID;
}

ProposalID PaxosNode::getLeaderProposalID() 
{
	return m_leaderProposalID;
}

void PaxosNode::setLeaderProposalID( const ProposalID& newLeaderID )
{
	m_leaderProposalID = newLeaderID;
}

bool PaxosNode::isAcquiringLeadership() 
{
	return m_acquiringLeadership;
}

void PaxosNode::prepare(bool incrementProposalNumber)
{
	if (incrementProposalNumber)
		m_acceptNACKs.clear();
	Paxos::prepare(incrementProposalNumber);
}

bool PaxosNode::leaderIsAlive() 
{
	return timestamp() - m_lastHeartbeatTimestamp <= m_livenessWindow;
}

bool PaxosNode::observedRecentPrepare()
{
	return timestamp() - m_lastPrepareTimestamp <= m_livenessWindow * 1.5;
}

void PaxosNode::pollLiveness() 
{
	if (!leaderIsAlive() && !observedRecentPrepare()) 
	{
		if (m_acquiringLeadership)
			prepare();
		else
			acquireLeadership();
	}
}

void PaxosNode::receiveHeartbeat(const std::string& fromUID, const ProposalID& proposalID)
{
	if (!m_leaderProposalID.isValid() || proposalID > m_leaderProposalID) {
		m_acquiringLeadership = false;
		std::string oldLeaderUID = m_leaderUID;
		
		m_leaderUID        = fromUID;
		m_leaderProposalID = proposalID;
		
		if (isLeader() && fromUID != getProposerUID()) {
			setLeader(false);
			m_messenger.onLeadershipLost();
			observeProposal(fromUID, proposalID);
		}
		
		m_messenger.onLeadershipChange(oldLeaderUID, fromUID);
	}
	
	if (m_leaderProposalID.isValid() && m_leaderProposalID == proposalID)
		m_lastHeartbeatTimestamp = timestamp();
}

void PaxosNode::pulse()
{
	if (isLeader()) 
	{
		receiveHeartbeat(getProposerUID(), getProposalID());
		m_messenger.sendHeartbeat(getProposalID());
	}
}

void PaxosNode::acquireLeadership() 
{
	if (leaderIsAlive())
		m_acquiringLeadership = false;
	else 
	{
		m_acquiringLeadership = true;
		prepare();
	}
}

void PaxosNode::receivePrepare(const std::string& fromUID, const ProposalID& proposalID)
{
	Paxos::receivePrepare(fromUID, proposalID);
	if (proposalID != getProposalID())
		m_lastPrepareTimestamp = timestamp();
}

void PaxosNode::receivePromise(const std::string& fromUID, const ProposalID& proposalID, const ProposalID& prevAcceptedID, const std::string& prevAcceptedValue)
{
	std::string preLeaderUID = m_leaderUID;
	
	Paxos::receivePromise(fromUID, proposalID, prevAcceptedID, prevAcceptedValue);
	
	if (preLeaderUID.empty() && isLeader()) 
	{
		std::string oldLeaderUID = getProposerUID();
		
		m_leaderUID           = getProposerUID();
		m_leaderProposalID    = getProposalID();
		m_acquiringLeadership = false;
		
		pulse();
		
		m_messenger.onLeadershipChange(oldLeaderUID, m_leaderUID);
	}
}

void PaxosNode::receivePrepareNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID)
{
	Paxos::receivePrepareNACK(proposerUID, proposalID, promisedID);
	
	if (m_acquiringLeadership)
	{
		prepare();
	}		
}

void PaxosNode::receiveAcceptNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID)
{
	Paxos::receiveAcceptNACK(proposerUID, proposalID, promisedID);
	
	if (proposalID == getProposalID())
		m_acceptNACKs.insert(proposerUID);
	
	if (isLeader() && m_acceptNACKs.size() >= getQuorumSize()) 
	{
		setLeader(false);
		m_leaderUID = "";
		m_leaderProposalID = ProposalID();
		m_messenger.onLeadershipLost();
		m_messenger.onLeadershipChange(getProposerUID(), m_leaderUID);
		observeProposal(proposerUID, proposalID);
	}
}
