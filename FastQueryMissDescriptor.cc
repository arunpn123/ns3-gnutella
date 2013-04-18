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
