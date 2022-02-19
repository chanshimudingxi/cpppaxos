#include "node.h"

Node::Node(const Messenger& messenger, const std::string& proposerUID, int quorumSize, const std::string& leaderUID, 
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

long Node::timestamp() 
{
	return (long)Util::GetMonoTimeMs();
}

std::string Node::getLeaderUID()
{
	return m_leaderUID;
}

ProposalID Node::getLeaderProposalID() 
{
	return m_leaderProposalID;
}

void Node::setLeaderProposalID( const ProposalID& newLeaderID )
{
	m_leaderProposalID = newLeaderID;
}

bool Node::isAcquiringLeadership() 
{
	return m_acquiringLeadership;
}

void Node::prepare(bool incrementProposalNumber)
{
	if (incrementProposalNumber)
		m_acceptNACKs.clear();
	Paxos::prepare(incrementProposalNumber);
}

bool Node::leaderIsAlive() 
{
	return timestamp() - m_lastHeartbeatTimestamp <= m_livenessWindow;
}

bool Node::observedRecentPrepare()
{
	return timestamp() - m_lastPrepareTimestamp <= m_livenessWindow * 1.5;
}

void Node::pollLiveness() 
{
	if (!leaderIsAlive() && !observedRecentPrepare()) 
	{
		if (m_acquiringLeadership)
			prepare();
		else
			acquireLeadership();
	}
}

void Node::receiveHeartbeat(const std::string& fromUID, const ProposalID& proposalID)
{
	
	if (!m_leaderProposalID.isValid() || proposalID.isGreaterThan(m_leaderProposalID)) {
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
	
	if (m_leaderProposalID.isValid() && m_leaderProposalID.equals(proposalID))
		m_lastHeartbeatTimestamp = timestamp();
}

void Node::pulse()
{
	if (isLeader()) {
		receiveHeartbeat(getProposerUID(), getProposalID());
		m_messenger.sendHeartbeat(getProposalID());
		m_messenger.schedule(m_heartbeatPeriod, new HeartbeatCallback (){ void execute() { pulse();}});
	}
}

void Node::acquireLeadership() 
{
	if (leaderIsAlive())
		m_acquiringLeadership = false;
	else {
		m_acquiringLeadership = true;
		prepare();
	}
}

void Node::receivePrepare(const std::string& fromUID, const ProposalID& proposalID)
{
	Paxos::receivePrepare(fromUID, proposalID);
	if (!proposalID.equals(getProposalID()))
		m_lastPrepareTimestamp = timestamp();
}

void Node::receivePromise(const std::string& fromUID, const ProposalID& proposalID, const ProposalID& prevAcceptedID, const std::string& prevAcceptedValue)
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

void Node::receivePrepareNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID)
{
	Paxos::receivePrepareNACK(proposerUID, proposalID, promisedID);
	
	if (m_acquiringLeadership)
	{
		prepare();
	}		
}

void Node::receiveAcceptNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID)
{
	Paxos::receiveAcceptNACK(proposerUID, proposalID, promisedID);
	
	if (proposalID.equals(getProposalID()))
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
