#pragma once

#include "proposalid.h"
#include "messenger.h"

#include <string>
#include <map>

class Learner
{

class Proposal 
{
    Proposal(int acceptCount, int retentionCount, const std::string& value) 
    {
        m_acceptCount    = acceptCount;
        m_retentionCount = retentionCount;
        m_value          = value;
    }
public:
    int    m_acceptCount;		//批准该议题的Acceptor个数
    int    m_retentionCount;	//批准该老议题的Acceptor个数
    std::string m_value;
};

public:
    Learner( const Messenger& messenger, int quorumSize );
	//整个状态机是否已经结束
	bool isComplete();

	/*处理收到Acceptor发的accept请求
	* @fromUID Acceptor的ID
	* @proposalID accept请求携带的议题编号
	* @acceptedValue accept请求携带的议题值
	*/
	void receiveAccepted(const std::string& fromUID, const ProposalID& proposalID, const std::string& acceptedValue);
	
    int getQuorumSize();
    std::string getFinalValue();
	ProposalID getFinalProposalID();
private:
	Messenger      m_messenger; //通信器
	int            m_quorumSize; //大多数的条件
	std::map<ProposalID, Proposal> m_proposals;	//记录Proposal的状态
	std::map<std::string,  ProposalID>  m_acceptors;	//记录Acceptor的状态
	std::string m_finalValue; //最终一致的议题值
	ProposalID m_finalProposalID;	//最终一致的议题编号
};