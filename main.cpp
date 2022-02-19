#include "server.h"

int main(int argc, char** argv){
	int ret = 0;
	
	std::string mySID = "123456789";
	char* myID;
	char *localIp = nullptr;
	int localPort = -1;	//paxos协议端口
	int localCPort = -1;	//业务信令端口
	std::string localSip = "127.0.0.1";
	char* dstIp = nullptr;
	int dstPort = -1;
	std::string dstSip = "127.0.0.1";

    while( (ret = getopt(argc, argv, "s:a:b:c:d:f:")) != -1 ){
        switch(ret){
			case 's':
				myID = optarg;
				break;
			case 'a':
				localIp = optarg;
				break;
			case 'b':
				localPort = atoi(optarg);
				break;
			case 'c':
				localCPort = atoi(optarg);
				break;
			case 'd':
				dstIp = optarg;
				break;
			case 'f':
				dstPort = atoi(optarg);
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
	if(localPort == -1){
		localPort = 10000;
	}
	if(localCPort == -1){
		localCPort = 20000;
	}
	if(dstPort == -1){
		dstPort = 20001;
	}

	Server server;
	if(!server.Init(mySID, localSip, localPort, localCPort, dstSip, dstPort)){
		return -1;
	}
	if(!server.Run()){
		return -1;
	}
    return 0;
}