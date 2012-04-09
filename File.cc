#include "File.h"

File::File(std::string name, ns3::Buffer data)
{
	name_ = name;
	data_ = data;
}

std::string File::GetFileName()
{
	return name_;
}
ns3::Buffer File::GetData()
{
	return data_;
}

void File::SetFileName(std::string name)
{
	name_ = name;
}
void File::SetData(ns3::Buffer data)
{
	data_ = data;
}
