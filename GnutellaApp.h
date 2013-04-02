#ifndef GNUTELLAAPP_H
#define GNUTELLAAPP_H
#include <string>
#include <map>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/inet-socket-address.h"
#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "Descriptor.h"
#include "Request.h"
#include "Peer.h"
#include "File.h"
#include "FileDownloadContainer.h"
#include "QueryLatencyContainer.h"
#include "Cache.h"

// Define Constants
#define MSG_CONNECT_REQUEST 		"GNUTELLA CONNECT/0.4\n\n"
#define MSG_CONNECT_OK 				"GNUTELLA OK\n\n"
#define MSG_HTTP_GET			"GET /get/"
#define MSG_HTTP_GET_SIZE		9
#define MSG_HTTP_OK			"HTTP/1.0 200 OK\r\n"
#define MSG_HTTP_OK_SIZE		17
#define SECONDS_REQUEST_TIMEOUT		60
#define DEFAULT_LISTENING_PORT		6346
#define MAXIMUM_PAYLOAD_LENGTH		150
#define DEFAULT_SPEED				1000
#define DEFAULT_TTL 				7

using namespace ns3;

class GnutellaApp: public Application
{
    public:
        GnutellaApp(Address addr, Address boots);
        ~GnutellaApp(); // Cleanup

        virtual void StartApplication();
        
        virtual void StopApplication();
        
        // Get bootstrap node. Initial peer to connect to. Provided by the simulation
        void SetInitPeer(Address peer_addr);
        void SendQuery(std::string filename);
        void PingPeers();
        uint32_t GetAverageQueryResponseTime();
        void LogAverageQueryResponseTime();

    private:
    
		// Send:
        //		Send a serialized descriptor to a node.
        void Send(Descriptor *desc, Peer *p);
        
    	void HandleRead (Ptr<Socket> socket);
        void HandlePeerMessage (Peer * p, const uint8_t * buffer);
    	
    	// ------Handle these descriptors when they're received
    	// HandlePing: 
    	//		1) Create pong desc. and respond w/ pong
    	//		2) Add Request to Request Table
    	//		3) Route ping desc. to everyone else
    	void HandlePing(PingDescriptor *desc, Peer *p);
    	
    	
    	// HandlePong:
    	//		1) Add ip/port in the pong desc. to PeerTable
    	//		2) If descriptor_id matches an entry in RequestTable,
    	//			forward the pong desc to the requester
    	void HandlePong(PongDescriptor *desc);
    	
    	// HandleQuery:
    	// 		1) Check if I have the file. If yes, send back a queryhit. 
    	//		2) Otherwise, forward query, using flooding.
    	void HandleQuery(QueryDescriptor* desc, Peer* p);
    	
    	// HandleQueryHit:
    	//		1) Forward descriptor if I am not the recipient
    	//		2) Otherwise, Initiate a download from the host with the file
    	void HandleQueryHit(QueryHitDescriptor *desc);

    	void HandleFastQueryMiss(FastQueryMissDescriptor* desc);

    	void HandlePush(PushDescriptor *desc);

        // HandleFileDownloadRequest:
        //	Called when a peer sends an HTTP request matching:
        //      	GET /get/<FileIndex>/<FileName>/
        //      1) Send an HTTP response matching the following:
        //      	HTTP/1.0 200 OK\r\n
        //	        Server: Gnutella/0.4 \r\n
        //      	Content-Type: application/binary\r\n
        //      	Content-Length: <FileSize>\r\n
        //		\r\n
        void HandleFileDownloadRequest(Peer *p, ns3::Buffer::Iterator it);
 
        // HandleFileDownloadResponse:
        //	Called when a peer sends an HTTP response matching:
        //     		HTTP/1.0 200 OK\r\n ...
        //	1) Create a new File and put it in the File container    
        void HandleFileDownloadResponse(Peer *p, ns3::Buffer::Iterator it);

        void SendFileDownloadRequest(Peer *p, uint32_t file_index, std::string
           file_name);

        void SendFileDownloadResponse(Peer *p, ns3::Buffer::Iterator it);
    	
    	
    	// Initalize a connection to a peer
    	Ptr<Socket> ConnectToPeer(Address from, bool dl = false);
    	
    	//Query caching gnutella
    	Ptr<Socket> ConnectToFastQueryDownloadPeer(Address from, std::string file_name);

    	// Accept a node into the network, called when GNUTELLA CONNECT is received
    	void AcceptGnutellaConnection(Ptr<Socket> socket, Address from);
    	
    	// Add a GnutellaConnectedPeer
        Peer * AddConnectedPeer(Address from, Ptr<Socket> socket);

		// Callbacks to socket events
    	void AcceptPeerConnection(Ptr<Socket> socket, const Address& from);
    	void PeerConnected(Ptr<Socket> socket, Address &from);
    	void ConnectionSucceeded(Ptr<Socket> socket);
    	void ConnectionFailed(Ptr<Socket> socket);
    	void DownloadConnectionSucceeded(Ptr<Socket> socket);
    	void DownloadConnectionFailed(Ptr<Socket> socket);
    	void HandlePeerClose (Ptr<Socket>);
		void HandlePeerError (Ptr<Socket>);
    	
    	// Generate some files
    	void GenerateFiles();
    	
    	// Compute Servent ID
    	void GetServentID(Ipv4Address ipv4, uint8_t *sid);

        void LogMessage(const char * message);

		// Member variables
		Address m_local;
		Address m_bootstrap;
		bool m_gnutella_joined;
		uint8_t m_servent_id[16];
		Ptr<Socket> m_socket; // Listening socket
		
		// Temp file structure		
		FileContainer m_files;

        //Ping timer runs a new ping request
        Timer m_pingtimer;
        Timer m_querytimer;
        
        // Peer Containers
        PeerContainer m_peers;
        ConnectedPeerContainer m_connected_peers;

        RequestContainer m_requests;

        FileDownloadContainer m_downloads;

        QueryLatencyContainer m_query_responses;

        LruCache<std::string, CacheEntry> cache;

        std::map <std::string, std::queue<QueryHitDescriptor> > fastqueryhit_responselist;
        std::map <std::string, int> fastquery_responsecount;
        std::map <std::string, int> fastquery_requestcount;
};

#endif
