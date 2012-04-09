#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "Peer.h"

Peer::Peer(ns3::Address addr)
    :address_(addr), gnutella_connected_(false)
{ 
}

Peer::Peer(ns3::Address addr, ns3::Ptr<ns3::Socket> socket)
    :address_(addr), socket_(socket), gnutella_connected_(false) 
{

}

ns3::Address Peer::GetAddress()
{
    return address_;
}

void Peer::GnutellaConnected()
{
    gnutella_connected_ = true;
}

void Peer::GnutellaDisconnected()
{
    gnutella_connected_ = false;
}

bool Peer::IsGnutellaConnected()
{
	if (gnutella_connected_ == true)
		return true;
	else
		return false;
}

void Peer::SetSocket(ns3::Ptr<ns3::Socket> socket)
{
    socket_ = socket;
}

ns3::Ptr<ns3::Socket> Peer::GetSocket()
{
	return socket_;
}
