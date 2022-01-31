#include "accepter.h"

Acceptor::Acceptor(const Messenger& messenger)
{
    m_messenger = messenger;
}

void Acceptor::receivePrepare(const std::string& fromUID, const ProposalID& proposalID) 
{
    if (m_promisedID.isValid() && proposalID.operator==(m_promisedID)) 
    { // duplicate message
        m_messenger.sendPromise(fromUID, proposalID, m_acceptedID, m_acceptedValue);
    }
    else if (!m_promisedID.isValid() || proposalID.operator>(m_promisedID)) 
    {
        m_promisedID = proposalID;
        m_messenger.sendPromise(fromUID, proposalID, m_acceptedID, m_acceptedValue);
    }
}

void Acceptor::receiveAcceptRequest(const std::string& fromUID, const ProposalID& proposalID, const std::string& value) 
{
    if (!promisedID.isValid() || 
        proposalID.operator>(m_promisedID) || 
        proposalID.operator==(m_promisedID) ) 
    {
        m_promisedID    = proposalID;
        m_acceptedID    = proposalID;
        m_acceptedValue = value;
        
        m_messenger.sendAccepted(m_acceptedID, m_acceptedValue);
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
