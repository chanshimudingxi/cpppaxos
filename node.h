#pragma once

#include <cstring>
#include <iostream>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <list>

#include "sys/log.h"
#include "net/tcp_socket.h"
#include "net/udp_socket.h"
#include "net/epoll_container.h"
#include "sys/log.h"
#include "proto/common_proto_parser.h"
#include "proto/test_message.h"

#include "messenger.h"
#include "acceptor.h"
#include "proposalid.h"
#include "proposer.h"
#include "learner.h"

#include "client.h"

class Node{
public:
    Node();
    ~Node();
	bool Init();
	bool Run();
	bool Listen(int port, int backlog, SocketType type);
	bool Connect(std::string ip, int port, SocketType type, int* pfd);
	void SendMsgToAll();
public:
	static bool HandleMessage(std::shared_ptr<Message> pMsg, SocketBase* s, void* instance);
	bool HandleMessage(std::shared_ptr<Message> pMsg, SocketBase* s);
	bool SendMessage(const Message& msg, SocketBase* s);
	bool HandleTestMessage(std::shared_ptr<TestMessage> pMsg, SocketBase* s);
private:
	EpollContainer* m_container;	//连接管理容器
	CommonProtoParser* m_commonProtoParser;	//协议解析器
	std::list<Client> m_clients;
};


class PaxosNode : public Node
{
public:

private:
	//节点可以同时具备Proposer、Acceptor、Learner三种系统角色的功能
	Proposer m_proposer;
	Acceptor m_acceptor;
	Learner  m_learner;

	Messenger m_messenger;
	std::string	m_leaderUID;
	ProposalID	m_leaderProposalID;
	long	m_lastHeartbeatTimestamp;
	long	m_lastPrepareTimestamp;
	long	m_heartbeatPeriod         = 1000; // Milliseconds
	long	m_livenessWindow          = 5000; // Milliseconds
	bool	m_acquiringLeadership     = false;
	std::set<std::string>	m_acceptNACKs;
}

