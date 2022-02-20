#include "server.h"
#include <sstream>

Server::Server(){
	m_container = new EpollContainer(1000, 1000);
	assert(nullptr != m_container);

	m_paxosParser = new CommonProtoParser();
	assert(nullptr != m_paxosParser);
	m_paxosParser->SetMessageCallback(HandlePaxosMessage, this);
	m_paxosParser->SetCloseCallback(HandlePaxosClose, this);

	m_signalParser = new CommonProtoParser();
	assert(nullptr != m_signalParser);
	m_signalParser->SetMessageCallback(HandleSignalMessage, this);
	m_signalParser->SetCloseCallback(HandleSignalClose, this);

	m_lastSendTime = 0;
}

Server::~Server(){
	if(nullptr != m_container){
		delete m_container;
	}
	if(nullptr != m_paxosParser){
		delete m_paxosParser;
	}
	if(nullptr != m_signalParser){
		delete m_signalParser;
	}
}

bool Server::Init(const std::string& myID, const std::string& localIP, uint16_t localTcpPort, uint16_t localUdpPort, 
	const std::string& dstIP, uint16_t dstTcpPort, uint16_t dstUdpPort){

    if(!m_container->Init()){
        return false;
    }
	m_myid = myID;
	m_localIP = inet_addr(localIP.c_str());
	m_localTcpPort = localTcpPort;
	m_localUdpPort = localUdpPort;

	PeerAddr addr;
	addr.m_ip = inet_addr(dstIP.c_str());

	addr.m_port = dstTcpPort;
	addr.m_socketType = SocketType::tcp;
	m_stableAddrs.push_back(addr);

	// addr.m_port = dstUdpPort;
	// addr.m_socketType = SocketType::udp;
	// m_stableAddrs.push_back(addr);

	return true;
}

bool Server::Listen(int port, int backlog, SocketType type, CommonProtoParser* parser){
	switch(type){
		case SocketType::tcp:{
			return TcpSocket::Listen(port, backlog, m_container, parser);
		}
		break;
		case SocketType::udp:{
			return UdpSocket::Listen(port, backlog, m_container, parser);
		}
		break;
		default:{
			return false;
		}
		break;
	}
	return true;
}

bool Server::Run(){
	if(!Listen(m_localTcpPort, 10, SocketType::tcp, m_signalParser)){
		return false;
	}
	if(!Listen(m_localUdpPort, 10, SocketType::udp, m_paxosParser)){
		return false;
	}

	while(true){
		uint64_t start = Util::GetMonoTimeUs();
		m_container->HandleSockets();
		HandleLoop();
		uint64_t end = Util::GetMonoTimeUs();
		//LOG_DEBUG("loop once cost:%lu", end - start);
    }
}

bool Server::HandlePaxosMessage(std::shared_ptr<Message> pMsg, SocketBase* s, void* instance){
	if(instance != nullptr){
		return static_cast<Server*>(instance)->HandlePaxosMessage(pMsg, s);
	}
	return false;
}

bool Server::HandlePaxosMessage(std::shared_ptr<Message> pMsg, SocketBase* s){
	switch (pMsg->ProtoID()){
		case PAXOS_PROTO_PING_MESSAGE:
			return HandlePingMessage(std::dynamic_pointer_cast<PingMessage>(pMsg), s);
		default:
			return false;
	}
}

bool Server::HandleSignalMessage(std::shared_ptr<Message> pMsg, SocketBase* s, void* instance){
	if(instance != nullptr){
		return static_cast<Server*>(instance)->HandleSignalMessage(pMsg, s);
	}
	return false;
}

bool Server::HandleSignalMessage(std::shared_ptr<Message> pMsg, SocketBase* s){
	switch (pMsg->ProtoID()){
		case PAXOS_PROTO_PING_MESSAGE:
			return HandlePingMessage(std::dynamic_pointer_cast<PingMessage>(pMsg), s);
		default:
			return false;
	}
}

bool Server::HandlePingMessage(std::shared_ptr<PingMessage> pMsg, SocketBase* s){
	uint64_t lastStamp = pMsg->m_stamp;
	std::string peerId = pMsg->m_myInfo.m_id;

	Peer peer;
	peer.m_id = peerId;
	std::stringstream os;
	for(int i = 0; i< pMsg->m_myInfo.m_addrs.size(); ++i){
		ProtoPeerAddr& addr = pMsg->m_myInfo.m_addrs[i];
		os<<"ip:"<<Util::UintIP2String(addr.m_ip)<<" port:"<<addr.m_port<<" type:"<<(addr.m_socketType == 0 ? "tcp ": "udp ");
		if(m_peers.find(peerId) == m_peers.end()){
			PeerAddr peeraddr;
			peeraddr.m_ip = addr.m_ip;
			peeraddr.m_port = addr.m_port;
			peeraddr.m_socketType = addr.m_socketType == 0 ? SocketType::tcp : SocketType::udp;
			peer.m_addrs.push_back(peeraddr);
		}
	}

	if(m_peers.find(peerId) == m_peers.end()){
		m_peers[peerId] = peer;
	}
	uint64_t now = Util::GetMonoTimeMs();
	uint64_t rtt = lastStamp > now ? lastStamp - now : 0;
	LOG_INFO("peer id:%s rtt:%llums %s", peerId.c_str(), rtt, os.str().c_str());
	return true;
}

void Server::HandlePaxosClose(SocketBase* s, void* instance){
	if(instance != nullptr){
		static_cast<Server*>(instance)->HandlePaxosClose(s);
	}
}

void Server::HandlePaxosClose(SocketBase* s){
	LOG_INFO("close socket:%p fd:%d peer:%s:%u", s, s->GetFd(), inet_ntoa(s->GetPeerAddr().sin_addr), ntohs(s->GetPeerAddr().sin_port));
	
}

void Server::HandleSignalClose(SocketBase* s, void* instance){
	if(instance != nullptr){
		static_cast<Server*>(instance)->HandleSignalClose(s);
	}
}

void Server::HandleSignalClose(SocketBase* s){
	LOG_INFO("close socket:%p fd:%d peer:%s:%u", s, s->GetFd(), inet_ntoa(s->GetPeerAddr().sin_addr), ntohs(s->GetPeerAddr().sin_port));
}


void Server::HandleLoop(){
	time_t now = time(0);
	if(m_lastSendTime == 0 || (m_lastSendTime > 0 && m_lastSendTime + 30 < now ) ){
		SendPingMessageToAllPeer();
		SendPingMessageToStablePeerAddrs();
		m_lastSendTime = now;
	}
}

bool Server::Connect(uint32_t ip, int port, SocketType type, int* pfd){
	bool ret = false;
	switch(type){
	case SocketType::tcp:
		ret = TcpSocket::Connect(ip, port, m_container, m_signalParser, pfd);
		break;
	case SocketType::udp:
		ret = UdpSocket::Connect(ip, port, m_container, m_paxosParser, pfd);
		break;
	default:
		break;
	}
	return ret;
}

bool Server::SendMessage(const Message& msg, SocketBase* s){
	std::string packet;
	CommonProtoParser::PackMessage(msg, &packet);
	if(!s->SendPacket(packet.data(),packet.size())){
		LOG_ERROR("fd:%d send packet failed", s->GetFd());
		return false;
	}
	return true;
}

void Server::SendMessageToPeerAddr(const Message& msg, PeerAddr& addr){
	if(addr.m_fd == -1){
		Connect(addr.m_ip, addr.m_port, addr.m_socketType, &addr.m_fd);
	}

	SocketBase* pSock = m_container->GetSocket(addr.m_fd);
	if(nullptr == pSock){
		LOG_ERROR("fd:%d get connect failed", addr.m_fd);
	}
	else{
		SendMessage(msg, pSock);
	}
}

void Server::SendMessageToAllPeer(const Message& msg){
	for(std::map<std::string, Peer>::iterator itr = m_peers.begin(); itr!=m_peers.end(); ++itr){
		std::string peerid = itr->first;
		Peer& peer = itr->second;
		for(int i=0; i < peer.m_addrs.size(); ++i){
			PeerAddr& peeraddr = peer.m_addrs[i];
			SendMessageToPeerAddr(msg, peeraddr);
			LOG_INFO("send ping to %s peer:%s addr[%d] %s:%u", (peeraddr.m_socketType == SocketType::tcp ? "tcp" : "udp"), peerid.c_str(), i, Util::UintIP2String(peeraddr.m_ip).c_str(), peeraddr.m_port);
		}
	}
	return;
}

void Server::SendPingMessageToAllPeer(){
	PingMessage ping;
	ping.m_stamp = Util::GetMonoTimeMs();
	ping.m_myInfo.m_id = m_myid;
	ProtoPeerAddr addr;
	addr.m_ip= m_localIP;
	addr.m_port = m_localTcpPort;
	addr.m_socketType = 0;//tcp
	ping.m_myInfo.m_addrs.push_back(addr);
	// addr.m_port = m_localUdpPort;
	// addr.m_socketType = 1;//udp
	// ping.m_myInfo.m_addrs.push_back(addr);

	SendMessageToAllPeer(ping);
}


void Server::SendPingMessageToStablePeerAddrs(){
	PingMessage ping;
	ping.m_stamp = Util::GetMonoTimeMs();
	ping.m_myInfo.m_id = m_myid;
	ProtoPeerAddr addr;
	addr.m_ip= m_localIP;
	addr.m_port = m_localTcpPort;
	addr.m_socketType = 0;//tcp
	ping.m_myInfo.m_addrs.push_back(addr);
	// addr.m_port = m_localUdpPort;
	// addr.m_socketType = 1;//udp
	// ping.m_myInfo.m_addrs.push_back(addr);

	for(int i=0; i < m_stableAddrs.size(); ++i){
		PeerAddr& peeraddr = m_stableAddrs[i];
		SendMessageToPeerAddr(ping, peeraddr);
		LOG_INFO("send ping to %s addr[%d] %s:%u", (peeraddr.m_socketType == SocketType::tcp ? "tcp" : "udp"), i, Util::UintIP2String(peeraddr.m_ip).c_str(), peeraddr.m_port);
	}
}