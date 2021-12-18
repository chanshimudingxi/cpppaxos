#include "node.h"

Node::Node(){
	m_container = new EpollContainer(1000, 1000);
	assert(nullptr != m_container);
	m_commonProtoParser = new CommonProtoParser();
	assert(nullptr != m_commonProtoParser);
	m_commonProtoParser->SetCallback(HandleMessage, this);
}

Node::~Node(){
	if(nullptr != m_container){
		delete m_container;
	}
	if(nullptr != m_commonProtoParser){
		delete m_commonProtoParser;
	}
}

bool Node::HandleMessage(std::shared_ptr<Message> pMsg, SocketBase* s, void* instance){
	if(instance != nullptr){
		return static_cast<Node*>(instance)->HandleMessage(pMsg, s);
	}
	return false;
}

bool Node::HandleMessage(std::shared_ptr<Message> pMsg, SocketBase* s){
	switch (pMsg->ProtoID()){
		case SSN_PROTO_TEST_MESSAGE:
			return HandleTestMessage(std::dynamic_pointer_cast<TestMessage>(pMsg), s);
		default:
			return false;
	}
}

bool Node::SendMessage(const Message& msg, SocketBase* s){
	std::string packet;
	CommonProtoParser::PackMessage(msg, &packet);
	if(!s->SendPacket(packet.data(),packet.size())){
		LOG_ERROR("fd:%d send packet failed", s->GetFd());
		return false;
	}
	return true;
}

bool Node::Init(){
    if(!m_container->Init()){
        return false;
    }
	Client c1("172.25.83.205", 8888, SocketType::tcp);
	m_clients.push_back(c1);
	Client c2("172.25.83.205", 8888, SocketType::udp);
	m_clients.push_back(c2);
	return true;
}

bool Node::Listen(int port, int backlog, SocketType type){
	switch(type){
		case SocketType::tcp:{
			return TcpSocket::Listen(port, backlog, m_container, m_commonProtoParser);
		}
		break;
		case SocketType::udp:{
			return UdpSocket::Listen(port, backlog, m_container, m_commonProtoParser);
		}
		break;
		default:{
			return false;
		}
		break;
	}
	return true;
}

bool Node::Connect(std::string ip, int port, SocketType type, int* pfd){
	uint32_t uip = inet_addr(ip.c_str());
	switch(type){
		case SocketType::tcp:{
			return TcpSocket::Connect(uip, port, m_container, m_commonProtoParser, pfd);
		}
		break;
		case SocketType::udp:{
			return UdpSocket::Connect(uip, port, m_container, m_commonProtoParser, pfd);
		}
		break;
		default:{
			return false;
		}
		break;
	}
	return true;
}

void Node::SendMsgToAll(){
	for(std::list<Client>::iterator itr = m_clients.begin(); itr!=m_clients.end(); ++itr){
		Client& c = *itr;
		if(c.m_alive){
			SocketBase* pSock = m_container->GetSocket(c.m_fd);
			if(nullptr == pSock){
				LOG_ERROR("fd:%d get connect failed", c.m_fd);
				continue;
			}
			else{
				TestMessage msg(SSN_PROTO, SSN_PROTO_TEST_MESSAGE);
				msg.m_uid = 12345;
				msg.m_text = "hello world";
				msg.m_fvalue = 1.45;
				msg.m_dvalue = 3.141592678;
				SendMessage(msg, pSock);
			}
		}
		else{
			if(Connect(c.m_ip, c.m_port, c.m_socketType, &c.m_fd)){
				c.m_alive = true;
			}
		}
	}
}

bool Node::Run(){
	if(!Listen(8888, 10, SocketType::tcp)){
		return false;
	}
	if(!Listen(8888, 10, SocketType::udp)){
		return false;
	}

	while(true){
		sleep(1);
		uint64_t start = Util::GetMonoTimeUs();
        m_container->HandleSockets();
		SendMsgToAll();
		uint64_t end = Util::GetMonoTimeUs();
		LOG_DEBUG("cost:%lu", end - start);
    }
}

bool Node::HandleTestMessage(std::shared_ptr<TestMessage> pMsg, SocketBase* s){
	LOG_INFO("%s fd:%d protoId:%u uid:%lu text:%s fvalue:%f dvalue:%.10lf",
		toString(s->GetType()).c_str(), s->GetFd(), pMsg->ProtoID(), 
		pMsg->m_uid, pMsg->m_text.c_str(), pMsg->m_fvalue, pMsg->m_dvalue);
	return true;
}
