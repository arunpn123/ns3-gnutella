#ifndef PEER_H
#define PEER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include <vector>
#include <map>

class Peer
{
    public:
        Peer(ns3::Address addr);
        Peer(ns3::Address addr, ns3::Ptr<ns3::Socket> socket);
//        Peer(Peer p, ns3::Ptr<ns3::Socket> socket);
        ns3::Address GetAddress();
        void GnutellaConnected();
        void GnutellaDisconnected();
        bool IsGnutellaConnected();
        void SetSocket(ns3::Ptr<ns3::Socket> socket);
        ns3::Ptr<ns3::Socket> GetSocket();
    private:
        ns3::Address address_;
        //TCP socket to peer
        ns3::Ptr<ns3::Socket> socket_;
        //Indicates we have sent or received Gnutella Connect to/from peer
        bool gnutella_connected_;
};

class PeerContainer
{
    public:
        virtual void AddPeer(Peer *p);
        Peer* RemovePeer(Peer * p);
        Peer* GetPeerByAddress(ns3::Address address);
        //Gets a peer which we are not connected to
        Peer* GetRemotePeer();
        Peer* GetPeer(unsigned int index);
        // Check if peer is in this container
        bool SearchPeer(Peer *p); 
        size_t GetSize();
    protected:
        void AddPeerToVector(Peer *p);
        void AddPeerToAddressMap(Peer *p);
        Peer *RemovePeerFromMap(Peer *p);
        Peer *RemovePeerFromVector(Peer *p);
    private:
        std::map<ns3::Address, Peer *> peer_address_map_;
        std::vector<Peer *> peer_vector_;
};

class ConnectedPeerContainer : public PeerContainer
{
    public:
        virtual void AddPeer(Peer *p);
        Peer* GetPeerBySocket(ns3::Ptr<ns3::Socket> socket);
    private:
        std::map<ns3::Ptr<ns3::Socket>, Peer* > peerSocketMap_;
        void AddPeerToSocketMap(Peer *p);
};

#endif
