#include "paxos.h"

Paxos::Paxos(const Messenger& messenger, const std::string& proposerUID, int quorumSize)
{
	m_proposerPtr = new Proposer(messenger, proposerUID, quorumSize);
	m_acceptorPtr = new Acceptor(messenger);
	m_learnerPtr  = new Learner(messenger, quorumSize);
}

Paxos::~Paxos()
{

}
	
bool Paxos::isActive() 
{
	return m_proposerPtr->isActive();
}

void Paxos::setActive(bool active)
{
	m_proposerPtr->setActive(active);
	m_acceptorPtr->setActive(active);
}

//-------------------------------------------------------------------------
// Learner
//
bool Paxos::isComplete() 
{
	return m_learnerPtr->isComplete();
}

void Paxos::receiveAccepted(const std::string& fromUID, const ProposalID& proposalID, const std::string& acceptedValue)
{
	m_learnerPtr->receiveAccepted(fromUID, proposalID, acceptedValue);
}

std::string Paxos::getFinalValue() 
{
	return m_learnerPtr->getFinalValue();
}

ProposalID Paxos::getFinalProposalID() 
{
	return m_learnerPtr->getFinalProposalID();
}

//-------------------------------------------------------------------------
// Acceptor
//
void Paxos::receivePrepare(const std::string& fromUID, const ProposalID& proposalID)
{
	m_acceptorPtr->receivePrepare(fromUID, proposalID);
}

void Paxos::receiveAcceptRequest(const std::string& fromUID, const ProposalID& proposalID, const std::string& value)
{
	m_acceptorPtr->receiveAcceptRequest(fromUID, proposalID, value);
}

ProposalID Paxos::getPromisedID() 
{
	return m_acceptorPtr->getPromisedID();
}

ProposalID Paxos::getAcceptedID() 
{
	return m_acceptorPtr->getAcceptedID();
}

std::string Paxos::getAcceptedValue() 
{
	return m_acceptorPtr->getAcceptedValue();
}

bool Paxos::persistenceRequired() 
{
	return m_acceptorPtr->persistenceRequired();
}

void Paxos::recover(const ProposalID& promisedID, const ProposalID& acceptedID, const std::string& acceptedValue)
{
	m_acceptorPtr->recover(promisedID, acceptedID, acceptedValue);
}

void Paxos::persisted() 
{
	m_acceptorPtr->persisted();
}

//-------------------------------------------------------------------------
// Proposer
//
void Paxos::setProposal(const std::string& value)
{
	m_proposerPtr->setProposal(value);
}

void Paxos::prepare() 
{
	m_proposerPtr->prepare();
}

void Paxos::prepare(bool incrementProposalNumber )
{
	m_proposerPtr->prepare(incrementProposalNumber);
}

void Paxos::receivePromise(const std::string& fromUID, const ProposalID& proposalID, const ProposalID& prevAcceptedID, const std::string& prevAcceptedValue)
{
	m_proposerPtr->receivePromise(fromUID, proposalID, prevAcceptedID, prevAcceptedValue);
}

Messenger Paxos::getMessenger()
{
	return m_proposerPtr->getMessenger();
}

std::string Paxos::getProposerUID() 
{
	return m_proposerPtr->getProposerUID();
}

int Paxos::getQuorumSize() 
{
	return m_proposerPtr->getQuorumSize();
}

ProposalID Paxos::getProposalID() 
{
	return m_proposerPtr->getProposalID();
}

std::string Paxos::getProposedValue() 
{
	return m_proposerPtr->getProposedValue();
}

ProposalID Paxos::getLastAcceptedID() 
{
	return m_proposerPtr->getLastAcceptedID();
}

int Paxos::numPromises() 
{
	return m_proposerPtr->numPromises();
}

void Paxos::observeProposal(const std::string& fromUID, const ProposalID& proposalID)
{
	m_proposerPtr->observeProposal(fromUID, proposalID);
}

void Paxos::receivePrepareNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID)
{
	m_proposerPtr->receivePrepareNACK(proposerUID, proposalID, promisedID);
}

void Paxos::receiveAcceptNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID)
{
	m_proposerPtr->receiveAcceptNACK(proposerUID, proposalID, promisedID);
}

void Paxos::resendAccept() 
{
	m_proposerPtr->resendAccept();
}

bool Paxos::isLeader()
{
	return m_proposerPtr->isLeader();
}

void Paxos::setLeader(bool leader)
{
	m_proposerPtr->setLeader(leader);
}


