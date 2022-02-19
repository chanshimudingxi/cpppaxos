#include "server.h"

int main(int argc, char* arvg[]){
	Server server;
	if(!server.Init()){
		return -1;
	}
	if(!server.Run()){
		return -1;
	}
    return 0;
}