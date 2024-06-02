#include "paxos_node.h"

#include <functional>

#include "sys/util.h"
#include "sys/log.h"

PaxosNode::PaxosNode(Messenger& messenger, const std::string& nodeUID, 
		int quorumSize, int heartbeatPeriod, int heartbeatTimeout, 
		int livenessWindow, std::string leaderUID):
		m_messenger(messenger),
		m_proposer(messenger, nodeUID, quorumSize),
		m_acceptor(messenger, nodeUID, livenessWindow),
		m_learner(messenger, nodeUID, quorumSize)
{
	m_heartbeatPeriod = heartbeatPeriod;
	m_heartbeatTimeout = heartbeatTimeout;
	m_lastHeartbeatTimestamp = deps::GetMonoTimeUs();
	m_leaderUID = leaderUID;
	m_nodeUID = nodeUID;

	if (!m_leaderUID.empty() && m_nodeUID == m_leaderUID)
	{
		m_proposer.setLeader(true);
	}
	
	m_acquiringLeadership = false;
}

PaxosNode::~PaxosNode()
{

}

ProposalID PaxosNode::getMyProposalID() const{
	return m_proposer.getProposalID();
}

bool PaxosNode::isLeader() const{
	return m_proposer.isLeader();
}

bool PaxosNode::isActive()
{
	bool ret = m_proposer.isActive() && m_acceptor.isActive() && m_learner.isActive();
	return ret;
}

void PaxosNode::setActive(bool active)
{
	m_proposer.setActive(active);
	m_acceptor.setActive(active);
	m_learner.setActive(active);
}

std::string PaxosNode::getLeaderUID()
{
	return m_leaderUID;
}

ProposalID PaxosNode::getLeaderProposalID() 
{
	return m_leaderProposalID;
}

void PaxosNode::setLeaderProposalID( const ProposalID& newLeaderUID )
{
	m_leaderProposalID = newLeaderUID;
}

bool PaxosNode::isAcquiringLeadership() 
{
	return m_acquiringLeadership;
}

void PaxosNode::prepare(bool incrementProposalNumber)
{
	if (incrementProposalNumber)
	{
		m_acceptNACKs.clear();
	}
	m_proposer.prepare(incrementProposalNumber);
}

bool PaxosNode::isLeaderAlive() 
{
	return deps::GetMonoTimeUs() - m_lastHeartbeatTimestamp <= m_heartbeatTimeout;
}

bool PaxosNode::isPrepareExpire()
{
	return m_acceptor.isPrepareExpire();
}

void PaxosNode::pollLiveness()
{
	/**
	 * 发送prepare消息的条件：
	 * 1. 本地存储的leader信息已经过期，需要重新获取leader信息，目的是为了后续能发送prepare消息。
	 * 2. prepare时间窗口已经过期，那就有重新prepare的资格。
	*/
	if (!isLeaderAlive() && isPrepareExpire()) 
	{
		if (isAcquiringLeadership())
		{
			/**
			 * 只有leader才能发送prepare消息。虽然这个leader信息可能已经过期，但是不影响算法正确性。
			 * 这里主要是减少prepare的频率，给accept提供完成的时间窗口，不然永远不可能accepted，进入死锁。
			*/
			prepare(true);
		}
		else
		{
			/**
			 * 本地存储的leadership已经过期，需要重新获取leadership，目的是为了后续能发送prepare消息。
			 * 因为只有leader才能发送prepare消息，通过先选举出leader，达到减少prepare的频率。
			*/
			acquireLeadership();
		}
	}
}

void PaxosNode::receiveHeartbeat(const std::string& localLeaderUID, const ProposalID& localLeaderPrososalID)
{
	//第一个收到的议题一定会被批准，或者收到的议题编号大于已经批准的最大议题编号也会被批准
	if (!m_leaderProposalID.isValid() || localLeaderPrososalID > m_leaderProposalID) {
		m_acquiringLeadership = false;
		std::string oldLeaderUID = m_leaderUID;

		LOG_INFO("leadership uid:%s proposalid:%s -> uid:%s proposalid:%s", 
			oldLeaderUID.c_str(), m_leaderProposalID.toString().c_str(),
			localLeaderUID.c_str(), localLeaderPrososalID.toString().c_str());

		m_leaderUID        = localLeaderUID;
		m_leaderProposalID = localLeaderPrososalID;
		
		if (m_proposer.isLeader() && localLeaderUID != m_proposer.getProposerUID()){
			m_proposer.setLeader(false);
			m_messenger.onLeadershipLost();
			m_proposer.observeProposal(localLeaderUID, localLeaderPrososalID);
		}
		
		m_messenger.onLeadershipChange(oldLeaderUID, localLeaderUID);
	}
	
	//说明本地存储的leadership是最新的，等一段时间以后再向集群索要最新的leadership
	if (m_leaderProposalID.isValid() && m_leaderProposalID == localLeaderPrososalID)
	{
		m_lastHeartbeatTimestamp = deps::GetMonoTimeUs();
	}
}

/**
 * 发送心跳的目的是为了选举出集群的leadership
*/
void PaxosNode::pulse()
{
	if (m_proposer.isLeader()) 
	{
		receiveHeartbeat(m_proposer.getProposerUID(), m_proposer.getProposalID());
		m_messenger.sendHeartbeat(m_proposer.getProposerUID(), m_proposer.getProposalID());
	}
}

void PaxosNode::acquireLeadership() 
{
	if (isLeaderAlive())
	{
		m_acquiringLeadership = false;
	}
	else 
	{
		m_acquiringLeadership = true;
		prepare(true);
	}
}

void PaxosNode::receivePrepare(const std::string& fromUID, const ProposalID& proposalID)
{
	m_acceptor.receivePrepare(fromUID, proposalID);
}

void PaxosNode::receivePromise(const std::string& fromUID, const ProposalID& proposalID, const ProposalID& prevAcceptedID, const std::string& prevAcceptedValue)
{
	std::string preLeaderID = m_leaderUID;
	
	m_proposer.receivePromise(fromUID, proposalID, prevAcceptedID, prevAcceptedValue);
	
	if (preLeaderID.empty() && m_proposer.isLeader()) 
	{
		std::string oldLeaderUID = m_proposer.getProposerUID();
		
		m_leaderUID           = m_proposer.getProposerUID();
		m_leaderProposalID    = m_proposer.getProposalID();
		m_acquiringLeadership = false;
		
		pulse();
		
		m_messenger.onLeadershipChange(oldLeaderUID, m_leaderUID);
	}
}

void PaxosNode::receivePrepareNACK(const std::string& fromUID, const ProposalID& proposalID, const ProposalID& promisedID)
{
	m_proposer.receivePrepareNACK(fromUID, proposalID, promisedID);
	
	if (m_acquiringLeadership)
	{
		prepare(true);
	}		
}

void PaxosNode::receiveAcceptNACK(const std::string& fromUID, const ProposalID& proposalID, const ProposalID& promisedID)
{
	m_proposer.receiveAcceptNACK(fromUID, proposalID, promisedID);
	
	if (proposalID == m_proposer.getProposalID())
		m_acceptNACKs.insert(fromUID);
	
	if (m_proposer.isLeader() && m_acceptNACKs.size() >= m_proposer.getQuorumSize()) 
	{
		m_proposer.setLeader(false);
		m_leaderUID = "";
		m_leaderProposalID = ProposalID();
		m_messenger.onLeadershipLost();
		m_messenger.onLeadershipChange(m_proposer.getProposerUID(), m_leaderUID);
		m_proposer.observeProposal(fromUID, proposalID);
	}
}
