/*
 * FastQueryMissDescriptor.cc
 *
 *  Created on: Apr 1, 2013
 *      Author: arun
 */
#include "Descriptor.h"

FastQueryMissDescriptor::FastQueryMissDescriptor(DescriptorHeader h, std::string file_name)
{
	type_ = FASTQUERYMISS;
	header_.descriptor_id = h.descriptor_id;
	file_name_.assign(file_name);
	//strcpy(file_name_, file_name.c_str());
}

FastQueryMissDescriptor *
FastQueryMissDescriptor::Create(DescriptorHeader h, std::string file_name)
{
    return new FastQueryMissDescriptor(h, file_name);
}

FastQueryMissDescriptor::~FastQueryMissDescriptor()
{

}
uint32_t
FastQueryMissDescriptor::GetSerializedSize()
{
    // header + min_speed + string + null terminator
    return 23+file_name_.size()+1;
}

void
FastQueryMissDescriptor::Serialize(ns3::Buffer::Iterator start)
{
    SerializeHeader(start);
}
