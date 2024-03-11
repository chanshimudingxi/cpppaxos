#pragma once

#include <cstring>
#include <iostream>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>

#include "sys/log.h"
#include "net/tcp_socket.h"
#include "net/udp_socket.h"
#include "net/epoll_container.h"
#include "sys/log.h"

#include "proto/common_proto_parser.h"
#include "proto/proto.h"
#include "paxos/paxos_node.h"
#include "paxos/messenger.h"

struct PeerAddr{
	PeerAddr():m_ip(0),m_port(0),m_socketType(SocketType::tcp),m_fd(-1){}
	~PeerAddr(){}

	uint32_t m_ip;
	uint16_t m_port;
	SocketType m_socketType;
	int m_fd;
	//节点id
	std::string m_id;
};


class Server : public Messenger, std::enable_shared_from_this<Server>
{
public:
    Server(const std::string& myid, int quorumSize);
    ~Server();
	bool Init(const std::string& localIP, uint16_t localTcpPort, uint16_t localUdpPort, 
		const std::string& dstIP, uint16_t dstTcpPort, uint16_t dstUdpPort);
	bool Run();
	bool Listen(int port, int backlog, SocketType type, CommonProtoParser* parser);

	static bool HandlePaxosMessage(std::shared_ptr<Message> pMsg, SocketBase* s, void* instance);
	bool HandlePaxosMessage(std::shared_ptr<Message> pMsg, SocketBase* s);
	static void HandlePaxosClose(SocketBase* s, void* instance);
	void HandlePaxosClose(SocketBase* s);

	static bool HandleSignalMessage(std::shared_ptr<Message> pMsg, SocketBase* s, void* instance);
	bool HandleSignalMessage(std::shared_ptr<Message> pMsg, SocketBase* s);
	static void HandleSignalClose(SocketBase* s, void* instance);
	void HandleSignalClose(SocketBase* s);

	//处理心跳消息
	bool HandleHeatBeatMessage(std::shared_ptr<HeartbeatMessage> pMsg, SocketBase* s);

	void HandleLoop();
	bool Connect(uint32_t ip, int port, SocketType type, int* pfd);
	bool SendMessage(const Message& msg, SocketBase* s);
	void SendMessageToPeer(const Message& msg, PeerAddr& addr);
	void SendMessageToAllPeer(const Message& msg);
	void SendMessageToStablePeer(const Message& msg);

	//选择Acceptor大多数
	void SelectMajorityAcceptors(std::set<std::string>& acceptors);
    //发送prepare请求
    virtual void sendPrepare(const ProposalID& proposalID);
    //发送prepare请求的承诺
    virtual void sendPromise(const std::string& proposerUID, const ProposalID& proposalID, 
        const ProposalID& acceptID, const std::string& acceptValue);
    //发送accept请求
    virtual void sendAccept(const ProposalID&  proposalID, 
		const std::string& proposalValue);
    //发送accept请求的批准
    virtual void sendAccepted(const ProposalID&  proposalID, 
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

	//发送心跳
	virtual void sendHeartbeat(const ProposalID& leaderProposalID);
	//丢失主
	virtual void onLeadershipLost();
	//主变更
	virtual void onLeadershipChange(const std::string& previousLeaderID, 
		const std::string& newLeaderID);

	bool GetLocalAddr(PeerAddr& addr);
private:
	//连接管理容器
	EpollContainer* m_container;
	//paxos业务协议解析器
	CommonProtoParser* m_paxosParser;
	//单个业务协议解析器
	CommonProtoParser* m_signalParser;

	//自己的节点信息
	std::string m_myUID;
	uint32_t m_localIP;
	uint16_t m_localTcpPort;
	uint16_t m_localUdpPort;
	//集群稳定的节点，相当于P2P网络中稳定的公有节点
	std::vector<PeerAddr> m_stableAddrs;

	//集群其他节点
	std::map<std::string, PeerAddr> m_peers;
	//已经选择过的最大的节点id
	std::string m_maxChoosenAcceptorID;
	//Acceptors集合
	std::set<std::string> m_majorityAcceptors;
	int m_quorumSize;

	PaxosNode m_paxosNode;
};
