#include "learner.h"

Learner::Learner( const Messenger& messenger, int quorumSize ) 
{
    m_messenger  = messenger;
    m_quorumSize = quorumSize;
}

bool Learner::isComplete() 
{
	//议题值已经达成最终一致，说明整个状态机已经可以结束了
    return !m_finalValue.empty();
}

void Learner::receiveAccepted(const std::string& fromUID, const ProposalID& proposalID, const std::string& acceptedValue) 
{
	//状态机已经结束
    if (isComplete())
        return;

	//Acceptor的新议题的编号小于等于老议题编号，直接丢弃accept请求
	ProposalID oldPID = m_acceptors[fromUID];
	if(oldPID.isValid() && !proposalID.operator>(oldPID))
	{
		return;
	}

    //记录Acceptor批准的新的议题状态
    m_acceptors[fromUID] = proposalID;

    if (oldPID.isValid())
    {
		Proposal& oldProposal = m_proposals[oldPID];
		//老议题已经少了一个Acceptor批准
		oldProposal.m_retentionCount -= 1;
		if (oldProposal.m_retentionCount == 0)
		{
			m_proposals.erase(oldPID);
		}
    }
    
	//Proposal中没有这个议题
    if (m_proposals.find(proposalID) == m_proposals.end())
    {
        m_proposals.insert<ProposalID, Proposal>(proposalID, Proposal(0, 0, acceptedValue));
	}

    Proposal& thisProposal = m_proposals[proposalID];	    
    thisProposal.m_acceptCount    += 1;
    thisProposal.m_retentionCount += 1;
	//批准个数满足大多数的条件
    if (thisProposal.m_acceptCount == m_quorumSize) 
    {
        m_finalProposalID = proposalID;
        m_finalValue      = acceptedValue;
        m_proposals.clear();
        m_acceptors.clear();
        
        m_messenger.onResolution(proposalID, acceptedValue);
    }
}

int Learner::getQuorumSize() 
{
    return m_quorumSize;
}

std::string Learner::getFinalValue() 
{
    return m_finalValue;
}

ProposalID Learner::getFinalProposalID() 
{
    return m_finalProposalID;
}
