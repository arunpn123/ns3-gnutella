/*
 * Cache.h
 *
 *  Created on: Apr 1, 2013
 *      Author: arun
 */

#ifndef CACHE_H_
#define CACHE_H_

#include <string>
#include <stdint.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/buffer.h"
#include "ns3/nstime.h"
#include "ns3/ipv4-address.h"
#include "File.h"
#include "Descriptor.h"

/**
 * LRU Key-Value container adapted from code originally by Tim Day
 * see http://timday.bitbucket.org/lru.html
 */

#include <map>
#include <list>
#include <utility>
#include <iostream>

class CacheEntry
{
	public:
	// 2 bytes
	uint16_t port_;
	// 4 bytes (size = 4)
	ns3::Ipv4Address ip_address_;
	// 4 bytes
	uint32_t file_index_;
	// 4 bytes
	uint32_t file_size_;
	// Null terminated string
	std::string file_name_;


	//static CacheEntry  Create(QueryHitDescriptor* desc);
	//CacheEntry()
	//{

	//}
	CacheEntry();
	CacheEntry(QueryHitDescriptor *desc);
	//~CacheEntry();
};
//planning to use the cache as a store for <filename, CachEntry> pairs

template <typename KeyType, typename ValueType>
class LruCache
{
public:
    typedef KeyType key_type;
    typedef ValueType value_type;
    unsigned int m_capacity;
    LruCache()
    {

    }
    LruCache(unsigned int capacity) : m_capacity(capacity)
    { }

    void put(const KeyType & key, const ValueType & value)
    {
        // see if this item is in the cache already
        typename CacheType::iterator it = m_cache.find(key);
        if(it == m_cache.end())
        {
            // insert a new value, possibly evicting an old one
            insert(key, value);
        }
    }

    bool get(const KeyType & key, ValueType & value)
    {
        // see if this item is in the cache
        typename CacheType::iterator it = m_cache.find(key);
        if(it == m_cache.end())
        {
            // this is a miss; just return false
            std::cout << "key not found: " << key << "\n";
            return false;
        }
        else
        {
            // found! update the timestamp for this key, and return
            // the value
            // this moves the key's position in the history to the end,
            // making it the most recently accessed item
            m_history.splice(
                m_history.end(),
                m_history,
                (*it).second.second);

            value = (*it).second.first;
            std::cout << "updated ts for key: " << key << "\n";
            return true;
        }
    }

    std::list<KeyType> get_keys() const
    {
        std::list<KeyType> out;
        for(typename LruHistoryType::const_reverse_iterator it = m_history.rbegin();
            it != m_history.rend();
            ++it)
        {
            std::cout << "get_keys: adding " << *it << "\n";
            out.push_back(*it);
        }
        return out;
    }

private:
    void insert(const KeyType & key, const ValueType & value)
    {
        if(m_cache.size() == m_capacity)
            evict();

        typename LruHistoryType::iterator it = m_history.insert(m_history.end(), key);
        m_cache.insert(
            std::make_pair(
                key,
                std::make_pair(value, it)));

       // std::cout << "inserted new item: key=" << key << ", value=" << value << "\n";
    }

    void evict()
    {
        // remove least recently used item
        typename CacheType::iterator to_evict = m_cache.find(m_history.front());
        std::cout << "evicting key: " << to_evict->first << "\n";
        m_cache.erase(to_evict);
        m_history.pop_front();
    }

    typedef std::list<KeyType> LruHistoryType;
    typedef std::pair<ValueType, typename LruHistoryType::iterator> ValueHistPair;
    typedef std::map<KeyType, ValueHistPair> CacheType;


    LruHistoryType m_history;
    CacheType m_cache;
};


#endif /* CACHE_H_ */
