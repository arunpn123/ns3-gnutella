#include "QueryLatencyContainer.h"
#include "Descriptor.h"

#include "ns3/core-module.h"
#include "ns3/network-module.h"

#include <map>
NS_LOG_COMPONENT_DEFINE ("QueryLatencyContainer");

void QueryLatencyContainer::AddQuery(DescriptorId d)
{
    ns3::Time now = ns3::Simulator::Now();
    query_map_[d] = now;
}

ns3::Time QueryLatencyContainer::FindTimeGivenQuery(DescriptorId d)
{
    std::map<DescriptorId, ns3::Time>::iterator i = query_map_.find(d);
    if (i == query_map_.end())
        return ns3::Time(0);
    return i->second;
}

ns3::Time QueryLatencyContainer::RemoveQuery(DescriptorId d)
{
    ns3::Time t = FindTimeGivenQuery(d);
    query_map_.erase(d);
    return t;
}

void QueryLatencyContainer::Update(DescriptorId d)
{
    ns3::Time now = ns3::Simulator::Now();
    ns3::Time sent = RemoveQuery(d);
    if (sent.IsZero())
        return;
    ns_sum_ += (uint64_t) ((now - sent).GetNanoSeconds());
    num_++;
}

uint32_t QueryLatencyContainer::GetAverageResponseTime()
{
    if (num_ == 0)
        return 0;
    return (uint32_t) (ns_sum_ / num_);
}
