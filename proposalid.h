#pragma once

#include <string>

class ProposalID
{
public:
    ProposalID();
    ProposalID(int number, std::string uid);
	ProposalID& operator=(const ProposalID& id);
    ~ProposalID();

    bool isValid();
    int getNumber();
    void setNumber(int number);
    void incrementNumber();
    std::string getUID();

    int compare(const ProposalID& id);    
    bool operator>(const ProposalID& id);
    bool operator<(const ProposalID& id);
    bool operator==(const ProposalID& id);
    bool operator!=(const ProposalID& id);
public:
    int m_number;
    std::string m_uid;
};