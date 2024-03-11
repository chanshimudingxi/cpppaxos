#include "proposer.h"

/**
 * @brief Construct a new Proposer:: Proposer object
 * 
 * @param messenger 通信接口
 * @param proposerUID proposer的ID
 * @param quorumSize 达成一致要求的最小Acceptor数量
 */
Proposer::Proposer(std::shared_ptr<Messenger> messenger, const std::string& proposerUID, int quorumSize)
{
    m_messenger = messenger;
    m_proposerID = proposerUID;
    m_quorumSize = quorumSize;
    m_proposalID = ProposalID(0, proposerUID);
}

Proposer::~Proposer()
{

}

/**
 * @brief 根据传入的参数，决定是否要递增议题编号，提出新的prepare请求。或者用原有议题编号提出prepare请求。
 * 	需要提出递增议题编号，提出新的prepare的场景：
 * 		1. 一开始启动的不知道谁是主Proposer的情况。
 * 		2. 感知不到主Proposer active的情况。
 * 		3. 其他情况也可以用，只不过这样相当于恢复到重新加入集群的时候。
 * 
 * @param incrementProposalNumber 是否需要递增议题编号。
 */
void Proposer::prepare( bool incrementProposalNumber ) 
{
	if (incrementProposalNumber) 
	{
		//leader标志回到最初状态
		m_leader = false;
		//active标志回到最初状态
		m_active = true;
		//清空已经收到的Acceptor响应
		m_promisesReceived.clear();
		//其他标志保持不变

		//协议编号加1
		m_proposalID.incrementNumber();
	}
	
	if (m_active)
	{
		//发送prepare请求，prepare请求不需要携带议题value，只需要发送议题编号
		m_messenger->sendPrepare(m_proposalID);
	}
}


/**
 * @brief 设置议题的值
 * 
 * @param value 议题的值
 */
void Proposer::setProposal(const std::string& value)
{
	if (m_proposedValue.empty()) 
	{
		m_proposedValue = value;
		
		//只有leader才能发起accept请求，因为leader表示prepare请求已经收到过大多数Acceptor的批准
		if (m_leader && m_active)
		{
			m_messenger->sendAccept(m_proposalID, m_proposedValue);
		}
	}
}

/**
 * @brief 
 * 
 * @param fromUID Acceptor的ID
 * @param proposalID prepare请求的议题编号
 * @param prevAcceptedID Acceptor当前批准的最大议题编号
 * @param prevAcceptedValue Acceptor当前批准的最大议题编号对应的value
 */
void Proposer::receivePromise(const std::string& fromUID, const ProposalID& proposalID, 
	const ProposalID& prevAcceptedID, const std::string& prevAcceptedValue)
{	
	observeProposal(fromUID, proposalID);

	if (//m_leader ||
		proposalID != m_proposalID || 
		m_promisesReceived.find(fromUID) != m_promisesReceived.end())
	{
		//不是当前议题编号m_proposalID对应的prepare请求的响应，或者已经收到过该Acceptor的响应，直接返回
		return;
	}

	m_promisesReceived.insert( fromUID );

	if (!m_lastAcceptedID.isValid() || prevAcceptedID > m_lastAcceptedID)
	{
		//Acceptor返回的议题编号大于Proposer保存的最大议题编号，更新最大议题的编号和value
		m_lastAcceptedID = prevAcceptedID;
		if (!prevAcceptedValue.empty())
		{
			m_proposedValue = prevAcceptedValue;
		}
	}

	//prepare请求收到了超过半数以上Acceptor的响应，那么可以发送accept请求
	if (m_promisesReceived.size() >= m_quorumSize) 
	{
		//自动成为leader
		m_leader = true;

		//向其他Proposer广播，希望自己的leader得到承认
		m_messenger->onLeadershipAcquired();

		if (!m_proposedValue.empty() && m_active)
		{
			m_messenger->sendAccept(m_proposalID, m_proposedValue);
		}
	}
}

/**
 * @brief 收到Acceptor的NACK响应，会在NACK响应里返回它当前不再批准任何编号小于promisedID的议题。
 * 
 * @param proposerUID Proposer的ID
 * @param proposalID prepare请求的议题编号
 * @param promisedID Acceptor对于所有prepare请求承诺的最大议题编号
 */
void Proposer::receivePrepareNACK(const std::string& fromUID, const ProposalID& proposalID, 
        const ProposalID& promisedID) 
{
	observeProposal(fromUID, promisedID);
}

void Proposer::receiveAcceptNACK(const std::string& proposerUID, 
	const ProposalID& proposalID, const ProposalID& promisedID)
{
}

/**
 * @brief 获取Proposer的ID
 * 
 * @return 协议号
 */
std::string Proposer::getProposerID() 
{
    return m_proposerID;
}

/**
 * @brief 获取共识机制要求的“大多数”的数量值
 * 
 * @return "大多数"的值 
 */

int Proposer::getQuorumSize() 
{
    return m_quorumSize;
}

/**
 * @brief 获取协议号ID
 * 
 * @return 协议号ID 
 */
ProposalID Proposer::getProposalID() 
{
    return m_proposalID;
}

/**
 * @brief 获取协议值
 * 
 * @return 协议值
 */
std::string Proposer::getProposedValue() 
{
    return m_proposedValue;
}

/**
 * @brief 获取所有Acceptor返回的批准的最大协议号ID
 * 
 * @return ProposalID 
 */
ProposalID Proposer::getLastAcceptedID() 
{
    return m_lastAcceptedID;
}

/**
 * @brief 获取返回prepare响应的Acceptor个数
 */
int Proposer::numPromises() 
{
    return m_promisesReceived.size();
}

/**
 * @brief 更新自己收到Acceptor已经批准的最大议题编号
 * 
 * @param fromUID 	acceptor的ID
 * @param proposalID 协议号
 */
void Proposer::observeProposal(const std::string& fromUID, const ProposalID& proposalID) 
{
	if (proposalID > m_proposalID)
	{
		m_proposalID.setNumber(proposalID.getNumber());
	}
}


/**
 * @brief 重发accept请求
 * 
 */
void Proposer::resendAccept() 
{
	if (m_leader && m_active && !m_proposedValue.empty())
	{		
		m_messenger->sendAccept(m_proposalID, m_proposedValue);
	}
}

/**
 * @brief 自己是否为Proposer的leader，只有leader才能提出议题
 * 
 */
bool Proposer::isLeader() 
{
	return m_leader;
}

void Proposer::setLeader(bool leader)
{
	m_leader = leader;
}

bool Proposer::isActive() 
{
	return m_active;
}

void Proposer::setActive(bool active) 
{
	m_active = active;
}
