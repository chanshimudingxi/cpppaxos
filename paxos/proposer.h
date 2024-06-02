#pragma once

#include "proposalid.h"
#include "messenger.h"

#include <string>
#include <set>

class Proposer
{
public:
    Proposer(Messenger& messenger, const std::string& proposerUID, int quorumSize);
    ~Proposer();

    void prepare(bool incrementProposalNumber);
    void setProposal(const std::string& value);
    void receivePromise(const std::string& fromUID, const ProposalID& proposalID, 
        const ProposalID& prevAcceptedID, const std::string& prevAcceptedValue);
    void observeProposal(const std::string& fromUID, const ProposalID& proposalID);
    void receivePrepareNACK(const std::string& fromUID, const ProposalID& proposalID, 
        const ProposalID& promisedID);
	void receiveAcceptNACK(const std::string& fromUID, const ProposalID& proposalID, 
        const ProposalID& promisedID);
    void resendAccept();

    std::string getProposerUID() const;
    size_t getQuorumSize();
    ProposalID getProposalID() const;
    std::string getProposedValue();
    ProposalID getLastAcceptedID();
    int numPromises();
	bool isLeader() const;
	void setLeader(bool leader);
	bool isActive();
	void setActive(bool active);
private:
    //网络通信接口
    Messenger& m_messenger;
    //Proposer的UID
    std::string m_proposerUID;
    //达成一致要求的最小Acceptor数量
    size_t m_quorumSize;

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