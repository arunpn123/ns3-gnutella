/*
 * FastQueryMissDescriptor.cc
 *
 *  Created on: Apr 1, 2013
 *      Author: arun
 */
#include "Descriptor.h"

FastQueryMissDescriptor::FastQueryMissDescriptor(DescriptorId id, std::string file_name)
{
	type_ = FASTQUERYMISS;
	header_.descriptor_id = id;
	strcpy(file_name_, file_name.c_str());
}

FastQueryMissDescriptor *
FastQueryMissDescriptor::Create(DescriptorId id, std::string file_name)
{
    return new FastQueryMissDescriptor(id, file_name);
}
