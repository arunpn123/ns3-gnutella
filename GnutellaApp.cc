#include "GnutellaApp.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

#include <iostream>
#include <utility>
#include <sstream>
#include <arpa/inet.h>
#include "ns3/buffer.h"
#include "Descriptor.h"
#include "Request.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("GnutellaApp");

GnutellaApp::GnutellaApp(Address addr, Address boots)
    :m_requests(ns3::Seconds(SECONDS_REQUEST_TIMEOUT))
{
	InetSocketAddress a = InetSocketAddress::ConvertFrom(addr);
	
//	InetSocketAddress b = InetSocketAddress::ConvertFrom(boots);
//	NS_LOG_INFO ("With bootstrap: " << b.GetIpv4() << ":" << b.GetPort());
	
	m_local = addr;
	m_bootstrap = boots;
	m_gnutella_joined = false;
	
	cache.m_capacity = 10;
	// generate serventID
	GetServentID(a.GetIpv4(), m_servent_id);
	
	
	// Generate some files
	this->GenerateFiles();
	
	NS_LOG_INFO ("Peer created: " << a.GetIpv4() << ":" << a.GetPort() << " sid: " << m_servent_id);
}

GnutellaApp::~GnutellaApp()
{
	for (size_t i = 0; i < m_peers.GetSize(); i++)
	{
		delete m_peers.GetPeer(i);
	}
}

void GnutellaApp::StartApplication()
{
    
    TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");

    m_pingtimer.SetFunction(&GnutellaApp::PingPeers, this);

//    std::stringstream ss;
//    ss << "Peer id:" << GetNode()->GetId();
//    LogMessage(ss.str().c_str());

    m_pingtimer.Schedule(Seconds(10));

    m_querytimer.SetFunction(&GnutellaApp::SendQuery, this);
	m_querytimer.SetArguments(std::string("foo"));
    m_querytimer.Schedule(Seconds(11)+Seconds(10*abs(NormalVariable().GetValue())));
//    m_querytimer.Schedule(Seconds(15));

    //Open socket for listening
    m_socket = Socket::CreateSocket (GetNode (), tid);    
    m_socket->Bind (InetSocketAddress(Ipv4Address::GetAny(), DEFAULT_LISTENING_PORT));
	m_socket->Listen ();

    m_socket->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, 
            const Address & >(),

    MakeCallback (&GnutellaApp::AcceptPeerConnection, this));

    if(m_bootstrap.IsInvalid())
    {
        LogMessage("No Bootstrap Given");
        return;
    }

    ConnectToPeer(m_bootstrap);
    
    std::stringstream ss2;
    ss2 << "Bootstraping node: " << InetSocketAddress::ConvertFrom(m_bootstrap).GetIpv4();
    LogMessage(ss2.str().c_str());

}

void GnutellaApp::StopApplication()
{
    //Remove ping timer event from the simulation
    m_pingtimer.Cancel();

	Peer *p;
	// Close all connected sockets
	for (size_t i = 0; i < m_connected_peers.GetSize(); i++)
	{
		p = m_connected_peers.GetPeer(i);
		if (p != NULL)
			p->GetSocket()->Close();
	}

     //LogAverageQueryResponseTime();
}

void GnutellaApp::SetInitPeer(Address peer_addr)
{
	// Add inital peer to PeerContainer
	if (!peer_addr.IsInvalid())
	{
		Peer *p = new Peer(peer_addr);
		m_peers.AddPeer(p);
	}
}

//socket contains a new socket reserved for this connection
void GnutellaApp::AcceptPeerConnection(Ptr<Socket> socket, const Address& from)
{
    LogMessage("Accepted Connection");

    socket->SetRecvCallback (MakeCallback (&GnutellaApp::HandleRead, this));
    // Close callbacks
    socket->SetCloseCallbacks (
		MakeCallback (&GnutellaApp::HandlePeerClose, this),
		MakeCallback (&GnutellaApp::HandlePeerError, this));
}

void GnutellaApp::AcceptGnutellaConnection(Ptr<Socket> socket, Address from)
{
    LogMessage("Received Gnutella Connect Request");

	// Accept a connection - send back a OK msg
	Ptr<Packet> packet = Create<Packet> ((const uint8_t*)MSG_CONNECT_OK, 13);
	socket->Send(packet);
	
	// Add peer to the ConnectedPeerContainer
    AddConnectedPeer(from, socket);
}	


void GnutellaApp::HandleRead (Ptr<Socket> socket)
{
//    NS_LOG_INFO ("GnutellaApp: " << this << "  Packet Received");
  	Ptr<Packet> packet;
  	Address from;

  	while (packet = socket->RecvFrom (from))
    {
      if (packet->GetSize () > 0)
        {	
         	uint8_t * buffer  = new uint8_t[packet->GetSize() + 1];
         	packet->CopyData(buffer, packet->GetSize());
            buffer[packet->GetSize()] = 0;

            Peer *p = m_connected_peers.GetPeerBySocket(socket);
            if(p != NULL)
            {
                if (strncmp((const char *) buffer, MSG_HTTP_OK, MSG_HTTP_OK_SIZE) == 0)
				{
					ns3::Buffer tb;
					tb.AddAtStart(packet->GetSize() + 1);
					ns3::Buffer::Iterator ti = tb.Begin();
					ti.Write(buffer, packet->GetSize());
					ti = tb.Begin();
					HandleFileDownloadResponse(p, ti);
				}
                HandlePeerMessage(p, buffer);
            }
            else
            {
                // Received OK to join network
                if (strcmp((const char *)buffer, MSG_CONNECT_OK) == 0) 
                {
                    LogMessage("Received Gnutella OK");
                    AddConnectedPeer(from, socket);
                }
                // Received a request to join network
                else if(strcmp((const char *)buffer, MSG_CONNECT_REQUEST) == 0) 
                {
                    AcceptGnutellaConnection(socket, from);
                }
            }
            
			if (strncmp((const char *) buffer, MSG_HTTP_GET, MSG_HTTP_GET_SIZE) == 0)
			{
				if (p == NULL)
					p = AddConnectedPeer(from, socket);
				ns3::Buffer tb;
				tb.AddAtStart(packet->GetSize() + 1);
				ns3::Buffer::Iterator ti = tb.Begin();
				ti.Write((uint8_t const *)buffer, (uint32_t) packet->GetSize() + 1);
				ti = tb.Begin();
				HandleFileDownloadRequest(p, ti);
			}
            
            

            delete [] buffer;
        }
    }
}

void GnutellaApp::HandlePeerMessage (Peer * p, const uint8_t * buffer)
{

    Descriptor *desc = Descriptor::Create(buffer);

    if(desc == NULL)  
        return; 

    switch(desc->GetType())
    {
        case Descriptor::PING:
            HandlePing((PingDescriptor*)desc, p);
            break;
        case Descriptor::PONG:
            HandlePong((PongDescriptor*)desc);
            break;

        case Descriptor::QUERY:
            HandleQuery((QueryDescriptor*)desc, p);
            break;
        case Descriptor::QUERYHIT:
            HandleQueryHit((QueryHitDescriptor*)desc);
            break;
        case Descriptor::PUSH:
			HandlePush((PushDescriptor*)(desc));
        case Descriptor::FASTQUERYHIT:
        	HandleFastQueryHit((CacheEntry*)entry, hit);
    }
}

void GnutellaApp::HandleFileDownloadRequest(Peer *p, ns3::Buffer::Iterator it)
{
	LogMessage("Download Request Received");
    //std::stringstream ss;
    uint8_t* buffer = new uint8_t[it.GetSize()];
    //buffer.CopyData(&ss, buffer.GetSize());
    // 1) Extract the file index and filename 
    // Ignore the first 9 characters (GET /get/)
    // Contents should now be <fileindex>/<filename>/
    //std::string contents = ss.str().substr(9);
    it.Read(buffer, it.GetSize());
    std::string contents((const char*)buffer);
    contents = contents.substr(9);
    size_t pos1 = contents.find('/');
    size_t pos2 = contents.find('/', pos1 + 1);
    std::string fileindexstr = contents.substr(0, pos1);
    uint32_t file_index;
    std::stringstream(fileindexstr) >> file_index;
    std::string filename = contents.substr(pos1 + 1, pos2 - pos1 - 1);
    // 2) Make sure the file index and filename exist
    File *f = m_files.GetFileByIndex(file_index);
//    NS_LOG_INFO ("Index: " << file_index << " File: " << f->GetFileName() << " Request: " << filename);
    if (f == NULL || filename.compare(f->GetFileName()) != 0)
        return;
    // 3) Send an HTTP response containing the file data
    ns3::Buffer::Iterator fit = f->GetData().Begin();
    SendFileDownloadResponse(p, fit);
    
    delete [] buffer;
}

void GnutellaApp::HandleFileDownloadResponse(Peer *p, ns3::Buffer::Iterator it)
{
    LogMessage("Received File");
    if (it.GetSize() <= 92)
		return;
    std::string filename;
    ns3::Buffer data;
    // 1) Extract the file size and data
    // Skip the initial HTTP
    // HTTP/1.0 200 OK\r\n
    it.Next(17);
    // Server: Gnutella/0.4\r\n
    it.Next(22);
    // Content-Type: application/binary\r\n
    it.Next(34);
    // Content-Length:_
    it.Next(16);
    // Figure out the data size
    std::stringstream ss;
    char c = (char) it.ReadU8();
    while (c != '\r')
     {
         ss << c;
         c = (char) it.ReadU8();
     }
    uint32_t size;
    ss >> size;
    // Skip \n\r\n
    it.Next(3);
    // Put the data in a buffer
    uint8_t *data_buffer = new uint8_t[size];
    it.Read(data_buffer, size);
    // Write to the file's data buffer
    ns3::Buffer::Iterator dit = data.Begin();
    dit.Write(data_buffer, size);

    // Figure out the filename from FileDownloadContainer
    filename = m_downloads.GetFileName(p); 
    
    // 2) Create a new File
    File *f = new File(filename, data);
    m_files.AddFile(f);

    // Remove the download record
    m_downloads.RemoveDownload(p);
   
    delete [] data_buffer;
}

void GnutellaApp::SendFileDownloadRequest(Peer *p, uint32_t file_index,
    std::string file_name)
{
	std::stringstream log;
	log << "Sending File Download Request to " << 
		(InetSocketAddress::ConvertFrom(p->GetAddress())).GetIpv4();
	LogMessage(log.str().c_str());
	
    std::stringstream ss;
    ss << "GET /get/" << file_index << "/" << file_name << "/ HTTP/1.0\r\n";
    ss << "User-Agent: Gnutella/0.4\r\n";
    ss << "Range: bytes=0-\r\n";
    ss << "Connection: Keep-Alive\r\n" << "\r\n"; 

    uint8_t *buffer = (uint8_t *) ss.str().c_str();
    
    if (!m_connected_peers.SearchPeer(p))
       return;

    Ptr<Packet> packet = Create<Packet>(buffer, ss.str().length());
    
    Ptr<Socket> socket = p->GetSocket();
    socket->Send(packet);
}

void GnutellaApp::SendFileDownloadResponse(Peer *p, ns3::Buffer::Iterator it)
{
    LogMessage("Sending File");
    std::stringstream ss; 
    ss << "HTTP/1.0 200 OK\r\n";
    ss << "Server: Gnutella/0.4\r\n";
    ss << "Content-Type: application/binary\r\n";
    ss << "Content-Length: " << it.GetSize() << "\r\n";
    ss << "\r\n";
    //data.CopyData(&ss, data.GetSize());
    uint8_t *temp = new uint8_t[it.GetSize()];
    it.Read(temp, it.GetSize());
    std::string strbuf1 = (const char *) temp;
	std::string strbuf = ss.str() + strbuf1;
    uint8_t *buffer = (uint8_t *) strbuf.c_str();
    
    if (!m_connected_peers.SearchPeer(p))
        return;

    Ptr<Packet> packet = Create<Packet>(buffer, strbuf.size());
   
    Ptr<Socket> socket = p->GetSocket();
    socket->Send(packet);
}

void GnutellaApp::HandlePing (PingDescriptor *desc, Peer *p)
{
    LogMessage("Received PING");

	// Respond with pong
    PongDescriptor * pong_desc = new PongDescriptor(
            desc->GetHeader().descriptor_id, m_local, 
            m_files.GetNumberOfFiles(),
            m_files.GetTotalBytes()/1000);

    LogMessage("Sending PONG");
    Send(pong_desc, p);

	// Decrement ping ttl, increment hops
	desc->Hop();
	
	Request *r = m_requests.GetRequestById(desc->GetHeader().descriptor_id);
	
	// Make sure TTL != 0 first.
	if (desc->GetTtl() > 0 && !(r != NULL && r->GetType() == desc->GetType()) )
	{
		for (size_t i = 0; i < m_connected_peers.GetSize(); i++)
		{
			if (m_connected_peers.GetPeer(i) != p)
            {
				Send(desc, m_connected_peers.GetPeer(i));
                LogMessage("Forwarding PING");
            }
		}

        m_requests.AddRequest(new Request(desc, p));
//        NS_LOG_INFO ("GnutellaApp: " << ns3::InetSocketAddress::ConvertFrom(m_local).GetIpv4() << " Requests Stored: " << m_requests.GetSize());
		
	}

}

void GnutellaApp::HandlePong (PongDescriptor *desc)
{
    std::stringstream sa;
    sa << "Received PONG for: " << desc->ip_address_;
    LogMessage(sa.str().c_str());

	InetSocketAddress addr(desc->ip_address_, desc->port_);
	Peer *p = new Peer(addr);
	
	if(InetSocketAddress::ConvertFrom(m_local).GetIpv4() != desc->ip_address_ && !m_peers.SearchPeer(p))
		m_peers.AddPeer(p);
	else
		delete p;

//    std::stringstream ss;
//    ss << "Peers Stored:" << m_peers.GetSize();
//    LogMessage(ss.str().c_str());
	
	// Forward pong to the node that sent you the ping request with the
	// same ID, if one is found
    Request * r = m_requests.GetRequestById(desc->GetHeader().descriptor_id);
    if(r != NULL)
    {
//        LogMessage("Forwarding PONG");
        desc->Hop();
        if (desc->GetTtl() > 0)
        {
            Send(desc, r->GetPeer());
        }
        m_requests.RemoveRequest(r);
    }
	
	// frees memory.
	delete desc;
}

void GnutellaApp::HandleQuery(QueryDescriptor* desc, Peer* p)
{
	LogMessage("Received QUERY");
	// Received query
    LogMessage(("QUERY Search Criteria: " + desc->GetSearchCriteria()).c_str());
	std::vector<File*> result = m_files.GetFileByName(desc->GetSearchCriteria());
	if (result.size() > 0) // Files found
	{
		// Create a new QueryHit descriptor
		Descriptor::DescriptorHeader header;
		// Store header fields
		// QueryHit's id is the same as the query's id
        header.descriptor_id = desc->GetHeader().descriptor_id;
		header.payload_descriptor = Descriptor::QUERYHIT;
		header.ttl = DEFAULT_TTL;
		header.hops = 0;
				
		// convert std::vector result set to an array
		Result* result_set = new Result[result.size()];
		for (size_t i = 0; i < result.size(); i++)
		{
			result_set[i].file_index = (uint32_t) m_files.GetIndexByFile(result[i]);
//			NS_LOG_INFO ("File index should be: " << result_set[i].file_index);
			result_set[i].file_size = result[i]->GetData().GetSize();
//			NS_LOG_INFO ("File size should be: " << result_set[i].file_size);

			result_set[i].shared_file_name = result[i]->GetFileName();
//			NS_LOG_INFO("File name should be: " << result_set[i].shared_file_name);
		}
		
		// compute size of result set
		uint32_t size = 0;
		for (size_t i = 0; i < result.size(); i++)
		{
		   size += (8 + result_set[i].shared_file_name.size() + 2); // 2 = \0\0
		}
			
		header.payload_length = 27 + size;
		
		// get ip/port
		InetSocketAddress a = InetSocketAddress::ConvertFrom(m_local);
		
		
		// create desc
		QueryHitDescriptor *qh_desc = new QueryHitDescriptor(header, result.size(),
			a.GetPort(), a.GetIpv4(), DEFAULT_SPEED, result_set, m_servent_id);
	
		Send(qh_desc, p);
		
		LogMessage("Sending QUERY_HIT");
		
		delete [] result_set;
		
	}
	else if(desc->query_type_==1)
	{//fast query - search in cache. do not forward
		CacheEntry entry;
		bool result = cache.get(desc->GetSearchCriteria(), entry);
		if(result == true)
		{
			Send(entry ,p);
			//return the cached entry
		}
		else
		{
			//need to write the Fast query failure descriptor and send it
		}
	}
	else
	{//slow query
	
		// Decrement ping ttl, increment hops
		desc->Hop();
		Request *r = m_requests.GetRequestById(desc->GetHeader().descriptor_id);
		// Make sure TTL != 0 first.
		if (desc->GetTtl() > 0 && !(r != NULL && r->GetType() == desc->GetType()) )
		{
			// Foward query
			for (size_t i = 0; i < m_connected_peers.GetSize(); i++)
			{
				if (m_connected_peers.GetPeer(i) != p)
				{
					Send(desc, m_connected_peers.GetPeer(i));
					LogMessage("Forwarding QUERY");
				}
			}

			// Add request to our table
			m_requests.AddRequest(new Request(desc, p));
		}
	}
	
}

void GnutellaApp::HandleQueryHit(QueryHitDescriptor *desc)
{
	LogMessage("Received QUERY_HIT");
	// Received QueryHit
	// Is it for me?
	// Forward it if no
	Request* r = m_requests.GetRequestById(desc->GetHeader().descriptor_id);
	if (r != NULL)
	{
		if (!r->IsSelf())
		{
			LogMessage("Forwarding QUERY_HIT");
			desc->Hop();
			if (desc->GetTtl() > 0)
			{
				//add entry to cache
				CacheEntry entry= CacheEntry::Create(desc);
				cache.put(desc->result_set_->shared_file_name, entry);
				Send(desc, r->GetPeer());
			}
			m_requests.RemoveRequest(r);
		}
		else if (r->IsSelf()) // it's for me
		{
			// Update the query response measurements
			DescriptorId desc_id = desc->GetHeader().descriptor_id;
            m_query_responses.Update(desc_id);
            
			Result* result_set = desc->GetResultSet();
			// Pick the first result for now..
			
			// The host with the file most likely is not a connected peer, but we test for it anyway
			InetSocketAddress inetaddr(desc->GetIp(), desc->GetPort());
			Address addr = inetaddr;
			Peer *p = m_connected_peers.GetPeerByAddress(addr);
			if (p == NULL) // The peer is not connected
			{
				// Initiate a connection
				// Wait for connection to succeed (DownloadConnectionSucceed)
				// then send a download request.
				Ptr<Socket> socket = ConnectToPeer(addr, true);
			
				// Add the host to our peer container
				// This action is not specified in the spec, but it makes
				// things simpler.
				// Note: The peer has not actually connected yet...
				p = AddConnectedPeer(addr, socket);
			}
			else // Connected; send request right away
			{
				SendFileDownloadRequest(p, result_set[0].file_index, result_set[0].shared_file_name);
			}
			// Store the peer/filename+index download pair because the response won't contain
			// it
			m_downloads.AddDownload(p, result_set[0].shared_file_name, result_set[0].file_index);
			
		}
		
	}
}

void GnutellaApp::HandleFastQueryHit(CacheEntry* entry, bool hit)
{

}

void GnutellaApp::HandlePush(PushDescriptor *desc)
{
	LogMessage("Received PUSH");
}

void GnutellaApp::Send(Descriptor *desc, Peer *p)
{
//    LogMessage("Sending Descriptor");

	uint32_t size = desc->GetSerializedSize();

	ns3::Buffer buffer;
    buffer.AddAtStart(size);

	ns3::Buffer::Iterator it = buffer.Begin();
	desc->Serialize(it);
    
	// Copy the ns3::Buffer back to a uint8_t string
	uint8_t * serialized_buffer = new uint8_t[size];
	buffer.CopyData(serialized_buffer, size);

	if (!m_connected_peers.SearchPeer(p)) // Not connected
        return;

    Ptr<Packet> packet = Create<Packet>(serialized_buffer, size );

	Ptr<Socket> socket = p->GetSocket();
    socket->Send(packet);

	delete [] serialized_buffer;
}

Ptr<Socket> GnutellaApp::ConnectToPeer(Address peeraddr, bool dl)
{
	TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
	Ptr<Socket> socket = Socket::CreateSocket (GetNode (), tid);
	socket->Bind();
	
	// Connect callbacks
	if(dl)
	{
		socket->SetConnectCallback (
			MakeCallback (&GnutellaApp::DownloadConnectionSucceeded, this),
			MakeCallback (&GnutellaApp::DownloadConnectionFailed, this));
	}
	else
	{
		socket->SetConnectCallback (
			MakeCallback (&GnutellaApp::ConnectionSucceeded, this),
			MakeCallback (&GnutellaApp::ConnectionFailed, this));
	}
    
    // Close callbacks
    socket->SetCloseCallbacks (
		MakeCallback (&GnutellaApp::HandlePeerClose, this),
		MakeCallback (&GnutellaApp::HandlePeerError, this));

	// Receive callbacks
    socket->SetRecvCallback (
            MakeCallback (&GnutellaApp::HandleRead, this));

	socket->Connect(peeraddr);
        
    return socket;
}

void GnutellaApp::ConnectionSucceeded(Ptr<Socket> socket)
{
    LogMessage("Connected to Peer");
    
    Ptr<Packet> packet = Create<Packet> ((const uint8_t*)MSG_CONNECT_REQUEST, 22);
    socket->Send(packet);

}
void GnutellaApp::ConnectionFailed(Ptr<Socket> socket)
{
    LogMessage("Failed to Connect");
}

void GnutellaApp::DownloadConnectionSucceeded(Ptr<Socket> socket)
{
    LogMessage("Connected to Peer (Download)");
    
    // Successfully connected to our host;
    // Send a download request.
    Peer *p = m_connected_peers.GetPeerBySocket(socket);
    
    // Get the filename/index
    int index = m_downloads.GetIndex(p);
    // Send file download request
    if (index != -1)
		SendFileDownloadRequest(p, (uint32_t) index, m_downloads.GetFileName(p));
}

void GnutellaApp::DownloadConnectionFailed(Ptr<Socket> socket)
{
    LogMessage("Failed to Connect (Download)");
    
    Peer *p = m_connected_peers.GetPeerBySocket(socket);
    m_connected_peers.RemovePeer(p);
}


void GnutellaApp::HandlePeerClose (Ptr<Socket> socket)
{
//    LogMessage("Peer close");
    // Remove peer from connected sockets list
    Peer *p = m_connected_peers.GetPeerBySocket(socket);
    if(p != NULL)
        m_connected_peers.RemovePeer(p);
}
 
void GnutellaApp::HandlePeerError (Ptr<Socket> socket)
{
  LogMessage("Peer Error");
//  NS_LOG_INFO ("GnutellaApp: peerError");
  // Remove peer from connected sockets list
  Peer *p = m_connected_peers.GetPeerBySocket(socket);
  m_connected_peers.RemovePeer(p);
}


Peer * GnutellaApp::AddConnectedPeer(Address from, Ptr<Socket> socket)
{
	if(!m_gnutella_joined)
    {
        m_gnutella_joined = true;
    }

    Peer * p = new Peer(from, socket);
    m_connected_peers.AddPeer(p);

//    std::stringstream ss;
//    ss << "Connected peers:" << m_connected_peers.GetSize();
//    LogMessage(ss.str().c_str());
    
    return p;
}

void GnutellaApp::PingPeers()
{
    LogMessage("Sending PINGs");

    for(size_t i = 0; i<m_connected_peers.GetSize(); i++)
    {
		PingDescriptor * pd = new PingDescriptor(GetNode());
        Send(pd, m_connected_peers.GetPeer(i));
	}

    m_pingtimer.Schedule(Seconds(100));
}

void GnutellaApp::SendQuery(std::string filename)
{
    LogMessage(("Sending Query: " + filename).c_str());

    //create a response queue for this file and add it to the node's list of response queues


    for (size_t i = 0; i < m_connected_peers.GetSize(); i++)
    {
		// A new descriptor is generated for each query;
		// because descriptor ID needs to be unique

    	//first send a fast query to all connected peers
		QueryDescriptor *q = new QueryDescriptor(GetNode(), 0, filename, 1);
 
        // Update the query response measurements
		DescriptorId desc_id = q->GetHeader().descriptor_id;
		m_query_responses.AddQuery(desc_id);
        
        // Store the request with the descriptor ID, so that we can identify the
        // responding query hit
        // self = true, peer = NULL
        m_requests.AddRequest(new Request(q, NULL, true));
        
        Send(q, m_connected_peers.GetPeer(i));
	}
}

void GnutellaApp::GenerateFiles()
{
	ns3::Buffer buf;
//        NS_LOG_INFO(buf.GetSize());
	File* f = new File("foo", buf);
	File* f2 = new File("fooobar", buf);
	m_files.AddFile(f);
	m_files.AddFile(f2);
}

void GnutellaApp::GetServentID(Ipv4Address ipv4, uint8_t *sid)
{
	// create servent ID.

	char temp[16];
	sprintf (temp, "%u", ipv4.Get());

	for (int i = 0; i < 9; i++)
		*(sid+i) = *((unsigned char*) temp+i);
	
	// the rest are zeros
	for (int i = 9; i < 16; i++)
	{
		sid[i] = '0';
	}
}

uint32_t GnutellaApp::GetAverageQueryResponseTime()
{
    return m_query_responses.GetAverageResponseTime();
}

void GnutellaApp::LogAverageQueryResponseTime()
{
    NS_LOG_INFO ("GnutellaApp: " << ns3::InetSocketAddress::ConvertFrom(this->m_local).GetIpv4() << " Average Query Response Time: " << GetAverageQueryResponseTime());
}

void GnutellaApp::LogMessage(const char * message)
{
        NS_LOG_INFO ("GnutellaApp: " << ns3::InetSocketAddress::ConvertFrom(this->m_local).GetIpv4() << " " << message);
}
