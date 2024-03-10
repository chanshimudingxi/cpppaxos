#pragma once

#include <string>
#include <memory>

#include "proposalid.h"
#include "messenger.h"
#include "sys/util.h"

class Acceptor 
{
public:
	Acceptor(std::shared_ptr<Messenger> messenger, const std::string& acceptorID, int livenessWindow);
	~Acceptor();

	void receivePrepare(const std::string& fromID, const ProposalID& proposalID);
	void receiveAcceptRequest(const std::string& fromID, const ProposalID& proposalID, 
		const std::string& value);
	bool isPrepareExpire();

	ProposalID getPromisedID();
	ProposalID getAcceptedID();
	std::string getAcceptedValue();

	bool persistenceRequired();
	void recover(const ProposalID& promisedID, const ProposalID& acceptedID, const std::string& acceptedValue);
	void persisted();
	bool isActive();
	void setActive(bool active);
private:
    std::shared_ptr<Messenger> m_messenger;
	std::string  m_acceptorID;
	//保活窗口的大小，单位微秒
	uint64_t m_livenessWindow; 
	//对prepare请求做出承诺的议题编号
	ProposalID m_promisedID;
	//对prepare请求做出承诺的Proposer的ID
	std::string  m_pendingPromise;
	//对prepare请求做出承诺的时间戳
	uint64_t m_lastPrepareTimestamp;
	//已经批准的议题的最大编号
	ProposalID m_acceptedID;    
	std::string m_acceptedValue;
	std::string  m_pendingAccepted;
	
	bool m_active;
};
