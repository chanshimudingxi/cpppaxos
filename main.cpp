#include "server.h"

int main(int argc, char** argv){
	int ret = 0;
	
	setLogLevel(LOG_LEVEL_DEBUG);

	std::string mySID = "123456789";
	char* myID;
	char *localIp = nullptr;
	std::string localSip = "127.0.0.1";
	int localTcpPort = -1;
	int localUdpPort = -1;
	char* dstIp = nullptr;
	std::string dstSip = "127.0.0.1";
	int dstTcpPort = -1;
	int dstUdpPort = -1;

    while( (ret = getopt(argc, argv, "s:a:b:c:d:e:f:")) != -1 ){
        switch(ret){
			case 's':
				myID = optarg;
				break;
			case 'a':
				localIp = optarg;
				break;
			case 'b':
				localTcpPort = atoi(optarg);
				break;
			case 'c':
				localUdpPort = atoi(optarg);
				break;
			case 'd':
				dstIp = optarg;
				break;
			case 'e':
				dstTcpPort = atoi(optarg);
				break;
			case 'f':
				dstUdpPort = atoi(optarg);
				break;
			default:
				break;
		}
    }

	if(myID != nullptr){
		mySID = myID;
	}
	if(localIp != nullptr){
		localSip = localIp;
	}
	if(dstIp != nullptr){
		dstSip = dstIp;
	}
	if(localTcpPort == -1){
		localTcpPort = 10000;
	}
	if(localUdpPort == -1){
		localUdpPort = 20000;
	}
	if(dstTcpPort == -1){
		dstTcpPort = 10001;
	}
	if(dstUdpPort == -1){
		dstUdpPort = 20001;
	}

	Server server;
	if(!server.Init(mySID, localSip, localTcpPort, localUdpPort, dstSip, dstTcpPort, dstUdpPort)){
		return -1;
	}
	if(!server.Run()){
		return -1;
	}
    return 0;
}