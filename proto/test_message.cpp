#include "test_message.h"

void TestMessage::EncodeToString(std::string* str) const{
    Serializer::PutUint64(m_uid, str);
    Serializer::PutString(m_text, str);
    Serializer::PutFloat(m_fvalue, str);
    Serializer::PutDouble(m_dvalue, str);
}

bool TestMessage::DecodeFromArray(const char* buf, size_t size){
    size_t offset = 0;

    if(!Serializer::GetUint64(buf + offset, size-offset, &m_uid)){
        return false;
    }
	offset += 8;

    if(!Serializer::GetString(buf + offset, size-offset, &m_text)){
        return false;
    }
	offset += m_text.size() + 4;


    if(!Serializer::GetFloat(buf + offset, size-offset, &m_fvalue)){
        return false;
    }
	offset += 4;

    if(!Serializer::GetDouble(buf + offset, size-offset, &m_dvalue)){
        return false;
    }
	offset += 8;

    return true;
}

