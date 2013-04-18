/*
 * Cache.cpp
 *
 *  Created on: Apr 1, 2013
 *      Author: arun
 */
#include "Cache.h"

//CacheEntry CacheEntry::Create(QueryHitDescriptor * desc)
//{
//	return new CacheEntry(desc);
//}
CacheEntry::CacheEntry()
{ }

CacheEntry::CacheEntry(QueryHitDescriptor * desc)
{
	port_ = desc->port_;
		// 4 bytes (size = 4)
	ip_address_= desc->ip_address_;
		// Size = num_hits
	file_index_= desc->result_set_->file_index;
	file_size_= desc->result_set_->file_size;
	std::strcpy(file_name_, desc->result_set_->shared_file_name);
}


//CacheEntry::~CacheEntry()
//{
//
//}
