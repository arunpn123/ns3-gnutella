#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "Peer.h"

NS_LOG_COMPONENT_DEFINE ("GnutellaPeerContainer");

void PeerContainer::AddPeer(Peer *p)
{
    if(GetPeerByAddress(p->GetAddress()) == NULL)
    {
        AddPeerToVector(p);
        AddPeerToAddressMap(p);
    }
}

//Protected helper function
void PeerContainer::AddPeerToVector(Peer *p)
{
    peer_vector_.push_back(p);
}

//Protected helper function
void PeerContainer::AddPeerToAddressMap(Peer *p)
{
		peer_address_map_.insert( 
                std::pair<ns3::Address, Peer* >(p->GetAddress(), p));
}

//Protected helper function
Peer *PeerContainer::RemovePeerFromVector(Peer *p)
{
    std::vector<Peer *>::iterator it;
    for(it = peer_vector_.begin(); it<peer_vector_.end(); it++)
    {
        if(*it == p)
        {
            peer_vector_.erase(it);
        }
    }

    if(it == peer_vector_.end())
        return (Peer *)NULL;
    else
        return p;
}

//Protected helper function
Peer *PeerContainer::RemovePeerFromMap(Peer *p)
{
    Peer * temp = peer_address_map_[p->GetAddress()];
    if(temp == p)
    {
        peer_address_map_.erase(p->GetAddress());
        return p;
    }
    return (Peer *)NULL;
}

Peer * PeerContainer::RemovePeer(Peer *p)
{
    RemovePeerFromMap(p);
    return RemovePeerFromVector(p);
}

Peer * PeerContainer::GetPeerByAddress(ns3::Address address)
{
    if(!peer_address_map_.count(address))
        return NULL;

    return peer_address_map_[address];
}

//Gets a peer which we are not connected to
Peer * PeerContainer::GetRemotePeer()
{
    return (Peer *)0;
}

Peer * PeerContainer::GetPeer(unsigned int index)
{
    //Check that this element exists
    if(!(peer_vector_.size() - index))
        return NULL;

    return peer_vector_[index];
}

size_t PeerContainer::GetSize()
{
   return peer_vector_.size(); 
}

bool PeerContainer::SearchPeer(Peer *p)
{
	for (size_t i = 0; i < peer_vector_.size(); i++)
	{
		if (peer_vector_[i]->GetAddress() == p->GetAddress())
		{
            return true;
		}
	}
	return false;
}
