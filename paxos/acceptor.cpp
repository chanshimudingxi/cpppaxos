#include "acceptor.h"

Acceptor::Acceptor(Messenger& messenger, const std::string& acceptorUID, int livenessWindow):m_messenger(messenger)
{
	m_acceptorUID = acceptorUID;
	m_livenessWindow = livenessWindow;
	m_lastPrepareTimestamp   = Util::GetMonoTimeUs();
}

Acceptor::~Acceptor(){}

/**
 * @brief 接收到prepare请求
 * 
 * @param fromUID Proposer的UID
 * @param proposalID 议题编号
 */
void Acceptor::receivePrepare(const std::string& fromUID, const ProposalID& proposalID) 
{
	if (m_promisedID.isValid() && proposalID == m_promisedID) 
	{ 
		//已经承诺过这个协议编号
		if (m_active)
		{
			//发送承诺
			m_messenger.sendPromise(fromUID, proposalID, m_acceptedID, m_acceptedValue);
		}
	}
	else if (!m_promisedID.isValid() || proposalID > m_promisedID) 
	{
		//协议编号大于已经承诺的协议编号，拒绝响应。是想要给已经给出承诺的协议一个缓冲时间来提交协议。
		//防止发生连续的prepare请求，导致acceptor不断的承诺新的协议编号。原来的Proposer没有办法
		//只能继续递增协议号，导致新的Proposer无法提交协议。
		if (m_pendingPromiseUID.empty()) 
		{
			m_promisedID = proposalID;
			if (m_active)
			{
				m_pendingPromiseUID = fromUID;
			}
		}
	}
	else
	{
		//协议编号小于已经承诺的协议编号，返回已经承诺的协议编号，这个消息只是个单纯的ACK消息，
		//不是承诺消息，Proposer不能用它来当做发起accept请求的依据。
		if (m_active)
		{
			m_messenger.sendPrepareNACK(fromUID, proposalID, m_promisedID);
		}
	}
	m_lastPrepareTimestamp = Util::GetMonoTimeUs();
}

/**
 * @brief 接收到accept请求
 * 
 * @param fromUID Proposer的UID
 * @param proposalID 议题编号
 * @param value 议题value
 */
void Acceptor::receiveAcceptRequest(const std::string& fromUID, const ProposalID& proposalID, 
	const std::string& value) 
{
	if (m_acceptedID.isValid() && proposalID == m_acceptedID && m_acceptedValue == value) 
	{
		//已经批准过这个协议，包括编号和value都相等
		if (m_active)
		{
			//发送批准
			m_messenger.sendPermit(fromUID, proposalID, value);
		}
	}
	else if (!m_promisedID.isValid() || proposalID > m_promisedID || proposalID == m_promisedID) 
	{
		if (m_pendingAcceptUID.empty()) 
		{
			m_promisedID    = proposalID;
			m_acceptedID    = proposalID;
			m_acceptedValue = value;
			
			if (m_active)
			{
				m_pendingAcceptUID = fromUID;
			}
		}
	}
	else 
	{
		if (m_active)
		{
			m_messenger.sendAcceptNACK(fromUID, proposalID, m_promisedID);
		}
	}
}

bool Acceptor::isPrepareExpire()
{
	uint64_t waitTime = Util::GetMonoTimeUs() - m_lastPrepareTimestamp;
	return waitTime > m_livenessWindow;
}

ProposalID Acceptor::getPromisedID() 
{
    return m_promisedID;
}

ProposalID Acceptor::getAcceptedID() 
{
    return m_acceptedID;
}

std::string Acceptor::getAcceptedValue() 
{
    return m_acceptedValue;
}


bool Acceptor::persistenceRequired()
{
	bool ret = !m_pendingAcceptUID.empty() || !m_pendingPromiseUID.empty();
	return ret;
}
	

void Acceptor::recover(const ProposalID& promisedID, const ProposalID& acceptedID, const std::string& acceptedValue) 
{
	m_promisedID    = promisedID;
	m_acceptedID    = acceptedID;
	m_acceptedValue = acceptedValue;
}

void Acceptor::persisted() 
{
	if (m_active) 
	{
		if (!m_pendingPromiseUID.empty())
		{
			m_messenger.sendPromise(m_pendingPromiseUID, m_promisedID, m_acceptedID, m_acceptedValue);
		}
		if (!m_pendingAcceptUID.empty())
		{
			m_messenger.sendPermit(m_pendingAcceptUID, m_acceptedID, m_acceptedValue);
		}
	}
	m_pendingPromiseUID.clear();
	m_pendingAcceptUID.clear();
}


bool Acceptor::isActive()
{
	return m_active;
}

void Acceptor::setActive(bool active)
{
	m_active = active;
}