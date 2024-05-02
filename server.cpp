#include "server.h"
#include <sstream>
#include <memory>
#include "paxos/proto.h"

Server::Server(const std::string& myid, int quorumSize):
	m_paxosNode(shared_from_this(), myid, quorumSize, 10000, 100000, 50000, "")
{
	m_container = new EpollContainer(1000, 1000);
	assert(nullptr != m_container);

	m_myUID = myid;
	m_quorumSize = quorumSize;
}

Server::~Server(){
	if(nullptr != m_container){
		delete m_container;
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

bool Server::Listen(int port, int backlog, SocketType type){
	switch(type){
		case SocketType::tcp:{
			return TcpSocket::Listen(port, backlog, m_container, this);
		}
		break;
		case SocketType::udp:{
			return UdpSocket::Listen(port, backlog, m_container, this);
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
	if(!Listen(m_localTcpPort, 10, SocketType::tcp)){
		return false;
	}
	if(!Listen(m_localUdpPort, 10, SocketType::udp)){
		return false;
	}

	while(true){
		uint64_t start = Util::GetMonoTimeUs();
		m_container->HandleSockets();
		HandleLooper();
		uint64_t end = Util::GetMonoTimeUs();
    }
}

int Server::HandlePacket(const char* data, size_t size, SocketBase* s){
    if(data == nullptr){
		LOG_ERROR("packet is null");
        return -1;
    }
    if(size < Decoder::minSize()){
		LOG_DEBUG("packet len:%zd too short",size);
        return 0;
    }
    uint16_t packetSize = Decoder::pickLen(data);
    if(packetSize > Decoder::maxSize()){
		LOG_ERROR("packet exceed limit:%u",packetSize);
        return -1;
    }
    if(packetSize > size){
		LOG_DEBUG("packet len:%zd too short",size);
        return 0;
    }
    uint32_t seq = Decoder::pickSeq(data);
	uint16_t subLen = Decoder::pickSubLen(data);
	if(subLen > Decoder::maxSize()){
		LOG_ERROR("packet exceed limit:%u",subLen);
        return -1;
    }
    if(subLen > size - Decoder::mainHeaderSize()){
		LOG_DEBUG("packet len:%zd too short",size);
        return 0;
    }
	uint16_t subCmd = Decoder::pickSubCmd(data);
	LOG_DEBUG("unpack:\n%s", Util::DumpHex(data, packetSize).c_str());

	std::shared_ptr<Marshallable> pMsg;
	switch (subCmd){
		case HeartbeatMessage::cmd:
			pMsg = std::make_shared<HeartbeatMessage>();
			break;
		case PrepareMessage::cmd:
			pMsg = std::make_shared<PrepareMessage>();
			break;
		case PromiseMessage::cmd:
			pMsg = std::make_shared<PromiseMessage>();
			break;
		case AcceptMessage::cmd:
			pMsg = std::make_shared<AcceptMessage>();
			break;
		case PermitMessage::cmd:
			pMsg = std::make_shared<PermitMessage>();
			break;
		case PrepareAckMessage::cmd:
			pMsg = std::make_shared<PrepareAckMessage>();
			break;
		case AcceptAckMessage::cmd:
			pMsg = std::make_shared<AcceptAckMessage>();
			break;
		default:
			break;
	}
	
	if(!pMsg){
		LOG_ERROR("unknow message seq:%u cmd:%u", seq, subCmd);
		return -1;
	}

	PacketHeader header;
	Decoder decoder(data, packetSize);
	decoder.deserialize(header, *pMsg);

	if(HandleMessage(header, pMsg, s)){
		return packetSize;
	}
	else{
		LOG_ERROR("message seq:%u cmd:%u handle failed", seq, subCmd);
		return -1;
	}
}

void Server::HandleClose(SocketBase* s){
	LOG_INFO("close socket:%p fd:%d peer:%s:%u", s, s->GetFd(), inet_ntoa(s->GetPeerAddr().sin_addr), ntohs(s->GetPeerAddr().sin_port));
	//TODO 依赖socket状态的地方都要清除
}

bool Server::HandleMessage(const PacketHeader& header, std::shared_ptr<Marshallable> pMsg, SocketBase* s){
	bool ret = false;
	switch (header.getSubCmd()){
		case HeartbeatMessage::cmd:
			ret = HandleHeatBeatMessage(header, std::dynamic_pointer_cast<HeartbeatMessage>(pMsg), s);
			break;
		case PrepareMessage::cmd:
			break;
		case PromiseMessage::cmd:
			break;
		case AcceptMessage::cmd:
			break;
		case PermitMessage::cmd:
			break;
		case PrepareAckMessage::cmd:
			break;
		case AcceptAckMessage::cmd:
			break;
		default:
			break;
	}
	return ret;
}


void Server::HandleLooper(){
	time_t now = time(0);
	//TODO
}

bool Server::Connect(uint32_t ip, int port, SocketType type, int* pfd){
	bool ret = false;
	switch(type){
	case SocketType::tcp:
		ret = TcpSocket::Connect(ip, port, m_container, this, pfd);
		break;
	case SocketType::udp:
		ret = UdpSocket::Connect(ip, port, m_container, this, pfd);
		break;
	default:
		break;
	}
	return ret;
}

bool Server::SendMessage(uint16_t cmd, const Marshallable& msg, SocketBase* s){
	Encoder encoder;
	encoder.serialize(cmd, msg);
	if(!s->SendPacket(encoder.data(),encoder.size())){
		LOG_ERROR("fd:%d send packet failed", s->GetFd());
		return false;
	}
	return true;
}

void Server::SendMessageToPeer(uint16_t cmd, const Marshallable& msg, PeerAddr& addr){
	if(addr.m_fd == -1){
		Connect(addr.m_ip, addr.m_port, addr.m_socketType, &addr.m_fd);
	}

	SocketBase* pSock = m_container->GetSocket(addr.m_fd);
	if(nullptr == pSock){
		LOG_ERROR("fd:%d get connect failed", addr.m_fd);
	}
	else{
		SendMessage(cmd, msg, pSock);
	}
}

void Server::SendMessageToAllPeer(uint16_t cmd, const Marshallable& msg){
	for(std::map<std::string, PeerAddr>::iterator itr = m_peers.begin(); itr!=m_peers.end(); ++itr){
		std::string peerid = itr->first;
		PeerAddr& peeraddr = itr->second;
		SendMessageToPeer(cmd, msg, peeraddr);
		LOG_INFO("send ping to %s peer:%s addr %s:%u",
			(peeraddr.m_socketType == SocketType::tcp ? "tcp" : "udp"), 
			peerid.c_str(), Util::UintIP2String(peeraddr.m_ip).c_str(), peeraddr.m_port);
	}
	return;
}

void Server::SendMessageToStablePeer(uint16_t cmd, const Marshallable& msg){
	for(int i=0; i < m_stableAddrs.size(); ++i){
		PeerAddr& peeraddr = m_stableAddrs[i];
		SendMessageToPeer(cmd, msg, peeraddr);
		LOG_INFO("send ping to %s addr[%d] %s:%u", 
			(peeraddr.m_socketType == SocketType::tcp ? "tcp" : "udp"), 
			i, Util::UintIP2String(peeraddr.m_ip).c_str(), peeraddr.m_port);
	}
	return;
}

bool Server::HandleHeatBeatMessage(const PacketHeader& header, std::shared_ptr<HeartbeatMessage> pMsg, SocketBase* s){
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
	auto itr = m_peers.upper_bound(m_maxChoosenAcceptorUID);
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
			SendMessageToPeer(PrepareMessage::cmd, prepare, peer);
		}
	}
}

/**
 * @brief 发送prepare请求的承诺
 * 
 * @param toUID 
 * @param proposalID 
 * @param acceptID 
 * @param acceptValue 
 */
void Server::sendPromise(const std::string& toUID, const ProposalID& proposalID, 
	const ProposalID& acceptID, const std::string& acceptValue){
	PromiseMessage promise;
	PPeerAddr addr;
	addr.m_ip= m_localIP;
	addr.m_port = m_localTcpPort;
	addr.m_socketType = 0;//tcp
	promise.m_myInfo.m_addrs.push_back(addr);
	promise.m_myInfo.m_id = m_myUID;

	promise.m_proposalID.m_number = proposalID.m_number;
	promise.m_proposalID.m_uid = proposalID.m_uid;
	promise.m_acceptID.m_number = acceptID.m_number;
	promise.m_acceptID.m_uid = acceptID.m_uid;
	promise.m_acceptValue = acceptValue;

	auto peerItr = m_peers.find(toUID);
	if(peerItr != m_peers.end()){
		PeerAddr& peer = peerItr->second;
		SendMessageToPeer(PromiseMessage::cmd, promise, peer);
	}
}

/**
 * @brief 发送accept请求
 * 
 * @param proposalID 
 * @param proposalValue 
 */
void Server::sendAccept(const ProposalID&  proposalID, 
	const std::string& proposalValue){
	AcceptMessage accept;
	PPeerAddr addr;
	addr.m_ip= m_localIP;
	addr.m_port = m_localTcpPort;
	addr.m_socketType = 0;//tcp
	accept.m_myInfo.m_addrs.push_back(addr);
	accept.m_myInfo.m_id = m_myUID;

	accept.m_proposalID.m_number = proposalID.m_number;
	accept.m_proposalID.m_uid = proposalID.m_uid;
	accept.m_proposalValue = proposalValue;

	for(auto acceptorUID : m_majorityAcceptors){
		auto peerItr = m_peers.find(acceptorUID);
		if(peerItr != m_peers.end()){
			PeerAddr& peer = peerItr->second;
			SendMessageToPeer(AcceptMessage::cmd, accept, peer);
		}
	}
}

/**
 * @brief 发送accept请求的批准
 * 
 * @param proposerUID 
 * @param proposalID 
 * @param acceptedValue 
 */
void Server::sendPermit(const std::string& proposerUID, const ProposalID&  proposalID, 
	const std::string& acceptedValue)
{
	PermitMessage premit;
	PPeerAddr addr;
	addr.m_ip= m_localIP;
	addr.m_port = m_localTcpPort;
	addr.m_socketType = 0;//tcp
	premit.m_myInfo.m_addrs.push_back(addr);
	premit.m_myInfo.m_id = m_myUID;
	premit.m_proposalID.m_number = proposalID.m_number;
	premit.m_proposalID.m_uid = proposalID.m_uid;
	premit.m_acceptedValue = acceptedValue;

	auto peerItr = m_peers.find(proposerUID);
	if(peerItr != m_peers.end()){
		PeerAddr& peer = peerItr->second;
		SendMessageToPeer(PermitMessage::cmd, premit, peer);
	}
}

/**
 * @brief 发送已经选定的协议号
 * 
 * @param proposalID 选定的协议编号
 * @param value 选定的协议值
 */
void Server::onResolution(const ProposalID&  proposalID, 
	const std::string& value)
{

}

/**
 * @brief 发送prepare请求的ack
 * 
 * @param proposerUID 
 * @param proposalID 
 * @param promisedID 
 */
void Server::sendPrepareNACK(const std::string& proposerUID, const ProposalID& proposalID, 
	const ProposalID& promisedID)
{
	PrepareAckMessage ack;
	PPeerAddr addr;
	addr.m_ip= m_localIP;
	addr.m_port = m_localTcpPort;
	addr.m_socketType = 0;//tcp
	ack.m_myInfo.m_addrs.push_back(addr);
	ack.m_myInfo.m_id = m_myUID;
	ack.m_proposalID.m_number = proposalID.m_number;
	ack.m_proposalID.m_uid = proposalID.m_uid;
	ack.m_promiseID.m_number = promisedID.m_number;
	ack.m_promiseID.m_uid = promisedID.m_uid;

	auto peerItr = m_peers.find(proposerUID);
	if(peerItr != m_peers.end()){
		PeerAddr& peer = peerItr->second;
		SendMessageToPeer(PrepareAckMessage::cmd, ack, peer);
	}
}

/**
 * @brief 发送accept请求的ack
 * 
 * @param proposerUID 
 * @param proposalID 
 * @param promisedID 
 */
void Server::sendAcceptNACK(const std::string& proposerUID, const ProposalID& proposalID, 
	const ProposalID& promisedID)
{
	AcceptAckMessage ack;
	PPeerAddr addr;
	addr.m_ip= m_localIP;
	addr.m_port = m_localTcpPort;
	addr.m_socketType = 0;//tcp
	ack.m_myInfo.m_addrs.push_back(addr);
	ack.m_myInfo.m_id = m_myUID;
	ack.m_proposalID.m_number = proposalID.m_number;
	ack.m_proposalID.m_uid = proposalID.m_uid;
	ack.m_promiseID.m_number = promisedID.m_number;
	ack.m_promiseID.m_uid = promisedID.m_uid;

	auto peerItr = m_peers.find(proposerUID);
	if(peerItr != m_peers.end()){
		PeerAddr& peer = peerItr->second;
		SendMessageToPeer(AcceptAckMessage::cmd, ack, peer);
	}

}

//尝试获取leader
void Server::onLeadershipAcquired()
{

}

//丢失leader
void Server::onLeadershipLost()
{

}

//leader变更
void Server::onLeadershipChange(const std::string& previousLeaderUID, 
	const std::string& newLeaderUID)
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

	SendMessageToAllPeer(HeartbeatMessage::cmd, heartbeat);
	SendMessageToStablePeer(HeartbeatMessage::cmd, heartbeat);
}
