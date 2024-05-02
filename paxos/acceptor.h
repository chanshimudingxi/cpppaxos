#pragma once

#include <string>
#include <memory>

#include "proposalid.h"
#include "messenger.h"
#include "sys/util.h"

class Acceptor 
{
public:
	Acceptor(std::shared_ptr<Messenger> messenger, const std::string& acceptorUID, int livenessWindow);
	~Acceptor();

	void receivePrepare(const std::string& fromUID, const ProposalID& proposalID);
	void receiveAcceptRequest(const std::string& fromUID, const ProposalID& proposalID, 
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
	std::string  m_acceptorUID;
	//保活窗口的大小，单位微秒
	uint64_t m_livenessWindow; 
	//对prepare请求做出承诺的议题编号
	ProposalID m_promisedID;
	//已经对Proposer(m_pendingPromiseUID)的prepare请求做出承诺
	std::string  m_pendingPromiseUID;
	//对prepare请求做出承诺的时间戳
	uint64_t m_lastPrepareTimestamp;
	//已经批准的议题的最大编号
	ProposalID m_acceptedID;
	//已经批准的议题value
	std::string m_acceptedValue;
	//等待接收该Proposer(m_pendingAcceptUID)的accept请求的
	std::string  m_pendingAcceptUID;
	
	bool m_active;
};
