#include "proposalid.h"

ProposalID::ProposalID():m_number(0),m_uid(""){}

ProposalID::ProposalID(int number, std::string uid):m_number(number),m_uid(uid)
{}

ProposalID::ProposalID(const ProposalID& id):m_number(id.m_number),m_uid(id.m_uid)
{}

ProposalID& ProposalID::operator=(const ProposalID& id)
{
    m_number = id.m_number;
    m_uid = id.m_uid;
    return *this;
}

ProposalID::~ProposalID()
{
}

bool ProposalID::isValid() const
{
    if(m_uid.empty())
    {
        return false;
    }
    return true;
}

int ProposalID::getNumber() const
{
    return m_number;
}

void ProposalID::setNumber(int number)
{
    m_number = number;
}

void ProposalID::incrementNumber()
{
    m_number++;
}

std::string ProposalID::toString() const
{
    std::string dumpstr;
    dumpstr.append("proposalid:").append(m_uid).append("_").append(std::to_string(m_number));
    return dumpstr;
}

int ProposalID::compare(const ProposalID& id) const
{
    if(m_uid > id.m_uid || (m_uid == id.m_uid && m_number > id.m_number) )
    {
        return 1;
    }
    else if(m_number == id.m_number && m_uid == id.m_uid)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

bool ProposalID::operator<(const ProposalID& id) const
{
	return this->compare(id) < 0;
}

bool ProposalID::operator==(const ProposalID& id) const
{
	return this->compare(id) == 0;
}

bool ProposalID::operator>(const ProposalID& id) const
{
	return this->compare(id) > 0;
}

bool ProposalID::operator!=(const ProposalID& id) const
{
	return this->compare(id) != 0;
}


void ProposalID::marshal(Pack & pk) const{
    pk << m_number << m_uid;
}

void ProposalID::unmarshal(const Unpack &up){
    up >> m_number >> m_uid;
}
