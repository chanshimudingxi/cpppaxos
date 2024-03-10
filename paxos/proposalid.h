#pragma once

#include <string>

/**
 * @brief 议题编号：议题编号由一个只能持续递增的数字和一个唯一的标识符组成，具有全局唯一性。
 * 
 */
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
    std::string getID() const;

    int compare(const ProposalID& id) const;
	bool operator<(const ProposalID& id) const;
	bool operator==(const ProposalID& id) const;
	bool operator>(const ProposalID& id) const;
	bool operator!=(const ProposalID& id) const;
public:
    int m_number;
    std::string m_uid;
};