#include "learner.h"

Learner::Learner(std::shared_ptr<Messenger> messenger, const std::string& learnerID, int quorumSize ) 
{
    m_messenger  = messenger;
    m_learnerID = learnerID;
    m_quorumSize = quorumSize;
}

Learner::~Learner(){}

/**
 * @brief 整个状态机是否已经结束
 * 
 * @return true 
 * @return false 
 */
bool Learner::isComplete() 
{
	//议题已经达成最终一致
    return !m_finalValue.empty();
}

/**
 * @brief 
 * @fromID Acceptor的ID
 * @proposalID accept请求携带的议题编号
 * @acceptedValue accept请求携带的议题值
 */
void Learner::receiveAccepted(const std::string& fromID, const ProposalID& proposalID, 
    const std::string& acceptedValue) 
{
	//状态机已经结束
    if (isComplete())
    {
        return;
    }

    if(!proposalID.isValid())
    {
        return;
    }

	//Acceptor的新议题的编号小于等于老议题编号，直接丢弃accept请求
    auto itrOld = m_acceptors.find(fromID);
    if(itrOld != m_acceptors.end() && (proposalID < itrOld->second || proposalID == itrOld->second))
    {
        return;
    }
    //记录Acceptor批准的新的议题状态
    m_acceptors[fromID] = proposalID;
    
    //更新老议题的状态
    if (itrOld != m_acceptors.end())
    {
		Proposal& oldProposal = m_proposals[itrOld->second];
		oldProposal.m_retentionCount -= 1;
		if (oldProposal.m_retentionCount == 0)
		{
			m_proposals.erase(itrOld->second);
		}
    }
    
	//更新新议题的状态
    auto itrNew = m_proposals.find(proposalID);
    if (itrNew == m_proposals.end())
    {
        m_proposals.insert(std::make_pair(proposalID, Proposal(0, 0, acceptedValue)));
	}
    else
    {
        itrNew->second.m_acceptCount    += 1;
        itrNew->second.m_retentionCount += 1;
        //批准个数满足大多数的条件
        if (itrNew->second.m_acceptCount >= m_quorumSize) 
        {
            m_finalProposalID = proposalID;
            m_finalValue      = acceptedValue;
            m_proposals.clear();
            m_acceptors.clear();
            
            m_messenger->onResolution(proposalID, acceptedValue);
        }
    }
}

std::string Learner::getFinalValue() 
{
    return m_finalValue;
}

ProposalID Learner::getFinalProposalID() 
{
    return m_finalProposalID;
}

int Learner::getQuorumSize() 
{
    return m_quorumSize;
}