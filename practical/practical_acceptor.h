#pragma once

#include "essential/acceptor.h"
#include "essential/proposal_id.h"

class PracticalAcceptor : public Acceptor
{
public:
	virtual bool persistenceRequired();
	virtual void recover(const ProposalID& promisedID, const ProposalID& acceptedID, const std::string& acceptedValue);
	virtual void persisted();
	virtual bool isActive();
	virtual void setActive(bool active);
};