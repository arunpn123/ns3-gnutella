#include "Descriptor.h"

#include "ns3/core-module.h"
#include "ns3/ipv4-address.h"

#include <arpa/inet.h>

#include <string>
#include <iostream>

NS_LOG_COMPONENT_DEFINE ("GnutellaQueryHitDescriptor");

QueryHitDescriptor *
QueryHitDescriptor::Create(DescriptorHeader header, const uint8_t *descriptor_payload)
{
    char num_hits[1];
    uint16_t port;
    uint32_t ip_address, speed;
    Result *results;
    uint8_t servant_identifier[16];

	
	NS_LOG_INFO ("QHDesc Payload: " << (int) *(descriptor_payload));
	
	sprintf(num_hits, "%d", *((const char*) descriptor_payload));
    port = ntohs(*((uint16_t*)(descriptor_payload+1)));
    ip_address = ntohl(*((uint32_t*)(descriptor_payload+3)));
    speed = ntohl(*((uint32_t*)(descriptor_payload+7)));
    
    NS_LOG_INFO ("Query Hit Descriptor Num Hits: " 
	 << *num_hits);
    NS_LOG_INFO ("Query Hit Descriptor Port: "
	<< std::dec << (uint32_t) port);
    NS_LOG_INFO ("Query Hit Descriptor Ip Address: "
	<< std::hex << ip_address);
    NS_LOG_INFO ("Query Hit Descriptor Speed: "
	<< std::dec << speed);

    results = new Result[atoi(num_hits)];
    int index = 11;
    for (int i = 0; i < atoi(num_hits); i++)
    {
        results[i].file_index = ntohl(*((uint32_t*)(descriptor_payload+index)));
        index += 4;
        results[i].file_size = ntohl(*((uint32_t*)(descriptor_payload+index)));
        index += 4;
        int nt_index = index;
        while (descriptor_payload[nt_index] != (uint8_t) '\0')
           nt_index++;
        results[i].shared_file_name = std::string((const char *)(descriptor_payload+index), (size_t) (nt_index - index)); 
        index = nt_index + 2;

	NS_LOG_INFO ("Query Hit Descriptor Result: ");
        NS_LOG_INFO ("\tFile Index: " << std::hex << results[i].file_index);
        NS_LOG_INFO ("\tFile Size: " << std::dec << results[i].file_size);
        NS_LOG_INFO ("\tShared File Name: " << results[i].shared_file_name);
    }

    memcpy(servant_identifier, descriptor_payload+index, 16);
    
    NS_LOG_INFO ("Query Hit Descriptor SID: "
	<< std::hex << servant_identifier);

    return new QueryHitDescriptor(header, (uint8_t) atoi(num_hits), port, ns3::Ipv4Address(ip_address), speed, results, servant_identifier);
}

void
QueryHitDescriptor::Serialize(ns3::Buffer::Iterator start)
{
    SerializeHeader(start);
    start.Next(23);
    start.WriteU8(num_hits_);
    start.WriteHtonU16(port_);
    start.WriteHtonU32(ip_address_.Get());
    start.WriteHtonU32(speed_);
    for (int i = 0; i < num_hits_ ; i++)
    {
        start.WriteHtonU32(result_set_[i].file_index);
        start.WriteHtonU32(result_set_[i].file_size);
        start.Write((uint8_t const *)result_set_[i].shared_file_name.c_str(),
            (uint32_t) result_set_[i].shared_file_name.size());
        start.WriteU8((uint8_t) '\0');
        start.WriteU8((uint8_t) '\0');
    }
    start.Write(servant_identifier_, 16);
}

uint32_t
QueryHitDescriptor::GetSerializedSize()
{
    return 23+1+2+4+4+GetResultSetSerializedSize()+16;
}

uint32_t
QueryHitDescriptor::GetResultSetSerializedSize()
{
    uint32_t size = 0;
    for (int i = 0; i < num_hits_ ; i++)
    {
       size += 8 + result_set_[i].shared_file_name.size() + 2;
    }

    return size;
}

QueryHitDescriptor::QueryHitDescriptor(DescriptorHeader h, uint8_t num_hits,
	uint16_t port, ns3::Ipv4Address ip_address, uint32_t speed,
	Result *result_set, uint8_t *servant_identifier)
	: Descriptor(h), num_hits_(num_hits), port_(port),
	ip_address_(ip_address), speed_(speed), result_set_(result_set)
{
	type_ = QUERYHIT;
	memcpy(servant_identifier_, servant_identifier, 16);
}

QueryHitDescriptor::QueryHitDescriptor(DescriptorId id, 
        uint8_t num_hits, ns3::Address addr, uint32_t speed, 
        Result *result_set, uint8_t *servant_identifier)
	: num_hits_(num_hits), speed_(speed), result_set_(result_set)
{
	header_.descriptor_id = id;
    header_.payload_descriptor = Descriptor::QUERYHIT;
    header_.ttl = Descriptor::DEFAULT_TTL;
    header_.hops = 0;
    header_.payload_length = 27 + 0; 

    ns3::InetSocketAddress a = ns3::InetSocketAddress::ConvertFrom(addr);
    port_ = a.GetPort();
    ip_address_ = a.GetIpv4();
}

QueryHitDescriptor::~QueryHitDescriptor()
{
	delete [] result_set_;
}


uint8_t QueryHitDescriptor::GetNumHits()
{
	return num_hits_;
}
uint16_t QueryHitDescriptor::GetPort()
{
	return port_;
}
ns3::Ipv4Address QueryHitDescriptor::GetIp()
{
	return ip_address_;
}
uint32_t QueryHitDescriptor::GetSpeed()
{
	return speed_;
}
Result* QueryHitDescriptor::GetResultSet()
{
	return result_set_;
}
uint8_t* QueryHitDescriptor::GetServentID()
{
	return servant_identifier_;
}
