#ifndef NODE_H_
#define NODE_H_

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

#endif