#pragma once

#include "essential/messenger.h"
#include "essential/proposal_id.h"

class PracticalMessenger : public Messenger
{
public:
	void sendPrepareNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID);
	void sendAcceptNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID);
	void onLeadershipAcquired();
};
