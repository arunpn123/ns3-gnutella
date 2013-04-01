/*
 * FastQueryMissDescriptor.cc
 *
 *  Created on: Apr 1, 2013
 *      Author: arun
 */

FastQueryMissDescriptor::FastQueryMissDescriptor(DescriptorId id, std::string file_name)
{
	type_ = FASTQUERYMISS;
	header_.descriptor_id = id;
	strcpy(file_name_, filename);
}

FastQueryMissDescriptor *
FastQueryMissDescriptor::Create(DescriptorId id, std::string file_name)
{
    return new FastQueryMissDescriptor(id, file_name);
}
