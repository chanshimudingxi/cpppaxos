#include "proposalid.h"

ProposalID::ProposalID()
{

}

ProposalID::ProposalID(int number, std::string uid)
{
    m_number = number;
    m_uid = uid;
}

ProposalID& ProposalID::operator=(const ProposalID& id)
{
    m_number = id.m_number;
    m_uid = id.m_uid;
    return *this;
}

ProposalID::~ProposalID()
{

}

bool ProposalID::isValid()
{
    if(m_uid.empty())
    {
        return false;
    }
    return true;
}

int ProposalID::getNumber()
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

std::string ProposalID::getUID()
{
    return m_uid;
}

int ProposalID::compare(const ProposalID& id)
{
    if( (m_uid == id.m_uid && m_number > id.m_number)||(m_uid > id.m_uid))
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

bool ProposalID::operator>(const ProposalID& id)
{
    if( (m_uid == id.m_uid && m_number > id.m_number)||(m_uid > id.m_uid))
    {
        return true;
    }
    return false;
}

bool ProposalID::operator<(const ProposalID& id)
{
    if( (m_uid == id.m_uid && m_number < id.m_number)||(m_uid < id.m_uid))
    {
        return true;
    }
    return false;
}

bool ProposalID::operator==(const ProposalID& id)
{
    if(m_number == id.m_number && m_uid == id.m_uid)
    {
        return true;
    }
    return false;
}

bool ProposalID::operator!=(const ProposalID& id)
{
    if(m_number == id.m_number && m_uid == id.m_uid)
    {
        return false;
    }
    return true;
}