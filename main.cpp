#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#ifndef __APPLE__
#include <sys/prctl.h>
#endif
#include "server.h"
#include "sys/util.h"

int daemonize()
{
    switch (fork()) {
    case -1:
        LOG_ERROR("fork() failed");
        return -1;

    case 0:
        break;

    default:
        exit(0);
    }

    if (setsid() == -1){
        LOG_ERROR("setsid failed");
        return -1;
    }

    umask(0);

    chdir("/");

    int fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
        LOG_ERROR("open /dev/null failed");
        return -1;
    }

    if (dup2(fd, STDIN_FILENO) == -1) {
        LOG_ERROR("dup2(STDIN) failed");
        return -1;
    }

    if (dup2(fd, STDOUT_FILENO) == -1) {
        LOG_ERROR("dup2(STDOUT) failed");
        return -1;
    }

    if (dup2(fd, STDERR_FILENO) == -1) {
        LOG_ERROR("dup2(STDOUT) failed");
        return -1;
    }
	
	if (fd > STDERR_FILENO) {
		if (close(fd) == -1) {
			LOG_ERROR("close() fd failed");
			return -1;
		}
	}

    return 0;
}

int set_openfd_limit(rlim_t limitSize){
	struct rlimit limit;

	if (getrlimit(RLIMIT_NOFILE,&limit) == -1) {
        LOG_ERROR("unable to obtain the current NOFILE limit (%s)", strerror(errno));
		return -1;
	}

	rlim_t oldlimit = limit.rlim_cur;
	rlim_t oldlimitmax = limit.rlim_max;

	limit.rlim_cur = limitSize;
	limit.rlim_max = limitSize;
	if (setrlimit(RLIMIT_NOFILE, &limit) == -1) {
		LOG_ERROR("unable to set the current NOFILE limit (%s)", strerror(errno));
		return -1;
	}

	LOG_INFO("set open files old limit: %llu:%llu to limit: %llu:%llu success", 
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
	LOG_INFO("set coredump file old limit: %llu:%llu to unlimited success", oldlimit, oldlimitmax);
	return 0;
}

void set_signal_handler(){
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
}

int main(int argc, char** argv){
	int ret = 0;

	const std::string logfile = Util::getCWD() + "/log";
	ret = setlogfile(logfile);
	if(ret < 0){
		fprintf(stderr, "setlogfile failed");
		return -1;
	}
	setloglevel(Logger::INFO);

	// if(daemonize() == -1){
	// 	return -1;
	// }

	set_signal_handler();

	if(set_openfd_limit(500000) == -1){
		return -1;
	}

	if(enalbe_coredump() == -1){
		return -1;
	}

	srandom(time(NULL));

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

	Server server(mySID, 3);
	if(!server.Init(localSip, localTcpPort, localUdpPort, dstSip, dstTcpPort, dstUdpPort)){
		return -1;
	}
	if(!server.Run()){
		return -1;
	}
    return 0;
}