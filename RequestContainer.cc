#include "Request.h" 

RequestContainer::RequestContainer(ns3::Time timeout)
    :timeout_period_(timeout)
{
}

void RequestContainer::AddRequest(Request * rq)
{
    rq_address_map_.insert(std::pair<DescriptorId, Request *>(rq->GetId(), rq));

}

Request * RequestContainer::RemoveRequest(Request * rq)
{
    Request * temp = rq_address_map_[rq->GetId()];
    if(temp == rq)
    {
        rq_address_map_.erase(rq->GetId());
        return rq;
    }
    else
        return (Request *)NULL;

}

Request * RequestContainer::GetRequestById(DescriptorId id)
{
    if(!rq_address_map_.count(id))
        return NULL;

    return rq_address_map_[id];
}

void RequestContainer::TimeoutRequests()
{
    std::map<DescriptorId,Request*>::iterator it;
    for (it = rq_address_map_.begin(); it != rq_address_map_.end();)
    {
        if (it->second->GetAge() > timeout_period_) rq_address_map_.erase(it++);
        else ++it;
    }
}
