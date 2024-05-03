#pragma once

#include "proposalid.h"
#include "messenger.h"

#include <string>
#include <map>

class Learner
{

struct Proposal
{
	Proposal(){}
    Proposal(int acceptCount, int retentionCount, const std::string& value) : 
		m_acceptCount(acceptCount),m_retentionCount(retentionCount),m_value(value){}
	~Proposal(){}
	//只要是批准过该议题的都加1
    int    m_acceptCount;
	//只有当前还保持批准状态才算
    int    m_retentionCount;
    std::string m_value;
};

public:
    Learner(Messenger& messenger, const std::string& learnerUID, int quorumSize);
	~Learner();
	bool isComplete();
	void receiveAccepted(const std::string& fromUID, const ProposalID& proposalID, 
		const std::string& acceptedValue);
		
    std::string getFinalValue();
	ProposalID getFinalProposalID();
	int getQuorumSize();
	bool isActive();
	void setActive(bool active);
private:
	Messenger& m_messenger;
	std::string    m_learnerUID;
	int            m_quorumSize;
	//记录Proposal的状态
	std::map<ProposalID, Proposal> m_proposals;
	//记录Acceptor的状态
	std::map<std::string,  ProposalID>  m_acceptors;
	//最终达成一致的议题值
	std::string m_finalValue;
	//最终达成一致的议题编号
	ProposalID m_finalProposalID;

	bool m_active;
};