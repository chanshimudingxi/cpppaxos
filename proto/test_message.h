#ifndef TEST_MESSAGE_H_
#define TEST_MESSAGE_H_

#include "message.h"
#include "net/serializer.h"
#include <cstddef>

class TestMessage : public Message{
public:
    TestMessage(uint16_t protoVersion, uint32_t protoId):
		Message(protoVersion,protoId){}
    ~TestMessage(){}
    virtual void EncodeToString(std::string* str) const;
    virtual bool DecodeFromArray(const char* buf, size_t size);
public:
    uint64_t m_uid;
    std::string m_text;
    float m_fvalue;
    double m_dvalue;
};

#endif