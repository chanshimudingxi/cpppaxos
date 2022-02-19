#pragma once

#include <string>
#include "proposalid.h"
#include "messenger.h"

class Acceptor 
{
public:
    Acceptor(const Messenger& messenger);
    ~Acceptor();

    //接收到prepare请求
    virtual void receivePrepare(const std::string& fromUID, const ProposalID& proposalID);
    //接收到accept请求
	virtual void receiveAcceptRequest(const std::string& fromUID, const ProposalID& proposalID, const std::string& value);

    Messenger getMessenger();
    ProposalID getPromisedID();
    ProposalID getAcceptedID();
    std::string getAcceptedValue();

	bool persistenceRequired();
	void recover(const ProposalID& promisedID, const ProposalID& acceptedID, const std::string& acceptedValue);
	void persisted();
	bool isActive();
	void setActive(bool active);
private:
    Messenger m_messenger;
	ProposalID m_promisedID;    //已经做出prepare请求响应的议题的最大编号
	ProposalID m_acceptedID;    //已经批准的议题的最大编号
	std::string m_acceptedValue;

	std::string  m_pendingAccepted;
	std::string  m_pendingPromise;
	bool m_active;
};
