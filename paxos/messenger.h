#pragma once

#include "proposalid.h"

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

	//发送prepare请求的ack
	virtual void sendPrepareNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID){}
	//发送accept请求的ack
	virtual void sendAcceptNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID){}
	//获得主
	virtual void onLeadershipAcquired(){}

	//发送心跳
	virtual void sendHeartbeat(const ProposalID& leaderProposalID){}
	//丢失主
	virtual void onLeadershipLost(){}
	//主变更
	virtual void onLeadershipChange(const std::string& previousLeaderUID, const std::string& newLeaderUID){}
};