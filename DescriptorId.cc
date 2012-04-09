#include "Descriptor.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"

DescriptorId
DescriptorId::Create(ns3::Ptr<ns3::Node> n)
{
    uint8_t desc_id[16];
    ns3::Ptr<ns3::NetDevice> d = n->GetDevice(1);
    ns3::Address a = d->GetAddress();
    ns3::Mac48Address mac = ns3::Mac48Address::ConvertFrom(a);
//    ns3::Mac48Address mac = ns3::Mac48Address::ConvertFrom(n->GetDevice(0)->GetAddress());
    ns3::Time newtime = ns3::Simulator::Now();
    if(newtime == DescriptorId::time_)
    {
        DescriptorId::time_count_++;
    }
    else
    {
        time_count_ = 0;
    }
    DescriptorId::time_ = newtime;

    //Writes mac address as 6 byte string
    mac.CopyTo(desc_id);
    //Store time in the next 8 bytes
    *((int64_t*)(desc_id+6)) = newtime.GetInteger();
    //Store countin the next 2 bytes
    *((uint16_t*)(desc_id+14)) = time_count_;

    return DescriptorId(desc_id);

}

DescriptorId::DescriptorId(const uint8_t * desc_id)
{
    memcpy(desc_id_, desc_id, 16);
}

DescriptorId::DescriptorId(const DescriptorId &d)
{
    memcpy(desc_id_, d.desc_id_, 16);
}

DescriptorId & DescriptorId::operator=(const DescriptorId &d)
{
    memcpy(desc_id_, d.desc_id_, 16);
    return *this;
}

void DescriptorId::Serialize(ns3::Buffer::Iterator start)
{
    start.Write(desc_id_, 16);
}

uint8_t DescriptorId::GetSerializedSize()
{
    return 16;
}

bool operator<(const DescriptorId &lhs, const DescriptorId &rhs)
{
    int result = memcmp(lhs.desc_id_, rhs.desc_id_, 16);
    if(result < 0)
        return true;
    else
        return false;
}

ns3::Time DescriptorId::time_;
uint16_t DescriptorId::time_count_ = 0;
