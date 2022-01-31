#include "learner.h"

Learner::Learner( const Messenger& messenger, int quorumSize ) 
{
    m_messenger  = messenger;
    m_quorumSize = quorumSize;
}

bool Learner::isComplete() 
{
    return !m_finalValue.empty();
}

void Learner::receiveAccepted(const std::string& fromUID, const ProposalID& proposalID, const std::string& acceptedValue) 
{
    if (isComplete())
        return;

    std::map<std::string,  ProposalID>::iterator itr= m_acceptors.find(fromUID);
    if(itr != m_acceptors.end())
    {
        ProposalID oldPID = itr->second;
        if(!proposalID.operator>(oldPID))
        {
            return;
        }
    }
    
    m_acceptors.insert<std::string,  ProposalID>(fromUID, proposalID);

    if (itr != m_acceptors.end()) 
    {
        ProposalID oldPID = itr->second;
        std::map<ProposalID, InnerProposal>::iterator itr2 = m_proposals.find(oldPID);
        if(itr2 != m_proposals.end())
        {
            InnerProposal oldProposal = itr2->second;
            oldProposal.retentionCount -= 1;
            if (oldProposal.retentionCount == 0)
                m_proposals.erase(oldPID);
        }
    }
    
    if (m_proposals.find(proposalID) == m_proposals.end())
    {
        m_proposals.insert<ProposalID, InnerProposal>(proposalID, InnerProposal(0, 0, acceptedValue));
    }

    InnerProposal& thisProposal = m_proposals[proposalID];	
    
    thisProposal.acceptCount    += 1;
    thisProposal.retentionCount += 1;
    
    if (thisProposal.acceptCount == m_quorumSize) 
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
