#include "proposer.h"

Proposer::Proposer(const Messenger& messenger, const std::string& proposerUID, int quorumSize)
{
    m_messenger = messenger;
    m_proposerUID = proposerUID;
    m_quorumSize = quorumSize;
    m_proposalID = ProposalID(0, proposerUID);
}

Proposer::~Proposer()
{

}

Messenger Proposer::getMessenger() 
{
    return m_messenger;
}

std::string Proposer::getProposerUID() 
{
    return m_proposerUID;
}

int Proposer::getQuorumSize() 
{
    return m_quorumSize;
}

ProposalID Proposer::getProposalID() 
{
    return m_proposalID;
}

std::string Proposer::getProposedValue() 
{
    return m_proposedValue;
}

ProposalID Proposer::getLastAcceptedID() 
{
    return m_lastAcceptedID;
}

int Proposer::numPromises() 
{
    return m_promisesReceived.size();
}

void Proposer::setProposal(const std::string& value)
{
	if ( m_proposedValue.empty()) 
	{
		m_proposedValue = value;
		
		if (m_leader && m_active)
		{
			m_messenger.sendAccept(m_proposalID, m_proposedValue);
		}
	}
}

void Proposer::prepare() 
{
	prepare(true);
}


void Proposer::prepare( bool incrementProposalNumber ) 
{
	if (incrementProposalNumber) 
	{
		m_leader = false;
		    
		//清空prepare请求响应者集合
		m_promisesReceived.clear();
		//协议编号加1
		m_proposalID.incrementNumber();
	}
	
	if (m_active)
	{
		//发送prepare请求，prepare请求不需要携带议题值，只需要发送议题编号
		m_messenger.sendPrepare(m_proposalID);
	}
}

void Proposer::observeProposal(const std::string& fromUID, const ProposalID& proposalID) 
{
	if (proposalID.isGreaterThan(m_proposalID))
	{
		m_proposalID.setNumber(proposalID.getNumber());
	}
}

void Proposer::receivePrepareNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID) 
{
	observeProposal(proposerUID, promisedID);
}

void Proposer::receiveAcceptNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID) 
{
	
}

void Proposer::resendAccept() 
{
	if (m_leader && m_active && !m_proposedValue.empty())
	{		
		m_messenger.sendAccept(m_proposalID, m_proposedValue);
	}
}

void Proposer::receivePromise(const std::string& fromUID, const ProposalID& proposalID, const ProposalID& prevAcceptedID, const std::string& prevAcceptedValue) 
{
	observeProposal(fromUID, proposalID);
	
	if ( m_leader || !proposalID.equals(m_proposalID) || m_promisesReceived.find(fromUID) != m_promisesReceived.end()) 
		return;
	//当前prepare请求的响应者ID
	m_promisesReceived.insert( fromUID );

	//协议编号大于最近批准的协议编号
	if (!m_lastAcceptedID.isValid() || prevAcceptedID.isGreaterThan(m_lastAcceptedID))
	{
		//更新最近被批准的议题编号
		m_lastAcceptedID = prevAcceptedID;
		//更新最近被批准的议题值
		if (!prevAcceptedValue.empty())
		{
			m_proposedValue = prevAcceptedValue;
		}
	}

	//prepare请求收到了超过半数以上Acceptor的响应，那么可以发送accept请求
	if (m_promisesReceived.size() == m_quorumSize) 
	{
		m_leader = true;
		m_messenger.onLeadershipAcquired();
		if (!m_proposedValue.empty() && m_active)
		{
			m_messenger.sendAccept(m_proposalID, m_proposedValue);
		}
	}
}

bool Proposer::isLeader() 
{
	return m_leader;
}

void Proposer::setLeader(bool leader)
{
	m_leader = leader;
}

bool Proposer::isActive() 
{
	return m_active;
}

void Proposer::setActive(bool active) 
{
	m_active = active;
}
