#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <signal.h>
#ifndef __APPLE__
#include <sys/prctl.h>
#endif

#include "sys/tool.h"
#include "sys/util.h"
#include "sys/log.h"
#include "net/socket_base.h"

#include "server.h"

int set_openfd_limit(unsigned long limitSize){
	struct rlimit limit;

	if (getrlimit(RLIMIT_NOFILE,&limit) == -1) {
        LOG_ERROR("unable to obtain the current NOFILE limit (%s)", strerror(errno));
		return -1;
	}

	rlim_t oldlimit = limit.rlim_cur;
	rlim_t oldlimitmax = limit.rlim_max;

	limit.rlim_cur = static_cast<rlim_t>(limitSize);
	limit.rlim_max = static_cast<rlim_t>(limitSize);
	if (setrlimit(RLIMIT_NOFILE, &limit) == -1) {
		LOG_ERROR("unable to set the current NOFILE limit (%s)", strerror(errno));
		return -1;
	}

	LOG_INFO("set open files old limit: %lu:%lu to limit: %lu:%lu success", 
		oldlimit, oldlimitmax, limitSize, limitSize);
	return 0;
}

int enalbe_coredump(){
	struct rlimit limit;

	if (getrlimit(RLIMIT_CORE,&limit) == -1) {
        LOG_ERROR("unable to obtain the current CORE limit (%s)", strerror(errno));
		return -1;
	}

	rlim_t oldlimit = limit.rlim_cur;
	rlim_t oldlimitmax = limit.rlim_max;

	limit.rlim_cur = RLIM_INFINITY;
	limit.rlim_max = RLIM_INFINITY;
	if (setrlimit(RLIMIT_CORE, &limit) == -1) {
		LOG_ERROR("unable to set the current CORE limit (%s)", strerror(errno));
		return -1;
	}

#ifndef __APPLE__
    if(prctl(PR_SET_DUMPABLE, 1) == -1){
		LOG_ERROR("unable to PR_SET_DUMPABLE (%s)", strerror(errno));
		return -1;
	}
#endif
	LOG_INFO("set coredump file old limit: %lu:%lu to unlimited success", oldlimit, oldlimitmax);
	return 0;
}

int main(int argc, char** argv){
	int ret = deps::daemonize();
	if(ret < 0){
		return -1;
	}

	if(argc < 2){
		fprintf(stderr, "Usage: %s log_path -s myID -t tcp/udp -x localIP -y localPort -m dstIP -n dstPort\n", argv[0]);
		return -1;
	}

	pid_t pid = getpid();
	const std::string logfile(argv[1]);
	ret = setlogfile(logfile);
	if(ret < 0){
		return -2;
	}
	setloglevel(deps::Logger::DEBUG);

	//忽略信号
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

	ret = set_openfd_limit(500000);
	if(ret < 0){
		return -3;
	}

	ret = enalbe_coredump();
	if(ret < 0){
		return -4;
	}

	srandom(time(NULL));

	char* myID;
	char *localIp = nullptr;
	char* dstIp = nullptr;
	char* localPort = nullptr;
	char* dstPort = nullptr;
	char* conntype = nullptr;
    while( (ret = getopt(argc, argv, "s:x:y:m:n:t:")) != -1 ){
        switch(ret){
			case 's':
				myID = optarg;
				break;
			case 't':
				conntype = optarg;
				break;
			case 'x':
				localIp = optarg;
				break;
			case 'y':
				localPort = optarg;
				break;
			case 'm':
				dstIp = optarg;
				break;
			case 'n':
				dstPort = optarg;
				break;
			default:
				break;
		}
    }

	std::string mySID = myID != nullptr ? myID : "123456789";
	std::string localSip = localIp != nullptr ? localIp : "127.0.0.1";
	int iLocalPort = localPort != nullptr ? atoi(localPort) : 10000;
	std::string dstSip = dstIp != nullptr ? dstIp : "127.0.0.1";
	int iDstSPort = dstPort != nullptr ? atoi(dstPort) : 20000;
	deps::SocketType type = deps::SocketType::tcp;
	if(conntype != nullptr && strncmp(conntype,"udp", 3) == 0){
		type = deps::SocketType::udp;
	}

	LOG_INFO("pname:%s pid:%d log path %s", argv[0], pid, logfile.c_str());
	LOG_INFO("mySID: %s, type:%s localIP: %s, localPort: %d, dstIP: %s, dstPort: %d", 
		mySID.c_str(), deps::SocketBase::toString(type).c_str(), localSip.c_str(), 
		iLocalPort, dstSip.c_str(), iDstSPort);
	Server server(mySID, 3);
	if(!server.Init(type, localSip, iLocalPort, dstSip, iDstSPort)){
		return -1;
	}
	if(!server.Run()){
		return -1;
	}
    return 0;
}