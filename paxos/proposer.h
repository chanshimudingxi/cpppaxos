#pragma once

#include "proposalid.h"
#include "messenger.h"

#include <string>
#include <set>

class Proposer
{
public:
    /*
     * @messenger 通信接口
     * @proposerUID Proposer的ID
     * @quorumSize 大多数要求的大小
     */
    Proposer(const Messenger& messenger, const std::string& proposerUID, int quorumSize);
    ~Proposer();

    //设置议题值
    virtual void setProposal(const std::string& value);
    
	//prepare请求
    virtual void prepare();

    /*
     *收到prepare请求的响应
     *@proposalID 议题编号
     *@prevAcceptedID 最大批准议题编号
     *@prevAcceptedValue 最大批准议题编号对应的议题值
     */
    virtual void receivePromise(const std::string& fromUID, const ProposalID& proposalID, const ProposalID& prevAcceptedID, const std::string& prevAcceptedValue);

    Messenger getMessenger();
    std::string getProposerUID();
    int getQuorumSize();
    ProposalID getProposalID();
    std::string getProposedValue();
    ProposalID getLastAcceptedID();
    int numPromises();

	void prepare(bool incrementProposalNumber);
	void observeProposal(const std::string& fromUID, const ProposalID& proposalID);
	void receivePrepareNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID);
	void receiveAcceptNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID);
	void resendAccept();
	bool isLeader();
	void setLeader(bool leader);
	bool isActive();
	void setActive(bool active);
private:
    Messenger m_messenger;
    std::string m_proposerUID;
    int m_quorumSize;
    ProposalID m_proposalID;    //prepare请求的议题编号
    std::string m_proposedValue;    //Proposer提出议题的议题值（可以使Acceptor返回的最近被批准的议题值）
    ProposalID m_lastAcceptedID;    //最近被批准的议题编号
    std::set<std::string> m_promisesReceived;   //prepare请求响应者Acceptor的ID

	bool m_leader;
	bool m_active;
};