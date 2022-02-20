/*
 *|----------|------------|------|
 *|len       |uint32      |包长度|
 *|version   |uint16      |协议版本|
 *|id		 |uint32      |协议号|
 *|body      |string      |协议内容|
 */
#include "common_proto_parser.h"
#include <memory>

CommonProtoParser::CommonProtoParser():m_callback(nullptr),m_instance(nullptr){

}

CommonProtoParser::~CommonProtoParser(){
	m_callback = nullptr;
	m_instance = nullptr;
}

size_t CommonProtoParser::PacketMaxSize(){
    return 1024*1024*10;//10M
}

int CommonProtoParser::PackMessage(const Message& msg, std::string* buffer){
    std::string smsg;
    msg.EncodeToString(&smsg);
    uint32_t size = smsg.size() + 10;
    Serializer::PutUint32(size, buffer);
    Serializer::PutUint16(msg.ProtoVersion(), buffer); //协议版本
    Serializer::PutUint32(msg.ProtoID(), buffer);	//协议号
    buffer->append(smsg);
	LOG_DEBUG("pack:\n%s", Util::DumpHex(buffer->data(), buffer->size()).c_str());

    return size;
}


int CommonProtoParser::HandlePacket(const char* data, size_t size, SocketBase* s){
    if(data == nullptr){
		LOG_ERROR("packet is null");
        return -1;
    }
    if(size < 10){
		LOG_DEBUG("packet len:%zd too short",size);
        return 0;
    }
    //包长度
    uint32_t packetSize = 0;
    if(!Serializer::GetUint32(data, size, &packetSize)){
		LOG_ERROR("packet no len");
        return -1;
    }
    if(packetSize > PacketMaxSize()){
		LOG_ERROR("packet exceed limit:%u",packetSize);
        return -1;
    }
    if(packetSize > size){
		LOG_DEBUG("packet len:%zd too short",size);
        return 0;
    }
    //协议版本
    uint16_t protoVersion = 0;
    if(!Serializer::GetUint16(data+4, size-4, &protoVersion)){
		LOG_ERROR("packet no proto version");
        return -1;
    }
    //协议号
    uint32_t protoId = 0;
    if(!Serializer::GetUint32(data+6, size-6, &protoId)){
		LOG_ERROR("packet no proto id");
        return -1;
    }
	
	LOG_DEBUG("unpack:\n%s", Util::DumpHex(data, packetSize).c_str());


	std::shared_ptr<Message> pMsg(MessageFactory::CreateMessage(protoVersion,protoId));
	if(nullptr == pMsg){
		LOG_ERROR("unknow message proroVersion:%u protoId:%u", protoVersion, protoId);
		return -1;
	}
    size_t len=0;
	if(!pMsg->DecodeFromArray(data+10, packetSize-10, &len)){
		LOG_ERROR("message proroVersion:%u protoId:%u len:%zd decode failed", protoVersion, protoId, len);
		return -1;
	}

	if(m_callback(pMsg, s, m_instance)){
		return packetSize;
	}
	else{
		LOG_ERROR("message proroVersion:%u protoId:%u handle failed", protoVersion, protoId);
		return -1;
	}
}


void CommonProtoParser::SetCallback(Message_Callback callback, void* instance)
{
	m_callback = callback;
	m_instance = instance;
}
