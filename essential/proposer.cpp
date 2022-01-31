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

void Proposer::setProposal(const std::string& value)
{
    m_proposedValue = value;
}


void Proposer::prepare()
{
    //清空prepare请求响应者集合
    m_promisesReceived.clear();
    //协议编号加1
    m_proposalID.incrementNumber();
    //发送prepare请求，prepare请求不需要携带议题值，只需要发送议题编号
    m_messenger.sendPrepare(m_proposalID);
}

void Proposer::receivePromise(const std::string& fromUID, const ProposalID& proposalID, 
    const ProposalID& prevAcceptedID, const std::string& prevAcceptedValue)
{
    if(m_proposalID.operator!=(proposalID) || m_promisesReceived.find(fromUID) != m_promisesReceived.end())
    {
        return;
    }
    //当前prepare请求的响应者ID
    m_promisesReceived.insert(fromUID);

    //协议编号大于最近批准的协议编号
    if( !m_lastAcceptedID.isValid() || prevAcceptedID.operator>(m_lastAcceptedID) )
    {
        //更新最近被批准的议题编号
        m_lastAcceptedID = prevAcceptedID;
        //更新最近被批准的议题值
        if(!prevAcceptedValue.empty())
        {
            m_proposedValue = prevAcceptedValue;
        }
    }

    //prepare请求收到了超过半数以上Acceptor的响应，那么可以发送accept请求
    if(m_promisesReceived.size() == m_quorumSize)
    {
        if(!m_proposedValue.empty())
        {
            m_messenger.sendAccept(m_proposalID, m_proposedValue);
        }
    }
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
