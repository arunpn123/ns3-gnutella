#ifndef REQUEST_H
#define REQUEST_H
#include "Descriptor.h" 
#include "Peer.h" 
#include "ns3/nstime.h"

class Request
{
    public:
        Request(Descriptor *d, Peer *p, bool self = false);
        Peer * GetPeer();
        ns3::Time GetAge();
        DescriptorId GetId();
        Descriptor::DescriptorType GetType();
        bool IsSelf();
    private:
        Peer * peer_;
        ns3::Time time_received_;
        Descriptor * desc_;
        bool self_;
};

class RequestContainer
{
    public:
        RequestContainer(ns3::Time timeout);
        void AddRequest(Request * rq);
        Request * RemoveRequest(Request * rq);
        Request * GetRequestById(DescriptorId id);
        size_t GetSize(){ return rq_address_map_.size();};
    protected:
        void TimeoutRequests();
    private:
        std::map<DescriptorId, Request *> rq_address_map_;
        ns3::Time timeout_period_;

};

#endif
