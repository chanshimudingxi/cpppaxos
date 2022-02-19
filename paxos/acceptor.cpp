#include "acceptor.h"

Acceptor::Acceptor(const Messenger& messenger)
{
    m_messenger = messenger;
}

void Acceptor::receivePrepare(const std::string& fromUID, const ProposalID& proposalID) 
{
	if (m_promisedID.isValid() && proposalID == m_promisedID) 
	{ // duplicate message
		if (m_active)
		{
			m_messenger.sendPromise(fromUID, proposalID, m_acceptedID, m_acceptedValue);
		}
	}
	else if (!m_promisedID.isValid() || proposalID > m_promisedID) 
	{
		if (m_pendingPromise.empty()) 
		{
			m_promisedID = proposalID;
			if (m_active)
			{
				m_pendingPromise = fromUID;
			}
		}
	}
	else 
	{
		if (m_active)
		{
			m_messenger.sendPrepareNACK(fromUID, proposalID, m_promisedID);
		}
	}
}


void Acceptor::receiveAcceptRequest(const std::string& fromUID, const ProposalID& proposalID, const std::string& value) 
{
	if (m_acceptedID.isValid() && proposalID == m_acceptedID && m_acceptedValue == value) 
	{
		if (m_active)
		{
			m_messenger.sendAccepted(proposalID, value);
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
				m_pendingAccepted = fromUID;
			}
		}
	}
	else 
	{
		if (m_active)
		{
			m_messenger.sendAcceptNACK(fromUID, proposalID, m_promisedID);
		}
	}
}


Messenger Acceptor::getMessenger() 
{
    return m_messenger;
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
			m_messenger.sendPromise(m_pendingPromise, m_promisedID, m_acceptedID, m_acceptedValue);
		}
		if (!m_pendingAccepted.empty())
		{
			m_messenger.sendAccepted(m_acceptedID, m_acceptedValue);
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