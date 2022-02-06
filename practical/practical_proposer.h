#pragma once

#include "essential/proposer.h"

class PracticalProposer : public Proposer
{
public:
	virtual void prepare(bool incrementProposalNumber);
	virtual void observeProposal(const std::string& fromUID, const ProposalID& proposalID);
	virtual void receivePrepareNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID);
	virtual void receiveAcceptNACK(const std::string& proposerUID, const ProposalID& proposalID, const ProposalID& promisedID);
	virtual void resendAccept();
	virtual bool isLeader();
	virtual void setLeader(bool leader);
	virtual bool isActive();
	virtual void setActive(bool active);
};