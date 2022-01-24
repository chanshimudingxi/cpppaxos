#ifndef CLIENT_H_
#define CLIENT_H_

struct Client{
	Client(std::string ip, int port, SocketType type):
		m_ip(ip), m_port(port), m_socketType(type),
		m_alive(false), m_fd(-1) {}
	~Client(){}
	std::string m_ip;
	int m_port;
	SocketType m_socketType;
	bool m_alive;
	int m_fd;
};

#endif