#include "server.h"

int main(int argc, char** argv){
	int ret = 0;
	
	char *localIp = nullptr;
	int localPort = -1;
	std::string localSip = "127.0.0.1";
	char* dstIp = nullptr;
	int dstPort = -1;
	std::string dstSip = "127.0.0.1";

    while( (ret = getopt(argc, argv, "a:b:c:d:")) != -1 ){
        switch(ret){
			case 'a':
				localIp = optarg;
				break;
			case 'b':
				localPort = atoi(optarg);
				break;
			case 'c':
				dstIp = optarg;
				break;
			case 'd':
				dstPort = atoi(optarg);
				break;
			default:
				break;
		}
    }

	if(localIp != nullptr){
		localSip = localIp;
	}
	if(dstIp != nullptr){
		dstSip = dstIp;
	}
	if(localPort == -1){
		localPort = 8888;
	}
	if(dstPort == -1){
		dstPort = 9999;
	}

	Server server;
	if(!server.Init(localSip, localPort, dstSip, dstPort)){
		return -1;
	}
	if(!server.Run()){
		return -1;
	}
    return 0;
}