#include "ns3/core-module.h"
#include "Descriptor.h"
#include "ns3/buffer.h"

NS_LOG_COMPONENT_DEFINE ("GnutellaPingDescriptor");

PingDescriptor *
PingDescriptor::Create(DescriptorHeader h, const uint8_t * payload)
{
    return new PingDescriptor(h);
}

PingDescriptor::PingDescriptor(DescriptorHeader h)
	: Descriptor(h)
{
	type_ = PING;
}

PingDescriptor::PingDescriptor(ns3::Ptr<ns3::Node> n)
{
	type_ = PING;

    header_.descriptor_id = DescriptorId::Create(n);
    header_.payload_descriptor = PING;
    header_.ttl = DEFAULT_TTL;
    header_.hops = 0;
    header_.payload_length = PAYLOAD_LENGTH;
}

uint32_t
PingDescriptor::GetSerializedSize()
{
    return 23;
}

void
PingDescriptor::Serialize(ns3::Buffer::Iterator start)
{
    SerializeHeader(start);
}

PingDescriptor::~PingDescriptor()
{
	// Intentionally blank
}
