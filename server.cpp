#include "server.h"
#include <memory>
#include "paxos/proto.h"

Server::Server(const std::string& myid, int quorumSize):
	m_paxosNode(*this, myid, quorumSize, 10000, 100000, 50000, "")
{
	m_container = new deps::EpollContainer(1000, 1000);
	assert(nullptr != m_container);

	m_myUID = myid;
	m_quorumSize = quorumSize;

	m_timerManager.addTimer(5000, std::bind(&Server::SendPingMessage, this));
	m_timerManager.addTimer(10000, std::bind(&PaxosNode::pulse, m_paxosNode));
	m_timerManager.addTimer(2000, std::bind(&Server::dumpStatus, this));
}

Server::~Server(){
	if(nullptr != m_container){
		delete m_container;
	}
}

void Server::dumpStatus(){
	bool isLeader = m_paxosNode.isLeader();
	LOG_INFO("local uid:%s proposalid:%s isLeader:%d leader uid:%s proposalid:%s", 
		m_myUID.c_str(), m_paxosNode.getMyProposalID().toString().c_str(), isLeader,
		m_paxosNode.getLeaderUID().c_str(),	m_paxosNode.getLeaderProposalID().toString().c_str());
}

bool Server::Init(deps::SocketType type, const std::string& localIP, uint16_t localPort, 
		const std::string& dstIP, uint16_t dstPort){

	m_localIP = inet_addr(localIP.c_str());
	m_localPort = localPort;
	m_socketType = type;

	PeerInfo peer;
	peer.m_addr.m_ip = inet_addr(dstIP.c_str());
	peer.m_addr.m_port = dstPort;
	peer.m_addr.m_socketType = type;
	m_stablePeers.push_back(peer);
	return true;
}

bool Server::Listen(int port, int backlog, deps::SocketType type){
	switch(type){
		case deps::SocketType::tcp:{
			return deps::TcpSocket::Listen(port, backlog, m_container, this);
		}
		break;
		case deps::SocketType::udp:{
			return deps::UdpSocket::Listen(port, backlog, m_container, this);
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
	if(!Listen(m_localPort, 10, m_socketType)){
		return false;
	}
	while(true){
		m_container->HandleSockets();
		m_timerManager.checkTimer();
    }
	return false;
}

int Server::HandlePacket(const char* data, size_t size, deps::SocketBase* s){
    if(data == nullptr){
		LOG_ERROR("packet is null");
        return -1;
    }
    if(size < deps::Decoder::minSize()){
		LOG_DEBUG("packet recv len:%zd too short",size);
        return 0;
    }
    uint16_t packetSize = deps::Decoder::pickLen(data);
    if(packetSize > deps::Decoder::maxSize()){
		LOG_ERROR("packet size:%u exceed limit:%zd",packetSize, deps::Decoder::maxSize());
        return -1;
    }
    if(packetSize > size){
		LOG_DEBUG("packet size:%u recv len:%zd too short", packetSize,  size);
        return 0;
    }
    uint32_t seq = deps::Decoder::pickSeq(data);
	uint16_t subCmd = deps::Decoder::pickSubCmd(data);
	LOG_TRACE("unpack:\n%s", deps::DumpHex(data, packetSize).c_str());

	std::shared_ptr<deps::Marshallable> pMsg;
	switch (subCmd){
		case PingMessage::cmd:
			pMsg = std::make_shared<PingMessage>();
			break;
		case PongMessage::cmd:
			pMsg = std::make_shared<PongMessage>();
			break;
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

	deps::PacketHeader header;
	deps::Decoder decoder(data, packetSize);
	decoder.deserialize(header, *pMsg);

	if(HandleMessage(header, pMsg, s)){
		return packetSize;
	}
	else{
		LOG_ERROR("message seq:%u cmd:%u handle failed", seq, subCmd);
		return -1;
	}
}

void Server::HandleClose(deps::SocketBase* s){
	LOG_INFO("close socket:%p fd:%d peer:%s:%u", s, s->GetFd(), inet_ntoa(s->GetPeerAddr().sin_addr), ntohs(s->GetPeerAddr().sin_port));
	//TODO 依赖socket状态的地方都要清除
	auto itr = m_socket2addr.find(s);
	if(itr != m_socket2addr.end()){
		PeerAddr& addr = itr->second;
		auto itr2 = m_addr2socket.find(addr);
		if(itr2 != m_addr2socket.end()){
			m_addr2socket.erase(itr2);
		}
		else{
			LOG_ERROR("socket:%p addr:%s not match", s, addr.toString().c_str());
		}
		m_socket2addr.erase(itr);
	}
	else{
		LOG_DEBUG("socket:%p is a accept socket", s);
	}
}

bool Server::HandleMessage(const deps::PacketHeader& header, std::shared_ptr<deps::Marshallable> pMsg, deps::SocketBase* s){
	bool ret = false;
	switch (header.getSubCmd()){
		case PingMessage::cmd:
			ret = HandlePingMessage(header, std::dynamic_pointer_cast<PingMessage>(pMsg), s);
			break;
		case PongMessage::cmd:
			ret = HandlePongMessage(header, std::dynamic_pointer_cast<PongMessage>(pMsg), s);
			break;
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

/**
 * @brief 获取本地地址
*/
PeerInfo Server::GetMyNodeInfo(){
	PeerInfo p;
	p.m_addr.m_ip = m_localIP;
	p.m_addr.m_port = m_localPort;
	p.m_addr.m_socketType = m_socketType;
	p.m_id = m_myUID;
	return std::move(p);
}

/**
 * @brief 同一个peer id的只允许一个地址，并且采取先到先得的原则
*/
bool Server::AddPeerInfo(std::string peerId, const PeerAddr& addr){
	if(peerId.empty()){
		LOG_ERROR("peer id is empty");
		return false;
	}
	else if(peerId == m_myUID){
		return false;
	}

	auto itr = m_peers.find(peerId);
	if(itr != m_peers.end()){
		PeerInfo& peer = itr->second;
		if(peer.m_addr != addr){
			LOG_ERROR("peer id:%s already exist but addr %s not match %s", 
				peerId.c_str(), addr.toString().c_str(), 
				peer.m_addr.toString().c_str());
			return false;
		}
	}
	else{
		PeerInfo peerinfo;
		peerinfo.m_addr = addr;
		peerinfo.m_id = peerId;
		m_peers[peerId] = peerinfo;
		updateStablePeers(peerId, addr);
	}
	return true;
}

/**
 * @brief 更新节点信息
*/
void Server::UpdatePeerInfo(std::string peerId, uint64_t rtt){
	auto itr = m_peers.find(peerId);
	if(itr != m_peers.end()){
		PeerInfo& peer = itr->second;
		peer.m_rtt = (peer.m_rtt * 3 + rtt)/4;
	}
}

/**
 * @brief 删除节点
*/
void Server::RemovePeerInfo(std::string peerId){
	m_peers.erase(peerId);
}

/**
 * @brief 同一个peer id的只允许一个地址，并且采取先到先得的原则
*/
void Server::updateStablePeers(std::string peerId, const PeerAddr& addr)
{
	for(size_t i = 0; i < m_stablePeers.size(); ++i){
		if(m_stablePeers[i].NoPeerId() && m_stablePeers[i].m_addr == addr){
			m_stablePeers[i].m_id = peerId;
		}
	}
}

/**
 * @brief 处理ping消息
*/
bool Server::HandlePingMessage(const deps::PacketHeader& header, std::shared_ptr<PingMessage> pMsg, deps::SocketBase* s){
	const std::string& peerId = pMsg->m_myInfo.m_id;
	const PeerAddr& peerAddr = pMsg->m_myInfo.m_addr;
	LOG_INFO("peer id:%s %s size:%zd", peerId.c_str(), peerAddr.toString().c_str(), pMsg->m_peers.size());

	//添加发送者信息到peer集合
	AddPeerInfo(peerId, peerAddr);

	//添加携带的peer信息到peer集合
	for(auto peer : pMsg->m_peers){
		AddPeerInfo(peer.m_id, peer.m_addr);
	}

	PongMessage rsp;
	rsp.m_timestamp = pMsg->m_timestamp;
	rsp.m_myInfo = GetMyNodeInfo();
	LOG_INFO("send pong message to peer id:%s %s", peerId.c_str(), peerAddr.toString().c_str());
	SendMessage(PongMessage::cmd, rsp, s);
	return true;
}

/**
 * @brief 处理pong消息
*/
bool Server::HandlePongMessage(const deps::PacketHeader& header, std::shared_ptr<PongMessage> pMsg, deps::SocketBase* s){
	uint64_t lastStamp = pMsg->m_timestamp;
	uint64_t now = deps::GetMonoTimeMs();
	uint64_t rtt = now > lastStamp ? now - lastStamp : 0;
	const std::string& peerId = pMsg->m_myInfo.m_id;
	PeerAddr& peerAddr = pMsg->m_myInfo.m_addr;
	LOG_INFO("peer id:%s %s rtt:%llu", peerId.c_str(), peerAddr.toString().c_str(), rtt);

	UpdatePeerInfo(peerId, rtt);
	return true;
}

/**
 * @brief 处理心跳消息
*/
bool Server::HandleHeatBeatMessage(const deps::PacketHeader& header, std::shared_ptr<HeartbeatMessage> pMsg, deps::SocketBase* s){
	const std::string& peerId = pMsg->m_myInfo.m_id;
	const PeerAddr& peerAddr = pMsg->m_myInfo.m_addr;
	const std::string& leaderUID = pMsg->m_leaderUID;
	const ProposalID& leaderProposalID = pMsg->m_leaderProposalID;
	LOG_INFO("peer id:%s %s %s", peerId.c_str(), peerAddr.toString().c_str(), 
		leaderProposalID.toString().c_str());

	m_paxosNode.receiveHeartbeat(leaderUID, leaderProposalID);
	return true;
}

/**
 * @brief 连接到指定的ip和端口
*/
deps::SocketBase* Server::Connect(uint32_t ip, int port, deps::SocketType type){
	deps::SocketBase* pSocket = nullptr;
	switch(type){
	case deps::SocketType::tcp:
		pSocket = deps::TcpSocket::Connect(ip, port, m_container, this);
		break;
	case deps::SocketType::udp:
		pSocket = deps::UdpSocket::Connect(ip, port, m_container, this);
		break;
	default:
		break;
	}
	return pSocket;
}

/**
 * @brief 发送ping消息
*/
void Server::SendPingMessage()
{
	PingMessage ping;
	ping.m_timestamp = deps::GetMonoTimeMs();
	ping.m_myInfo = GetMyNodeInfo();
	for(auto& peer : m_peers){
		ping.m_peers.insert(peer.second);
	}
	SendMessageToAllPeer(PingMessage::cmd, ping);
	LOG_INFO("send ping message timestamp:%llu size:%zd", ping.m_timestamp, ping.m_peers.size());
}

/**
 * @brief 发送消息给指定的socket
*/
bool Server::SendMessage(uint16_t cmd, const deps::Marshallable& msg, deps::SocketBase* s){
	deps::Encoder encoder;
	encoder.serialize(cmd, msg);
	if(!s->SendPacket(encoder.data(),encoder.size())){
		LOG_ERROR("fd:%d send packet failed", s->GetFd());
		return false;
	}
	return true;
}

/**
 * @brief 发送消息给指定的peer
*/
void Server::SendMessageToPeer(uint16_t cmd, const deps::Marshallable& msg, PeerAddr& addr){
	auto itr = m_addr2socket.find(addr);
	if(itr == m_addr2socket.end()){
		deps::SocketBase* pSocket = Connect(addr.m_ip, addr.m_port, addr.m_socketType);
		if(pSocket == nullptr){
			LOG_ERROR("connect to %s failed", addr.toString().c_str());
			return;
		}
		m_addr2socket[addr] = pSocket;
		m_socket2addr[pSocket] = addr;
	}
	else{
		deps::SocketBase* pSocket = itr->second;
		if(nullptr == pSocket){
			LOG_ERROR("%s get null socket", addr.toString().c_str());
			return;
		}
		SendMessage(cmd, msg, pSocket);
	}
}

/**
 * @brief 发送消息给当前所有的peer
*/
void Server::SendMessageToAllPeer(uint16_t cmd, const deps::Marshallable& msg){
	for(std::map<std::string, PeerInfo>::iterator itr = m_peers.begin(); itr!=m_peers.end(); ++itr){
		const std::string& peerid = itr->first;
		PeerInfo& peer = itr->second;
		SendMessageToPeer(cmd, msg, peer.m_addr);
		LOG_DEBUG("send message cmd:%hu to peer id:%s %s", cmd, peerid.c_str(), peer.m_addr.toString().c_str());
	}

	for(size_t i=0; i < m_stablePeers.size(); ++i){
		PeerInfo& peer  = m_stablePeers[i];
		//只有在当前的peer集合中没有找到的时候才发送
		if(m_peers.find(peer.m_id) == m_peers.end()){
			SendMessageToPeer(cmd, msg, peer.m_addr);
			LOG_DEBUG("send message cmd:%hu to stable addr[%zd] %s", cmd, i, peer.m_addr.toString().c_str());
		}
	}
	return;
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

	size_t cnt = 0;
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
	prepare.m_myInfo = GetMyNodeInfo();

	for(auto acceptorUID : m_majorityAcceptors){
		auto peerItr = m_peers.find(acceptorUID);
		if(peerItr != m_peers.end()){
			auto& peer = peerItr->second;
			SendMessageToPeer(PrepareMessage::cmd, prepare, peer.m_addr);
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
	promise.m_myInfo = GetMyNodeInfo();

	promise.m_proposalID.m_number = proposalID.m_number;
	promise.m_proposalID.m_uid = proposalID.m_uid;
	promise.m_acceptID.m_number = acceptID.m_number;
	promise.m_acceptID.m_uid = acceptID.m_uid;
	promise.m_acceptValue = acceptValue;

	auto peerItr = m_peers.find(toUID);
	if(peerItr != m_peers.end()){
		auto& peer = peerItr->second;
		SendMessageToPeer(PromiseMessage::cmd, promise, peer.m_addr);
	}
	else{
		LOG_ERROR("peer id:%s not found", toUID.c_str());
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
	accept.m_myInfo = GetMyNodeInfo();

	accept.m_proposalID.m_number = proposalID.m_number;
	accept.m_proposalID.m_uid = proposalID.m_uid;
	accept.m_proposalValue = proposalValue;

	for(auto acceptorUID : m_majorityAcceptors){
		auto peerItr = m_peers.find(acceptorUID);
		if(peerItr != m_peers.end()){
			auto& peer = peerItr->second;
			SendMessageToPeer(AcceptMessage::cmd, accept, peer.m_addr);
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
	premit.m_myInfo = GetMyNodeInfo();
	premit.m_proposalID.m_number = proposalID.m_number;
	premit.m_proposalID.m_uid = proposalID.m_uid;
	premit.m_acceptedValue = acceptedValue;

	auto peerItr = m_peers.find(proposerUID);
	if(peerItr != m_peers.end()){
		auto& peer = peerItr->second;
		SendMessageToPeer(PermitMessage::cmd, premit, peer.m_addr);
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
	ack.m_myInfo = GetMyNodeInfo();
	ack.m_proposalID.m_number = proposalID.m_number;
	ack.m_proposalID.m_uid = proposalID.m_uid;
	ack.m_promiseID.m_number = promisedID.m_number;
	ack.m_promiseID.m_uid = promisedID.m_uid;

	auto peerItr = m_peers.find(proposerUID);
	if(peerItr != m_peers.end()){
		auto& peer = peerItr->second;
		SendMessageToPeer(PrepareAckMessage::cmd, ack, peer.m_addr);
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
	ack.m_myInfo = GetMyNodeInfo();
	ack.m_proposalID.m_number = proposalID.m_number;
	ack.m_proposalID.m_uid = proposalID.m_uid;
	ack.m_promiseID.m_number = promisedID.m_number;
	ack.m_promiseID.m_uid = promisedID.m_uid;

	auto peerItr = m_peers.find(proposerUID);
	if(peerItr != m_peers.end()){
		auto& peer = peerItr->second;
		SendMessageToPeer(AcceptAckMessage::cmd, ack, peer.m_addr);
	}

}

/**
 * @brief 尝试获取leader
*/
void Server::onLeadershipAcquired()
{

}

/**
 * @brief 丢失leader
*/
void Server::onLeadershipLost()
{

}

/**
 * @brief leader变更
*/
void Server::onLeadershipChange(const std::string& previousLeaderUID, 
	const std::string& newLeaderUID)
{

}

/**
 * @brief 发送心跳
*/
void Server::sendHeartbeat(const std::string& leaderUID, const ProposalID& leaderProposalID)
{	
	HeartbeatMessage heartbeat;
	heartbeat.m_myInfo = GetMyNodeInfo();
	heartbeat.m_leaderUID = leaderUID;
	heartbeat.m_leaderProposalID.m_number = leaderProposalID.m_number;
	heartbeat.m_leaderProposalID.m_uid = leaderProposalID.m_uid;
	SendMessageToAllPeer(HeartbeatMessage::cmd, heartbeat);
	LOG_INFO("send heartbeat message leader uid:%s proposalid:%s", 
		leaderUID.c_str(), leaderProposalID.toString().c_str());
}