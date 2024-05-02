#pragma once

#include <cstring>
#include <iostream>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include <memory>
#include <sstream>

#include "net/tcp_socket.h"
#include "net/udp_socket.h"
#include "net/epoll_container.h"
#include "net/packet_handler.h"
#include "net/packet.h"
#include "sys/logger.h"

#include "paxos/proto.h"
#include "paxos/paxos_node.h"
#include "paxos/messenger.h"

struct PeerAddr{
	PeerAddr():m_ip(0),m_port(0),m_socketType(SocketType::tcp),m_fd(-1),m_rtt(100){}
	~PeerAddr(){}

	bool operator !=(const PeerAddr& addr) const{
		return m_ip != addr.m_ip || m_port != addr.m_port || m_socketType != addr.m_socketType;
	}
	bool operator ==(const PeerAddr& addr) const{
		return m_ip == addr.m_ip && m_port == addr.m_port && m_socketType == addr.m_socketType;
	}
	bool operator <(const PeerAddr& addr) const{
		if(m_ip < addr.m_ip){
			return true;
		}else if(m_ip == addr.m_ip){
			if(m_port < addr.m_port){
				return true;
			}else if(m_port == addr.m_port){
				return m_socketType < addr.m_socketType;
			}
		}
		return false;
	}
	bool operator >(const PeerAddr& addr) const{
		if(m_ip > addr.m_ip){
			return true;
		}else if(m_ip == addr.m_ip){
			if(m_port > addr.m_port){
				return true;
			}else if(m_port == addr.m_port){
				return m_socketType > addr.m_socketType;
			}
		}
		return false;
	}

	bool operator <=(const PeerAddr& addr) const{
		return *this < addr || *this == addr;
	}

	bool operator >=(const PeerAddr& addr) const{
		return *this > addr || *this == addr;
	}

	std::string toString() const{
		std::stringstream os;
		os<<"ip:"<<Util::UintIP2String(m_ip)
			<<" port:"<<m_port
			<<" type:"<<SocketBase::toString(m_socketType);
		return os.str();
	}

	uint32_t m_ip;
	uint16_t m_port;
	SocketType m_socketType;
	int64_t m_rtt;
	int m_fd;
};

struct Peer{
	Peer(){}
	~Peer(){}

	std::string m_id;
	PeerAddr m_addr;
};

class Server : public Messenger, public PacketHandler, std::enable_shared_from_this<Server>
{
public:
    Server(const std::string& myid, int quorumSize);
    ~Server();
	bool Init(const std::string& localIP, uint16_t localTcpPort, uint16_t localUdpPort, 
		const std::string& dstIP, uint16_t dstTcpPort, uint16_t dstUdpPort);
	bool Run();
	bool Listen(int port, int backlog, SocketType type);
	
    virtual int HandlePacket(const char* data, size_t size, SocketBase* s);
	virtual void HandleClose(SocketBase* s);

	bool HandleMessage(const PacketHeader& header, std::shared_ptr<Marshallable> pMsg, SocketBase* s);
	//处理心跳消息
	bool HandleHeatBeatMessage(const PacketHeader& header, std::shared_ptr<HeartbeatMessage> pMsg, SocketBase* s);
	bool HandleHeatBeatMessage(const PacketHeader& header, std::shared_ptr<HeartbeatMessageRsp> pMsg, SocketBase* s);

	bool Connect(uint32_t ip, int port, SocketType type, int* pfd);
	bool SendMessage(uint16_t cmd, const Marshallable& msg, SocketBase* s);
	void SendMessageToPeer(uint16_t cmd, const Marshallable& msg, PeerAddr& addr);
	void SendMessageToAllPeer(uint16_t cmd, const Marshallable& msg);
	void SendMessageToStablePeer(uint16_t cmd, const Marshallable& msg);

	//获取本地地址
	PPeerAddr GetMyTcpAddr();
	PPeerAddr GetMyUdpAddr();
	
	//选择Acceptor大多数
	void SelectMajorityAcceptors(std::set<std::string>& acceptors);
    //发送prepare请求
    virtual void sendPrepare(const ProposalID& proposalID);
    //发送prepare请求的承诺
    virtual void sendPromise(const std::string& toUID, const ProposalID& proposalID, 
        const ProposalID& acceptID, const std::string& acceptValue);
    //发送accept请求
    virtual void sendAccept(const ProposalID&  proposalID, 
		const std::string& proposalValue);
    //发送accept请求的批准
    virtual void sendPermit(const std::string& proposerUID, const ProposalID&  proposalID, 
		const std::string& acceptedValue);
    //解决
    virtual void onResolution(const ProposalID&  proposalID, 
		const std::string& value);

	//发送prepare请求的ack
	virtual void sendPrepareNACK(const std::string& proposerUID, const ProposalID& proposalID, 
		const ProposalID& promisedID);
	//发送accept请求的ack
	virtual void sendAcceptNACK(const std::string& proposerUID, const ProposalID& proposalID, 
		const ProposalID& promisedID);
	//尝试获取leader
	virtual void onLeadershipAcquired();

	//丢失主
	virtual void onLeadershipLost();
	//主变更
	virtual void onLeadershipChange(const std::string& previousLeaderUID, 
		const std::string& newLeaderUID);
	//发送心跳
	virtual void sendHeartbeat(const ProposalID& leaderProposalID);
private:
	//连接管理容器
	EpollContainer* m_container;

	//自己的节点信息
	std::string m_myUID;
	uint32_t m_localIP;
	uint16_t m_localTcpPort;
	uint16_t m_localUdpPort;
	//集群稳定的节点，相当于P2P网络中稳定的公有节点
	std::vector<PeerAddr> m_stableAddrs;

	//集群其他节点
	std::map<std::string, Peer> m_peers;
	//已经选择过的最大的节点id
	std::string m_maxChoosenAcceptorUID;
	//Acceptors集合
	std::set<std::string> m_majorityAcceptors;
	int m_quorumSize;

	PaxosNode m_paxosNode;
};
