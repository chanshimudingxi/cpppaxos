#pragma once

#include "proposal_id.h"
#include <string>
#include <map>

class Learner
{

class InnerProposal 
{
    InnerProposal(int acceptCount, int retentionCount, const std::string& value) 
    {
        m_acceptCount    = acceptCount;
        m_retentionCount = retentionCount;
        m_value          = value;
    }
public:
    int    m_acceptCount;
    int    m_retentionCount;
    std::string m_value;
};

public:
    Learner( const Messenger& messenger, int quorumSize );

	bool isComplete();
	void receiveAccepted(const std::string& fromUID, const ProposalID& proposalID, const std::string& acceptedValue);
	
    int getQuorumSize();
    std::string getFinalValue();
	ProposalID getFinalProposalID();
private:
	Messenger      m_messenger;
	int            m_quorumSize;
	std::map<ProposalID, InnerProposal> m_proposals;
	std::map<std::string,  ProposalID>  m_acceptors;
	std::string m_finalValue;
	ProposalID m_finalProposalID;
};