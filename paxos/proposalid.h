#pragma once

#include <string>

#include "net/marshall.h"

/**
 * @brief 议题编号：议题编号由一个只能持续递增的数字和一个唯一的标识符组成，具有全局唯一性。
 * 
 */
class ProposalID : public deps::Marshallable
{
public:
    ProposalID();
    ProposalID(int number, std::string uid);
    ProposalID(const ProposalID& id);
	ProposalID& operator=(const ProposalID& id);
    ~ProposalID();

    bool isValid() const;
    int getNumber() const;
    void setNumber(int number);
    void incrementNumber();
    std::string toString() const;
    int compare(const ProposalID& id) const;
	bool operator<(const ProposalID& id) const;
	bool operator==(const ProposalID& id) const;
	bool operator>(const ProposalID& id) const;
	bool operator!=(const ProposalID& id) const;
    bool operator<=(const ProposalID& id) const;
    bool operator>=(const ProposalID& id) const;
    void marshal(deps::Pack & pk) const;
    void unmarshal(const deps::Unpack &up);
public:
    uint32_t m_number;
    std::string m_uid;
};