#include "server.h"

int main(int argc, char** argv){
	int ret = 0;
	int port = 0;
    while( (ret = getopt(argc, argv, "p:")) != -1 )
    {
        switch(ret)
		{
			case 'p':
				port = atoi(optarg);
				break;
			default:
				break;
		}
    }

	if(port == -1){
		LOG_DEBUG("invalid run args");
		return -1;
	}

	Server server;
	if(!server.Init(port)){
		return -1;
	}
	if(!server.Run()){
		return -1;
	}
    return 0;
}