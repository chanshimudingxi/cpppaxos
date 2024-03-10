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
};

struct Peer{
	std::string m_id;
	std::vector<PeerAddr> m_addrs;
};

class Server : public Messenger
{
public:
    Server();
    ~Server();
	bool Init(const std::string& myID, const std::string& localIP, uint16_t localTcpPort, uint16_t localUdpPort, 
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

	bool HandlePingMessage(std::shared_ptr<PingMessage> pMsg, SocketBase* s);

	void HandleLoop();
	bool Connect(uint32_t ip, int port, SocketType type, int* pfd);
	bool SendMessage(const Message& msg, SocketBase* s);
	void SendMessageToPeerAddr(const Message& msg, PeerAddr& addr);
	void SendMessageToPeer(const Message& msg, Peer& peer);
	void SendMessageToAllPeer(const Message& msg);
	void SendPingMessageToAllPeer();
	void SendPingMessageToStablePeerAddrs();

    //发送prepare请求
    virtual void sendPrepare(const ProposalID& proposalID){}
    //发送prepare请求的承诺
    virtual void sendPromise(const std::string& proposerID, const ProposalID& proposalID, 
        const ProposalID& previousID, const std::string& acceptValue){}
    //发送accept请求
    virtual void sendAccept(const ProposalID&  proposalID, 
		const std::string& proposalValue){}
    //发送accept请求的批准
    virtual void sendAccepted(const ProposalID&  proposalID, 
		const std::string& acceptedValue){}
    //解决
    virtual void onResolution(const ProposalID&  proposalID, 
		const std::string& value){}

	//发送prepare请求的ack
	virtual void sendPrepareNACK(const std::string& proposerID, const ProposalID& proposalID, 
		const ProposalID& promisedID){}
	//发送accept请求的ack
	virtual void sendAcceptNACK(const std::string& proposerID, const ProposalID& proposalID, 
		const ProposalID& promisedID){}
	//尝试获取leader
	virtual void onLeadershipAcquired(){}

	//发送心跳
	virtual void sendHeartbeat(const ProposalID& leaderProposalID){}
	//丢失主
	virtual void onLeadershipLost(){}
	//主变更
	virtual void onLeadershipChange(const std::string& previousLeaderID, 
		const std::string& newLeaderID){}
private:
	EpollContainer* m_container;	//连接管理容器
	CommonProtoParser* m_paxosParser;	//paxos业务协议解析器
	CommonProtoParser* m_signalParser;	//singal业务协议解析器

	std::map<std::string, Peer> m_peers;	//所有加入paxos集群的节点

	std::string m_myid;
	uint32_t m_localIP;
	uint16_t m_localTcpPort;
	uint16_t m_localUdpPort;

	std::vector<PeerAddr> m_stableAddrs;

	time_t m_lastSendTime;

	PaxosNode* m_paxosNode;
};
