#include "server.h"
#include <sstream>

Server::Server(const std::string& myid, int quorumSize):
	m_paxosNode(shared_from_this(), myid, quorumSize, 10000, 100000, 50000, "")
{
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

	m_myUID = myid;
	m_quorumSize = quorumSize;
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

bool Server::Init(const std::string& localIP, uint16_t localTcpPort, uint16_t localUdpPort, 
	const std::string& dstIP, uint16_t dstTcpPort, uint16_t dstUdpPort){

    if(!m_container->Init()){
        return false;
    }
	m_localIP = inet_addr(localIP.c_str());
	m_localTcpPort = localTcpPort;
	m_localUdpPort = localUdpPort;

	PeerAddr addr;
	addr.m_ip = inet_addr(dstIP.c_str());
	addr.m_port = dstTcpPort;
	addr.m_socketType = SocketType::tcp;
	m_stableAddrs.push_back(addr);
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
			return HandleHeatBeatMessage(std::dynamic_pointer_cast<HeartbeatMessage>(pMsg), s);
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
			return HandleHeatBeatMessage(std::dynamic_pointer_cast<HeartbeatMessage>(pMsg), s);
		default:
			return false;
	}
}

void Server::HandlePaxosClose(SocketBase* s, void* instance){
	if(instance != nullptr){
		static_cast<Server*>(instance)->HandlePaxosClose(s);
	}
}

void Server::HandlePaxosClose(SocketBase* s){
	LOG_INFO("close socket:%p fd:%d peer:%s:%u", s, s->GetFd(), inet_ntoa(s->GetPeerAddr().sin_addr), ntohs(s->GetPeerAddr().sin_port));
	//TODO 依赖socket状态的地方都要清除
}

void Server::HandleSignalClose(SocketBase* s, void* instance){
	if(instance != nullptr){
		static_cast<Server*>(instance)->HandleSignalClose(s);
	}
}

void Server::HandleSignalClose(SocketBase* s){
	LOG_INFO("close socket:%p fd:%d peer:%s:%u", s, s->GetFd(), inet_ntoa(s->GetPeerAddr().sin_addr), ntohs(s->GetPeerAddr().sin_port));
	//TODO 依赖socket状态的地方都要清除
}


void Server::HandleLoop(){
	time_t now = time(0);
	//TODO
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

void Server::SendMessageToPeer(const Message& msg, PeerAddr& addr){
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
	for(std::map<std::string, PeerAddr>::iterator itr = m_peers.begin(); itr!=m_peers.end(); ++itr){
		std::string peerid = itr->first;
		PeerAddr& peeraddr = itr->second;
		SendMessageToPeer(msg, peeraddr);
		LOG_INFO("send ping to %s peer:%s addr %s:%u",
			(peeraddr.m_socketType == SocketType::tcp ? "tcp" : "udp"), 
			peerid.c_str(), Util::UintIP2String(peeraddr.m_ip).c_str(), peeraddr.m_port);
	}
	return;
}

void Server::SendMessageToStablePeer(const Message& msg){
	for(int i=0; i < m_stableAddrs.size(); ++i){
		PeerAddr& peeraddr = m_stableAddrs[i];
		SendMessageToPeer(msg, peeraddr);
		LOG_INFO("send ping to %s addr[%d] %s:%u", 
			(peeraddr.m_socketType == SocketType::tcp ? "tcp" : "udp"), 
			i, Util::UintIP2String(peeraddr.m_ip).c_str(), peeraddr.m_port);
	}
	return;
}

bool Server::HandleHeatBeatMessage(std::shared_ptr<HeartbeatMessage> pMsg, SocketBase* s){
	uint64_t lastStamp = pMsg->m_stamp;
	const std::string& peerId = pMsg->m_myInfo.m_id;

	std::stringstream os;
	PPeerAddr& addr = pMsg->m_myInfo.m_addrs[0];

	os<<"ip:"<<Util::UintIP2String(addr.m_ip)
		<<" port:"<<addr.m_port
		<<" type:"<<(addr.m_socketType == 0 ? "tcp ": "udp ");

	PeerAddr peeraddr;
	peeraddr.m_ip = addr.m_ip;
	peeraddr.m_port = addr.m_port;
	peeraddr.m_socketType = addr.m_socketType == 0 ? SocketType::tcp : SocketType::udp;
	peeraddr.m_id = peerId;

	m_peers[peerId] = peeraddr;

	uint64_t now = Util::GetMonoTimeMs();
	uint64_t rtt = lastStamp > now ? lastStamp - now : 0;
	LOG_INFO("peer id:%s rtt:%llums %s", peerId.c_str(), rtt, os.str().c_str());
	return true;
}

/**
 * @brief 选择一个Acceptor大多数构成的集合，采用轮训机制
 * 
 * @param acceptors 
 */
void Server::SelectMajorityAcceptors(std::set<std::string>& acceptors){
	if(m_peers.size() < m_quorumSize){
		return;
	}

	acceptors.clear();

	int cnt = 0;
	auto itr = m_peers.upper_bound(m_maxChoosenAcceptorID);
	while(cnt < m_quorumSize){
		if(itr != m_peers.end()){
			acceptors.insert(itr->second.m_id);
			++cnt;
		}
		itr == m_peers.end() ? itr = m_peers.begin() : ++itr;
	}
}

/**
 * @brief 发送prepare请求
 * 
 * @param proposalID 
 */
void Server::sendPrepare(const ProposalID& proposalID){
	SelectMajorityAcceptors(m_majorityAcceptors);
	if(m_majorityAcceptors.empty()){
		LOG_ERROR("choosen acceptors failed");
		return;
	}
	PrepareMessage prepare;
	prepare.m_proposalID.m_number = proposalID.m_number;
	prepare.m_proposalID.m_uid = proposalID.m_uid;
	prepare.m_myInfo.m_id = m_myUID;
	PPeerAddr addr;
	addr.m_ip= m_localIP;
	addr.m_port = m_localTcpPort;
	addr.m_socketType = 0;//tcp
	prepare.m_myInfo.m_addrs.push_back(addr);

	for(auto acceptorUID : m_majorityAcceptors){
		auto peerItr = m_peers.find(acceptorUID);
		if(peerItr != m_peers.end()){
			PeerAddr& peer = peerItr->second;
			SendMessageToPeer(prepare, peer);
		}
	}
}

//发送prepare请求的承诺
void Server::sendPromise(const std::string& proposerUID, const ProposalID& proposalID, 
	const ProposalID& acceptID, const std::string& acceptValue){

}

//发送accept请求
void Server::sendAccept(const ProposalID&  proposalID, 
	const std::string& proposalValue){

}

//发送accept请求的批准
void Server::sendAccepted(const ProposalID&  proposalID, 
	const std::string& acceptedValue)
{
	
}

//解决
void Server::onResolution(const ProposalID&  proposalID, 
	const std::string& value)
{

}

//发送prepare请求的ack
void Server::sendPrepareNACK(const std::string& proposerUID, const ProposalID& proposalID, 
	const ProposalID& promisedID)
{

}
//发送accept请求的ack
void Server::sendAcceptNACK(const std::string& proposerUID, const ProposalID& proposalID, 
	const ProposalID& promisedID)
{

}

//尝试获取leader
void Server::onLeadershipAcquired()
{

}

//发送心跳
void Server::sendHeartbeat(const ProposalID& leaderProposalID)
{
	HeartbeatMessage heartbeat;
	heartbeat.m_stamp = Util::GetMonoTimeMs();
	heartbeat.m_myInfo.m_id = m_myUID;
	PPeerAddr addr;
	addr.m_ip= m_localIP;
	addr.m_port = m_localTcpPort;
	addr.m_socketType = 0;//tcp
	heartbeat.m_myInfo.m_addrs.push_back(addr);

	SendMessageToAllPeer(heartbeat);
	SendMessageToStablePeer(heartbeat);
}

//丢失主
void Server::onLeadershipLost()
{

}

//主变更
void Server::onLeadershipChange(const std::string& previousLeaderID, 
	const std::string& newLeaderID)
{

}