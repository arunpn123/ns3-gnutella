#include "ns3/core-module.h"
#include "ns3/ipv4-address.h"
#include "Descriptor.h"

#include <cstring>
#include <string>
#include <iostream>
#include <arpa/inet.h>
#include <ios>

NS_LOG_COMPONENT_DEFINE ("GnutellaPongDescriptor");

PongDescriptor * PongDescriptor::Create(DescriptorHeader header, const uint8_t *p_descriptor)
{
    uint32_t ip_addr, num_shared, kb_shared;
    uint16_t port;

	port = ntohs(*((uint16_t*)(p_descriptor+0)));
	ip_addr = ntohl(*((uint32_t*)(p_descriptor+2)));
	num_shared = ntohl(*((uint32_t*)(p_descriptor+6)));
	kb_shared = ntohl(*((uint32_t*)(p_descriptor+10)));

    NS_LOG_INFO ("Pong Descriptor Port: " 
            << std::hex << (uint32_t)port);
    NS_LOG_INFO ("Pong Descriptor IP: " 
            << std::hex << (uint32_t)ip_addr);
    NS_LOG_INFO ("Pong Descriptor Shared: " 
            << std::hex << (uint32_t)num_shared);
    NS_LOG_INFO ("Pong Descriptor KB Shared: " 
            << std::hex << (uint32_t)kb_shared);

    return new PongDescriptor(header, port, ns3::Ipv4Address(ip_addr), num_shared, kb_shared);
}

uint32_t
PongDescriptor::GetSerializedSize()
{
    return 23+14;
}

void
PongDescriptor::Serialize(ns3::Buffer::Iterator start)
{
    SerializeHeader(start);
    start.Next(23); //Move over header bytes
    start.WriteHtonU16(port_); 
    start.WriteHtonU32(ip_address_.Get()); 
    start.WriteHtonU32(files_shared_); 
    start.WriteHtonU32(kb_shared_); 
}


PongDescriptor::PongDescriptor(DescriptorHeader h, 
        uint16_t port, 
        ns3::Ipv4Address ip_address, 
        uint32_t files_shared, 
        uint32_t kb_shared)
	: Descriptor(h), port_(port), ip_address_(ip_address),
      files_shared_(files_shared), kb_shared_(kb_shared)
{
    type_ = PONG;
}

PongDescriptor::PongDescriptor(DescriptorId id, ns3::Address addr, 
        uint32_t files_shared, uint32_t kb_shared)
{
	header_.descriptor_id = id;
    header_.payload_descriptor = Descriptor::PONG;
    header_.ttl = Descriptor::DEFAULT_TTL;
    header_.hops = 0;
    // constant defined in GnutellaApp.h
    header_.payload_length = PONG_PAYLOAD_LENGTH; 

    ns3::InetSocketAddress a = ns3::InetSocketAddress::ConvertFrom(addr);

    port_ = a.GetPort();
    ip_address_ = a.GetIpv4();
    files_shared_ = files_shared;
    kb_shared_ = kb_shared;
}

PongDescriptor::~PongDescriptor()
{
}
