#include "paxosnode.h"

PaxosNode::PaxosNode(const Messenger& messenger, const std::string& proposerUID, int quorumSize)
{
	m_proposerPtr = new Proposer(messenger, proposerUID, quorumSize);
	m_acceptorPtr = new Acceptor(messenger);
	m_learnerPtr  = new Learner(messenger, quorumSize);
}

PaxosNode::~PaxosNode()
{

}
	
bool PaxosNode::isActive() 
{
	return m_proposerPtr->isActive();
}

void PaxosNode::setActive(bool active)
{
	m_proposerPtr->setActive(active);
	m_acceptorPtr->setActive(active);
}

//-------------------------------------------------------------------------
// Learner
//
bool PaxosNode::isComplete() 
{
	return m_learnerPtr->isComplete();
}

void PaxosNode::receiveAccepted(const std::string& fromUID, const ProposalID& proposalID, const std::string& acceptedValue)
{
	m_learnerPtr->receiveAccepted(fromUID, proposalID, acceptedValue);
}

std::string PaxosNode::getFinalValue() 
{
	return m_learnerPtr->getFinalValue();
}

ProposalID PaxosNode::getFinalProposalID() 
{
	return m_learnerPtr->getFinalProposalID();
}

//-------------------------------------------------------------------------
// Acceptor
//
void PaxosNode::receivePrepare(const std::string& fromUID, const ProposalID& proposalID)
{
	m_acceptorPtr->receivePrepare(fromUID, proposalID);
}

void PaxosNode::receiveAcceptRequest(const std::string& fromUID, const ProposalID& proposalID, const std::string& value)
{
	m_acceptorPtr->receiveAcceptRequest(fromUID, proposalID, value);
}

ProposalID PaxosNode::getPromisedID() 
{
	return m_acceptorPtr->getPromisedID();
}

ProposalID PaxosNode::getAcceptedID() 
{
	return m_acceptorPtr->getAcceptedID();
}

std::string PaxosNode::getAcceptedValue() 
{
	return m_acceptorPtr->getAcceptedValue();
}

bool PaxosNode::persistenceRequired() 
{
	return m_acceptorPtr->persistenceRequired();
}

void PaxosNode::recover(const ProposalID& promisedID, const ProposalID& acceptedID, const std::string& acceptedValue)
{
	m_acceptorPtr->recover(promisedID, acceptedID, acceptedValue);
}

void PaxosNode::persisted() 
{
	m_acceptorPtr->persisted();
}

//-------------------------------------------------------------------------
// Proposer
//
void PaxosNode::setProposal(const std::string& value)
{
	m_proposerPtr->setProposal(value);
}

void PaxosNode::prepare() 
{
	m_proposerPtr->prepare();
}

void PaxosNode::prepare(bool incrementProposalNumber )
{
	m_proposerPtr->prepare(incrementProposalNumber);
}

void PaxosNode::receivePromise(const std::string& fromUID, const ProposalID& proposalID, const ProposalID& prevAcceptedID, const std::string& prevAcceptedValue)
{
	m_proposerPtr->receivePromise(fromUID, proposalID, prevAcceptedID, prevAcceptedValue);
}

Messenger PaxosNode::getMessenger()
{
	return m_proposerPtr->getMessenger();
}

std::string PaxosNode::getProposerUID() 
{
	return m_proposerPtr->getProposerUID();
}

int PaxosNode::getQuorumSize() 
{
	return m_proposerPtr->getQuorumSize();
}

ProposalID PaxosNode::getProposalID() 
{
	return m_proposerPtr->getProposalID();
}

std::string PaxosNode::getProposedValue() 
{
	return m_proposerPtr->getProposedValue();
}

ProposalID PaxosNode::getLastAcceptedID() 
{
	return m_proposerPtr->getLastAcceptedID();
}

int PaxosNode::numPromises() 
{
	return m_proposerPtr->numPromises();
}

void PaxosNode::observeProposal(const std::string& fromUID, const ProposalID& proposalID)
{
	m_proposerPtr->observeProposal(fromUID, proposalID);
}

void PaxosNode::receivePrepareNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID)
{
	m_proposerPtr->receivePrepareNACK(proposerUID, proposalID, promisedID);
}

void PaxosNode::receiveAcceptNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID)
{
	m_proposerPtr->receiveAcceptNACK(proposerUID, proposalID, promisedID);
}

void PaxosNode::resendAccept() 
{
	m_proposerPtr->resendAccept();
}

bool PaxosNode::isLeader()
{
	return m_proposerPtr->isLeader();
}

void PaxosNode::setLeader(bool leader)
{
	m_proposerPtr->setLeader(leader);
}


////////////////////////////////////////////////////////////////心跳/////////////////////////////////////////////

HeartbeatNode::HeartbeatNode(const Messenger& messenger, const std::string& proposerUID, int quorumSize, const std::string& leaderUID, 
	int heartbeatPeriod, int livenessWindow): PaxosNode(messenger, proposerUID, quorumSize)
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

long HeartbeatNode::timestamp() 
{
	return (long)Util::GetMonoTimeMs();
}

std::string HeartbeatNode::getLeaderUID()
{
	return m_leaderUID;
}

ProposalID HeartbeatNode::getLeaderProposalID() 
{
	return m_leaderProposalID;
}

void HeartbeatNode::setLeaderProposalID( const ProposalID& newLeaderID )
{
	m_leaderProposalID = newLeaderID;
}

bool HeartbeatNode::isAcquiringLeadership() 
{
	return m_acquiringLeadership;
}

void HeartbeatNode::prepare(bool incrementProposalNumber)
{
	if (incrementProposalNumber)
		m_acceptNACKs.clear();
	PaxosNode::prepare(incrementProposalNumber);
}

bool HeartbeatNode::leaderIsAlive() 
{
	return timestamp() - m_lastHeartbeatTimestamp <= m_livenessWindow;
}

bool HeartbeatNode::observedRecentPrepare()
{
	return timestamp() - m_lastPrepareTimestamp <= m_livenessWindow * 1.5;
}

void HeartbeatNode::pollLiveness() 
{
	if (!leaderIsAlive() && !observedRecentPrepare()) 
	{
		if (m_acquiringLeadership)
			prepare();
		else
			acquireLeadership();
	}
}

void HeartbeatNode::receiveHeartbeat(const std::string& fromUID, const ProposalID& proposalID)
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

void HeartbeatNode::pulse()
{
	if (isLeader()) {
		receiveHeartbeat(getProposerUID(), getProposalID());
		m_messenger.sendHeartbeat(getProposalID());
		m_messenger.schedule(m_heartbeatPeriod, new HeartbeatCallback (){ void execute() { pulse();}});
	}
}

void HeartbeatNode::acquireLeadership() 
{
	if (leaderIsAlive())
		m_acquiringLeadership = false;
	else {
		m_acquiringLeadership = true;
		prepare();
	}
}

void HeartbeatNode::receivePrepare(const std::string& fromUID, const ProposalID& proposalID)
{
	PaxosNode::receivePrepare(fromUID, proposalID);
	if (!proposalID.equals(getProposalID()))
		m_lastPrepareTimestamp = timestamp();
}

void HeartbeatNode::receivePromise(const std::string& fromUID, const ProposalID& proposalID, const ProposalID& prevAcceptedID, const std::string& prevAcceptedValue)
{
	std::string preLeaderUID = m_leaderUID;
	
	PaxosNode::receivePromise(fromUID, proposalID, prevAcceptedID, prevAcceptedValue);
	
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

void HeartbeatNode::receivePrepareNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID)
{
	PaxosNode::receivePrepareNACK(proposerUID, proposalID, promisedID);
	
	if (m_acquiringLeadership)
	{
		prepare();
	}		
}

void HeartbeatNode::receiveAcceptNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID)
{
	PaxosNode::receiveAcceptNACK(proposerUID, proposalID, promisedID);
	
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
