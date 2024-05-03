#ifndef PEER_H_
#define PEER_H_

#include "net/marshall.h"
#include "sys/util.h"
#include "net/socket_base.h"

#include <stdint.h>
#include <string>


struct PeerAddr: public Marshallable{
	PeerAddr():m_ip(0),m_port(0),m_socketType(SocketType::tcp),m_fd(-1),m_rtt(100){}
	~PeerAddr(){}

	virtual void marshal(Pack & pk) const{
		uint8_t socketType = m_socketType == SocketType::tcp ? 0 : 1;
		pk << m_ip << m_port << socketType;
	}

	virtual void unmarshal(const Unpack &up){
		uint8_t socketType;
		up >> m_ip >> m_port >> socketType;
		socketType == 0 ? m_socketType = SocketType::tcp : m_socketType = SocketType::udp;
	}

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

struct PeerInfo: public Marshallable{
	PeerInfo(){}
	~PeerInfo(){}
	std::string m_id;
	PeerAddr m_addr;

	virtual void marshal(Pack & pk) const{
		pk << m_id << m_addr;
	}

	virtual void unmarshal(const Unpack &up){
		up >> m_id >> m_addr;
	}
};

#endif