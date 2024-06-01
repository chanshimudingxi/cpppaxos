#ifndef PEER_H_
#define PEER_H_

#include "net/marshall.h"
#include "sys/util.h"
#include "net/socket_base.h"

#include <stdint.h>
#include <string>

struct PeerAddr: public deps::Marshallable{
	PeerAddr():m_ip(0),m_port(0),m_socketType(deps::SocketType::tcp){}
	PeerAddr(const PeerAddr& addr):m_ip(addr.m_ip),m_port(addr.m_port),m_socketType(addr.m_socketType){}
	PeerAddr& operator =(const PeerAddr& addr){
		m_ip = addr.m_ip;
		m_port = addr.m_port;
		m_socketType = addr.m_socketType;
		return *this;
	}
	~PeerAddr(){}

	virtual void marshal(deps::Pack & pk) const{
		uint8_t socketType = m_socketType == deps::SocketType::tcp ? 0 : 1;
		pk << m_ip << m_port << socketType;
	}

	virtual void unmarshal(const deps::Unpack &up){
		uint8_t socketType;
		up >> m_ip >> m_port >> socketType;
		socketType == 0 ? m_socketType = deps::SocketType::tcp : m_socketType = deps::SocketType::udp;
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
		os<<"ip:"<<deps::UintIP2String(m_ip)
			<<" port:"<<m_port
			<<" type:"<<deps::SocketBase::toString(m_socketType);
		return os.str();
	}

	uint32_t m_ip;
	uint16_t m_port;
	deps::SocketType m_socketType;
};

struct PeerInfo: public deps::Marshallable{
	PeerInfo(){}
	PeerInfo(const PeerInfo& p):m_id(p.m_id), m_addr(p.m_addr), m_rtt(100){
	}
	PeerInfo& operator = (const PeerInfo& p){
		m_id = p.m_id;
		m_addr = p.m_addr;
		m_rtt = p.m_rtt;
		return *this;
	}
	~PeerInfo(){}

	bool NoPeerId() const{
		return m_id.empty();
	}

	virtual void marshal(deps::Pack & pk) const{
		pk << m_id << m_addr;
	}

	virtual void unmarshal(const deps::Unpack &up){
		up >> m_id >> m_addr;
	}

	bool operator != (const PeerInfo& p) const{
		return m_id != p.m_id;
	}
	bool operator == (const PeerInfo& p) const{
		return m_id == p.m_id;
	}
	bool operator < (const PeerInfo& p) const{
		return m_id < p.m_id;
	}
	bool operator > (const PeerInfo& p) const{
		return m_id > p.m_id;
	}
	bool operator <=(const PeerInfo& p) const{
		return *this < p || *this == p;
	}
	bool operator >=(const PeerInfo& p) const{
		return *this > p || *this == p;
	}

	std::string m_id;
	PeerAddr m_addr;
	uint64_t m_rtt;
};

#endif