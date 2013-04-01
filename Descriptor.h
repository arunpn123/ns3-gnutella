#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <string>
#include <stdint.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/buffer.h"
#include "ns3/nstime.h"
#include "ns3/ipv4-address.h"
#include "File.h"

#define PONG_PAYLOAD_LENGTH 		14

// Note:
// NS3 applications receive packets as ns3::Packet objects.
// Data from ns3::Packets can be retrieved via ostream.
// Here we are assuming that the ostream has been read and separated into
// character buffers for the descriptor header and payload.

class Result
{
    public:
    Result(){};
    Result(File f);
    void Serialize();
    uint32_t GetSerializedSize();

	// 4 bytes
	uint32_t file_index;
	// 4 bytes
	uint32_t file_size;
	// Null terminated string
	std::string shared_file_name;
};

// Forward declarations
class PingDescriptor;
class PongDescriptor;
class QueryDescriptor;
class QueryHitDescriptor;
class CacheQueryHitDescriptor;

class DescriptorId
{
    public:
        static DescriptorId Create(ns3::Ptr<ns3::Node> n);
        DescriptorId(){};
        DescriptorId(const uint8_t * desc_id);
        DescriptorId(const DescriptorId &desc_id);
        DescriptorId & operator=(const DescriptorId &desc_id);
        void Serialize(ns3::Buffer::Iterator start);
        uint8_t GetSerializedSize();
        friend bool operator<(const DescriptorId &lhs, const DescriptorId &rhs);
    private:
        static ns3::Time time_;
        static uint16_t time_count_;
        uint8_t desc_id_[16];
};

class Descriptor
{
    public:
        typedef enum  
        {
            PING = 0x00,
            PONG = 0x01,
            QUERY = 0x80,
            QUERYHIT = 0x81,
            PUSH = 0x40,
            FASTQUERY,
            FASTQUERYHIT
        } DescriptorType;
	/*static PingDescriptor * Create(const char *p_descriptor);
	static PongDescriptor * Create(const char *p_descriptor);
	static QueryDescriptor * Create(const char *p_descriptor);
	static QueryHitDescriptor * Create(const char *p_descriptor);*/

        typedef struct  {
            // 16 bytejs
//            uint8_t descriptor_id[16];
            DescriptorId descriptor_id;
            // 1 byte
            uint8_t payload_descriptor;
            // 1 byte
            uint8_t ttl;
            // 1 byte
            uint8_t hops;
            // 4 bytes
            uint32_t payload_length;
        } DescriptorHeader;

        static Descriptor * Create(const uint8_t * p_descriptor);

        uint8_t GetTtl();

        //Increment hops and decerment TTL simultaneously
        void Hop();

        //Increment header hops
        void AddHop();
        
        //Decrement the header TTL
        void DecTtl();

        virtual void Serialize(ns3::Buffer::Iterator start);
        virtual uint32_t GetSerializedSize();

        DescriptorType GetType(){ return type_;}
        DescriptorHeader GetHeader() const { return header_;}

        ~Descriptor();

    protected:
        void SerializeHeader(ns3::Buffer::Iterator start);
        DescriptorHeader header_;
        DescriptorType type_;
        Descriptor();
        Descriptor(DescriptorHeader h);

        static const uint8_t DEFAULT_TTL = 7;
};

class PingDescriptor : public Descriptor
{
    public:
        void Serialize(ns3::Buffer::Iterator start);
        uint32_t GetSerializedSize();

        static PingDescriptor * Create(DescriptorHeader header, 
                const uint8_t *descriptor_payload);

        PingDescriptor(DescriptorHeader header);
        PingDescriptor(ns3::Ptr<ns3::Node> n);
        PingDescriptor();
        ~PingDescriptor();
    protected:
        static const uint32_t PAYLOAD_LENGTH = 0;
};

class PongDescriptor : public Descriptor
{
    private:

    public:
        // 2 bytes
        uint16_t port_;
        // 4 bytes (size = 4)
        ns3::Ipv4Address ip_address_;
        // 4 bytes
        uint32_t files_shared_;
        // 4 bytes
        uint32_t kb_shared_;
        static PongDescriptor * Create(DescriptorHeader header, const uint8_t *descriptor_payload);

        void Serialize(ns3::Buffer::Iterator start);
        uint32_t GetSerializedSize();

        PongDescriptor(DescriptorId id, ns3::Address addr, 
                uint32_t files_shared, uint32_t kb_shared);

        PongDescriptor(DescriptorHeader header, uint16_t port, 
                ns3::Ipv4Address ip_address, 
                uint32_t files_shared, 
                uint32_t kb_shared);
        ~PongDescriptor();
};

class QueryDescriptor : public Descriptor
{
	// 2 bytes
	uint16_t min_speed_;
	// Null terminated string
	std::string search_criteria_;
//Query cacheing gnutella
	uint16_t query_type_;   //0 - Slow query  1 - Fast query

public:
    static QueryDescriptor * Create(DescriptorHeader header, 
            const uint8_t *descriptor_payload);
  
    void Serialize(ns3::Buffer::Iterator start);
    uint32_t GetSerializedSize();
    
    std::string GetSearchCriteria();
    uint16_t GetMinSpeed();

	QueryDescriptor(DescriptorHeader header, 
            uint16_t min_speed, 
            std::string search_criteria);
	QueryDescriptor(ns3::Ptr<ns3::Node> n, uint16_t min_speed, std::string filename, uint16_t query_type);
	~QueryDescriptor();
};

class QueryHitDescriptor : public Descriptor
{
	// 1 byte
	uint8_t num_hits_;
	// 2 bytes
	uint16_t port_;
	// 4 bytes (size = 4)
	ns3::Ipv4Address ip_address_;
	// 4 bytes
	uint32_t speed_;
	// Size = num_hits
	Result *result_set_;
	// 16 bytes
	uint8_t servant_identifier_[16];

    public:
    static QueryHitDescriptor * Create(DescriptorHeader header, const uint8_t *descriptor_payload);

	void Serialize(ns3::Buffer::Iterator start);
	uint32_t GetSerializedSize();
	uint32_t GetResultSetSerializedSize();
	
	uint8_t GetNumHits();
	uint16_t GetPort();
	ns3::Ipv4Address GetIp();
	uint32_t GetSpeed();
	Result* GetResultSet();
	uint8_t* GetServentID();

    QueryHitDescriptor(DescriptorId id, 
            uint8_t num_hits, ns3::Address addr, uint32_t speed, 
            Result *result_set, uint8_t *servant_identifier);

	QueryHitDescriptor(DescriptorHeader header, uint8_t num_hits, 
		uint16_t port, ns3::Ipv4Address ip_address, uint32_t speed, 
		Result *result_set, uint8_t *servant_identifier);
	~QueryHitDescriptor();
};

class PushDescriptor : public Descriptor
{
	public:
	static PushDescriptor * Create(DescriptorHeader header, const uint8_t *descriptor_payload);
};

#endif
