#include "acceptor.h"

Acceptor::Acceptor(std::shared_ptr<Messenger> messenger, const std::string& acceptorID, int livenessWindow)
{
    m_messenger = messenger;
	m_acceptorID = acceptorID;
	m_livenessWindow = livenessWindow;
	m_lastPrepareTimestamp   = Util::GetMonoTimeUs();
}

Acceptor::~Acceptor(){}

/**
 * @brief 接收到prepare请求
 * 
 * @param fromID Acceptor的ID
 * @param proposalID 议题编号
 */
void Acceptor::receivePrepare(const std::string& fromID, const ProposalID& proposalID) 
{
	if (m_promisedID.isValid() && proposalID == m_promisedID) 
	{ // duplicate message
		if (m_active)
		{
			m_messenger->sendPromise(fromID, proposalID, m_acceptedID, m_acceptedValue);
		}
	}
	else if (!m_promisedID.isValid() || proposalID > m_promisedID) 
	{
		if (m_pendingPromise.empty()) 
		{
			m_promisedID = proposalID;
			if (m_active)
			{
				m_pendingPromise = fromID;
			}
		}
	}
	else
	{
		if (m_active)
		{
			m_messenger->sendPrepareNACK(fromID, proposalID, m_promisedID);
		}
	}
	m_lastPrepareTimestamp = Util::GetMonoTimeUs();
}

/**
 * @brief 接收到accept请求
 * 
 * @param fromID Proposer的ID
 * @param proposalID 议题编号
 * @param value 议题value
 */
void Acceptor::receiveAcceptRequest(const std::string& fromID, const ProposalID& proposalID, 
	const std::string& value) 
{
	if (m_acceptedID.isValid() && proposalID == m_acceptedID && m_acceptedValue == value) 
	{
		if (m_active)
		{
			m_messenger->sendAccepted(proposalID, value);
		}
	}
	else if (!m_promisedID.isValid() || proposalID > m_promisedID || proposalID == m_promisedID) 
	{
		if (m_pendingAccepted.empty()) 
		{
			m_promisedID    = proposalID;
			m_acceptedID    = proposalID;
			m_acceptedValue = value;
			
			if (m_active)
			{
				m_pendingAccepted = fromID;
			}
		}
	}
	else 
	{
		if (m_active)
		{
			m_messenger->sendAcceptNACK(fromID, proposalID, m_promisedID);
		}
	}
}

bool Acceptor::isPrepareExpire()
{
	uint64_t waitTime = Util::GetMonoTimeUs() - m_lastPrepareTimestamp;
	return waitTime > m_livenessWindow;
}

ProposalID Acceptor::getPromisedID() 
{
    return m_promisedID;
}

ProposalID Acceptor::getAcceptedID() 
{
    return m_acceptedID;
}

std::string Acceptor::getAcceptedValue() 
{
    return m_acceptedValue;
}


bool Acceptor::persistenceRequired() 
{
	return !m_pendingAccepted.empty() || !m_pendingPromise.empty();
}
	

void Acceptor::recover(const ProposalID& promisedID, const ProposalID& acceptedID, const std::string& acceptedValue) 
{
	m_promisedID    = promisedID;
	m_acceptedID    = acceptedID;
	m_acceptedValue = acceptedValue;
}

void Acceptor::persisted() 
{
	if (m_active) 
	{
		if (!m_pendingPromise.empty())
		{
			m_messenger->sendPromise(m_pendingPromise, m_promisedID, m_acceptedID, m_acceptedValue);
		}
		if (!m_pendingAccepted.empty())
		{
			m_messenger->sendAccepted(m_acceptedID, m_acceptedValue);
		}
	}
	m_pendingPromise.clear();
	m_pendingAccepted.clear();
}


bool Acceptor::isActive()
{
	return m_active;
}

void Acceptor::setActive(bool active)
{
	m_active = active;
}