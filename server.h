#pragma once

#include <cstring>
#include <iostream>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include <memory>
#include <sstream>

#include "net/tcp_socket.h"
#include "net/udp_socket.h"
#include "net/epoll_container.h"
#include "net/packet_handler.h"
#include "net/packet.h"
#include "sys/log.h"

#include "paxos/proto.h"
#include "paxos/paxos_node.h"
#include "paxos/messenger.h"

#include "eztimer.h"

class Server : public Messenger, deps::PacketHandler, std::enable_shared_from_this<Server>
{
public:
    Server(const std::string& myid, int quorumSize);
    ~Server();

	bool Init(deps::SocketType type, const std::string& localIP, uint16_t localPort, 
		const std::string& dstIP, uint16_t dstPort);
	bool Run();
	bool Listen(int port, int backlog, deps::SocketType type);
    virtual int HandlePacket(const char* data, size_t size, deps::SocketBase* s);
	virtual void HandleClose(deps::SocketBase* s);
	bool HandleMessage(const deps::PacketHeader& header, std::shared_ptr<deps::Marshallable> pMsg, deps::SocketBase* s);
	deps::SocketBase* Connect(uint32_t ip, int port, deps::SocketType type);
	bool SendMessage(uint16_t cmd, const deps::Marshallable& msg, deps::SocketBase* s);
	void SendMessageToPeer(uint16_t cmd, const deps::Marshallable& msg, PeerAddr& addr);
	void SendMessageToAllPeer(uint16_t cmd, const deps::Marshallable& msg);

	/****************************集群网络结构信息************************/
	//获取本地地址
	PeerInfo GetMyNodeInfo();
	//节点加入集群
	bool AddPeerInfo(std::string peerId, const PeerAddr& peerAddr);
	//更新节点信息
	void UpdatePeerInfo(std::string peerId, uint64_t rtt);
	//删除节点
	void RemovePeerInfo(std::string peerId);
	//更新稳定节点信息
	void updateStablePeers(std::string peerId, const PeerAddr& addr);
	
	//处理ping消息
	bool HandlePingMessage(const deps::PacketHeader& header, std::shared_ptr<PingMessage> pMsg, deps::SocketBase* s);
	//处理pong消息
	bool HandlePongMessage(const deps::PacketHeader& header, std::shared_ptr<PongMessage> pMsg, deps::SocketBase* s);
	//发送心跳消息
	void SendPingMessage();
	
	/************************************paxos******************************/
	//处理心跳消息
	bool HandleHeatBeatMessage(const deps::PacketHeader& header, std::shared_ptr<HeartbeatMessage> pMsg, deps::SocketBase* s);

	//选择Acceptor大多数
	void SelectMajorityAcceptors(std::set<std::string>& acceptors);
    //发送prepare请求
    virtual void sendPrepare(const ProposalID& proposalID);
    //发送prepare请求的承诺
    virtual void sendPromise(const std::string& toUID, const ProposalID& proposalID, 
        const ProposalID& acceptID, const std::string& acceptValue);
    //发送accept请求
    virtual void sendAccept(const ProposalID&  proposalID, 
		const std::string& proposalValue);
    //发送accept请求的批准
    virtual void sendPermit(const std::string& proposerUID, const ProposalID&  proposalID, 
		const std::string& acceptedValue);
    //解决
    virtual void onResolution(const ProposalID&  proposalID, 
		const std::string& value);

	//发送prepare请求的ack
	virtual void sendPrepareNACK(const std::string& proposerUID, const ProposalID& proposalID, 
		const ProposalID& promisedID);
	//发送accept请求的ack
	virtual void sendAcceptNACK(const std::string& proposerUID, const ProposalID& proposalID, 
		const ProposalID& promisedID);
	//尝试获取leader
	virtual void onLeadershipAcquired();

	//丢失主
	virtual void onLeadershipLost();
	//主变更
	virtual void onLeadershipChange(const std::string& previousLeaderUID, 
		const std::string& newLeaderUID);
	//发送心跳
	virtual void sendHeartbeat(const std::string& leaderUID, const ProposalID& leaderProposalID);
private:
	void dumpStatus();
private:
	//连接管理容器
	deps::EpollContainer* m_container;
	PaxosNode m_paxosNode;
	EzTimerManager m_timerManager;
	//自己的节点信息
	std::string m_myUID;
	uint32_t m_localIP;
	uint16_t m_localPort;
	deps::SocketType m_socketType;
	//集群稳定的节点，相当于P2P网络中稳定的公有节点
	std::vector<PeerInfo> m_stablePeers;
	//集群所有节点
	std::map<std::string, PeerInfo> m_peers;
	//集群所有节点
	std::map<PeerAddr, deps::SocketBase*> m_addr2socket;
	std::map<deps::SocketBase*, PeerAddr> m_socket2addr;
	//已经选择过的最大的节点id
	std::string m_maxChoosenAcceptorUID;

	//Acceptors集合
	std::set<std::string> m_majorityAcceptors;
	size_t m_quorumSize;
};
