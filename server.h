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
#include "sys/logger.h"

#include "paxos/proto.h"
#include "paxos/paxos_node.h"
#include "paxos/messenger.h"

#include "eztimer.h"

class Server : public Messenger, PaxosNode, public PacketHandler, std::enable_shared_from_this<Server>
{
public:
    Server(const std::string& myid, int quorumSize);
    ~Server();
	bool Init(const std::string& localIP, uint16_t localTcpPort, uint16_t localUdpPort, 
		const std::string& dstIP, uint16_t dstTcpPort, uint16_t dstUdpPort);
	bool Run();
	bool Listen(int port, int backlog, SocketType type);
    virtual int HandlePacket(const char* data, size_t size, SocketBase* s);
	virtual void HandleClose(SocketBase* s);
	bool HandleMessage(const PacketHeader& header, std::shared_ptr<Marshallable> pMsg, SocketBase* s);
	bool Connect(uint32_t ip, int port, SocketType type, int* pfd);
	bool SendMessage(uint16_t cmd, const Marshallable& msg, SocketBase* s);
	void SendMessageToPeer(uint16_t cmd, const Marshallable& msg, PeerAddr& addr);
	void SendMessageToAllPeer(uint16_t cmd, const Marshallable& msg);
	void SendMessageToStablePeer(uint16_t cmd, const Marshallable& msg);

	//获取本地地址
	PeerInfo GetMyNodeInfo(SocketType type);
	bool SetPeerAddr(std::string peerId, const PeerAddr& peerAddr);
	void updateStableAddr(std::string peerId, const PeerAddr& addr);
	bool HandlePingMessage(const PacketHeader& header, std::shared_ptr<PingMessage> pMsg, SocketBase* s);
	bool HandlePongMessage(const PacketHeader& header, std::shared_ptr<PongMessage> pMsg, SocketBase* s);
	void SendPingMessage();
	//paxos协议
	//处理心跳消息
	bool HandleHeatBeatMessage(const PacketHeader& header, std::shared_ptr<HeartbeatMessage> pMsg, SocketBase* s);

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
	virtual void sendHeartbeat(const ProposalID& leaderProposalID);
	//注册定时器
	virtual void addTimer(long millisecondDelay, std::function<void()> callback);
private:
	//连接管理容器
	EpollContainer* m_container;

	EzTimerManager m_timers;
	//自己的节点信息
	std::string m_myUID;
	uint32_t m_localIP;
	uint16_t m_localTcpPort;
	uint16_t m_localUdpPort;
	//集群稳定的节点，相当于P2P网络中稳定的公有节点
	std::vector<PeerInfo> m_stableAddrs;
	//集群其他节点
	std::map<std::string, PeerInfo> m_peers;
	//已经选择过的最大的节点id
	std::string m_maxChoosenAcceptorUID;

	//Acceptors集合
	std::set<std::string> m_majorityAcceptors;
	int m_quorumSize;
};
