#include "Request.h"
#include "ns3/simulator.h"

Request::Request(Descriptor *d, Peer *p, bool self)
    :peer_(p), desc_(d), self_(self)
{
    time_received_ = ns3::Simulator::Now();
}

Peer * Request::GetPeer()
{
    return peer_;
}
    
ns3::Time Request::GetAge()
{
   return ns3::Simulator::Now() - time_received_;
}

DescriptorId Request::GetId()
{
    return desc_->GetHeader().descriptor_id;
}

Descriptor::DescriptorType Request::GetType()
{
	return desc_->GetType();
}

bool Request::IsSelf()
{
	return self_;
}
