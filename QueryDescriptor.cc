#include "ns3/core-module.h"

#include "Descriptor.h"

#include <string>
#include <arpa/inet.h>

NS_LOG_COMPONENT_DEFINE ("GnutellaQueryDescriptor");

// QueryDescriptor
QueryDescriptor * 
QueryDescriptor::Create(DescriptorHeader header, const uint8_t *p_descriptor)
{
//	type = QUERY;
//	min_speed = getNByteUInt(descriptor_payload, 2);
//	search_criteria = std::string(descriptor_payload + 2,
//		header_.payload_length);	i
    uint16_t min_speed;
    std::string search_criteria;
	
    min_speed = ntohs(*((uint16_t*)(p_descriptor+0)));	
    search_criteria = std::string((const char*)p_descriptor+2, (int)header.payload_length-3);

    NS_LOG_INFO ("Query Descriptor Min Speed: "
	<< std::hex << (uint32_t) min_speed);
    NS_LOG_INFO ("Query Descriptor Search Criteria: "
	<< search_criteria);

    return new QueryDescriptor(header, min_speed, search_criteria);
}

uint32_t
QueryDescriptor::GetSerializedSize()
{
    // header + min_speed + string + null terminator
    return 23+2+search_criteria_.size()+1;
}

void
QueryDescriptor::Serialize(ns3::Buffer::Iterator start)
{
	SerializeHeader(start);
	start.Next(23);
	start.WriteHtonU16(min_speed_);
	start.Write((uint8_t*)search_criteria_.c_str(), 
		search_criteria_.size()+1);
}

std::string QueryDescriptor::GetSearchCriteria()
{
	return search_criteria_;
}

uint16_t QueryDescriptor::GetMinSpeed()
{
	return min_speed_;
}

QueryDescriptor::QueryDescriptor(DescriptorHeader h, 
        uint16_t min_speed, 
        std::string search_criteria)
	: Descriptor(h), min_speed_(min_speed), 
	search_criteria_(search_criteria)
{
	type_ = QUERY;
}

QueryDescriptor::QueryDescriptor(ns3::Ptr<ns3::Node> n, uint16_t min_speed, std::string filename, uint16_t query_type)
{
    type_ = QUERY;
    header_.descriptor_id = DescriptorId::Create(n);
    header_.payload_descriptor = Descriptor::QUERY;
    header_.ttl = Descriptor::DEFAULT_TTL;
    header_.hops = 0;
    // 2 bytes for speed, 1 byte for null terminator
    header_.payload_length = 2 + filename.length() + 1;
    min_speed_ = min_speed;
    search_criteria_ = filename;
    query_type_ = query_type;
}

QueryDescriptor::~QueryDescriptor()
{
	// Intentionally blank
}
