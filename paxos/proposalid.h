#pragma once

#include <string>

class ProposalID
{
public:
    ProposalID();
    ProposalID(int number, std::string uid);
	ProposalID& operator=(const ProposalID& id);
    ~ProposalID();

    bool isValid() const;
    int getNumber() const;
    void setNumber(int number);
    void incrementNumber();
    std::string getUID() const;

    int compare(const ProposalID& id) const;
	bool operator<(const ProposalID& id) const;
	bool operator==(const ProposalID& id) const;
	bool operator>(const ProposalID& id) const;
	bool operator!=(const ProposalID& id) const;
public:
    int m_number;
    std::string m_uid;
};