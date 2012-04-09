#include "Peer.h"

void ConnectedPeerContainer::AddPeer(Peer *p)
{
    AddPeerToVector(p);
    AddPeerToAddressMap(p);
	AddPeerToSocketMap(p);
}

void ConnectedPeerContainer::AddPeerToSocketMap(Peer *p)
{
	peerSocketMap_.insert(std::pair<ns3::Ptr<ns3::Socket>, Peer*>(p->GetSocket(), p));
}

Peer* ConnectedPeerContainer::GetPeerBySocket(ns3::Ptr<ns3::Socket> socket)
{
    if(!peerSocketMap_.count(socket))
        return NULL;

    return peerSocketMap_[socket];

}
