#pragma once

#include "proposalid.h"

class Messenger
{
public:
    //发送prepare请求
    virtual void sendPrepare(const ProposalID& proposalID) = 0;
    //发送prepare请求的承诺
    virtual void sendPromise(const std::string& proposerUID, const ProposalID& proposalID, 
        const ProposalID& acceptID, const std::string& acceptValue) = 0;
    //发送accept请求
    virtual void sendAccept(const ProposalID&  proposalID, 
		const std::string& proposalValue) = 0;
    //发送accept请求的批准
    virtual void sendAccepted(const ProposalID&  proposalID, 
		const std::string& acceptedValue) = 0;
    //解决
    virtual void onResolution(const ProposalID&  proposalID, 
		const std::string& value) = 0;

	//发送prepare请求的ack
	virtual void sendPrepareNACK(const std::string& proposerUID, const ProposalID& proposalID, 
		const ProposalID& promisedID)= 0;
	//发送accept请求的ack
	virtual void sendAcceptNACK(const std::string& proposerUID, const ProposalID& proposalID, 
		const ProposalID& promisedID) = 0;
	//尝试获取leader
	virtual void onLeadershipAcquired() = 0;

	//发送心跳
	virtual void sendHeartbeat(const ProposalID& leaderProposalID) = 0;
	//丢失主
	virtual void onLeadershipLost() = 0;
	//主变更
	virtual void onLeadershipChange(const std::string& previousLeaderID, 
		const std::string& newLeaderID) = 0;
};