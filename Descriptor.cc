#include "ns3/core-module.h"
#include "Descriptor.h"

#include <string>
#include <iostream>
#include <ios>
#include <cstring>
#include <cstdio>
#include <arpa/inet.h>

NS_LOG_COMPONENT_DEFINE ("GnutellaDescriptor");

Descriptor::Descriptor()
{
}

Descriptor::Descriptor(DescriptorHeader h)
    :header_(h)
{
}

void Descriptor::Hop()
{
    header_.hops++;
    header_.ttl--;
}

void Descriptor::AddHop()
{
    header_.hops++;
}

void Descriptor::DecTtl()
{
    header_.ttl--;
}

void 
Descriptor::Serialize(ns3::Buffer::Iterator)
{
}

Descriptor * Descriptor::Create(const uint8_t *p_descriptor)
{
    NS_LOG_INFO ("Buffer: " << std::hex << (uint32_t*)p_descriptor);
    Descriptor::DescriptorHeader header;

	// Store header fields
	header.descriptor_id = DescriptorId(p_descriptor);
	header.payload_descriptor = (uint8_t)p_descriptor[16];
    if(header.payload_descriptor > 0x81)
        return NULL;
	header.ttl = p_descriptor[17];
	header.hops = p_descriptor[18];
	// Translate to host endianess
	header.payload_length = ntohl(*((uint32_t*)(p_descriptor+19)));

    if(header.payload_length > 4096)
        return NULL;

    NS_LOG_INFO ("Descriptor Type: " 
            << std::hex << (uint32_t)header.payload_descriptor << std::dec);
    NS_LOG_INFO ("Payload Length: " 
            << std::hex << (uint32_t)header.payload_length << std::dec);

	uint8_t * payload = new uint8_t[header.payload_length];
    memcpy(payload, p_descriptor+23, header.payload_length);


//    NS_LOG_INFO ("Descriptor Type: " 
//            << std::hex << (uint32_t)header.payload_descriptor);
//    NS_LOG_INFO ("Descriptor TTL: " 
//            << std::hex << (uint32_t)header.ttl);
//    NS_LOG_INFO ("Descriptor Hops: " 
//            << std::hex << (uint32_t)header.hops);
//    NS_LOG_INFO ("Payload Length: " 
//            << std::hex << (uint32_t)header.payload_length);

    switch(header.payload_descriptor)
    {
        case PING:
            return PingDescriptor::Create(header, payload);
        case PONG:
            return PongDescriptor::Create(header, payload);
        case QUERY:
            return QueryDescriptor::Create(header, payload);
        case QUERYHIT:
            return QueryHitDescriptor::Create(header, payload);
        case PUSH:
			return PushDescriptor::Create(header, payload);
        default:
            return (Descriptor *)NULL;

    }
}

Descriptor::~Descriptor()
{
}

uint32_t
Descriptor::GetSerializedSize()
{
    return 23;
}

void
Descriptor::SerializeHeader(ns3::Buffer::Iterator start)
{
    header_.descriptor_id.Serialize(start);
    start.Next(header_.descriptor_id.GetSerializedSize()); //Move over header bytes
    start.WriteU8(header_.payload_descriptor);
    start.WriteU8(header_.ttl);
    start.WriteU8(header_.hops);
    start.WriteHtonU32(header_.payload_length);
}

//Testing
uint8_t 
Descriptor::GetTtl()
{
    return header_.ttl;
};
