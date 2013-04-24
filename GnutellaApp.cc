#include "GnutellaApp.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

#include <assert.h>
#include <iostream>
#include <utility>
#include <sstream>
#include <arpa/inet.h>
#include "ns3/buffer.h"
#include "Descriptor.h"
#include "Request.h"
#include "Cache.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("GnutellaApp");

bool isSameIpAddress(const Address & addr1, const Address & addr2)
{
    InetSocketAddress inet1 = InetSocketAddress::ConvertFrom(addr1);
    InetSocketAddress inet2 = InetSocketAddress::ConvertFrom(addr2);
    return (inet1.GetIpv4() == inet2.GetIpv4());
}

GnutellaApp::GnutellaApp(Address addr, Address boots)
    :m_requests(ns3::Seconds(SECONDS_REQUEST_TIMEOUT))
{
	InetSocketAddress a = InetSocketAddress::ConvertFrom(addr);
	
//	InetSocketAddress b = InetSocketAddress::ConvertFrom(boots);
//	NS_LOG_INFO ("With bootstrap: " << b.GetIpv4() << ":" << b.GetPort());
	
	m_local = addr;
	m_bootstrap = boots;
	m_gnutella_joined = false;
   // cache = new LruCache(10);
	cache.m_capacity = 10;
	// generate serventID
	GetServentID(a.GetIpv4(), m_servent_id);
	
	
 // Generate some files
//    	this->GenerateFiles();
       m_max_files = 10;
       m_uniform_rng = CreateObject<UniformRandomVariable>();
	
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
		
		myqueries=0;
    m_node_id = GetNode()->GetId();
    m_stats.set_id(m_node_id);

    GenerateFiles(m_max_files);

    TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");

    m_pingtimer.SetFunction(&GnutellaApp::PingPeers, this);

//    std::stringstream ss;
//    ss << "Peer id:" << GetNode()->GetId();
//    LogMessage(ss.str().c_str());

    m_pingtimer.Schedule(Seconds(10));

    m_querytimer.SetFunction(&GnutellaApp::SendQuery, this);
//    m_querytimer.SetArguments(std::string("foo"));
    m_querytimer.SetArguments(getRandomFilename());
    // blaub
    m_querytimer.Schedule(Seconds(10));
///    m_querytimer.Schedule(Seconds(11)+Seconds(10*abs(NormalVariable().GetValue())));
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
    // blaub
    m_querytimer.Cancel();
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
        case Descriptor::FASTQUERY:
            HandleQuery((QueryDescriptor*)desc, p);
            break;
        case Descriptor::QUERYHIT:
        case Descriptor::FASTQUERYHIT:
            HandleQueryHit((QueryHitDescriptor*)desc);
            break;
        case Descriptor::PUSH:
			HandlePush((PushDescriptor*)(desc));
			break;
        case Descriptor::FASTQUERYMISS:
        	HandleFastQueryMiss((FastQueryMissDescriptor*)desc);
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
    if (f == NULL || filename.compare(f->GetFileName()) != 0){
    	SendFileDownloadResponseError(p, filename);
    	LogMessage("File not found when searching locally -while Download in progress");
    	return;
    }
    // 3) Send an HTTP response containing the file data
    ns3::Buffer::Iterator fit = f->GetData().Begin();
    SendFileDownloadResponse(p, fit);
    
    delete [] buffer;
}

void GnutellaApp::HandleFileDownloadResponse(Peer *p, ns3::Buffer::Iterator it)
{
    m_stats.incr("file_download_res");

    LogMessage("Received File");
    if (it.GetSize() <= 92)
        return;
    std::string filename;
    ns3::Buffer data;
    uint8_t *header_buffer = new uint8_t[20];
    // 1) Extract the file size and data
    // Skip the initial HTTP
    // HTTP/1.0 200 OK\r\n
    ns3::Buffer::Iterator itCopy;

    it.Read(header_buffer, 16);
    std::string header_string((char*)header_buffer);
    if(header_string.find("404")!= std::string::npos)
    {
        std::map<std::string, std::queue<QueryHitDescriptor> >::iterator it;
    	it = fastqueryhit_responselist.find(filename);
    	int responsecount= it->second.size();
    	if(responsecount >0)
    	{
    		QueryHitDescriptor desc = it->second.front();
    		it->second.pop();
    		// The host with the file most likely is not a connected peer, but we test for it anyway
    		InetSocketAddress inetaddr(desc.GetIp(), desc.GetPort());
    		Address addr = inetaddr;
    		Peer *p = m_connected_peers.GetPeerByAddress(addr);
    		if(p == NULL) // The peer is not connected
    		{
    			Ptr < Socket > socket = ConnectToFastQueryDownloadPeer(addr, filename);
    			p = AddConnectedPeer(addr, socket);
    		}
    		else // Connected; send request right away
    		{
    			SendFileDownloadRequest(p, desc.result_set_[0].file_index,
    					desc.result_set_[0].shared_file_name);
    		}
    		// Store the peer/filename+index download pair because the response won't contain it
    		m_downloads.AddDownload(p, desc.result_set_[0].shared_file_name,
    				desc.result_set_[0].file_index);
    	}
    	return;
    }
    it.Prev(16);
    LogMessage("Extracting data from File");
    it.Next(17);
    LogMessage("Extracting data from File - after 17");
    // Server: Gnutella/0.4\r\n
    it.Next(22);
    LogMessage("Extracting data from File - after 22");
    // Content-Type: application/binary\r\n
    it.Next(34);
    LogMessage("Extracting data from File - after 34");
    // Content-Length:_
    it.Next(16);
    LogMessage("Extracting data from File Error after 16");
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
    LogMessage("Extracting data from File - next 3");
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

    LogMessage("Starting delete of File from map");
    //delete the file and queue in the map
    std::map<std::string, std::queue<QueryHitDescriptor> >::iterator mapIteratorLocal1;
    mapIteratorLocal1 = fastqueryhit_responselist.find(filename);
    if(mapIteratorLocal1 == fastqueryhit_responselist.end())
    {
    	LogMessage("Not found file in map queue");
    }
    else
    {
		fastqueryhit_responselist.erase(mapIteratorLocal1);
		LogMessage("Deleted the fastquery response queue for the file");
    }
    std::map<std::string, int >::iterator mapIteratorLocal2;
    mapIteratorLocal2= fastquery_responsecount.find(filename);
    if(mapIteratorLocal2 == fastquery_responsecount.end())
	{
		LogMessage("Not found responsecount in map");
	}
	else
	{
		fastquery_responsecount.erase(mapIteratorLocal2);
		LogMessage("Deleted the responsecount for the file");
	}

    mapIteratorLocal2= fastquery_requestcount.find(filename);
    if(mapIteratorLocal2 == fastquery_requestcount.end())
	{
		LogMessage("Not found requestcount in map");
	}
	else
	{
		fastquery_requestcount.erase(mapIteratorLocal2);
		LogMessage("Deleted the requestcount for the file");
	}
}

void GnutellaApp::SendFileDownloadRequest(Peer *p, uint32_t file_index,
    std::string file_name)
{
    m_stats.incr("file_download_req");

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

void GnutellaApp::SendFileDownloadResponseError(Peer *p, std::string fileName)
{
	LogMessage("Sending Error File");
	    std::stringstream ss;
	    ss << "HTTP/1.0 404 OK\r\n";
	    ss << "Server: Gnutella/0.4\r\n";
	    ss << "Content-Type: application/binary\r\n";
	    ss << "Content-Length: " << 0 << "\r\n";
	    ss << "\r\n";
	    //data.CopyData(&ss, data.GetSize());
	//    uint8_t *temp = new uint8_t[it.GetSize()];
	 //   it.Read(temp, it.GetSize());
	    //std::string strbuf1 = (const char *) temp;
		std::string strbuf = ss.str() + fileName;
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
    // blaub: DONT forward your own query
    bool hit_mine = false;
    
    Request *r = m_requests.GetRequestById(desc->GetHeader().descriptor_id);
    if(r && r->IsSelf())
    {
        m_stats.incr("query_routing_loop");
        return;
    }

    m_stats.incr("query_recieved");
	LogMessage("Received QUERY");
	// For any query, first search if file is present in filesystem, then search in cache, then handle other conditions
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
		//blaub
		hit_mine = true;
		LogMessage("Sending QUERY_HIT");
		
		delete [] result_set;
		
	}
	else if(desc->query_type_==1)
	{//fast query - search in cache. do not forward
		LogMessage("Received a FASTQUERY. Searching in cache");
		CacheEntry entry;
		bool cacheresult = cache.get(desc->search_criteria_, entry);
		if(cacheresult == true)
		{
			//prepare a queryhitdescriptor and populate it with the cache entries and send
			//TODO: not sure about serverid
			// Create a new QueryHit descriptor
			Descriptor::DescriptorHeader header;
			header.descriptor_id = desc->GetHeader().descriptor_id;
			header.payload_descriptor = Descriptor::QUERYHIT;
			header.ttl = DEFAULT_TTL;
			header.hops = 0;

			// convert std::vector result set to an array
			Result* result_set = new Result[1];
			for (size_t i = 0; i < 1; i++)
			{
				result_set[i].file_index = (uint32_t) entry.file_index_;
				result_set[i].file_size = entry.file_size_;
				result_set[i].shared_file_name.assign(entry.file_name_);
			}
			// compute size of result set
			uint32_t size = 0;
			for (size_t i = 0; i < 1; i++)
			{
			   size += (8 + result_set[i].shared_file_name.size() + 2); // 2 = \0\0
			}

			header.payload_length = 27 + size;
			// create desc
			QueryHitDescriptor *qh_desc = new QueryHitDescriptor(header, 1,
				entry.port_, entry.ip_address_, DEFAULT_SPEED, result_set, 0);
		//TODO : check with Vimal
			qh_desc->query_hit_type_ = 1;

			Send(qh_desc, p);
			LogMessage("Sending FAST_QUERY_HIT");
			delete [] result_set;
		}
		else
		{
			//need to write the Fast query failure descriptor and send it
//			Descriptor* parent_desc;
//			FastQueryMissDescriptor miss_desc= FastQueryMissDescriptor::Create(desc->GetHeader(), desc->search_criteria_);
//			parent_desc = &miss_desc;

			FastQueryMissDescriptor * miss_desc = new FastQueryMissDescriptor(desc->GetHeader(), desc->search_criteria_);
			Send(miss_desc, p);
			LogMessage("Sending FAST_QUERY_MISS");
		}
	}
	else
	{//slow query
	    // (a bad assumption?)
	    if(hit_mine)
	    {
		m_stats.incr("hit_mine");
		return;
	    }

		LogMessage("Received SLOWQUERY");
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
					m_stats.incr("query_forwarded");
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
    m_stats.incr("query_hit");
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
				CacheEntry  entry(desc);
				cache.put(desc->result_set_->shared_file_name, entry);
				Send(desc, r->GetPeer());
                m_stats.incr("query_hit_forward");
				LogMessage("Adding Entry to Cache");
			}
			m_requests.RemoveRequest(r);
		}
		else if (r->IsSelf()) // it's for me
		{
			// Update the query response measurements
            m_stats.incr("query_hits_for_me");
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
				Ptr < Socket > socket;
				// Initiate a connection
				// Wait for connection to succeed (DownloadConnectionSucceed)
				// then send a download request.
				//Check for query_hit_type. If Not a FastQuery, ConnectToPeer with Download = true
				if(desc->query_hit_type_ == 0)
				{
					socket = ConnectToPeer(addr, true);
					LogMessage("Received QueryHit - SlowQuery");
				}
				else
				{
					LogMessage("Received QueryHit - FastQuery");
					//If FastQuery check if response count is 0. If not 0, increment response count and add QueryHitDescriptor to the queue
					std::map<std::string, int>::iterator it;
					it = fastquery_responsecount.find(desc->result_set_[0].shared_file_name) ;
					if( it->second > 0)
					{
						std::map<std::string, std::queue<QueryHitDescriptor> >::iterator itQueue;
						it->second++;
						itQueue->second.push(*desc);
					}
					else
					{
						//responseCount = 0 => first response. So make a connection to download the file
						it->second++;
						socket = ConnectToFastQueryDownloadPeer(
								addr, desc->result_set_[0].shared_file_name);
					}
				}
				// Add the host to our peer container
				// This action is not specified in the spec, but it makes
				// things simpler.
				// Note: The peer has not actually connected yet...
				p = AddConnectedPeer(addr, socket);
			}
			else // Connected; send request right away
			{

				if (desc->query_hit_type_ == 0)
				{
					SendFileDownloadRequest(p, result_set[0].file_index, result_set[0].shared_file_name);
					LogMessage("Received QueryHit - SlowQuery");
				}
				else
				{
					//If FastQuery check if response count is 0. If not 0, increment response count and add QueryHitDescriptor to the queue
					LogMessage("Received QueryHit - FastQuery");
					std::map<std::string, int>::iterator it;
					it = fastquery_responsecount.find(desc->result_set_[0].shared_file_name);
					if (it->second > 0)
					{
						std::map<std::string, std::queue<QueryHitDescriptor> >::iterator itQueue;
						it->second++;
						itQueue->second.push(*desc);
					}
					else
					{	//responseCount = 0 => first response. So download the file
						it->second++;
						SendFileDownloadRequest(p, result_set[0].file_index, result_set[0].shared_file_name);
					}
				}
			}
			// Store the peer/filename+index download pair because the response won't contain
			// it
			m_downloads.AddDownload(p, result_set[0].shared_file_name, result_set[0].file_index);
			
		}
	}
}

void GnutellaApp::HandleFastQueryMiss(FastQueryMissDescriptor* desc)
{ // This is called at the node which sends out the FastQuery
	std::stringstream sa;
	sa << "Received Fast QueryMiss " << desc->file_name_;
	LogMessage(sa.str().c_str());

	//fastquery_responsecount++;
	std::map<std::string, int>::iterator it;
	it = fastquery_responsecount.find(desc->file_name_);
	int responsecount = it->second;
	fastquery_responsecount.erase(it);
    responsecount++;
    fastquery_responsecount.insert(std::pair<std::string, int>(desc->file_name_, responsecount));

    it = fastquery_requestcount.find(desc->file_name_);
	int requestcount = it->second;

    if(responsecount == requestcount)
    {
    	// send slow query
    	int neighbors_count= m_connected_peers.GetSize();
		for (int i = 0; i < neighbors_count; i++)
		{
			QueryDescriptor *q = new QueryDescriptor(GetNode(), 0, desc->file_name_, 0);  //0 represents slow query
			DescriptorId desc_id = q->GetHeader().descriptor_id;
			m_query_responses.AddQuery(desc_id);
			m_requests.AddRequest(new Request(q, NULL, true));
			LogMessage("Sending SLOWQUERY");
			Send(q, m_connected_peers.GetPeer(i));
		}
    }
}

void GnutellaApp::HandlePush(PushDescriptor *desc)
{
	LogMessage("Received PUSH");
}

void GnutellaApp::Send(Descriptor *desc, Peer *p)
{
    LogMessage("Sending Descriptor");

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
{if(isSameIpAddress(peeraddr, m_local))
        m_stats.incr("connect_to_self");

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

//struct MyCallback
//{
//	MyCallback(std::string filename, GnutellApp * app) : m_filename(filename)
//	{ }
//
//	void ConnectFailed(Ptr<Socket> s)
//	{
//		// do something with m_filename and s
//		m_app->
//	}
//
//	std::string m_filename;
//};

Ptr<Socket> GnutellaApp::ConnectToFastQueryDownloadPeer(Address from, std::string file_name)
{
	TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
	Ptr<Socket> socket = Socket::CreateSocket (GetNode (), tid);
	socket->Bind();


	socket->SetConnectCallback (
			MakeCallback (&GnutellaApp::FastQueryDownloadConnectionSucceeded, this),
			MakeCallback (&GnutellaApp::FastQueryDownloadConnectionFailed , this ));


    // Close callbacks
    socket->SetCloseCallbacks (
		MakeCallback (&GnutellaApp::HandlePeerClose, this),
		MakeCallback (&GnutellaApp::HandlePeerError, this));

	// Receive callbacks
    socket->SetRecvCallback (
            MakeCallback (&GnutellaApp::HandleRead, this));

	socket->Connect(from);

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

void GnutellaApp::FastQueryDownloadConnectionSucceeded(Ptr<Socket> socket)
{
    LogMessage("Connected to FastQuery Peer (Download)");
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

void GnutellaApp::FastQueryDownloadConnectionFailed( Ptr<Socket> socket)
{
    LogMessage("Failed to Connect FastQuery result (Download)");

    Peer *p = m_connected_peers.GetPeerBySocket(socket);

    // Get the filename/index
    int index = m_downloads.GetIndex(p);
    // Send file download request
    std::string file_name;
    if (index != -1)
    {
    	file_name = m_downloads.GetFileName(p);
    }
    m_connected_peers.RemovePeer(p);

    //pop from response queue and send download request

    std::map<std::string, std::queue<QueryHitDescriptor> >::iterator it;
	it = fastqueryhit_responselist.find(file_name);
	int responsecount= it->second.size();
	if(responsecount >0)
	{
		QueryHitDescriptor desc = it->second.front();
		it->second.pop();
		// The host with the file most likely is not a connected peer, but we test for it anyway
		InetSocketAddress inetaddr(desc.GetIp(), desc.GetPort());
		Address addr = inetaddr;
		Peer *p = m_connected_peers.GetPeerByAddress(addr);
		if(p == NULL) // The peer is not connected
		{
			Ptr < Socket > socket = ConnectToFastQueryDownloadPeer(addr, file_name);
			p = AddConnectedPeer(addr, socket);
		}
		else // Connected; send request right away
		{
			SendFileDownloadRequest(p, desc.result_set_[0].file_index,
					desc.result_set_[0].shared_file_name);
		}
		// Store the peer/filename+index download pair because the response won't contain it
		m_downloads.AddDownload(p, desc.result_set_[0].shared_file_name,
				desc.result_set_[0].file_index);
	}
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
  m_stats.incr("peer_error");

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
    
    m_stats.max("connected_peers_max", m_connected_peers.GetSize());
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
    LogMessage(("Sending FastQuery: " + filename).c_str());
// blaub: if i already have this file, don't query (??)
    std::vector<File *> have = m_files.GetFileByName(filename);

    m_stats.incr("send_query");

    std::ostringstream lg;
    lg << Simulator::Now() << ": sending query: " << filename;
    //LogMessage(lg.str().c_str());
    std::cout << lg.str() << "\n";


    //create a response queue for this file and add it to the node's list of response queues

    int neighbors_count= m_connected_peers.GetSize();
    std::queue<QueryHitDescriptor> queue;

    fastquery_requestcount.insert(std::pair<std::string, int>(filename, neighbors_count));
    fastquery_responsecount.insert(std::pair<std::string, int>(filename, 0));
    fastqueryhit_responselist.insert(std::pair<std::string, std::queue<QueryHitDescriptor> >(filename, queue));

    for (int i = 0; i < neighbors_count; i++)
    {
		// A new descriptor is generated for each query;
		// because descriptor ID needs to be unique

    	//first send a fast query to all connected peers . 1->fast query
		QueryDescriptor *q = new QueryDescriptor(GetNode(), 0, filename, 1);
 
        // Update the query response measurements
		DescriptorId desc_id = q->GetHeader().descriptor_id;
		m_query_responses.AddQuery(desc_id);
        
        // Store the request with the descriptor ID, so that we can identify the
        // responding query hit
        // self = true, peer = NULL
        m_requests.AddRequest(new Request(q, NULL, true));
        
	        if(isSameIpAddress(m_connected_peers.GetPeer(i)->GetAddress(), m_local))
            m_stats.incr("send_query_to_self");

        Send(q, m_connected_peers.GetPeer(i));
        
        InetSocketAddress who = InetSocketAddress::ConvertFrom(m_connected_peers.GetPeer(i)->GetAddress());
        m_stats.incr("send_query_actual");
        std::cout << "node " << m_node_id << ": send_query_actual to "
                  << m_connected_peers.GetPeer(i)->GetAddress() << " - " << who.GetIpv4() << ":" << who.GetPort() << "\n";
    }

    m_querytimer.SetArguments(getRandomFilename());
    if(myqueries<9)
    {
		myqueries++;
	    	std::cout << "Node: " << m_node_id << " Scheduling query no." << myqueries << "\n";
	    	m_querytimer.Schedule(Seconds(10.0));
 }
}
void GnutellaApp::GenerateFiles(unsigned int num_files)
{
////
////    // TODO
////    ns3::Buffer buf;
////    File* fasdf = new File("foo", buf);
////    m_files.AddFile(fasdf);
////
////    return;
////

    // blaub
    uint32_t node_id = m_node_id; //GetNode()->GetId();
    for(unsigned int i = 0; i < num_files; ++i)
    {
        std::ostringstream ss;
        ss << "Node" << node_id << "File" << i;
        ns3::Buffer buf;
        File * f = new File(ss.str(), buf);
        m_files.AddFile(f);
    }

//    ns3::Buffer buf;
////        NS_LOG_INFO(buf.GetSize());
//    File* f = new File("foo", buf);
//    File* f2 = new File("fooobar", buf);
//    m_files.AddFile(f);
//    m_files.AddFile(f2);

    // blaub
//    std::cout << "node " << node_id << " stores these files:\n";
//    std::vector<File *> filelist = m_files.GetAllFiles();
//    for(unsigned int i = 0; i < filelist.size(); ++i)
//    {
//        File * f = filelist[i];
//        std::cout << "  " << f->GetFileName() << "\n";
//    }
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
std::string GnutellaApp::getRandomFilename() const
{
    uint32_t my_id = GetNode()->GetId();
    uint32_t max_node_id = NodeList::GetNNodes() - 1;

    uint32_t target_node = my_id;
    while(target_node == my_id)
        target_node = m_uniform_rng->GetInteger(0, max_node_id);

    assert(target_node != my_id);

    uint32_t file_id = m_uniform_rng->GetInteger(0, m_max_files);
    char buf[1024];
    snprintf(buf, 1024, "Node%dFile%d", target_node, file_id);

    return std::string(buf);
}
