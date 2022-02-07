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



	
PaxosNode::PaxosNode(Messenger messenger, String proposerUID, int quorumSize) 
{
	m_proposer = new PracticalProposerImpl(messenger, proposerUID, quorumSize);
	m_acceptor = new PracticalAcceptorImpl(messenger);
	m_learner  = new EssentialLearnerImpl(messenger, quorumSize);
}
	
boolean PaxosNode::isActive() 
{
	return proposer.isActive();
}

void PaxosNode::setActive(boolean active) {
	proposer.setActive(active);
	acceptor.setActive(active);
}

//-------------------------------------------------------------------------
// Learner
//
boolean PaxosNode::isComplete() 
{
	return learner.isComplete();
}

void PaxosNode::receiveAccepted(String fromUID, ProposalID proposalID,Object acceptedValue) 
{
	learner.receiveAccepted(fromUID, proposalID, acceptedValue);

}

Object PaxosNode::getFinalValue() 
{
	return learner.getFinalValue();
}

ProposalID PaxosNode::getFinalProposalID() 
{
	return learner.getFinalProposalID();
}

//-------------------------------------------------------------------------
// Acceptor
//
void PaxosNode::receivePrepare(String fromUID, ProposalID proposalID) 
{
	acceptor.receivePrepare(fromUID, proposalID);
}

void PaxosNode::receiveAcceptRequest(String fromUID, ProposalID proposalID,Object value) 
{
	acceptor.receiveAcceptRequest(fromUID, proposalID, value);
}

ProposalID PaxosNode::getPromisedID() 
{
	return acceptor.getPromisedID();
}

ProposalID PaxosNode::getAcceptedID() 
{
	return acceptor.getAcceptedID();
}

Object PaxosNode::getAcceptedValue() 
{
	return acceptor.getAcceptedValue();
}

boolean PaxosNode::persistenceRequired() 
{
	return acceptor.persistenceRequired();
}

void PaxosNode::recover(ProposalID promisedID, ProposalID acceptedID, Object acceptedValue){
	acceptor.recover(promisedID, acceptedID, acceptedValue);
}

void PaxosNode::persisted() 
{
	acceptor.persisted();
}

//-------------------------------------------------------------------------
// Proposer
//
void PaxosNode::setProposal(Object value)
{
	proposer.setProposal(value);
}

void PaxosNode::prepare() 
{
	proposer.prepare();
}

void PaxosNode::prepare( boolean incrementProposalNumber ) 
{
	proposer.prepare(incrementProposalNumber);
}

void PaxosNode::receivePromise(String fromUID, ProposalID proposalID, ProposalID prevAcceptedID, Object prevAcceptedValue) 
{
	proposer.receivePromise(fromUID, proposalID, prevAcceptedID, prevAcceptedValue);
}

PracticalMessenger PaxosNode::getMessenger() 
{
	return proposer.getMessenger();
}

String PaxosNode::getProposerUID() 
{
	return proposer.getProposerUID();
}

int PaxosNode::getQuorumSize() 
{
	return proposer.getQuorumSize();
}

ProposalID PaxosNode::getProposalID() 
{
	return proposer.getProposalID();
}

Object PaxosNode::getProposedValue() 
{
	return proposer.getProposedValue();
}

ProposalID PaxosNode::getLastAcceptedID() 
{
	return proposer.getLastAcceptedID();
}

int PaxosNode::numPromises() 
{
	return proposer.numPromises();
}

void PaxosNode::observeProposal(String fromUID, ProposalID proposalID) 
{
	proposer.observeProposal(fromUID, proposalID);
}

void PaxosNode::receivePrepareNACK(String proposerUID, ProposalID proposalID, ProposalID promisedID) 
{
	proposer.receivePrepareNACK(proposerUID, proposalID, promisedID);
}

void PaxosNode::receiveAcceptNACK(String proposerUID, ProposalID proposalID, ProposalID promisedID) 
{
	proposer.receiveAcceptNACK(proposerUID, proposalID, promisedID);
}

void PaxosNode::resendAccept() 
{
	proposer.resendAccept();
}

boolean PaxosNode::isLeader() 
{
	return proposer.isLeader();
}

void PaxosNode::setLeader(boolean leader)
{
	proposer.setLeader(leader);
}

PaxosNode::HeartbeatNode(HeartbeatMessenger messenger, String proposerUID, int quorumSize, String leaderUID, int heartbeatPeriod, int livenessWindow) 
{
	super(messenger, proposerUID, quorumSize);
	
	this.messenger       = messenger;
	this.leaderUID       = leaderUID;
	this.heartbeatPeriod = heartbeatPeriod;
	this.livenessWindow  = livenessWindow;
	
	leaderProposalID       = null;
	lastHeartbeatTimestamp = timestamp();
	lastPrepareTimestamp   = timestamp();
	
	if (leaderUID != null && proposerUID.equals(leaderUID))
		setLeader(true);
}

long PaxosNode::timestamp() 
{
	return System.currentTimeMillis();
}

String PaxosNode::getLeaderUID() 
{
	return leaderUID;
}

ProposalID PaxosNode::getLeaderProposalID() 
{
	return leaderProposalID;
}

void PaxosNode::setLeaderProposalID( ProposalID newLeaderID ) 
{
	leaderProposalID = newLeaderID;
}

boolean PaxosNode::isAcquiringLeadership() 
{
	return acquiringLeadership;
}

void PaxosNode::prepare(boolean incrementProposalNumber) 
{
	if (incrementProposalNumber)
		acceptNACKs.clear();
	super.prepare(incrementProposalNumber);
}

boolean PaxosNode::leaderIsAlive() 
{
	return timestamp() - lastHeartbeatTimestamp <= livenessWindow;
}

boolean PaxosNode::observedRecentPrepare()
{
	return timestamp() - lastPrepareTimestamp <= livenessWindow * 1.5;
}

void PaxosNode::pollLiveness() 
{
	if (!leaderIsAlive() && !observedRecentPrepare()) {
		if (acquiringLeadership)
			prepare();
		else
			acquireLeadership();
	}
}

void PaxosNode::receiveHeartbeat(String fromUID, ProposalID proposalID) 
{
	
	if (leaderProposalID == null || proposalID.isGreaterThan(leaderProposalID)) {
		acquiringLeadership = false;
		String oldLeaderUID = leaderUID;
		
		leaderUID        = fromUID;
		leaderProposalID = proposalID;
		
		if (isLeader() && !fromUID.equals(getProposerUID())) {
			setLeader(false);
			messenger.onLeadershipLost();
			observeProposal(fromUID, proposalID);
		}
		
		messenger.onLeadershipChange(oldLeaderUID, fromUID);
	}
	
	if (leaderProposalID != null && leaderProposalID.equals(proposalID))
		lastHeartbeatTimestamp = timestamp();
}

void PaxosNode::pulse() 
{
	if (isLeader()) {
		receiveHeartbeat(getProposerUID(), getProposalID());
		messenger.sendHeartbeat(getProposalID());
		messenger.schedule(heartbeatPeriod, new HeartbeatCallback () { 
			void execute() { pulse(); }
		});
	}
}

void PaxosNode::acquireLeadership() 
{
	if (leaderIsAlive())
		acquiringLeadership = false;
	else {
		acquiringLeadership = true;
		prepare();
	}
}

void PaxosNode::receivePrepare(String fromUID, ProposalID proposalID) 
{
	super.receivePrepare(fromUID, proposalID);
	if (!proposalID.equals(getProposalID()))
		lastPrepareTimestamp = timestamp();
}

void PaxosNode::receivePromise(String fromUID, ProposalID proposalID, ProposalID prevAcceptedID, Object prevAcceptedValue)
{
	String preLeaderUID = leaderUID;
	
	super.receivePromise(fromUID, proposalID, prevAcceptedID, prevAcceptedValue);
	
	if (preLeaderUID == null && isLeader()) {
		String oldLeaderUID = getProposerUID();
		
		leaderUID           = getProposerUID();
		leaderProposalID    = getProposalID();
		acquiringLeadership = false;
		
		pulse();
		
		messenger.onLeadershipChange(oldLeaderUID, leaderUID);
	}
}

void PaxosNode::receivePrepareNACK(String proposerUID, ProposalID proposalID, ProposalID promisedID) 
{
	super.receivePrepareNACK(proposerUID, proposalID, promisedID);
	
	if (acquiringLeadership)
		prepare();
}

void PaxosNode::receiveAcceptNACK(String proposerUID, ProposalID proposalID, ProposalID promisedID) 
{
	super.receiveAcceptNACK(proposerUID, proposalID, promisedID);
	
	if (proposalID.equals(getProposalID()))
		acceptNACKs.add(proposerUID);
	
	if (isLeader() && acceptNACKs.size() >= getQuorumSize()) {
		setLeader(false);
		leaderUID        = null;
		leaderProposalID = null;
		messenger.onLeadershipLost();
		messenger.onLeadershipChange(getProposerUID(), null);
		observeProposal(proposerUID, proposalID);
	}
}
