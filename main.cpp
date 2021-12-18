#include "node.h"

int main(int argc, char* arvg[]){
	Node node;
	if(!node.Init()){
		return -1;
	}
	if(!node.Run()){
		return -1;
	}
    return 0;
}