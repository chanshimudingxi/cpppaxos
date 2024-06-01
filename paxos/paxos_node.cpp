#include "paxos_node.h"

#include <functional>

#include "sys/util.h"

PaxosNode::PaxosNode(Messenger& messenger, const std::string& nodeUID, 
		int quorumSize, int heartbeatPeriod, int heartbeatTimeout, 
		int livenessWindow, std::string leaderUID):
		m_messenger(messenger),
		m_proposer(messenger, nodeUID, quorumSize),
		m_acceptor(messenger, nodeUID, livenessWindow),
		m_learner(messenger, nodeUID, quorumSize)
{
	m_heartbeatPeriod = heartbeatPeriod;
	m_heartbeatTimeout = heartbeatTimeout;
	m_lastHeartbeatTimestamp = deps::GetMonoTimeUs();
	m_leaderUID = leaderUID;
	m_nodeUID = nodeUID;

	if (!m_leaderUID.empty() && m_nodeUID == m_leaderUID)
	{
		m_proposer.setLeader(true);
	}

	m_acquiringLeadership = false;
}

PaxosNode::~PaxosNode()
{

}


bool PaxosNode::isActive()
{
	bool ret = m_proposer.isActive() && m_acceptor.isActive() && m_learner.isActive();
	return ret;
}

void PaxosNode::setActive(bool active)
{
	m_proposer.setActive(active);
	m_acceptor.setActive(active);
	m_learner.setActive(active);
}

ProposalID PaxosNode::getLeaderProposalID() 
{
	return m_leaderProposalID;
}

void PaxosNode::setLeaderProposalID( const ProposalID& newLeaderUID )
{
	m_leaderProposalID = newLeaderUID;
}

bool PaxosNode::isAcquiringLeadership() 
{
	return m_acquiringLeadership;
}

void PaxosNode::prepare(bool incrementProposalNumber)
{
	if (incrementProposalNumber)
	{
		m_acceptNACKs.clear();
	}
	m_proposer.prepare(incrementProposalNumber);
}

bool PaxosNode::isLeaderAlive() 
{
	return deps::GetMonoTimeUs() - m_lastHeartbeatTimestamp <= m_heartbeatTimeout;
}

bool PaxosNode::isPrepareExpire()
{
	return m_acceptor.isPrepareExpire();
}

void PaxosNode::pollLiveness()
{
	if (!isLeaderAlive() && isPrepareExpire()) 
	{
		if (isAcquiringLeadership())
		{
			prepare(true);
		}
		else
		{
			acquireLeadership();
		}
	}
}

void PaxosNode::receiveHeartbeat(const std::string& fromUID, const ProposalID& proposalID)
{
	if (!m_leaderProposalID.isValid() || proposalID > m_leaderProposalID) {
		m_acquiringLeadership = false;
		std::string oldLeaderUID = m_leaderUID;
		
		m_leaderUID        = fromUID;
		m_leaderProposalID = proposalID;
		
		if (m_proposer.isLeader() && fromUID != m_proposer.getProposerUID()) {
			m_proposer.setLeader(false);
			m_messenger.onLeadershipLost();
			m_proposer.observeProposal(fromUID, proposalID);
		}
		
		m_messenger.onLeadershipChange(oldLeaderUID, fromUID);
	}
	
	if (m_leaderProposalID.isValid() && m_leaderProposalID == proposalID)
	{
		m_lastHeartbeatTimestamp = deps::GetMonoTimeUs();
	}
}

void PaxosNode::pulse()
{
	if (m_proposer.isLeader()) 
	{
		receiveHeartbeat(m_proposer.getProposerUID(), m_proposer.getProposalID());
		m_messenger.sendHeartbeat(m_proposer.getProposalID());
	}
}

void PaxosNode::acquireLeadership() 
{
	if (isLeaderAlive())
	{
		m_acquiringLeadership = false;
	}
	else 
	{
		m_acquiringLeadership = true;
		prepare(true);
	}
}

void PaxosNode::receivePrepare(const std::string& fromUID, const ProposalID& proposalID)
{
	m_acceptor.receivePrepare(fromUID, proposalID);
}

void PaxosNode::receivePromise(const std::string& fromUID, const ProposalID& proposalID, const ProposalID& prevAcceptedID, const std::string& prevAcceptedValue)
{
	std::string preLeaderID = m_leaderUID;
	
	m_proposer.receivePromise(fromUID, proposalID, prevAcceptedID, prevAcceptedValue);
	
	if (preLeaderID.empty() && m_proposer.isLeader()) 
	{
		std::string oldLeaderUID = m_proposer.getProposerUID();
		
		m_leaderUID           = m_proposer.getProposerUID();
		m_leaderProposalID    = m_proposer.getProposalID();
		m_acquiringLeadership = false;
		
		pulse();
		
		m_messenger.onLeadershipChange(oldLeaderUID, m_leaderUID);
	}
}

void PaxosNode::receivePrepareNACK(const std::string& fromUID, const ProposalID& proposalID, const ProposalID& promisedID)
{
	m_proposer.receivePrepareNACK(fromUID, proposalID, promisedID);
	
	if (m_acquiringLeadership)
	{
		prepare(true);
	}		
}

void PaxosNode::receiveAcceptNACK(const std::string& fromUID, const ProposalID& proposalID, const ProposalID& promisedID)
{
	m_proposer.receiveAcceptNACK(fromUID, proposalID, promisedID);
	
	if (proposalID == m_proposer.getProposalID())
		m_acceptNACKs.insert(fromUID);
	
	if (m_proposer.isLeader() && m_acceptNACKs.size() >= m_proposer.getQuorumSize()) 
	{
		m_proposer.setLeader(false);
		m_leaderUID = "";
		m_leaderProposalID = ProposalID();
		m_messenger.onLeadershipLost();
		m_messenger.onLeadershipChange(m_proposer.getProposerUID(), m_leaderUID);
		m_proposer.observeProposal(fromUID, proposalID);
	}
}
