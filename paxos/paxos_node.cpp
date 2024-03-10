#include "paxos_node.h"

PaxosNode::PaxosNode(std::shared_ptr<Messenger> messenger, const std::string& nodeID, 
		int quorumSize, int heartbeatPeriod, int heartbeatTimeout, 
		int livenessWindow, std::string leaderID):
		m_proposer(messenger, nodeID, quorumSize),
		m_acceptor(messenger, nodeID, livenessWindow),
		m_learner(messenger, nodeID, quorumSize),
		m_messenger(messenger)
{
	m_heartbeatPeriod = heartbeatPeriod;
	m_heartbeatTimeout = heartbeatTimeout;
	m_lastHeartbeatTimestamp = Util::GetMonoTimeUs();
	m_leaderID = leaderID;

	if (!m_leaderID.empty() && nodeID == m_leaderID)
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
	return m_proposer.isActive() && m_acceptor.isActive();
}

void PaxosNode::setActive(bool active)
{
	m_proposer.setActive(active);
	m_acceptor.setActive(active);
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
	{
		m_acceptNACKs.clear();
	}
	m_proposer.prepare(incrementProposalNumber);
}

bool PaxosNode::isLeaderAlive() 
{
	return Util::GetMonoTimeUs() - m_lastHeartbeatTimestamp <= m_heartbeatTimeout;
}

bool PaxosNode::isPrepareExpire()
{
	return m_acceptor.isPrepareExpire();
}

void PaxosNode::pollLiveness()
{
	if (!isLeaderAlive() && isPrepareExpire()) 
	{
		if (m_acquiringLeadership)
		{
			prepare(true);
		}
		else
		{
			acquireLeadership();
		}
	}
}

void PaxosNode::receiveHeartbeat(const std::string& fromID, const ProposalID& proposalID)
{
	if (!m_leaderProposalID.isValid() || proposalID > m_leaderProposalID) {
		m_acquiringLeadership = false;
		std::string oldLeaderID = m_leaderID;
		
		m_leaderID        = fromID;
		m_leaderProposalID = proposalID;
		
		if (m_proposer.isLeader() && fromID != m_proposer.getProposerID()) {
			m_proposer.setLeader(false);
			m_messenger->onLeadershipLost();
			m_proposer.observeProposal(fromID, proposalID);
		}
		
		m_messenger->onLeadershipChange(oldLeaderID, fromID);
	}
	
	if (m_leaderProposalID.isValid() && m_leaderProposalID == proposalID)
	{
		m_lastHeartbeatTimestamp = Util::GetMonoTimeUs();
	}
}

void PaxosNode::pulse()
{
	if (m_proposer.isLeader()) 
	{
		receiveHeartbeat(m_proposer.getProposerID(), m_proposer.getProposalID());
		m_messenger->sendHeartbeat(m_proposer.getProposalID());
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

void PaxosNode::receivePrepare(const std::string& fromID, const ProposalID& proposalID)
{
	m_acceptor.receivePrepare(fromID, proposalID);
}

void PaxosNode::receivePromise(const std::string& fromID, const ProposalID& proposalID, const ProposalID& prevAcceptedID, const std::string& prevAcceptedValue)
{
	std::string preLeaderID = m_leaderID;
	
	m_proposer.receivePromise(fromID, proposalID, prevAcceptedID, prevAcceptedValue);
	
	if (preLeaderID.empty() && m_proposer.isLeader()) 
	{
		std::string oldLeaderID = m_proposer.getProposerID();
		
		m_leaderID           = m_proposer.getProposerID();
		m_leaderProposalID    = m_proposer.getProposalID();
		m_acquiringLeadership = false;
		
		pulse();
		
		m_messenger->onLeadershipChange(oldLeaderID, m_leaderID);
	}
}

void PaxosNode::receivePrepareNACK(const std::string& proposerID, const ProposalID& proposalID, const ProposalID& promisedID)
{
	m_proposer.receivePrepareNACK(proposerID, proposalID, promisedID);
	
	if (m_acquiringLeadership)
	{
		prepare(true);
	}		
}

void PaxosNode::receiveAcceptNACK(const std::string& proposerID, const ProposalID& proposalID, const ProposalID& promisedID)
{
	m_proposer.receiveAcceptNACK(proposerID, proposalID, promisedID);
	
	if (proposalID == m_proposer.getProposalID())
		m_acceptNACKs.insert(proposerID);
	
	if (m_proposer.isLeader() && m_acceptNACKs.size() >= m_proposer.getQuorumSize()) 
	{
		m_proposer.setLeader(false);
		m_leaderID = "";
		m_leaderProposalID = ProposalID();
		m_messenger->onLeadershipLost();
		m_messenger->onLeadershipChange(m_proposer.getProposerID(), m_leaderID);
		m_proposer.observeProposal(proposerID, proposalID);
	}
}
