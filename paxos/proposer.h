#pragma once

#include "proposalid.h"
#include "messenger.h"

#include <string>
#include <set>

class Proposer
{
public:
    Proposer(std::shared_ptr<Messenger> messenger, const std::string& proposerID, int quorumSize);
    ~Proposer();

    void prepare(bool incrementProposalNumber);
    void setProposal(const std::string& value);
    void receivePromise(const std::string& fromID, const ProposalID& proposalID, 
        const ProposalID& prevAcceptedID, const std::string& prevAcceptedValue);
    void observeProposal(const std::string& fromID, const ProposalID& proposalID);
    void receivePrepareNACK(const std::string& fromID, const ProposalID& proposalID, 
        const ProposalID& promisedID);
	void receiveAcceptNACK(const std::string& proposerID, const ProposalID& proposalID, const ProposalID& promisedID);
    void resendAccept();

    std::string getProposerID();
    int getQuorumSize();
    ProposalID getProposalID();
    std::string getProposedValue();
    ProposalID getLastAcceptedID();
    int numPromises();
	bool isLeader();
	void setLeader(bool leader);
	bool isActive();
	void setActive(bool active);
private:
    //网络通信接口
    std::shared_ptr<Messenger> m_messenger;
    //Proposer的ID
    std::string m_proposerID;
    //达成一致要求的最小Acceptor数量
    int m_quorumSize;

    //提出议题的编号
    ProposalID m_proposalID;
    //提出议题的value
    std::string m_proposedValue;
    //Acceptor批准的最大的议题编号
    ProposalID m_lastAcceptedID;

    //对当前prepare请求进行承诺的Acceptor列表
    std::set<std::string> m_promisesReceived;
    //是否是leader
	bool m_leader;
    //是否是活跃的
	bool m_active;
};