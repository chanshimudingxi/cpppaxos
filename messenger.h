#pragma once

#include "proposalid.h"

class HeartbeatCallback 
{	
public:
	virtual void execute(){}
};

class Messenger
{
public:
    //发送prepare请求
    virtual void sendPrepare(const ProposalID& proposalID){}
    //发送prepare请求响应
    virtual void sendPromise(const std::string& proposerUID, const ProposalID& proposalID, 
        const ProposalID& previousID, const std::string& acceptValue){}
    //发送accept请求
    virtual void sendAccept(const ProposalID&  proposalID, const std::string& proposalValue){}
    //发送accept请求响应
    virtual void sendAccepted(const ProposalID&  proposalID, const std::string& acceptedValue){}
    //解决
    virtual void onResolution(const ProposalID&  proposalID, const std::string& value){}


	virtual void sendPrepareNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID){}
	virtual void sendAcceptNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID){}
	virtual void onLeadershipAcquired(){}

	virtual void sendHeartbeat(const ProposalID& leaderProposalID){}
	virtual void schedule(long millisecondDelay, HeartbeatCallback callback){}
	virtual void onLeadershipLost(){}
	virtual void onLeadershipChange(const std::string& previousLeaderUID, const std::string& newLeaderUID){}
};