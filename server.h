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

struct PeerAddr{
	PeerAddr():m_ip(0),m_port(0),m_socketType(SocketType::tcp),m_fd(-1),m_alive(false){}
	~PeerAddr(){}

	uint32_t m_ip;
	uint16_t m_port;
	SocketType m_socketType;
	int m_fd;
	bool m_alive;
};

struct Peer{
	std::string m_id;
	std::vector<PeerAddr> m_addrs;
};

class Server
{
public:
    Server();
    ~Server();
	bool Init();
	bool Run();
	bool Listen(int port, int backlog, SocketType type);

	static bool HandleMessage(std::shared_ptr<Message> pMsg, SocketBase* s, void* instance);
	bool HandleMessage(std::shared_ptr<Message> pMsg, SocketBase* s);
	bool HandlePingMessage(std::shared_ptr<PingMessage> pMsg, SocketBase* s);

	void TimerCheck();
	bool Connect(std::string ip, int port, SocketType type, int* pfd);
	bool SendMessage(const Message& msg, SocketBase* s);
	bool SendMessageToPeer(const Message& msg, const Peer& peer);
	void SendMessageToAll(const Message& msg);
	void SendPingMessageToAll();
private:
	EpollContainer* m_container;	//连接管理容器
	CommonProtoParser* m_commonProtoParser;	//协议解析器

	std::map<std::string, Peer> m_peers;	//所有加入paxos集群的节点
};