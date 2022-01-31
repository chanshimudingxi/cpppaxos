#pragma once

#include "proposal_id.h"
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
    void setProposal(const std::string& value);
    //prepare请求
    void prepare();
    /*
     *收到prepare请求的响应
     *@proposalID 议题编号
     *@prevAcceptedID 最大批准议题编号
     *@prevAcceptedValue 最大批准议题编号对应的议题值
     */
    void receivePromise(const std::string& fromUID, const ProposalID& proposalID, 
        const ProposalID& prevAcceptedID, const std::string& prevAcceptedValue);

    Messenger getMessenger();
    std::string getProposerUID();
    int getQuorumSize();
    ProposalID getProposalID();
    std::string getProposedValue();
    ProposalID getLastAcceptedID();
    int numPromises();
private:
    Messenger m_messenger;
    std::string m_proposerUID;
    int m_quorumSize;
    ProposalID m_proposalID;    //prepare请求的议题编号
    std::string m_proposedValue;    //Proposer提出议题的议题值（可以使Acceptor返回的最近被批准的议题值）
    ProposalID m_lastAcceptedID;    //最近被批准的议题编号
    std::set<std::string> m_promisesReceived;   //prepare请求响应者Acceptor的ID
};